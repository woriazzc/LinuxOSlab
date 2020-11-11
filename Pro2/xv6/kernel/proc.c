#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;


int mxtick[NPriority] = {0x3f3f3f3f, 32, 16, 8};
int wait_limit[NPriority] = {500, 320, 160, 80};
static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->priority = NPriority - 1;
  for(int i = 0; i < NPriority; i++){
	  p->waited_ticks[i] = 0;
	  p->used_ticks[i] = 0;
  }
  release(&ptable.lock);

  // Allocate kernel stack if possible.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  acquire(&ptable.lock);
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);
 
  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  iput(proc->cwd);
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct proc *pr[NPriority], *prunning[NPriority];
  int poccur = 0, flg = 0;
  proc = 0;
  for(int i = 0; i < NPriority; i++){
	  prunning[i] = NULL;
  }
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    flg = 0;
    for(int i = 0; i < NPriority; i++){
    	if(prunning[i]!=NULL){
    		flg = 1;
    		break;
    	}
    }
    if(poccur || !flg){
    	for(int i = 0; i < NPriority; i++)pr[i] = NULL;
    	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    	      if(p->state != RUNNABLE)continue;
    	      if(pr[p->priority] == NULL)pr[p->priority] = p;
    	}
    	poccur = 0;
    }
    for(int i = NPriority - 1; i >= 0; i--){
    	if(pr[i] != NULL){
    		proc = pr[i];
    		break;
    	}
    }
    if(proc == NULL){
    	release(&ptable.lock);
    	continue;
    }
    prunning[proc->priority] = proc;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    	if(p != proc){
    		p->waited_ticks[p->priority]++;
			if(p->priority == NPriority - 1)continue;
			if((p->priority == 0 && p->waited_ticks[p->priority] >= 500)
				|| p->waited_ticks[p->priority] >= 10*mxtick[p->priority]){
				p->priority++;
				p->waited_ticks[p->priority] = 0;
				p->used_ticks[p->priority] = 0;
				if(p->priority > proc->priority)poccur = 1;
			}
    	}
    	else{
    		for(int i = 0; i < NPriority; i++){
				proc->waited_ticks[i] = 0;
			}
			proc->used_ticks[proc->priority]++;
			if(proc->used_ticks[proc->priority] >= mxtick[proc->priority]){
				prunning[proc->priority] = NULL;
				proc->priority--;
				proc->used_ticks[proc->priority] = 0;
			}
    	}
    }

	switchuvm(proc);
	proc->state = RUNNING;
	swtch(&cpu->scheduler, proc->context);
	switchkvm();

	if(proc->state != RUNNABLE){
		prunning[proc->priority] = NULL;
	}
	// Process is done running for now.
	// It should have changed its p->state before coming back.
	proc = 0;
    release(&ptable.lock);
  }
}

//void
//scheduler(void)
//{
//	struct proc *p;
//	struct proc* p3 = NULL;//zjh:queue pointers
//	struct proc* p2 = NULL;
//	struct proc* p1 = NULL;
//	struct proc* p0 = NULL;
//	int quetag[4] = { 0, 0, 0, 0 };//zjh:is each queue has a process
//	int quefir[4] = { 0, 0, 0, 0 };
//	int poccurs = 0;
//	int i;
//	struct proc* t;
//
//	for (;;) {
//		//zjh:interrupts on this processor, process will experience sleep and wakeup
//		//cprintf("qlock:%d %d %d %d\n",quetag[0],quetag[1],quetag[2],quetag[3]);//zjh:ues for
//		sti();
//		acquire(&ptable.lock);
//		/*----------zjh:step1:find next process and add in its queue-------------*/
//		if (!(quetag[0] || quetag[1] || quetag[2] || quetag[3]) || poccurs == 1) {
//			// no process is running, need to relocate the queue pointers
//			poccurs = 0;
//			p0 = p1 = p2 = p3 = NULL;
//			quefir[0] = quefir[1] = quefir[2] = quefir[3] = 0;
//			for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
//				if (p->state != RUNNABLE) continue;
//				if (p->priority == 3 && quefir[3] == 0) {
//					quefir[3] = 1;
//					p3 = p;
//				}
//				else if (p->priority == 2 && quefir[2] == 0) {
//					quefir[2] = 1;
//					p2 = p;
//				}
//				else if (p->priority == 1 && quefir[1] == 0) {
//					quefir[1] = 1;
//					p1 = p;
//				}
//				else if (p->priority == 0 && quefir[0] == 0) {
//					quefir[0] = 1;
//					p0 = p;
//				}
//			}
//		}
//		/*----------zjh:step2:find and give it to proc-------------*/
//		if (p3 != NULL)          proc = p3;
//		else if (p2 != NULL)   proc = p2;
//		else if (p1 != NULL)   proc = p1;
//		else if (p0 != NULL)     proc = p0;
//		else { //zjh:interupt and there will be a runnable (sooner or later)
//			release(&ptable.lock);
//			continue;//zjh:whitch means goto for(;;)
//		}
//
//		/*----------zjh:step3:set item about time of all processes before run-------------*/
//		quetag[proc->priority] = 1;
//		proc->used_ticks[proc->priority]++;
//		for (int i = 0; i < 4; i++) { // zjh:set all the wait ticks to be 0 when it starts runni
//				proc->waited_ticks[i] = 0;
//		}
//
//		for (t = ptable.proc; t < &ptable.proc[NPROC]; t++)
//		{
//			if (t->state == RUNNABLE && t->pid != proc->pid) {
//					t->waited_ticks[t->priority]++;
//			}
//			if (t->waited_ticks[t->priority] == wait_limit[t->priority] && t->priority != 3) {
//				for (i = 0; i < 4; i++) {  // set all the wait ticks to be 0 when it starts running.
//
//					t->waited_ticks[i] = 0;
//				}
//				t->priority++;
//				if (t->priority > proc->priority)
//					poccurs = 1;
//			}
//		}
//
//		if (proc->used_ticks[proc->priority] % mxtick[proc->priority] == 0 && proc->priority != 0)
//		{//zjh:run out its time slice, priority--
//			quetag[proc->priority] = 0;
//			proc->priority--;
//		}
//
//		/*----------zjh:step4:it's time to run it (some of these are intrinsic)-------------*/
//		switchuvm(proc);
//		proc->state = RUNNING;
//		swtch(&cpu->scheduler, proc->context);
//		switchkvm();
//		if (proc->state != RUNNABLE) {
//			quetag[proc->priority] = 0;
//		}
//		proc = 0;
//		release(&ptable.lock);
//	}
//}


// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int getpinfo(struct pstat * pst){
	if(pst == NULL)return -1;
	struct proc *p;
	int i;
	for(i = 0, p = ptable.proc; i < NPROC && p < &ptable.proc[NPROC]; i++, p++){
		if(p->state == SLEEPING || p->state == RUNNABLE || p->state == RUNNING)
			pst->inuse[i] = 1;
		else pst->inuse[i] = 0;
		pst->pid[i] = p->pid;
		pst->priority[i] = p->priority;
		pst->state[i] = p->state;
		for(int j = 0; j < NPriority; j++){
			pst->ticks[i][j] = p->used_ticks[j];
			pst->wait_ticks[i][j] = p->waited_ticks[j];
		}
	}
	return 0;
}

#include <stdlib.h>
#include <stdint.h>

unsigned int
xchg(volatile unsigned int *addr, unsigned int newval)
{
    unsigned int result;
    asm volatile("lock; xchgl %0, %1" : "+m" (*addr), "=a" (result) : "1" (newval) : "cc");
    return result;
}
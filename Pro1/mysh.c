#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <errno.h>

#define N 512
char error_message[30] = "An error has occurred\n";
char pro_message[] = "mysh> ";

int is_batch, is_redirc, is_bkg, is_buildin;

int n, err, fd_out, fd_STDOUT, status;

pid_t child;

char rawline[N+10], s[N+10];
char cmd[N][N];
char *args[N], *line;

void print_err()
{
	write(STDERR_FILENO, error_message, strlen(error_message));
}

void print_pro()
{
	if(!is_batch)write(STDOUT_FILENO, pro_message, strlen(pro_message));
}

int parse_cmd()
{
	int cnt = 0;
	char* token;
	line = strdup(rawline);
	token = strtok(line, " ");
	while(token != NULL){
		if(strcmp(token, " ") != 0){
			strcpy(cmd[cnt], token);
			cnt++;
		}
		token = strtok(NULL, " ");
	}
	free(line);
	return cnt;
}

int ck_redirc()
{
	err = 0;
	char* token;
	line = strdup(rawline);
	strtok(line, ">");
	strtok(NULL, ">");
	if((token = strtok(NULL, ">")) != NULL){
		printf("%s\n", token);
		err = 1;
		return 0;
	}
	line = strdup(rawline);
	token = strtok(line, ">");
	if(strlen(token) == strlen(rawline))return 0;
	token = strtok(NULL, ">");
	line = strdup(token);
	token = strtok(line, " ");
	fd_out = open(token, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
	if(fd_out < 0){
		err = 1;
		return 0;
	}
	fd_STDOUT = dup(1);
	dup2(fd_out, 1);
	return 1;
}

int ck_bkg()
{
	char* token;
	line = strdup(rawline);
	token = strtok(line, "&");
	strcpy(rawline, token);
	if(strcmp(cmd[n - 1], "&") == 0){
		n--;
		return 1;
	}
	if(cmd[n - 1][strlen(cmd[n - 1]) - 1] == '&'){
		cmd[n - 1][strlen(cmd[n - 1]) - 1] = '\0';
		return 1;
	}

	return 0;
}

int main(int argc, char const *argv[])
{
	FILE* in_file;

	//从batch或cmd读指令
	if(argc == 1){
		is_batch = 0;
		in_file = stdin;
	}
	else if(argc == 2){
		in_file = fopen(argv[1], "r");
		if(in_file == NULL){
			print_err();
			exit(1);
		}
		else{
			is_batch = 1;
		}
	}
	else{
		print_err();
		exit(1);
	}
	while(1){
		is_buildin = 0;
		dup2(fd_STDOUT, 1);
		print_pro();
		if(fgets(rawline, N + 2, in_file) == NULL){
			exit(0);
		}
		if(is_batch) write(STDOUT_FILENO, rawline, strlen(rawline));
		if(rawline[strlen(rawline) - 1] == '\n')rawline[strlen(rawline) - 1] = '\0';
		if(strlen(rawline) > N){
			print_err();
			continue;
		}
		if((n = parse_cmd()) == 0){
			continue;
		}
		// printf("%d\n", n);
		// for(int i=0;i<n;i++)printf("%s ",cmd[i]);printf("\n");
		//Background Jobs
		is_bkg = ck_bkg();
		//Redirection
		is_redirc = ck_redirc();
		if(err){
			print_err();
			dup2(fd_STDOUT, 1);
			continue;
		}

		//exit
		if(strcmp(cmd[0], "exit") == 0){
			if(n != 1)print_err();
			exit(0);
		}
		//pwd
		else if(strcmp(cmd[0], "pwd") == 0){
			if(n != 1){
				print_err();
				dup2(fd_STDOUT, 1);
				continue;
			}
			is_buildin = 1;
			getcwd(s, N);
			write(1, s, strlen(s));
			write(1, "\n", strlen("\n"));
			dup2(fd_STDOUT, 1);
			continue;
		}
		//cd
		else if(strcmp(cmd[0], "cd") == 0){
			is_buildin = 1;
			if(n > 2){
				print_err();
				dup2(fd_STDOUT, 1);
				continue;
			}
			if(n == 1){
				if(chdir(getenv("HOME")) == -1){
					print_err();
					dup2(fd_STDOUT, 1);
					continue;
				}
			}
			else{
				if(chdir(cmd[1]) == -1){
					print_err();
					dup2(fd_STDOUT, 1);
					continue;
				}
			}
		}
		//wait
		else if(strcmp(cmd[0], "wait") == 0){
			if(n != 1){
				print_err();
				dup2(fd_STDOUT, 1);
				continue;
			}
			while (waitpid(-1, NULL, 0)) {
        		if (errno == ECHILD)  break;
      		}
			dup2(fd_STDOUT, 1);
			continue;
		}
		else{
			if(!is_buildin){
				child = fork();
				char* token;
				token= strtok(cmd[n - 1], "&");
				strcpy(cmd[n - 1], token);
				if(child == 0){
					for(int i = 0; i < n; i++){
						line = strdup(cmd[i]);
						token = strtok(line, ">");
						args[i] = token;
						if(token == NULL || strlen(token) != strlen(line)){
							args[i + 1] = NULL;
							break;
						}
					}
					args[n] = NULL;
					execvp(args[0], args);
					print_err();
				}
				else if(child == -1){										
					print_err();
					dup2(fd_STDOUT, 1);
					continue;
				}
				else{
					if(!is_bkg)wait(&status);
				}
			}
		}
		dup2(fd_STDOUT, 1);
	}
	return 0;
}
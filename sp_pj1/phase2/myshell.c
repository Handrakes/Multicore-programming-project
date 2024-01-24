/* $begin shellmain */


#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv, char *cmdline);
int builtin_command(char **argv); 
//pid_t Fork(void);
//void unix_error(char *msg); //for error message
double m_pow(double base, double n);


/*for pipeline*/
void pipe_check(char *cmdline);
int pipe_count;
int parseline_kick = 0;
void pipe_exec(char *buf, char *cmdline, int num, int *prev_fd);

FILE *file;
char prev_command[MAXLINE];
char *why;
char *why1;
char dir[MAXARGS];


int main() 
{
    //char cmdline[MAXLINE]; /* Command line */

    //file = fopen("history.txt", "a+");
 	strcpy(prev_command, "cmdline\n");
	why = getcwd(dir, MAXARGS);
	strcat(dir, "/history_2.txt");

    while (1) {
	/* Read */
	printf("CSE4100-SP-P#4> ");

        char cmdline[MAXLINE];           
	why1 = fgets(cmdline, MAXLINE, stdin); 
	
	if (feof(stdin)){
	    exit(0);
	}
	
	pipe_check(cmdline);
	/* Evaluate */
	
	eval(cmdline);
    } 

}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    	char *argv[MAXARGS]; /* Argument list execve() */
    	char buf[MAXLINE];   /* Holds modified command line */
    	int bg;              /* Should the job run in bg or fg? */
    	pid_t pid;           /* Process id */
	
	char sss[MAXLINE]; /*sss = cmd */
    	file = fopen(dir, "a+");	
	char c;
	c = '!';
    	//strcpy(buf, cmdline); //cmd -> buf
	//bg = parseline(buf, argv);  //명령어 분리buf -> argv
	
	
	//char prev_command[MAXLINE]; //이전 명령어
	int k = 0;//indexing 
	int k_s = 0 ;
	int f = 0;
	int t = 0;
	int flag_flag = 0;
	
	if(!strcmp(cmdline,"!!\n")){
		while(1){//sss 에 마지막 명령어 보관
			char string2[MAXLINE];
			char *str2 = fgets(string2, MAXLINE, file);
			if(str2 != NULL){
				strcpy(sss, str2);
			}
			else
				break;
		}
		strcpy(buf, sss);
		printf("%s", buf);	
	}
	else if(cmdline[0] == c && cmdline[1] != c){
		for(int i = (strlen(cmdline)-2); i > 0; i--){
			k_s += (((int)cmdline[i])-48) * (m_pow(10,f));
			f++;
		}
		rewind(file);
		while(1){
			char string4[MAXLINE];
			char *str4 = fgets(string4, MAXLINE, file);
			//printf("str4: %s",str4);
			if(str4 != NULL){	
				t++;
				if(t == k_s){
					strcpy(sss, str4);
					break;
				}	
			}
			else{
				strcpy(sss, cmdline);
				flag_flag = 1;
				break;
			}		
		}
		strcpy(buf,sss);
		if(flag_flag != 1)
			printf("%s", buf);
	}
	else if(cmdline[0] == c && cmdline[1] == c){
		char test[MAXLINE];
		char tmp_prev[MAXLINE];
		strcpy(tmp_prev, prev_command);
		strcpy(test, cmdline + 2);
		tmp_prev[strlen(tmp_prev) - 1] = '\0';
		strcat(tmp_prev, test);
		strcpy(sss, tmp_prev);
		
		strcpy(buf, sss);
		printf("%s", sss);
	}	
	else{
		strcpy(buf, cmdline);
	}

	//printf("my name is buf! : %s\n", buf);

/*
	int z;
	z = pipe_count + 1;
	if(pipe_count != 0){
		pipe_exec(buf, cmdline ,z,NULL);
		pipe_count = 0;
		parseline_kick = 0;
		return;
	}	
	*/

	int a = 0;
	int b = 0;
	int e = 0;
	if(strcmp(cmdline, "exit\n")){
        	a = strcmp(cmdline, "!!\n");
		b = strcmp(cmdline, prev_command);
		if(cmdline[0] == c && cmdline[1] != c){
			rewind(file);
			fprintf(file, "%s", sss);
			rewind(file);
		}
		else if(!(strcmp(buf, "\n")))
			e = 1;	
                else if((a != 0) && (b != 0 )){
                        rewind(file); 
                        fprintf(file, "%s", buf);
			rewind(file);
                }
        }


	if(!(strcmp(buf, "\n")))
		a = 5;
	else
		strcpy(prev_command, buf);


	//case_* : pipe line processing
	int z;
	z = pipe_count + 1;	
	if(pipe_count != 0){
		pipe_exec(buf,cmdline,z, NULL);
		pipe_count = 0;
		parseline_kick = 0;
		return;
	}


	bg = parseline(buf, argv, cmdline); //buf -> argv
        //printf("flag\n");
	/*
	int a = 0;
	if(strcmp(argv[0], "exit")){
		a = strcmp(cmdline, "!!\n");	
		if(a != 0 ){
    			rewind(file);
			fprintf(file, "%s\n", argv[0]);
		}
	}
*/
    	//case_1 : empty lines//
	if (argv[0] == NULL)  
		return;   /* Ignore empty lines */
    
    	//case_2 : built-in command가 아닌 경우
	if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
    		if(!strcmp(argv[0], "cd")) { //cd가 입력된 경우
			if(argv[1] == NULL){
				int x; //return 값 받아주기 용
				x = chdir(getenv("HOME"));
			}
			else{ //cd 뒤에 경로가 입력된 경우
				int y; 
				y = chdir(argv[1]);
			}
		}	
		else if(!strcmp(argv[0], "history")){ //history가 입력된 경우
			int indexing = 0;
			rewind(file);
			while(1){
				char string[MAXLINE];
				char* str = fgets(string, MAXLINE, file);
				if(str == NULL)
					break;
				indexing++;
				printf("%d  %s", indexing, string);
			}
		}
 		else{ //cd, history 명령어를 제외한 명령어들
			char bin[MAXLINE];
			strcpy(bin, "/bin/");
			strcat(bin, argv[0]); //bin : /bin/cmdline 이 형태 완성
		
			if((pid = Fork()) == 0){//자식 프로세서
				if (execve(bin, argv, environ) < 0) {	//ex) /bin/ls ls -al &
            				printf("%s: Command not found.\n", argv[0]);
            				exit(0);
        			}
			}
			/* Parent waits for foreground job to terminate */
			if (!bg){ //foreground
	    			int status;
				if(waitpid(pid, &status, 0) < 0)
					unix_error("waitpid error\n");
			}
			else//when there is background process!
	    			printf("%d %s", pid, cmdline);
    		}
    	}
	fclose(file);
	//parseline_kick = 0;
    	return;

}

void pipe_exec(char *buf, char *cmdline, int num, int *prev_fd){ //num = commands
	int fd[2];
	pid_t pid;
	int status;
	char *argv_s[MAXARGS]; //명령어를 받아오기 위한 선언
	int bg;

	bg = parseline(buf, argv_s, cmdline);
	

	if(num == 1){ // last command
		pid = fork();
		if(pid == -1){
			perror("fork 1\n");
			exit(1);
		}

		if(pid == 0){
			//child process
			if(prev_fd != NULL){
				dup2(prev_fd[0], STDIN_FILENO);
				close(prev_fd[0]);
			}		
			//printf("%s\n", argv[0]);
			if(execvp(argv_s[0], argv_s) == -1){
				perror("execvp 2\n");
				exit(1);
			}
			//printf("ha\n");
		}
		else{
			//parent process
			waitpid(pid, &status, 0);
			if(prev_fd != NULL){
				close(prev_fd[0]);
			}
		}
	}
	else{ 
		//recursive case
		if(pipe(fd) == -1){
			perror("pipe 3\n");
			exit(1);
		}

		pid = fork();
		if(pid == -1){
			perror("fork 4\n");
			exit(1);
		}
		if(pid == 0){
			//child process
			if(prev_fd != NULL){
				dup2(prev_fd[0], STDIN_FILENO);
				close(prev_fd[0]);
			}	
	
			dup2(fd[1], STDOUT_FILENO);
			close(fd[1]);

			if(execvp(argv_s[0], argv_s) == -1){
				perror("execvp 5\n");
				exit(1);
			}
		}
		else{
			//parent process
			close(fd[1]);
			pipe_exec(buf, cmdline, num - 1, fd);
			waitpid(pid, &status, 0);
			close(fd[0]);
		}
	}
}

void pipe_check(char *cmdline){
	pipe_count = 0; // pipe_count 초기화
	char c;
	c = '\n';
	for(int i=0; cmdline[i] != c ; i++){
		if(cmdline[i] == '|')
			pipe_count++;
	}
}

/*
void unix_error(char *msg){
	//에러메세지 출력 함수
	printf("%s : %s\n", msg, strerror(errno));
	exit(0);
}

pid_t Fork(void){
	pid_t pid;
	pid = fork();
	if(pid < 0){
		unix_error("Fork error");
	}

	return pid;
}
*/
double m_pow(double n, double m)
{
     double nn = n;
     if (m == 0)
       return 1.0;
     for (int i = 0; i < (m-1); i++)
         n *= nn;
     return n;
 }




/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) //cd, exit, history 는 built in command
{
   // if (!strcmp(argv[0], "quit")) /* quit command */
	//exit(0);  
    if (!strcmp(argv[0], "exit")) //terminate when "exit"
	exit(0);
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	return 1;
	
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv, char *cmdline) 
{
	
	if(parseline_kick == 0){ //처음 parseline으로 진입
		if(pipe_count != 0){
			buf = strtok(cmdline, "|");
			//printf("1st: %s\n", buf);
			parseline_kick++;
		}
	}
	else{
		buf = strtok(NULL, "|");
		//printf("2nd: %s\n", buf);
	}

    	char *delim;         /* Points to first space delimiter */
    	int argc;            /* Number of args */
    	int bg;              /* Background job? */

	char tmp[MAXARGS]; /* for " and '*/

    	buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    	while (*buf && (*buf == ' ')) /* Ignore leading spaces */
		buf++;

    	/* Build the argv list */
    	argc = 0;
    	while ((delim = strchr(buf, ' '))) {
		//printf("%s : %d\n", buf, (int)strlen(buf));
		if((buf[0] == 96) || (buf[0] == 34)){
			//printf("i'm in!!\n");
			strncpy(tmp, buf + 1, (int)strlen(buf) - 3);
			//printf("%s\n", tmp);
			tmp[(int)strlen(buf) - 3] = '\0';
			//printf("%s\n", tmp);
			strcpy(buf, tmp);
		}

		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* Ignore spaces */
            		buf++;
    	}
    	
	argv[argc] = NULL;
    
    	if (argc == 0)  /* Ignore blank line */
		return 1;

    	/* Should the job run in the background? */
    	if ((bg = (*argv[argc-1] == '&')) != 0)
		argv[--argc] = NULL;

///    return bg;

//	printf("%s\n", argv[0]);
//	printf("%s\n", argv[1]);
//	printf("argc : %d\n", argc);
	
	return bg;
}
/* $end parseline */


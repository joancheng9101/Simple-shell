#ifndef _POSIX_C_SOURCE
	#define  _POSIX_C_SOURCE 200809L
#endif

#define COMMAND_SIZE 1000
#define TOKEN_LONG 50
#define TOKEN_NUM 500

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>     //file op

bool inshell = true;
bool inbg = false;
int pip = 0;
//bool notbi = false;
char record[17][TOKEN_NUM];
//char result[16][TOKEN_NUM];
int record_index = 0 ;
//char buffer[2048];
char bg_temp[TOKEN_NUM]; 
//char not_buildin[TOKEN_LONG];

void hello(void);
void read_command(void);
void remove_record(void);
void split_command(char *command);
void analyze_command(char token[TOKEN_NUM][TOKEN_LONG] , bool bg, bool Ir, bool Or , int pip, int token_num );
void choose_function(char *token_command[TOKEN_LONG],int pip, bool Ir, bool Or);
void help(void);
void cd(char* directory);
void echo(int n , char *token_command[TOKEN_LONG], int flag);
void record_command(void);
void reply(int num,int pip,bool Ir,bool Or,bool bg);
void mypid(int mode,char* num);
void write_file(char* directory);
void exit_word(void);

int main(int argc, char **argv){
	hello();
	while(inshell){
		if(!inbg){
			read_command();
		}
		else{
			split_command(bg_temp);
			return 0;
		}
	}
  	return 0;
}

void hello(void){
	 printf("=================================================\n");
     printf("* Wellcome to my little shell:                  *\n");
	 printf("*                                               *\n");
	 printf("* Type ""help"" to see builtin functions.           *\n");
	 printf("*                                               *\n");
     printf("* If you want to do thing below:                *\n");
	 printf("* + redirection: "">"" or ""<""                         *\n");
	 printf("* + pipe: ""|""                                     *\n");
	 printf("* + background: ""&""                               *\n");
     printf("* Make sure they are seperated by ""(space)""       *\n");
	 printf("*                                               *\n");
	 printf("* Have fun!!                                    *\n");
     printf("=================================================\n");
}

void read_command(void){
	char command[COMMAND_SIZE];
	int ch,i=0;
	printf(">>> $ ");
	while((ch = getchar()) != '\n') command[i++] = ch;
	command[i] = '\0';
	if(record_index <= 16){
		strcpy(record[record_index],command);
	}else{
		record_index = 16;
		remove_record();
		strcpy(record[record_index],command);
	}
	split_command(record[record_index++]);
}

void remove_record(void){
	for(int i=0 ; i<17 ; i++){
		strcpy(record[i],record[i+1]);
	}
}

void split_command(char *command){
	int c=1, i=0, j=1;
	char token[TOKEN_NUM][TOKEN_LONG];
	bool bg = false, Ir = false, Or = false;
	int pip=0;
	token[0][0] = command[0];
	while(command[c]!='\0'){
		if(command[c]=='&') bg = true;
		else if(command[c]=='|') pip++;
		else if(command[c]=='<') Ir = true;
		else if(command[c]=='>') Or = true;
		if(command[c]==' '){
			token[i++][j] = '\0';
			j = 0;
			c++;
		}else{
			token[i][j++]= command[c++];
		}
	}
	token[i][j] = '\0';
	analyze_command(token,bg,Ir,Or,pip,i+1);
}

void analyze_command(char token[TOKEN_NUM][TOKEN_LONG] , bool bg, bool Ir, bool Or , int pip, int token_num ){
	if(bg){
		if(strcmp(token[0],"reply")==0){
			reply(atoi(token[1]),pip,Ir,Or,bg);
			bg = false;
		}else{
			strcpy(bg_temp,record[record_index-1]);
			bg_temp[strlen(bg_temp)-2] = '\0';  // no space and &
			inbg = true;
			pid_t ret,dead;
			int status;
			ret = fork();
			if(ret <0){
				fprintf(stderr,"Fork Failed\n");
				exit(-1);
			}else if(ret ==0){  //child
				;
			}else{   // parent
				printf("[Pid]: %d\n",ret);
				inbg = false;
			}
		}
	}else{
		char *token_command[TOKEN_LONG];
		int t = 0, c=0;
		int p = 0;
		int command_num = pip + 1;
		int tokens_num = token_num;
		bool have_reply = false;
		if(pip==0 && !Ir && !Or){
			if(strcmp(token[0]," ")==0) return;
			if(strcmp(token[0],"\t")==0) return;
			if(strcmp(token[0],"")==0) return;
			int i = 0;
			while(i<tokens_num){
				token_command[i] = token[i];
				i++;
			}
			token_command[i] = NULL;
			choose_function(token_command,pip,Ir,Or);
		}else if (pip == 0 && Ir && !Or){   
			c = 0;
			t = 0;
			while(strcmp(token[t],"<")!=0){
				token_command[c] = token[t];
				c++;
				t++;
			}
			t++;
			token_command[c] = token[t];  //input_file
			c++;
			token_command[c] = NULL;
			choose_function(token_command,pip,Ir,Or);
		}else if(pip == 0 && !Ir && Or){
			c = 0;
			t = 0;
			while(strcmp(token[t],">")!=0){
				token_command[c] = token[t];
				c++; t++;
			}
			token_command[c] = NULL;
			t++;
			write_file(token[t]);
			choose_function(token_command,pip,Ir,Or);
		}else if(pip == 0 && Ir && Or){
			c = 0;
			t = 0;
			while(strcmp(token[t],"<")!=0){
				token_command[c] = token[t];
				c++;
				t++;
			}
			t++;
			token_command[c] = token[t];  //input_file
			c++;
			token_command[c] = NULL;
			t+=2; //output_file
			write_file(token[t]);
			choose_function(token_command,pip,Ir,Or);
		}
		while(command_num > 0 && pip > 0){ //have pip
			if(Ir && p==0){ //first
				c = 0;
				t = 0;
				while(strcmp(token[t],"<")!=0){
					token_command[c] = token[t];
					c++;
					t++;
				}
				t++;
				token_command[c] = token[t];  //input_file
				c++;
				t+=2;
				token_command[c] = NULL;
			}else if(Or && command_num == 1 ){ //last
				c = 0;
				while(strcmp(token[t],">")!=0){
					token_command[c] = token[t];
					c++; t++;
				}
				token_command[c] = NULL;
				t++;
				write_file(token[t]);
			}else{ 
				c = 0;
				while(strcmp(token[t],"|")!=0){
					token_command[c] = token[t];
					c++; t++;
					if(t>=tokens_num) break;
				}
				token_command[c] = NULL;
				t++;
			} 
			if(strcmp(token_command[0],"reply")==0){
					have_reply = true;
					break;
			}
			int pipefd_odd[2];
			int pipefd_even[2];
			pid_t cpid;
			int cstatus;
			if(p % 2 != 0){
				if(pipe(pipefd_even) == -1){
					perror("pipe");
					exit(EXIT_FAILURE);
				}
			}else{
				if(pipe(pipefd_odd) == -1){
					perror("pipe");
					exit(EXIT_FAILURE);
				}
			}
			if(strcmp(token_command[0],"help")!=0
			&& strcmp(token_command[0],"cd")!=0
			&& strcmp(token_command[0],"echo")!=0
			&& strcmp(token_command[0],"record")!=0
			&& strcmp(token_command[0],"reply")!=0
			&& strcmp(token_command[0],"mypid")!=0
			&& strcmp(token_command[0],"exit")!=0){  // not build-in
				cpid = fork();
				if(cpid==-1){
					perror("fork");
					exit(EXIT_FAILURE);
				}else if(cpid==0){   // child
					if(p == 0){  // first     
						dup2(pipefd_odd[1],STDOUT_FILENO);
					}else if(command_num == 1){  //last
						if(!Or) write_file("/dev/tty");
						if(p % 2 != 0) dup2(pipefd_odd[0],STDIN_FILENO);
						else dup2(pipefd_even[0],STDIN_FILENO);
					}else{  // middle
						if(p % 2 != 0){
							dup2(pipefd_odd[0],STDIN_FILENO);
							dup2(pipefd_even[1],STDOUT_FILENO);
						}else{
							dup2(pipefd_even[0],STDIN_FILENO);
							dup2(pipefd_odd[1],STDOUT_FILENO);
						}
					} 
					if (execvp(token_command[0],(char *const *)token_command)==-1){
						exit(0);
					}
				}
				if(p==0){  //first
					close(pipefd_odd[1]);
				}else if(command_num == 1){ //last
					if(p % 2 != 0) close(pipefd_odd[0]);
					else close(pipefd_even[0]);
				}else{
					if(p % 2 != 0){
						close(pipefd_odd[0]);
						close(pipefd_even[1]);
					}else{
						close(pipefd_even[0]);
						close(pipefd_odd[1]);
					}
				}
				wait(&cstatus);
			}else{ // build-in (only pipeout)
				dup2(pipefd_odd[1],STDOUT_FILENO);
				choose_function(token_command,pip,Ir,Or);
				close(pipefd_odd[1]);
			}
			p++;
			command_num--;
		}
		if(have_reply) choose_function(token_command,pip,Ir,Or);
		else write_file("/dev/tty");
	}
}

void choose_function(char *token_command[TOKEN_LONG],int pip, bool Ir, bool Or){
	if(strcmp(token_command[0],"help")==0){
		help();
	}else if(strcmp(token_command[0],"cd")==0){
		cd(token_command[1]);
	}else if(strcmp(token_command[0],"echo")==0){
		if(strcmp(token_command[1],"-n")==0) echo(1,token_command,2);
		else echo(0, token_command, 1);
	}else if(strcmp(token_command[0],"record")==0){
		record_command();
	}else if(strcmp(token_command[0],"reply")==0){
		if(strcmp(token_command[1],"1")==0 || strcmp(token_command[1],"1")==0 ||
		strcmp(token_command[1],"2")==0 || strcmp(token_command[1],"3")==0 || 
		strcmp(token_command[1],"4")==0 || strcmp(token_command[1],"5")==0 || 
		strcmp(token_command[1],"6")==0 || strcmp(token_command[1],"7")==0 || 
		strcmp(token_command[1],"8")==0 || strcmp(token_command[1],"9")==0 ||
		strcmp(token_command[1],"10")==0 || strcmp(token_command[1],"11")==0 ||
		strcmp(token_command[1],"12")==0 || strcmp(token_command[1],"13")==0 || 
		strcmp(token_command[1],"14")==0 || strcmp(token_command[1],"15")==0 ||
		strcmp(token_command[1],"16")==0){
			reply(atoi(token_command[1]),pip,Ir,Or,false);
		}else{
			printf("wrong args\n");
			if(record_index >= 17 ){
				for(int j = 16 ; j>0 ; j-- ){
					strcpy(record[j],record[j-1]);
				}
			}else{
				record_index -- ;
			}
		}
	}else if(strcmp(token_command[0],"mypid")==0){
		if(strcmp(token_command[1],"-i")==0){
			mypid(1,0);
		}else if(strcmp(token_command[1],"-p")==0){
			mypid(2,token_command[2]);
		}else if(strcmp(token_command[1],"-c")==0){
			mypid(3,token_command[2]);
		}
	}else if(strcmp(token_command[0],"exit")==0){
		exit_word();
	}else{  //not build-in
		pid_t ret,dead;
		int status;
		ret = fork();
		if(ret < 0){
			fprintf(stderr,"Fork Failed");
			exit(-1);
		}else if(ret == 0){
			if (execvp(token_command[0],(char *const *)token_command)==-1){
				exit(0);
			}
		}else{
			dead = wait(&status);
		}
	}			
}

void help(void){
	 printf("=================================================\n");
     printf("My little shell:\n");
	 printf("Type program names and arguments, and hit enter\n");
	 printf("The follow in are built in\n");
	 printf("1. help:   show all built-in function info\n");
     printf("2: cd:     change directory\n");
	 printf("3: echo:   echo the string to standard output\n");
	 printf("4: record: show last-16 cmds you typed in\n");
	 printf("5: reply:  re-execute the cmd showed in record\n");
     printf("6. mypid:  find and print process-ids\n");
	 printf("7. exit:   exit shell\n");
     printf("===================================================\n");
}

void cd(char* directory){
	if(chdir(directory) != 0){
		printf("no such directory\n");
	}
}

void echo(int n , char *token_command[TOKEN_LONG], int flag){
	int t= flag;  // point to start
	while(token_command[t]!= NULL){
		if(t==flag) {
			printf("%s",token_command[t++]); 
		}else {
			printf(" %s",token_command[t++]);
		}
	}
	if (n) ;
	else printf("\n");  
}

void record_command(void){
	printf("history cmd:\n");  
	if(record_index >= 17){
		for(int i=1 ; i < record_index ; i++){
			printf("%2d: %s\n",i,record[i]);
		}
	}else{
		for(int i=0 ; i < record_index ; i++){
			printf("%2d: %s\n",i+1,record[i]);
		}
	}
}

void reply(int num,int pip,bool Ir,bool Or,bool bg){
	int i = record_index-1; 
	char buffer[COMMAND_SIZE];
	int r1=0, r2=0;
	int re_long;
	if(pip==0 && !Ir && !Or){
		if(bg){
			re_long = strlen(record[num-1]);
			strcpy(record[i],record[num-1]);
			record[i][re_long] = ' ';
			record[i][re_long+1] = '&';
			record[i][re_long+2] = '\0';
			split_command(record[i]);
		}else{
			strcpy(record[i],record[num-1]);
			split_command(record[num-1]);
		}
	}else if (pip == 0 && Ir && !Or){   
		; // build-in input redirection 
	}else if(pip == 0 && Or){
		r1 = 0;
		r2 = 0;
		strcpy(buffer,record[num-1]);
		while(record[i][r1++] != '>') ;
		r2 = strlen(record[num-1]);
		buffer[r2++] = ' ';
		buffer[r2++] = '>';
		while(record[i][r1] != '\0'){
			buffer[r2] = record[i][r1];
			r1++; r2++;
		}
		buffer[r2] = '\0';
		strcpy(record[i],buffer);
		split_command(record[i]);
	}else if(pip){
		r1 = 0;
		r2 = 0;
		strcpy(buffer,record[num-1]);
		while(record[i][r1++] != '|') ;
		r2 = strlen(record[num-1]);
		buffer[r2++] = ' ';
		buffer[r2++] = '|';
		while(record[i][r1] != '\0'){
			buffer[r2] = record[i][r1];
			r1++; r2++;
		}
		buffer[r2] = '\0';
		strcpy(record[i],buffer);
		split_command(record[i]);
	}
}

void mypid(int mode,char* num){
	pid_t pid;
	char path[100] = "/proc/";
	char one[10];
	char two[10];
	char three[10];
	char child_buffer[100];
	char four[10];
	switch (mode)
	{
	case 1:  //my
		pid = getpid();
		printf("%d\n",pid);
		break;
	case 2:    //parent
		strcat(path,num);
		strcat(path,"/stat");
		FILE *parent = fopen(path,"r");
		if(parent == NULL){
			printf("process id not exit\n");
		}else{
			fscanf(parent,"%s %s %s %s",one,two,three,four);
			printf("%s\n",four);
		}
		break;
	case 3:    //children
		strcat(path,num);
		strcat(path,"/task/");
		strcat(path,num);
		strcat(path,"/children");
		int children = open(path,O_RDONLY);
		int i;
		if(children == -1){
			printf("process id not exit\n");
		}else{
			while((i = read(children, child_buffer, sizeof(child_buffer)))>0){
				printf("%s\n",child_buffer);
			}
		}
		break;
	default:
		break;
	}
}

void write_file(char* directory){
	int file_ouput;
	file_ouput = open(directory, O_CREAT | O_TRUNC | O_WRONLY, 0600);  //0600 mean read(4) & write(2)
	dup2(file_ouput,STDOUT_FILENO);
	close(file_ouput);
}

void exit_word(void){
	printf("my little shell: See you next time\n");
	inshell = false;
}
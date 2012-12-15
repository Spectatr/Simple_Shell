/* Authors:
	Andrew 
	Christopher 
	Kevin 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

/*Declare static array that holds built in functions.*/
struct func_list
{
	char *name;
	int (*func)(int, char**);
};

void runsource(int pfd[],char* ARG,char**srcARG,int numcmmds){ 
	int pid;
	int i=0;

	switch (pid = fork()) { 
		case 0: /* child */ 
			dup2(pfd[1], 1); 
			while(i<(2*(numcmmds-1))){	/*closes all pipes*/
				close(pfd[i]);
				i++;
			}
			execvp(ARG,srcARG); 
			perror(ARG); 
		default:
			break; 
		case -1: 
			perror("fork"); 
			exit(1); 
	} 
} 

void rundest(int pfd[],char* ARG,char** srcARG,int numcmmds){
	int pid; 
	int i=0;

	switch (pid = fork()) { 
		case 0: 
			dup2(pfd[((numcmmds-1)*2)-2], 0);
			while(i<(2*(numcmmds-1))){	/*closes all pipes*/
				close(pfd[i]);
				i++;
			}
			execvp(ARG,srcARG);  
			perror(ARG);  
		default:  
			break; 
		case -1: 
			perror("fork"); exit(1); 
	} 
}

/*this runs only when there are 3 or more commands*/
void runmiddle(int pfd[],char* ARG,char**srcARG,int numcmmds,int i){ 
	int pid;
	int j=0;

	switch (pid = fork()) { 
		case 0: /* child */ 
			dup2(pfd[(i*2)-2],0);	/*Based on the number of commands, this will bridge the pipes*/
			dup2(pfd[(i*2)+1],1); 
			while(j<(2*(numcmmds-1))){
				close(pfd[j]);
				j++;
			}
			execvp(ARG,srcARG); 
			perror(ARG); 
		default:
			break; 
		case -1: 
			perror("fork"); 
			exit(1); 
	} 
} 

int myCD(int argc, char **argv){	/*Our implementation of the "cd" command*/
	int result;
	char* home;
	char *pathname;
	char buf[1024];
	
	if(argc==1){	/*If theres no arguements, it defaults to home*/
		home=getenv("HOME");
		argv[0]=home;
		argc++;
	}
	if(argc!=2){
		fprintf(stderr," cd Error: Too many arguments\n");
		return;
	}
	if((result = chdir(argv[0])) < 0){
		fprintf(stderr,"Change Working Directory Error\n");
	}
	if((pathname = getcwd(buf,sizeof(buf))) == NULL)
		fprintf(stderr,"Get Current Working Directory Error\n");
}

int myExit(int argc, char **argv){	/*Our implementation of the "exit" command*/
    exit(EXIT_SUCCESS);
}

static struct func_list myfuncs[] = {	/*This is a structure of pointers of our*/
	{	.name = "cd",				 	/*built in commands*/
		.func = myCD
	},
	{	.name = "exit",
		.func = myExit
	},
	{	.name = NULL
	}
};

void runCommand(char ***cmdArr){		/*This runs the commands after they have been tokenized*/
	int i=0,x;							/*using the execvp, fork, pipe and dup2 system calls*/
	int numcmmds=0;
	int numarg=0;
	int notinfunclist=1;
	char *func_name = cmdArr[0][0];	
	int j=0;

	struct func_list *curr;
	while(cmdArr[i][0]){	/*Counts the number of commands*/
		numcmmds++;
		i++;
	}
	if(numcmmds==0){	/*If there is no input, then just do nothing*/
		return;
	}
	while(cmdArr[0][0][i]){		/*If there are no letters, ex only blank spaces and tabs, then do nothing*/
		x=isalpha(cmdArr[0][0][i]);
		i++;
	}
	if(x==0) return;
	i=0;	/*resets i*/
	while(cmdArr[0][i]){	/*Counts the number of arguements in each command*/
		numarg++;
		i++;
	}
	i=0;
	for(curr = myfuncs; curr->name != NULL; curr++){	/*This tests for the built in functions*/
		if(strcmp(curr->name, func_name) == 0)
		{
			curr->func(numarg, &cmdArr[0][1]);
			return;
		}
	}
	i=0;
	if(numcmmds==1){	/*If there is only one process, then execute the following code*/
		int pid = fork();
		if(pid){		/*parent process*/
			int childStatus;
			int wake_pid;
			wake_pid=wait(&childStatus);
			printf("process %i exits with 0\n",wake_pid);
		}
		else if(!pid){		/*child process*/
			execvp(cmdArr[0][0],cmdArr[0]);
			perror("execvp");
			exit(1);
		}
		else{
			printf("\n...\n");	
		}
	}
	else if(numcmmds>2){	/*If there are more then two commands, then...*/
		int fd[2*(numcmmds-1)];
		int pid, status; 
		for(i=0;i<numcmmds*2;i+=2){ /*Makes X number of pipes depending on how many commands there are*/
			pipe(fd+i);
		}
		runsource(fd,cmdArr[0][0],cmdArr[0],numcmmds);/*Runs source only once*/
		for(i=1;i<numcmmds-1;i++){
			runmiddle(fd,cmdArr[i][0],cmdArr[i],numcmmds,i);/*Runs middle based on how many cmmd*/
		}													/*there aree*/
		rundest(fd,cmdArr[i][0],cmdArr[i],numcmmds);		/*Runs destination process only once to*/
		i=0;												/*end the piping*/
		while(i<(2*(numcmmds-1))){	/*Closes all pipes opened*/
			close(fd[i]);
			i++;
		}
		while ((pid = wait(&status)) != -1) {
			fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status)); 
		}
	}
	else if(numcmmds==2){	/*If there are only two process, then pipe using simple logic*/ 
		int fd[2];
		int pid, status; 
		pipe(fd);
		runsource(fd,cmdArr[0][0],cmdArr[0],numcmmds);
		i=1;
		rundest(fd,cmdArr[i][0],cmdArr[i],numcmmds);
		close(fd[0]);
		close(fd[1]);
		while ((pid = wait(&status)) != -1) {
			fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status)); 
		}
	}
	else{	/*In case of unforsee circumstances*/
		printf("Wrong number of commands");
	}
	j=0;
	/*for(i=0;i<numcmmds;i++){		this doesnt work and we didnt have enough time to figure it out.
		for(j=0;j<numarg;j++){		we know that we need to free each element, and not just 
			free(cmdArr[i][j]); 	the array holding everything beacuse that just deletes the memory location
		}							of where the tokens/etc start, and not what we malloced earlier.
	}*/
}

char ***getCommand(char ** toks){	/*Groups tokens in between pipe symbols from the tokens*/
	int cmmds; /*current command array*/
	int arg_c; /*current token*/
	char ***holder = malloc(50 * sizeof(char**)); /*holds all the command arrays*/
	int i=0;

	for(cmmds=0; cmmds<50; cmmds++){
		holder[cmmds] = malloc(30 * sizeof(char*));
		for(arg_c=0; arg_c <80; arg_c++){
			holder[cmmds][arg_c] = malloc(50 * sizeof(char));
		}
	}
	i=0;
	cmmds=0;
	arg_c =0;
	while(toks[arg_c][0] != 0){
		if(toks[arg_c][0] == '|'){ /*end current command array start another*/
			holder[cmmds][i]='\0';
			cmmds++;			
			i=0;
		}
		else{
			holder[cmmds][i]= toks[arg_c];
			i++;
		}
		arg_c++;
	}
	holder[cmmds][i]='\0';
	holder[cmmds+1][0]='\0';
	return holder;
}

void bracketchecker(char* line){	/*Checks for unbalanced single and double quotes*/
	int i=0;
	char *ch=line;
	int singlequotecounter=0;
	int doublequotecounter=0;
	while(*ch){
		if(*ch=='\'') singlequotecounter++;
		if(*ch=='\"') doublequotecounter++;
		ch++;
	}
	if(singlequotecounter%2 != 0 || doublequotecounter%2 != 0){
		printf("Unbalanced quotes\n");
		exit(0);
	}
}

char **getToken(char * line){/*Tokenizes input lines*/
	char *ch = line;
	int argc = 0;
	char **argv = malloc(51 * sizeof(char*));
	int i;
	int started = 0;
	char delimiter;
	
	bracketchecker(line);	/*Checks if thesre are unbalanced quotes*/
	for(i = 0; i < 51; i++)
		argv[i] = malloc(80 * sizeof(char));
	i = 0;
	while(1){
		if(argc>=49){ /*Gracefully exit if too many input arguments*/
			printf("Too many arguements\n");
			exit(0);
		}
		if(*ch == ' ' && !started){ /*ignore white space*/
			/*ignore*/
		}
		else if(started && *ch == delimiter){ /*finish token*/
			started = 0;
			argv[argc][i] = '\0';
			i = 0;
			argc++;
		}
		else if(*ch == '|' && (!started || delimiter == ' ')){ /* end current command and token*/
			if(started){
				/*add next args to next command*/
				argv[argc][i] = '\0';
				argc++;
				started =0;
			}
			argv[argc][0] = *ch;
			argv[argc][1] = '\0';
			argc++;
			i = 0;
		}
		else if((*ch == '\'' || *ch == '\"') && !started){ /*start token with " or ' delimiter*/
			started = 1;
			delimiter = *ch;
		}
		else if(*ch !='\n' && !started){ /*start token normal*/
			started = 1;
			delimiter = ' ';
			argv[argc][i] = *ch;
			i++;
		}
		else if(started && *ch == '\n'){  /*end of line, end of current command last arg had delimitor space*/
			argv[argc][i] = '\0';
			break;
		}
		else if(*ch == '\n'){ /*finish collecting tokens for given line*/
			break;
		}
		else if(started && (*ch == '\'' || *ch == '\"')){ /*to cover the case the current token delimiter */
			if(delimiter == *ch){							/*is either a ' or " and either end the token*/
				started=1;									/*ignore the delimiter*/
				argv[argc][i] = '\0';
				i = 0;
				argc++;
			}
			argv[argc][i] = *ch;
			i++;	
		}
		else if(started && *ch != delimiter){ /*token has been started but another potential */
			argv[argc][i] = *ch;				/*delimiter has been encountered*/
			i++;
		}
		else{ /*Error collecting token*/
			printf("something went wrong on character argv[%d][%d]\n", argc, i); 
		}
		ch++;	
	}
	argc++;
	argv[argc][0]=0;
	return argv;
}

void cwd(){ /*This function returns the current working directory*/
	char* cwd=(char*) malloc(sizeof(char)*50);

	cwd=getcwd(NULL,0);
	if(cwd!=NULL){
		printf("**%s**: ",cwd);
	}
	else{
		printf("unknown cwd:$$");
	}
	free(cwd);
}

int main(void){ /*MAIN FUNCTION*/
	char linebuf[1024];
	char* ret;

	if(isatty(0)) cwd();
	while((ret=fgets(linebuf, sizeof(linebuf), stdin))!=NULL){
		/*fprintf(stdout,"got line\n");*/
		
		char** tokens = getToken(linebuf);
		/*fprintf(stdout,"got tokens\n");*/

		char*** cmds = getCommand(tokens);
		/*fprintf(stdout,"\ngot commands\n");*/
		
		runCommand(cmds);
		/*fprintf(stdout,"ran command\n");*/

		if(isatty(0)) cwd();
	}
	return 0;
}
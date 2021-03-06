#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <signal.h>

int pidarray[1000];
int pidarraycount;
int numpids;
int exitstatus;
int lastforeground;
int pidskilled;
int skip;
int ignored;
static volatile int keeprunning = 1;

struct LinkedList{
	int data;
	struct LinkedList *next;
};
typedef struct LinkedList *node;


//le o input de string de quantidade desconhecida e retorna um array char
char *inputString(FILE* fp, size_t size){
	char *str;
	int ch;
	size_t len = 0;
	str = realloc(NULL, sizeof(char)*size);
	if(!str)return str;
	while(EOF!=(ch=fgetc(fp)) && ch != '\n'){
		str[len++]=ch;
		if(len==size){
			str = realloc(str, sizeof(char)*(size+=16));
			if(!str)return str;
		}
	}
	str[len++]='\0';
	return realloc(str, sizeof(char)*len);
}

//pega ctrl-c e termina um processo
void myhandler(int dummy) {
	printf("terminated by signal %d\n", dummy);
}

//pega ctrl-z, e troca entre background mode/normal mode
void myzhandler(int dummy){
	if(ignored == 0){
		printf("Entrando no modo primeiro plano (& agora eh ignorado)\n");
		ignored = 1;
	}else if(ignored == 1){
		printf("Saindo do modo de primeiro plano\n");
		ignored = 0;
	}
}

int main(){
	//lida com ctrl-z ctrl-c
	ignored = 0;
	struct sigaction sigih;
	sigih.sa_handler = myhandler;
	sigemptyset(&sigih.sa_mask);
	sigih.sa_flags = 0;
	sigaction(SIGINT, &sigih, NULL);
	sigih.sa_handler = myzhandler;
	sigemptyset(&sigih.sa_mask);
	sigih.sa_flags = 0;
	sigaction(SIGTSTP, &sigih, NULL);


	//acompanha processos em segundo plano
	pidskilled = 0;
	pidarraycount = 0;
	char *line;
	char *backup;
	int exitv = 0;
	int exitsignal = 0;
	int terminatedby = 0;

	//while para o input do usuario
 
	while(exitv == 0){
		skip = 0;

		//checa pidarray, se um processo foi terminado imprime PID
		int j = 0;
		int status2;

		
 	//se tem pids no pidarray, passa pelo vetor e imprime os que foram terminados
 		
		if(numpids > 0){
			for(j = 0; j < pidarraycount; j++){
				pid_t result = waitpid(pidarray[j], &status2, WNOHANG);
				if(pidarray[j] != 0){
					if(result != 0){
						//processo terminou, mata e remove do vetor
						printf("background pid %d terminou: exit value %d\n", result, status2);
						kill(pidarray[j], SIGKILL);
						pidskilled++;
						pidarray[j] = 0;
					}

				}
			}
		}
		printf("Qual o seu comando? ");   //inicia o shell

  //manda stdin para funcao onde retorna o input como char arrayof de tamanho correto

		line = inputString(stdin, 10);
		fflush(stdin);
		fflush(stdout);

 //analisa uma linha em suas diferentes possibilidades, como primeira palavra, primeiros 6 chars, ultima letra, etc
 
		if(strcmp("exit $", line)==0){
			exitv++;
		}

		int L1 = strstr(line, "$$")-line;
		int L2 = (strstr(line, "$$")-line)+1;

		if(L1 > 0 && L2 > 0){
			line[L1] = '%';
			line[L2] = 'd';
			char expandinput[200];
			sprintf(expandinput, line, getpid());
			strncpy(line, expandinput, 50);
		}

//analisa linha individualmente para caso de caracteres especiais
		char firstLetter[1];
		char temp = line[0];
		firstLetter[0] = temp;
		firstLetter[1] = '\0';
		char firstTwoLetters[2] = "  ";
		if(strlen(line) >= 2){
			firstTwoLetters[0] = line[0];
			firstTwoLetters[1] = line[1];
			firstTwoLetters[2] = '\0';
		}
		char * space;
		int spaces = 1;
		char * spaceCount;
		space=strchr(line, ' ');
		spaceCount=strchr(line, ' ');
		char lastletter[2] = "  ";
		int lastSpaceCount = 0;
		while (spaceCount!=NULL){
				spaces++;
				lastSpaceCount = spaceCount-line;
				spaceCount=strchr(spaceCount+1, ' ');
		}
		lastletter[1] = '\0';
		memcpy(lastletter, &line[strlen(line)-1], lastSpaceCount);
		char linearray[spaces+1][200];
		spaceCount=strchr(line, ' ');
		spaces = 0;
		lastSpaceCount = 0;
		int prev = 0;
		char indv[1024];
		while (spaceCount!=NULL){
			lastSpaceCount = spaceCount-line;
			indv[1024];
			memcpy(indv, &line[prev], lastSpaceCount);
			indv[lastSpaceCount - prev] = '\0';
			strcpy(linearray[spaces], indv);
			prev = lastSpaceCount +1;
			spaces++;
			spaceCount=strchr(spaceCount+1, ' ');
		}
		memcpy(indv, &line[prev], strlen(line));
		indv[strlen(line) - prev] = '\0';
		strcpy(linearray[spaces], indv);
		int i = 0;
		char *args2[spaces+1];
		for(i=0; i <= spaces; i++){
			args2[i] = linearray[i];
		}
		args2[spaces+1] = NULL;
		char firstword[2];
		if(space-line > 0){
			firstword[space-line];
			memcpy(firstword, &line[0], space-line);
			firstword[space-line] = '\0';
		}
		char firstSix[6] = "      ";
		if(strlen(line) >= 6){
			memcpy(firstSix, &line[3], strlen(line));
		}

 //detecta se o input e um comando builtin
		if(strcmp(line, "fim")==0){
			exitv++;
		}else if(strcmp(firstTwoLetters, "cd")==0){
			if(strlen(line) == 2){
				const char* s = getenv("HOME");
				chdir(s);
			}else{
				char dir[strlen(line)-3];
				memcpy(dir, &line[3], strlen(line));
				chdir(dir);
			}
		}else if(strcmp(args2[0], "status")==0){
			if(lastforeground != 1){
				printf("valor de saida %d\n", exitstatus);
			}else{
				printf("terminado pelo sinal %d\n", terminatedby);
			}
		}else if(strcmp(firstLetter, "#")!=0){

//se input nao for builtin:
// determina qualquer redirecionamento i/o a ser feito

//acha i_place (onde <= ocorre") e o_place (onde => ocorre)

			i = 0;
			int i_place = 0;
			int o_file = 0;
			int o_place = 0;
			int OS_string_end = 0;
			for(i=0; i<= spaces; i++){
				if(strcmp(args2[i], "<=")==0){
					i_place = i;
				}else if(strcmp(args2[i], "=>")==0){
					o_place = i;
					o_file = i+1;
				}
			}
//cria e prenche o vetor os_string
			spaces++;
			char *OS_string[spaces];
			i = 0;
			for(i=0; i < spaces; i++){
				OS_string[i] = args2[i];
			}
			char *command = OS_string[0];
			char *s;
			char *t;
//faz redirecionamento de i/o se necessario
			int sourceFD = 0;
			int targetFD = 0;
			int result = 0;
			int stat = 0;
			if(i_place != 0){

				if(access(OS_string[i_place+1], F_OK)!= -1){
				}else{
					printf("nao pode abrir %s para o input\n", OS_string[i_place+1]);
					skip = 1;
				}

				stat = 1;
				sourceFD = open(OS_string[i_place+1], O_RDONLY);
				OS_string_end = i_place;
				if(o_place != 0){
					stat = 2;
					targetFD = open(OS_string[o_place+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				}
			}else{
				if(o_place == 0 && i_place == 0){
					stat = 3;
				}else{
		    	if(o_place != 0){
						stat = 4;
			     			targetFD = open(OS_string[o_place+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
					}
				}
			}

//targetfd e sourcefd foram setados dependendo se a string tem input, output, ambos, ou uma combinacao (nao conta para &/processos em segundo plano)
 
			int size = 0;
			if(i_place != 0 && o_place != 0){
				size = i_place;  //input and output
			}else{
				if(i_place != 0 && o_place == 0){
					size = i_place; //input and no output
				}else if(i_place == 0 && o_place != 0){
					size = o_place; //only output
				}else if(i_place == 0 && o_place == 0){
					size = spaces;
				}
			}
			int background = 0;

//seta background = 1 se o ultimo char na linha eh &, pois significa q eh um processo em segundo plano; seta redirecionamento i/o para um arquivo de lixo

			if(strcmp(OS_string[spaces-1], "&")==0){
				if(ignored == 0){
					background = 1;
					numpids = numpids + 1;
					targetFD = open("junktrashbackground", O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if(stat == 3 || stat == 4){
						sourceFD = open("junktrashbackground2", O_WRONLY);
					}
				}else if(ignored == 1){
					background == 0;
				}
			}


 //coloca o que sera executado no comando execv em um vetor chamado exec_string

			char *exec_string[size];
			i = 0;
			if(ignored == 1){
				background = 1;
			}
			for(i=0; i < size-background; i++){
				exec_string[i] = OS_string[i];
			}
			exec_string[size-background] = NULL;
			if(ignored == 1){ background = 0; }
			pid_t spawnpid = -5;
			int childExitMethod = -5;
			int ten = 10;
			result = 0;
			int backgroundpid = 99;
			spawnpid = fork();
			int spaces2 = 0;
			if(skip == 1){
				sourceFD = open("/dev/null", O_WRONLY);
			}

//bifurca
			if(ignored == 1){
				background == 0;
			}
			switch (spawnpid){
				case -1:
					perror("Hull Breach!");
					exit(1);
					exitsignal = 1;
					break;
				case 0:   //caso filho

				if(background==0){
					spaces2 = spaces;
					if(stat == 1){ //somente redirecionamento de inputs
						result = dup2(sourceFD, 0);
						if(skip == 1){

						}
						if(result ==-1){
							perror("source dup2()");
							exitstatus = 1;
						}
					}
					if(stat == 2){ //redirecionamento de ambos input e output
						result = dup2(sourceFD, 0);
						if(result ==-1){
							perror("source dup2()");
							exitstatus = 1;
						}
						result = dup2(targetFD, 1);
						if(result ==-1){
							perror("target dup2()");
							exitstatus = 1;
						}
						OS_string[1] = NULL;
						if(sourceFD == -1){
							perror("source open()");
							exitstatus = 1;
						}
						if(targetFD == -1){
							perror("target open()");
							exitstatus = 1;
						}
					}else if(stat == 3){ //comando individual

					}else if(stat == 4){ //output individual
						result = dup2(targetFD, 1);
						if(result ==-1){
							perror("target dup2()");
							exitstatus = 1;
						}
					}
			}else if(background == 1){
						result = dup2(targetFD, 1);
						if(stat == 3 || stat == 4){
							result = dup2(sourceFD, 0);
						}

					}

 //redirecionamento dup2 i/o agora esta setado, roda o comando
 
					if(skip == 1){
						execlp("echo", "echo", " ", NULL);
					}else{

					OS_string[spaces2] = NULL;
					if(background==1){
						fclose(stdin);
						fopen("/dev/null", "r");
						execvp(exec_string[0], exec_string);
						exit(1);
					}else{
						execvp(exec_string[0], exec_string);
						exit(1);
					}
				}
					perror("CHILD: exec failure\n");
					break;

				default: //classe pai
					if(background == 1 ){
						printf("background pid eh %d\n", spawnpid);
					}else if(background == 0){
						waitpid(spawnpid, &childExitMethod, 0);
						kill(spawnpid, SIGKILL);
					}
					break;
			}
			//fora do fork
			//espera o programa fechar, ou nao espera se for &/processo de segundo plano
			if(background == 1){
				pidarray[pidarraycount] = spawnpid;
				pidarraycount=pidarraycount+1;
			}
			int exitStatus = WEXITSTATUS(childExitMethod);
			exitstatus = exitStatus;
			int termSignal = WTERMSIG(childExitMethod);

		}
	}
}

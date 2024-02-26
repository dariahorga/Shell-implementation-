//se compileaza cu flag-ul: -lreadline
//inainte de compilare se instaleaza GNU Readline:
//$ sudo apt-get install libreadline-dev

#include<stdlib.h>
#include<errno.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<dirent.h> 
#include<readline/readline.h>
#include<readline/history.h>
#include<time.h>

#define MAX_COMMANDS 100

//Comenzi definite
int shellHelp();
int changeDirectoy(char**);
int printCurrentDirectory();
int listFiles();
int copyFiles(char**);
int createDirectory(char**);
int deleteDirectory(char**);
int deleteFile(char**);
int clearTerminal();
int exitShell();

int executeCommand(char **);

//contine un vector de siruri de caractere
//reprezinta o comanda
//folositoare pipes cand avem mai multe comenzi
struct command
{
 	char **argv;
};

//sunt stocate numele functiilor
char* commands[] =
{
	"new_help",
	"new_cd", 
	"new_pwd",
	"new_ls",
	"new_cp",
	"new_mkdir",
	"new_rmdir",
	"new_rm",
	"new_clear"
};

//sunt stocate functiile
int (*functions[]) (char**) =
{
	&shellHelp,
	&changeDirectoy, 
	&printCurrentDirectory,
	&listFiles,
	&copyFiles,
	&createDirectory,
	&deleteDirectory,
	&deleteFile,
	&clearTerminal
};

int shellHelp()
{
	printf("Commands:\n");
	printf("----------------------------------------------------------\n\n");
	printf("new_cd: change directory \n");
	printf("new_pwd: show the path of the current working directory \n");
	printf("new_ls: list of files from the current directory \n");
	printf("new_cp: copy files from source1 to source2 \n");
	printf("new_mkdir: make a new directory\n");
	printf("new_rmdir: delete a directory \n");
	printf("new_rm: delete a file \n");
	printf("new_clear: clear the terminal \n");
	printf("new_exit: exit the shell\n");
	printf("Rest of the commands are the same \n");
	return 0;
}

//implementare metoda cd; schimbam directorul
int changeDirectoy(char** args)
{
	if(args[1] == NULL)
	{
		fprintf(stderr, "Expected an argument for the directory's name!\n");
	}
	else
	{
		if(chdir(args[1]) != 0)
		{
			perror("Error");
			return 1;
		}
	}

	return 0;
}

//implementare metoda pwd; afisam calea catre directorul in care ne aflam 
int printCurrentDirectory()
{
	char buff[1024];
	getcwd(buff, sizeof(buff));  ////ia numele directorului curent si il pune in buf
	printf("Directory: %s\n", buff);
	return 0;
}

//implementare motoda ls; afisam continutul unui director; pe 4 coloane 
int listFiles()
{
	int n;
	struct dirent** namelist; //structura care detine informatii despre directorii de intrare
	
	//"."=scanare director parinte
	//alphasort le sorteaza alfabetic
	n = scandir(".", &namelist, NULL, alphasort);
	if(n == -1)
	{
		perror("scandir");
		return 1;
	}
	int columns = 4;
	while(n--)
	{
		//d_name are numele fisierului
		//le afisam cate 4 
		printf("%s        ", namelist[n]->d_name);
		columns--;
		if(columns == 0)
		{
			printf("\n");
			columns = 4;
		}
		free(namelist[n]);
	}
	free(namelist);
	printf("\n");
	return 0;
}

//functia cp; copiaza continutul unui fisier in altul 
int copyFiles(char **args) 
{
	FILE *sourceFile, *destinationFile;

	if (args[1] == NULL || args[2] == NULL) {
		fprintf(stderr, "Usage: new_cp <source_file> <destination_file>\n");
		return 1;
	}

	if ((sourceFile = fopen(args[1], "r")) == NULL) {
		perror("Unable to open source file");
		return 1;
	}

	if ((destinationFile = fopen(args[2], "w")) == NULL) { //citeste caracter cu caracter
		perror("Unable to open or create destination file");
		fclose(sourceFile);
		return 1;
	}

	int c;
	//EOF-end of file
	while ((c = fgetc(sourceFile)) != EOF) {
		if (fputc(c, destinationFile) == EOF) { //scrie caracter cu caracter
			    perror("Error writing to destination file");
			    fclose(sourceFile);
			    fclose(destinationFile);
			    return 1;
		}
	}

	if (fclose(sourceFile) == EOF) {
		perror("Error closing source file");
		fclose(destinationFile);
		return 1;
	}

	if (fclose(destinationFile) == EOF) {
		perror("Error closing destination file");
		return 1;
	}
	return 0;
}

//mkdir; cream un nou director
int createDirectory(char **args) 
{
	     if(args[1] == NULL)
	{
		fprintf(stderr, "Expected an argument for the directory's name!\n");
		return 1;
	}
	//creaza un nou director cu 
	//numele a ce se afla in args[1]
	//0777 (octal) este un mod de permisiune
	if(mkdir(args[1], 0777) == -1)
	{
		printf("Unable to create directory! \n");
		return 1;
	}
	else
	{
		printf("Directory created. \n");
	}

	return 0;

}

//rmdir; sterge directorul 
int deleteDirectory(char **args) 
{
  if(args[1] == NULL)
	{
		fprintf(stderr, "Expected an argument for the directory's name!\n");
		return 1;
	}
	
	if(rmdir(args[1]) == -1)
	{
		printf("Unable to delete directory!\n");
		return 1;
	}

	else
	{
		printf("Directory deleted. \n");
	}

	return 0;
}

//rm; sterge fisierul
int deleteFile(char** args)

{	if(args[1] == NULL)
	{
		fprintf(stderr, "Expected an argument for the file's name!\n");
		return 1;
	}
	if(!remove(args[1]))
	{

		printf("File deleted.\n");
	}
	else
	{
		printf("Unable to delete file!\n");
		return 1;
	}
	return 0;
}

//\033[2J\033[H- secventa de control ANSI
// \033[2J sterge ecranul 
//\033[H plaseaza cursorul in partea staga sus a ecranului 
int clearTerminal()
{
	printf("\033[2J\033[H");
	return 0;
}

//functie care converteste o linie de comanda intr-o linie de argumente 
char** parseCommand(char *line, int *dimension)
{
	//impartirea liniei in lista de argumente
	int pos=0;
	int buff=64;
	
	char **tokens = malloc( sizeof(char) * buff );
    	char *token;
    	//token memoreaza argumentele date
	//(cd, ls, rmdir, mkdir+ arg lor)
	
	if(!tokens) {
		//specificam numele fisierului si linia
		//la care a aparut eroarea
		fprintf(stderr, "Allocation error in file %s at line # %d\n", __FILE__, __LINE__);
		exit(1);
	}
	//extragere cuvinte
	token = strtok(line," \n");
	while(token != NULL)
	{
		//strdup duplica fiecare token si il salveaza in array-ul tokens
		tokens[pos] = token;
		pos++;
		if(pos >= buff)
		{
			buff += 64;
			tokens = realloc(tokens, sizeof(char*) * buff);
			if(!tokens)
			{
				fprintf(stderr, "Allocation error in file %s at line # %d\n", __FILE__, __LINE__);
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL," \n");
	}
	*dimension = pos;
	tokens[pos] = NULL ;
	return tokens;
}

// ajunge in aceasta metoda pentru orice comanda nedefinita; adica nu este de tipul new_com
// exemple: echo, cat etc
int otherCommand(char** args)
{
	int status; //salveaza starea finala a procesului copil
	pid_t pid;
	if((pid = fork()) == 0)
	{
		char s[] = "/bin/";
		strcat(s,(char*)args[0]);
		//se concateneaza numele comenzii la calea catre directorul /bin/, pentru a forma calea absoluta a comenzii
                //comenzi precum ls, cp sunt gazduite in directorul /bin/; sunt executabile de bază și 
                //sunt executabile de baza si sunt incluse in calea de cautare a sistemului
		if(execvp(args[0], args) == -1)
		{
			printf("Error: command not found or failed to execute");
			printf("\n");
			exit(1); }
		exit(0);	
	}
	else if(pid < 0)
	{
		perror("Error creating a new process!");
		exit(1);
	}
	//procesul parinte asteapta ca procesul copil sa se incheie si salveaza starea sa 
	waitpid(pid, &status, 0);
	return WEXITSTATUS(status);
}

//executa comenzile
int executeCommand(char** args)
{
	int i;
	if(args[0] == NULL)
	{
		return 1;
	}
	
	int dimension = sizeof(commands)/sizeof(char*);

	for(i = 0; i < dimension; i++)
	{
		//cauta comanda printre cele definite aici
		if(strcmp(args[0], commands[i]) == 0)
		{	
		return (*functions[i])(args);	
		}
	}
	//daca nu e comanda definita
	return otherCommand(args);
}

//functia creeaza un proces copil
//gestioneaza redirectionarea intrarii si/sau a iesirii in funcție de valorile in si out 
//dup2- duplica un descriptor de fișier intr-un alt descriptor de fisier
int executeCommandWithIO(int input, int output, struct command *cmd) 
{
	pid_t pid;
	if ((pid = fork()) == 0) {

		if (input != 0) {
		    dup2(input, 0);
		    //se realizeaza redirectionarea intrarii standard pentru procesul copil
		    close(input);
		}

		if (output != 1) {
		    dup2(output, 1);
		    //se realizeaza redirectionarea iesirii standard pentru procesul copil
		    close(output);
		}

		return execvp(cmd->argv[0], cmd->argv);
		//procesul copil executa comanda specificata in structura cmd
		//functia execvp inlocuieste imaginea procesului curent cu cea a comenzii specificate
	}

	return pid;
}

//functie pentru executia comenzilor folosind pipe-uri
int executeCommandsWithPipes(int nr, struct command *cmd) 
{
    int i;
    int input, fd[2];
    pid_t pid;

    input = 0;

    for (i = 0; i < nr; ++i) 
    {
        pipe(fd);

        if ((pid = fork()) == 0) 
        {
            //proces copil

            //daca nu este prima comanda, redirectioneaza stdin pentru a citi din inputul pipe-ului anterior
            if (input != 0) {
                dup2(input, 0);
                close(input);
            }

            //daca nu este ultima comanda, redirectioneaza stdout pentru a scrie in outputul pipe-ului curent
            if (i != nr - 1) {
                dup2(fd[1], 1);
            }

            //inchide capatul neutilizat al pipe-ului
            close(fd[0]);

            //executa comanda
            execvp(cmd[i].argv[0], cmd[i].argv);

            //iesire in caz de eroare
            perror("execvp");
            return 1;
        }
        else if (pid < 0) 
        {
            // Eroare la fork
            perror("fork");
            return 1;
        }

        //proces parinte
        close(fd[1]);
        input = fd[0];
    }

    //asteapta terminarea tuturor proceselor copil
    for (i = 0; i < nr; ++i) {
        wait(NULL);
    }

    return 0;
}

void printHistory() {
    HIST_ENTRY** the_history_list;
    int i;

    the_history_list = history_list();

    printf("Ati folosit:\n");
    for (i = 0; the_history_list[i]; i++) {
        printf("%d: %s\n", i + history_base, the_history_list[i]->line);
    }
}



int main() {
	printf("Type 'new_help' to see the shell commands.\n");

	char* line;
	char** args;
	
	int status;
	int dimension = 0;
	struct command pipecmd [MAX_COMMANDS];
	int nrp = 0;
	
	clock_t start_parsing, end_parsing, start_executing, end_executing;
	do {
	
		//citim comanda
		line = readline("Shell$ ");
	
		if (line[0] != 0) 
		{
			start_parsing = clock();
		        //separare in argumente
		    	args = parseCommand(line, &dimension);
		    	
		    	end_parsing = clock();
                        double parsing_time = ((double)(end_parsing - start_parsing)) / CLOCKS_PER_SEC;
                        printf("Elapsed time of parsing: %f seconds\n", parsing_time);


                         //adaugare in istoric
			add_history(line);
		    
			//&&
			int* position = (int*)malloc(dimension*sizeof(int));
			int k = 0;

                        //|
			int* position1 = (int*)malloc(dimension*sizeof(int));
			int k1 = 0;

			for(int i = 0;i < dimension; i++)
				if(strcmp(args[i], "&&") == 0)
				{
					position[k++] = i; // marcam pozitiile pe care exista &&
					
				}

			for(int i = 0; i < dimension; i++)
				if(strcmp(args[i], "|") == 0)
				{
					position1[k1++] = i; //marcam pozitiile pe care exista |
					
				}

			//tratam cazul in care comanda contine &&
			if(k > 0)
			{	
				int i = 0; //iterare prin vectorul de pozitii
				int j = 0; //iterare prin argumente
				int n = 0;
				int limit;
				do
				{
					char ** cmd;
					cmd = (char**)malloc(dimension*sizeof(char*));
					
					n = 0;
					//verificam pozitia &&
					if(i == k)
					{
						limit = dimension;
					}
					else
					{
						limit = position[i];
					}

					for(;j < limit; j++)
					{ 
					        //se copiaza comanda
						cmd[n] = (char*)malloc(strlen(args[j])*sizeof(char*));
						strncpy(cmd[n], args[j], strlen(args[j]));
						cmd[n][strlen(args[j])] = '\0';
						n++;
					}

						
					j = position[i]+1;
					i++;
					
					cmd[n] = NULL;

					start_executing = clock();
					status = executeCommand(cmd);
					end_executing = clock();
					if(status == 1) //a avut eroare
					{
						printf("Wrong command! \n");
						break;
					}
					
					free(cmd);
							
				}
				while(i <= k);
					
			
			}
                        
                        //tratam cazul in care comanda contine |
			else if(k1 > 0)
			{	
				int i = 0;//iterare prin vectorul de pozitii
				int j = 0;//iterare prin argumente
				int n = 0;
				int limit;

				do
				{
					char ** cmd;
					cmd = (char**)malloc(dimension*sizeof(char*));
					
					n = 0;
					//verificam pozitia | 
					if(i == k1)
					{
						limit = dimension;
					}
					else
					{
						limit = position1[i];
					}

					for(;j<limit;j++)
					{        
					         //se copiaza comanda
						cmd[n] = (char*)malloc(strlen(args[j])*sizeof(char*));
						strncpy(cmd[n], args[j], strlen(args[j]));
						cmd[n][strlen(args[j])] = '\0';
						n++;
					}

					j = position1[i] + 1;
					i++;
					cmd[n] = NULL;
					//cmd[0] - numele functiei
					//cmd[1] - cmd[n-1]
					
					pipecmd[nrp++].argv = cmd;
								
				} while(i <= k1);	
				start_executing = clock();	
				executeCommandsWithPipes(nrp, pipecmd);
				end_executing = clock();
			
			}	
			
			//tratam cazul pentru functia history
			else if(strcmp(args[0], "history") == 0)
			{
				start_executing = clock();
				printHistory();
				end_executing = clock();
			}
                       //tratam cazul pentru functia exit 
			else if(strcmp(args[0], "new_exit") == 0)
			{
				return 0;
			}

                        //daca nu a intrat pe vreun caz de mai sus executam comanda 
			else
			{
			      start_executing = clock(); 
			      executeCommand(args);
			      end_executing = clock();
			}
			float executing_duration = ((float) (end_executing - start_executing)) / CLOCKS_PER_SEC;
			printf("Elapsed time of execution: %f seconds\n", executing_duration);
			free(line);
			free(args);
		}
	} while (1);

	return 0;
}

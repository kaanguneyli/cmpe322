#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include "bello.c"
#include "create_alias.c"

#define HOSTBUFFER_LENGTH 256                   // max length for hostname
#define CWD_LENGTH 1024                         // max length for current working directory
#define INPUT_LENGTH 2048                       // max length for user input
#define BUILTIN_LENGTH 32                       // max length for built-in command name
#define BUILTIN_NUMBER 5                        // max length for built-in commands
#define ARGS_LENGTH 128                         // length for the arguments list
#define MAX_PATH_LENGTH 4096                    // max length for the path of a command
#define MAX_ALIAS_NUMBER 1000                   // max number of aliases
#define ALIAS_FILE "files/kaan_alias.txt"       // the file which stores the aliases 
     


/* 
   Takes a command as argument   
   Returns the directory of path for that command
*/
char* search_path_for_command(char *command) {
   char *path = getenv("PATH");
   char path_copy[MAX_PATH_LENGTH]; 
   strcpy(path_copy, path);
   char *directory = strtok(path_copy, ":");
   
   while (directory != NULL) {
      size_t path_len = strlen(directory) + strlen(command) + 2; // 1 for '/', 1 for '\0'
      char *executable_path = malloc(path_len);
      snprintf(executable_path, path_len, "%s/%s\0", directory, command);
      if (access(executable_path, X_OK) == 0) {
         return executable_path;
      }
      free(executable_path);
      directory = strtok(NULL, ":");
   }
   return NULL;
}


/*
   Takes start and end indexes and a text as input
   Creates a token between these indexes and returns it
*/
char* createToken(int start, int end, char* input) {
   char *token;
   token = malloc(sizeof(char) * (end - start));
   for (int j = start + 1, k=0; j < end; j++, k++) {
      token[k] = input[j];
   }
   token[end - start - 1] = '\0';
   return token;
}


/*
   Takes a text and a file
   Appends the text to the file in reverse
*/
void writeReverse(char readText[], char* writeFile) {
   FILE *write = fopen(writeFile, "a+");

   int size = 0;
   for (int i = 0; readText[i] != NULL; i++) {
      size++;
   }
   size--; // reduce size by 1 to remove unwanted behaviour

   // writes the text char by char starting from the end
   while (size >= 0) {
      char c = readText[size];
      fputc(c, write);
      size--;
   }
   
   //clean the array that holds to text to prevent unwanted behaviour
   for (int i = 0; readText[i] != NULL; i++) {
      readText[i] = (char *) NULL;
   }
   fclose(write);
}


/*
   cleans the memory when we move onto a new command 
*/
void cleanMemory(char *token, char *inputBuffer, char **arguments) {
   free(token);
   free(inputBuffer);
   for (int j=0; j<ARGS_LENGTH; j++) { arguments[j] = (char*) NULL; }
}


int main() {

   char *alias_names[MAX_ALIAS_NUMBER];            // holds the names of aliases
   char *alias_info[MAX_ALIAS_NUMBER];             // holds the command of aliases
   int   original_stdout = dup(STDOUT_FILENO);
   char *lastExecuted = " ";
   char  built_in[BUILTIN_NUMBER][BUILTIN_LENGTH] = {"exit", "bello", "alias"}; // the list of built-in commands
   char *alias_arguments[ARGS_LENGTH];             // helps tokenizing arguments
   char *arguments[ARGS_LENGTH];                   // this array will hold the tokens created from the input
   
   signal(SIGCHLD, SIG_IGN);                       // prevents daemons

   while (true) {
      dup2(original_stdout, STDOUT_FILENO);
      
      // read the alias file and store the names and the commmands they refer to in different arrays
      // this can be done out of the while loop in terms of efficiency but this way is safer
      FILE *alias_file = fopen(ALIAS_FILE, "r");
      if (alias_file != NULL) {
         int i=0;  
         char name[INPUT_LENGTH];
         char info[INPUT_LENGTH];
         while (fscanf(alias_file, "%s", name) == 1) {
            fgets(info, INPUT_LENGTH, alias_file);
            alias_names[i] = strdup(name);
            alias_info[i] = strdup(info);
            for (int j=0; j<INPUT_LENGTH; j++) {
               if (alias_info[i][j] == '\n') {
                  alias_info[i][j] = '\0';
                  break;
               }
            }
            i++;
         }
      }
      fclose(alias_file);
   

// PRINTING PROMPT
      char *username = getenv("USER");
      char hostbuffer[HOSTBUFFER_LENGTH];
      gethostname(hostbuffer, sizeof(hostbuffer));
      char cwd[CWD_LENGTH];
      getcwd(cwd, sizeof(cwd));
      printf("%s@%s %s --- ", username, hostbuffer, cwd);


// READING INPUT
      char* inputBuffer = NULL;
      size_t inputBufferSize = 0;
      
      if (getline(&inputBuffer, &inputBufferSize, stdin) == -1) {
         continue;
      }
      // handling the case of empty input
      if (inputBuffer == NULL || strspn(inputBuffer, " \t\n\r") == strlen(inputBuffer)) {
         cleanMemory(NULL, inputBuffer, arguments);
         continue;
      }


// PARSING      
      char *currentChar = inputBuffer;
      int   arg = 0;
      int   lasti = -1;
      char *token;
      int   i = 0;
      bool  isRedirect = false;
      bool  isBackground = false;
      int   redirectIndex = 0;

      /* traverse the input and create tokens based on blank spaces
         new line's are also added here because a new line char in a token prevents executing command
         we also ignore " signs because this way is nice to handle quotations */
      for (; *currentChar != '\0'; i++) {
         if (*currentChar == ' ' || *currentChar == '\n'|| *currentChar == '"') {
            if (lasti == i-1) {
               lasti = i;
               ++currentChar;
               continue;
            }
            token = createToken(lasti, i, inputBuffer);
            if (strcmp(token, ">") == 0 || strcmp(token, ">>") == 0 || strcmp(token, ">>>") == 0) {  // flag if you encounter redirection
               isRedirect = true;
               redirectIndex = arg;
            } else if (strcmp(token, "&") == 0) { // flag if you encounter background process
               isBackground = true;
               break;
            }
            arguments[arg] = malloc(sizeof(char) * (i - lasti));
            snprintf(arguments[arg], (i - lasti), "%s", token);  // add the token to arguments list
            arg++;
            lasti = i;
         }    
         ++currentChar;
      }

      // this part probably doesn' get executed, but it is one more layer to handle empty inputs
      if (arguments[0] == NULL) {
         cleanMemory(token, inputBuffer, arguments);
      }


// REDIRECTION
      bool isReverse = false;
      char *reverse_outputFile;
      int outputFile;
      char *outputFileName; 

      if (isRedirect) {   
         if (strcmp(arguments[redirectIndex], ">") == 0){
            outputFile = open(arguments[redirectIndex + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);  // open file in overwrite mode
         } else if (strcmp(arguments[redirectIndex], ">>") == 0) {
            outputFile = open(arguments[redirectIndex + 1], O_WRONLY | O_CREAT | O_APPEND, 0666); // open file in append mode
         } if (strcmp(arguments[redirectIndex], ">>>") == 0) {
            isReverse = true;
            outputFileName = arguments[redirectIndex + 1];
         }
         if (! isReverse) {
            dup2(outputFile, STDOUT_FILENO); // direct the standard output to the file from the input
            close(outputFile);
         }
         // remove the redirection sign and the file name from arguments list and slide the other arguments
         arguments[redirectIndex] = (char *) NULL;  
         arguments[redirectIndex+1] = (char *) NULL;
         for (int i=redirectIndex+2; arguments[i] != NULL; i++) {
               arguments[i-2] = strdup(arguments[i]);
         }
      }


// ALIAS
      bool isAlias = false;
      int argsIndex = 0;
      int alias_info_index = 0;

      // look for the command name in the alias list
      for (int i=0; i < MAX_ALIAS_NUMBER; i++) {
         char *item = alias_names[i];
         if (item == NULL) {
            break;
         }
         if (strcmp(item, arguments[0]) == 0) {
            char *token_alias = strtok(alias_info[i], " ");
            for (; token_alias != NULL; alias_info_index++) {
               alias_arguments[alias_info_index] = strdup(token_alias);
               token_alias = strtok(NULL, " ");
            }            
            isAlias = true;
            break;
         }
      }
      
      /* if the command is an alias remove the alias name from arguments
         then slide the other arguments */
      if (isAlias) {
         int i=0;
         int i_count=0;
         int j_count=0;

         for (; alias_arguments[i] != NULL; i++) {
            i_count++;
         }
         for (int j=0; arguments[j] != NULL; j++) {
            j_count++;
         }
         for (int k=j_count - 1; k>0; k--) {
            arguments[k + i_count - 1] = strdup(arguments[k]);
         }
         for (int m=0; m < i_count; m++) {
            arguments[m] = strdup(alias_arguments[m]);
         }
         arg += i_count - 1;

      }
      // clean the array to prevent unwanted behaviour
      for (int j=0; j<ARGS_LENGTH; j++) { alias_arguments[j] = (char*) NULL; }


// BUILT-IN
      bool isBuiltin = false;
      bool error = false;

      // look for the commmand in the built-in list
      for (int i = 0; i < BUILTIN_LENGTH; i++) {
         char *item = built_in[i];
         if (strcmp(item, arguments[0]) == 0) {
            if (i==0) {  // exit
               exit(0);
            } 
            else if (i==1) {   // bello
               bello(lastExecuted);
               lastExecuted = "bello";
            } 
            else if (i==2) {   // alias
               for (int i=0; i<4; i++) {
                  if (arguments[i] == NULL) {  // error if there are not enough arguments
                     error = true;
                     break;
                  }
               }
               if (error || strcmp(arguments[2],"=") != 0) {  // error if the third argument is not =
                  error = true;
                  break;
               } else {
                  struct index_and_line ial = createAlias(ALIAS_FILE, arguments); // create the alias and store the line number and the content of alias in a variable
                  // update the alias lists
                  alias_names[ial.index] = arguments[1]; 
                  strtok(ial.string, " ");
                  char *info = strtok(NULL, "");
                  alias_info[ial.index] = strdup(info);
                  for (int j=0; j<INPUT_LENGTH; j++) {
                     if (alias_info[ial.index][j] == '\n') {
                        alias_info[ial.index][j] = '\0';
                        break;
                     }
                  }
                  lastExecuted = "alias";   // I consider alias as a command too, so I store it as the last executed command
               }
            }
            isBuiltin = true;
            break;
         }
      }
      if (error) {
         printf("Invalid input.\n");
         cleanMemory(token, inputBuffer, arguments);
         continue;
      }
      if (isBuiltin) { // if you execute a built-in command, you shouldn't execute the rest
         cleanMemory(token, inputBuffer, arguments);
         continue;
      }

// PATH
      // if the command is not alias or built-in, we have to find it in the path
      char* path = search_path_for_command(arguments[0]);

      /* put the items in the arguments array to a new array called args
         this new array does not have unnecessarry spaces so it prevents uunwanted behaviour */
      if (isRedirect) {
         arg-=2;
      }
      char *args[arg+1];
      for (int i=0; i<arg; i++)  {
         args[i] = arguments[i];
      }
      args[arg] = (char *) NULL;

      if (path == NULL) {
         printf("Command not found.\n");
         cleanMemory(token, inputBuffer, arguments);
         continue;
      }

      // fork a child
      pid_t pid = fork();     
      if (pid < 0) {
         printf("Fork Failed");
         cleanMemory(token, inputBuffer, arguments);
         continue;
      } else if (pid == 0) {
         /* if the process contains >>> this part works
            this part creates a pipe and forks a grandchild which processes the input command
            then the child (parent of grandchild) writes it to the input file in reverse */
         if (isReverse) { 
            int pipefd[2];
            if (pipe(pipefd) == -1) {
               printf("Pipe creation failed\n");
               cleanMemory(token, inputBuffer, arguments);
               continue;
            }
            pid_t grandchild = fork();
            if (grandchild == -1) {
               printf("Fork Failed\n");
               cleanMemory(token, inputBuffer, arguments);
               continue;
            }
            if (grandchild == 0) {
               close(pipefd[0]);
               dup2(pipefd[1], STDOUT_FILENO); // direct the output to the pipe
               close(pipefd[1]);
               execv(path, args);
            } else {
               close(pipefd[1]);
               waitpid(grandchild, NULL, 0);
               char text[10000];
               read(pipefd[0], text, 10000);
               close(pipefd[0]);
               writeReverse(text, outputFileName);
               exit(EXIT_SUCCESS);
            }
         } else {
            execv(path, args);
         }
      } else {
         // do not wait if the process is on background
         if (!isBackground) {
            waitpid(pid, NULL, 0);
         }
      }

      lastExecuted = arguments[0];
      cleanMemory(token, inputBuffer, arguments);
   }

   for (int j=0; j<ARGS_LENGTH; j++) { free(arguments[j]); }
   for (int j=0; j<ARGS_LENGTH; j++) { free(alias_arguments[j]); }
   close(original_stdout);
   return 0;
}


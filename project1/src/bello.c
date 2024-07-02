#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define HOSTBUFFER_LENGTH 256
#define CWD_LENGTH 1024

/*
    Very similar to >>> case in main
    Fork a child, run a command there and write the output to a pipe
    Takes command, buffer name and size as argument
    Runs the command and writes the output to buffer
*/
int execute_command(char *args[], char *output_buffer, size_t buffer_size) {
    int pipe_fd[2];
    pid_t child_pid;
    
    if (pipe(pipe_fd) == -1) {
        printf("pipe error");
        return 1;
    }

    if ((child_pid = fork()) == -1) {
        printf("fork error");
        return 1;
    }

    if (child_pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        if (execvp(args[0], args) == -1) {
            printf("execvp error");
            return 1;
        }
    } else {
        close(pipe_fd[1]);

        ssize_t bytes_read;
        size_t total_bytes_read = 0;
        while ((bytes_read = read(pipe_fd[0], output_buffer + total_bytes_read, buffer_size - total_bytes_read)) > 0) {
            total_bytes_read += bytes_read;
            if (total_bytes_read >= buffer_size - 1) {
                break;
            }
        }

        output_buffer[total_bytes_read] = '\0';

        close(pipe_fd[0]);

        waitpid(child_pid, NULL, 0);
    }

    return 0;
}

int bello(char *lastExecuted) {

    char *username = getenv("USER");

    char hostbuffer[HOSTBUFFER_LENGTH];
    gethostname(hostbuffer, sizeof(hostbuffer));

    char *tty_name = ttyname(STDIN_FILENO);
    
    // find the parent processes name (parent process is the shell)
    pid_t pid_shell = getppid();
    char pid_shell_str[8];
    sprintf(pid_shell_str, "%d", pid_shell);
    char *shell[] = {"ps", "-o", "args=", "-p", pid_shell_str, (char *) NULL};
    char shell_name[1024];
    execute_command(shell, shell_name, sizeof(shell));
    // sometimes in background processes it finds the main as shell, so we run the command one more time to find the actual shell
    if (strcmp(shell_name, "./myshell\n")==0) { 
        execute_command(shell, shell_name, sizeof(shell));
    }

    char *home = getenv("HOME");

    time_t mytime = time(NULL);

    // this command prints the current processes on this tty
    char *currentProcesses[] = {"ps", "-t", tty_name,  (char *) NULL};
    char currentProcessesOutput[1024];
    execute_command(currentProcesses, currentProcessesOutput, sizeof(currentProcessesOutput));
    // count the number of lines
    int processes = 0;
    for (int i=0; currentProcessesOutput[i] != NULL; i++) {
        if (currentProcessesOutput[i] == '\n') {
            processes++;
        }
    }
    processes -= 2; // subtract 2 irrelevent lines
    printf("%s\n%s\n%s\n%s\n%s\n%s\n%s%d\n", username, hostbuffer, lastExecuted, tty_name, shell_name, home, ctime(&mytime), processes);
    return 0;
}
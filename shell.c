/***************************************************************************//**

  @file         shell.c

  @author       Le Ngoc Hieu - 22520435

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LINE 100 
#define clear() printf("\033[H\033[J") // for fun :d

char *history[15];
int historyIndex = 0;

void help()
{
    printf("Le Ngoc Hieu's Shell\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built-in:\n");
    printf("\tls\n");
    printf("Use the man command for any other programs.\n");
}
void init_Shell() 
{ 
    clear();
    printf("\n\n******************************************"); 
    printf("\n\n\t**** LE NGOC HIEU'S SHELL ****"); 
    printf("\n\n\t-----USE AT YOUR OWN RISK-----"); 
    printf("\n\n******************************************"); 
    char* username = getenv("USER"); 
    printf("\nWelcome user: @%s", username); 
    printf("\n\nType `help` for more info\n\n"); 
    sleep(0.7);
}

void add_History(char *command) {
    if (historyIndex < 15) {
        history[historyIndex] = strdup(command);
        historyIndex++;
    } else {
        for (int i = 0; i < 15 ; i++) {
            history[i] = history[i + 1];
        }
        history[14] = strdup(command);
        historyIndex++;
    }
}

void print_History() {
    if (historyIndex > 0)
    {
        for (int i = 0; i < historyIndex; i++)
        {
            printf("%s\n", history[i]);
        }
    }
    else
    {
        printf("No command in history\n");
    }
}

void execute_Pipeline(char ***command_list, int num_commands) {
    int pipes[num_commands - 1][2];

    for (int i = 0; i < num_commands; ++i) {
        if (i < num_commands - 1) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork();

        if (pid == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }

            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            execvp(command_list[i][0], command_list[i]);
            perror("Failed");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (i > 0) {
            close(pipes[i - 1][0]);
            close(pipes[i - 1][1]);
        }

        if (pid > 0) wait(NULL);
    }
}

void execute_Command(char *command) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        char *arguments[MAX_LINE / 2 ];
        char *token = strtok(command, " ");
        int i = 0;
        int num_commands = 0;
        char **command_list[MAX_LINE/2 ];

        while (token != NULL) {
            if (strcmp(token, "|") == 0) {
                arguments[i] = NULL;
                command_list[num_commands] = malloc(sizeof(char*) * (i + 1));
                memcpy(command_list[num_commands], arguments, sizeof(char*) * i);
                command_list[num_commands][i] = NULL;
                ++num_commands;

                i = 0;
            }
            else {
                arguments[i] = token;
                i++;
            }
            token = strtok(NULL, " ");
        }

        arguments[i] = NULL; 

        int redirectOutput = 0;
        int redirectInput = 0;

        for (int j = 0; j < i; j++) {
            if (strcmp(arguments[j], ">") == 0) {
                redirectOutput = j;
            } else if (strcmp(arguments[j], "<") == 0) {
                redirectInput = j;
            }
        }

        if (redirectOutput > 0) {

            int fd = open(arguments[redirectOutput + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd == -1) {
                perror("Open file failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);

            for (int j = redirectOutput; j < i - 2; j++) {
                arguments[j] = arguments[j + 2];
            }
            arguments[i - 2] = NULL;
        }

        if (redirectInput > 0) {
            int fd = open(arguments[redirectInput + 1], O_RDONLY);
            if (fd == -1) {
                perror("Open failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);

            for (int j = redirectInput; j < i - 2; j++) {
                arguments[j] = arguments[j + 2];
            }
            arguments[i - 2] = NULL;
        }
       if (i > 0) {
            arguments[i] = NULL;
            command_list[num_commands] = malloc(sizeof(char*) * (i + 1));
            memcpy(command_list[num_commands], arguments, sizeof(char*) * i);
            command_list[num_commands][i] = NULL;
            ++num_commands;
        }  

        if (num_commands > 1) {
            add_History(command);
            execute_Pipeline(command_list, num_commands);
        } 
        else {
            if (strcmp(command, "HF") == 0) {
            print_History(command);
        } 
            else {
            add_History(command);
            execvp(arguments[0], arguments);
            perror("Failed");
            exit(EXIT_FAILURE);
            }
        }
    } 
    else {
        waitpid(pid, NULL, 0);
    }
}


void sigint_Handler(int sig_num)
{
    printf("\nUse `exit` to exit to your shell\n");
    fflush(stdout);
}


int main(void) {
    char *command;
    int should_run = 1; 
    int bufsize = 1024;
    int c;

    signal(SIGINT, sigint_Handler);
    init_Shell();

    while (should_run) {
        printf("it007sh>");
        fflush(stdout);
        fflush(stdin);
        int position = 0;
        char *buffer = malloc(sizeof(char) * bufsize);
        while (1) {
            c = getchar();

            if (c == EOF) {
            exit(EXIT_SUCCESS);
            } else if (c == '\n') {
                buffer[position] = '\0';
                break;
            } else {
                buffer[position] = c;
            }
            position++;

            if (position >= bufsize) {
                bufsize += 1024;
                buffer = realloc(buffer, bufsize);
                if (!buffer) {
                    fprintf(stderr, "shell: allocation error\n");
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (strcmp(buffer, "help") == 0) {
            help();
        }
        else if (strcmp(buffer, "exit") == 0) {
            return 0;
        } else
        {
            add_History(buffer);
            execute_Command(buffer);
        }
    }
    return 0;
}

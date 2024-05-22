#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_CMD_LEN 1000 // max number of characters supported
#define MAX_ARGS 100 // max number of arguments supported

// Function to clear the screen
#define clear_screen() printf("\033[H\033[J")

// Function to initialize the shell
void initialize_shell()
{
    clear_screen();
    printf("\n\n\n\n***********************************");
    printf("\n\n\n\t****WELCOME TO MY SHELL****");
    printf("\n\n\t-USE AT YOUR OWN RISK-");
    printf("\n\n\n\n***********************************");
    char* username = getenv("USER");
    printf("\n\n\nUSER: @%s", username);
    printf("\n");
    sleep(1);
    clear_screen();
}

// Function to get input from the user
int get_input(char* input)
{
    char* buffer;

    buffer = readline("\n>>> ");
    if (strlen(buffer) != 0) {
        add_history(buffer);
        strcpy(input, buffer);
        return 0;
    } else {
        return 1;
    }
}

// Function to print the current directory
void print_current_directory()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\nDir: %s", cwd);
}

// Function to execute non-builtin commands
void execute_command(char** args)
{
    pid_t pid = fork();

    if (pid == -1) {
        printf("\nFailed to fork process..");
        return;
    } else if (pid == 0) {
        if (execvp(args[0], args) < 0) {
            printf("\nCommand execution failed..");
        }
        exit(0);
    } else {
        wait(NULL);
        return;
    }
}

// Function to execute piped commands
void execute_piped_commands(char** args1, char** args2)
{
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0) {
        printf("\nPipe initialization failed");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("\nForking failed");
        return;
    }

    if (p1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(args1[0], args1) < 0) {
            printf("\nFirst command execution failed..");
            exit(0);
        }
    } else {
        p2 = fork();

        if (p2 < 0) {
            printf("\nForking failed");
            return;
        }

        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(args2[0], args2) < 0) {
                printf("\nSecond command execution failed..");
                exit(0);
            }
        } else {
            wait(NULL);
            wait(NULL);
        }
    }
}

// Help command builtin
void display_help()
{
    puts("\n***MY SHELL HELP***"
         "\n-Use at your own risk..."
         "\nList of Commands supported:"
         "\n>cd"
         "\n>ls"
         "\n>exit"
         "\n>all other standard UNIX commands"
         "\n>pipe handling"
         "\n>space handling");
}

// Function to handle builtin commands
int handle_builtin_commands(char** args)
{
    int num_builtins = 4, i, command_index = 0;
    char* builtin_commands[] = {"exit", "cd", "help", "hello"};
    char* username;

    for (i = 0; i < num_builtins; i++) {
        if (strcmp(args[0], builtin_commands[i]) == 0) {
            command_index = i + 1;
            break;
        }
    }

    switch (command_index) {
    case 1:
        printf("\nGoodbye\n");
        exit(0);
    case 2:
        chdir(args[1]);
        return 1;
    case 3:
        display_help();
        return 1;
    case 4:
        username = getenv("USER");
        printf("\nHello %s.\nThis is not a place to play around."
               "\nUse help to know more..\n", username);
        return 1;
    default:
        break;
    }

    return 0;
}

// Function to check for pipes
int find_pipe(char* input, char** piped_commands)
{
    int i;
    for (i = 0; i < 2; i++) {
        piped_commands[i] = strsep(&input, "|");
        if (piped_commands[i] == NULL)
            break;
    }

    if (piped_commands[1] == NULL)
        return 0; // No pipe found
    else {
        return 1;
    }
}

// Function to parse the input command
void parse_command(char* input, char** args)
{
    int i;

    for (i = 0; i < MAX_ARGS; i++) {
        args[i] = strsep(&input, " ");

        if (args[i] == NULL)
            break;
        if (strlen(args[i]) == 0)
            i--;
    }
}

// Function to process the input string
int process_input_string(char* input, char** args, char** piped_args)
{
    char* piped_commands[2];
    int piped = 0;

    piped = find_pipe(input, piped_commands);

    if (piped) {
        parse_command(piped_commands[0], args);
        parse_command(piped_commands[1], piped_args);
    } else {
        parse_command(input, args);
    }

    if (handle_builtin_commands(args))
        return 0;
    else
        return 1 + piped;
}

int main()
{
    char input_string[MAX_CMD_LEN], *args[MAX_ARGS];
    char* piped_args[MAX_ARGS];
    int exec_flag = 0;
    initialize_shell();

    while (1) {
        print_current_directory();
        if (get_input(input_string))
            continue;
        exec_flag = process_input_string(input_string, args, piped_args);

        if (exec_flag == 1)
            execute_command(args);

        if (exec_flag == 2)
            execute_piped_commands(args, piped_args);
    }
    return 0;
}

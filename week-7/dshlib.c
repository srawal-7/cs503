#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "dshlib.h"

int last_status = 0; // To store the last command's return code

// Command structure
typedef struct {
    char **args;
} Command;

// Function to allocate memory for command buffer
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX * sizeof(char));
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    return OK;
}

// Function to free command buffer memory
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    free(cmd_buff->_cmd_buffer);
    return OK;
}

// Function to clear command buffer
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

// Function to trim leading and trailing whitespaces (spaces and tabs)
char *trim_whitespace(char *str) {
    // Trim leading spaces
    while (isspace((unsigned char)*str)) {
        str++;
    }

    // If the string is empty, return it
    if (*str == 0) {
        return str;
    }

    // Trim trailing spaces
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    // Null-terminate the string after trimming
    *(end + 1) = '\0';

    return str;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    char *token = cmd_line;
    int in_quotes = 0;  // Flag to track if we're inside quotes
    char *start = NULL; // Pointer to the start of the current token

    // Trim leading whitespace
    trim_whitespace(token);

    while (*token != '\0' && cmd_buff->argc < CMD_ARGV_MAX) {
        // Skip leading whitespace unless we're inside quotes
        if (isspace((unsigned char)*token) && !in_quotes) {
            token++;
            continue;
        }

        // Handle opening quote (start of quoted string)
        if (*token == '"' && !in_quotes) {
            in_quotes = 1;  // Enter "quoted" mode
            token++;  // Skip the opening quote
            continue;
        }

        // Handle closing quote (end of quoted string)
        if (*token == '"' && in_quotes) {
            in_quotes = 0;  // Exit "quoted" mode
            token++;  // Skip the closing quote
            continue;
        }

        // Process the token (start of a new token)
        if (start == NULL) {
            start = token;
        }

        // Move token to the next space or quote, but keep inside quoted sections
        while (*token != '\0' && (in_quotes || !isspace((unsigned char)*token))) {
            token++;
        }

        // If we have a complete token (non-empty)
        if (start != NULL && token != start) {
            int token_length = token - start;

            // If the token ends with a quote (and we're outside quotes), remove it
            if (token_length > 0 && *(token - 1) == '"') {
                token_length--;  // Remove the quote at the end
            }

            char *arg = malloc(token_length + 1);
            if (arg == NULL) {
                return ERR_MEMORY;
            }

            // Copy the token (argument) into a newly allocated string
            strncpy(arg, start, token_length);
            arg[token_length] = '\0';

            // Store the argument in the command buffer
            cmd_buff->argv[cmd_buff->argc++] = arg;

            // Reset start for the next token
            start = NULL;
        }

        // Skip any trailing whitespace after a token (outside quotes)
        while (*token != '\0' && isspace((unsigned char)*token) && !in_quotes) {
            token++;
        }
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;  // Null-terminate the arguments

    return OK;
}

// Function to execute built-in commands
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    // Handle the exit command
    if (strcmp(cmd->argv[0], EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    }

    // Handle the cd command
    if (strcmp(cmd->argv[0], "cd") == 0) {
        char *dir = cmd->argv[1];

        // If no argument provided, default to /tmp instead of HOME
        if (dir == NULL) {
            dir = "/tmp";  // Change this to "/tmp"
        }

        // Change directory
        if (chdir(dir) == -1) {
            perror("cd failed");
            return BI_CMD_CD;
        }
        
        return BI_CMD_CD;
    }

    // Handle the rc command (Return Code Command)
    if (strcmp(cmd->argv[0], "rc") == 0) {
        printf("Last Status: %d\n", last_status);  // Print the last exit status
        return BI_EXECUTED;  // Indicate that the built-in command was handled
    }

    // Handle other built-in commands (like 'which', etc.)
    return BI_NOT_BI; // Return this for non-built-in commands
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    memset(clist->commands, 0, sizeof(clist->commands)); // Clear out any previous data in clist->commands

    char *cmd_token;
    char *command_copy = strdup(cmd_line);  // Make a copy of the input command line for strtok
    int cmd_count = 0;

    // Trim leading/trailing spaces from the input command line
    trim_whitespace(command_copy);

    // If the command is empty, return a warning
    if (command_copy[0] == '\0') {
        printf("warning: no commands provided\n");
        free(command_copy);
        return WARN_NO_CMDS;
    }

    // Split the input line by the pipe character '|'
    cmd_token = strtok(command_copy, PIPE_STRING); // Split by pipe
    while (cmd_token != NULL) {
        if (cmd_count >= CMD_MAX) {
            free(command_copy);
            // Too many commands in a single line
            return ERR_TOO_MANY_COMMANDS;
        }

        // Trim whitespace from the command
        trim_whitespace(cmd_token);

        // Initialize argc (argument count) to 0 for the current command
        clist->commands[cmd_count].argc = 0;

        // Start iterating over the command string and split into exe and args
        char *exe_token = cmd_token;
        char *args_token = NULL;

        // Find the first space or the end of the string
        if (*exe_token != '\0') {
            // Move to the first space or end of the string
            while (*exe_token != '\0' && !isspace(*exe_token)) {
                exe_token++;  // Move to the first space or end of the string
            }

            if (*exe_token == '\0') {
                // If no space is found, the entire string is the executable
                strncpy(clist->commands[cmd_count].argv[clist->commands[cmd_count].argc], cmd_token, EXE_MAX - 1);
                clist->commands[cmd_count].argv[clist->commands[cmd_count].argc][EXE_MAX - 1] = '\0';  // Null-terminate the executable
                clist->commands[cmd_count].argc++;  // Increment argc for the executable
            } else {
                // Otherwise, split the string into the executable and arguments
                *exe_token = '\0';  // Null-terminate the executable
                strncpy(clist->commands[cmd_count].argv[clist->commands[cmd_count].argc], cmd_token, EXE_MAX - 1);
                clist->commands[cmd_count].argv[clist->commands[cmd_count].argc][EXE_MAX - 1] = '\0';  // Null-terminate the executable
                clist->commands[cmd_count].argc++;  // Increment argc for the executable

                // Move to the argument string (skipping spaces)
                args_token = exe_token + 1;
                trim_whitespace(args_token);  // Remove leading spaces for the arguments

                // If there are arguments, split them and add to argv[]
                if (args_token[0] != '\0') {
                    char *arg_token = strtok(args_token, " ");
                    while (arg_token != NULL) {
                        // Use strncpy to copy arguments into argv[]
                        strncpy(clist->commands[cmd_count].argv[clist->commands[cmd_count].argc], arg_token, ARG_MAX - 1);
                        clist->commands[cmd_count].argv[clist->commands[cmd_count].argc][ARG_MAX - 1] = '\0';  // Null-terminate each argument
                        clist->commands[cmd_count].argc++;  // Increment argc for each argument
                        arg_token = strtok(NULL, " ");
                    }
                }
            }
        }

        cmd_count++;

        // Move to the next command segment after the pipe
        cmd_token = strtok(NULL, PIPE_STRING);  // Get the next command after pipe
        
        // After strtok, skip any leading spaces in the next segment
        if (cmd_token != NULL) {
            // Skip leading spaces (if any)
            while (*cmd_token == SPACE_CHAR) {
                cmd_token++;  // Skip leading spaces after pipe
            }
        }
    }

    // Update the number of commands in the list
    clist->num = cmd_count;

    // Check if no commands were parsed
    if (clist->num == 0) {
        printf("warning: no commands provided\n");
        free(command_copy);
        return WARN_NO_CMDS;
    }

    free(command_copy); // Free the duplicate string
    return OK;
}


int execute_pipeline(command_list_t *clist) {
    // We will still need pipes, but now we get the commands from clist->commands.
    int pipes[clist->num - 1][2];  // Array of pipes for the number of commands
    pid_t pids[clist->num];         // Array to store process IDs

    // Create all necessary pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_MEMORY;  // Return error code on failure
        }
    }

    // Create processes for each command
    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            return ERR_MEMORY;  // Return error code on failure
        }

        if (pids[i] == 0) {  // Child process
            // Set up input pipe for all except first process
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);  // Redirect input from the previous pipe
            }

            // Set up output pipe for all except last process
            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);  // Redirect output to the next pipe
            }

            // Close all pipe ends in child
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            if (execvp(clist->commands[i].argv[0], clist->commands[i].argv) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Parent process: close all pipe ends
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], NULL, 0);
    }

    return OK;  // Successfully executed the pipeline
}


// Main execution loop to read commands and process them
int exec_local_cmd_loop() {
    char *cmd_line = NULL;
    size_t len = 0;
    ssize_t read;

    cmd_buff_t cmd_buff;

    while (1) {
        printf(SH_PROMPT);

        // Read the command line using getline
        read = getline(&cmd_line, &len, stdin);  // Read the input command line

        if (read == -1) {
            if (feof(stdin)) {
                break;  // EOF reached, exit loop
            } else {
                perror("getline failed");
                free(cmd_line);  // Free cmd_line to avoid memory leak
                return ERR_MEMORY;
            }
        }

        // Remove newline character (if present) after input is read
        cmd_line[strcspn(cmd_line, "\n")] = 0;

        // Trim leading and trailing whitespace from the command line
        trim_whitespace(cmd_line);  // Trim any unwanted spaces

        if (strlen(cmd_line) == 0) {
            continue;  // Skip empty commands
        }

        clear_cmd_buff(&cmd_buff);  // Clear any previous buffer

        // Build command buffer from input line
        if (build_cmd_buff(cmd_line, &cmd_buff) != OK) {
            free(cmd_line);  // Free cmd_line to avoid memory leak
            return ERR_MEMORY;
        }

        Built_In_Cmds built_in_cmd = exec_built_in_cmd(&cmd_buff);
        if (built_in_cmd != BI_NOT_BI) {
            // If it's a built-in command, update last_status and continue
            if (built_in_cmd == BI_CMD_EXIT) {
                break;  // Exit the shell
            }
            last_status = OK;  // Successful execution of built-in commands
            continue;  // Skip processing for built-in commands
        }

        // For external commands, use fork-exec
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execvp(cmd_buff.argv[0], cmd_buff.argv) == -1) {
                // Handle exec failure (set the last status to the error code)
                switch (errno) {
                    case ENOENT:
                        fprintf(stderr, "Command not found in PATH\n");
                        exit(2);  // Command not found, exit with status 2
                    case EACCES:
                        fprintf(stderr, "Permission denied\n");
                        exit(13);  // Permission denied, exit with status 126
                    default:
                        fprintf(stderr, "Failed to execute command: %s\n", strerror(errno));
                        exit(1);  // Generic error, exit with status 1
                }
            }
        } else if (pid > 0) {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);  // Get the exit status of the command
            } else {
                last_status = -1;  // If the child process didn't exit normally, set status to -1
            }
        } else {
            perror("fork failed");
            free(cmd_line);  // Free cmd_line to avoid memory leak
            return ERR_MEMORY;
        }

        // Free all arguments (allocated memory for each token)
        for (int i = 0; i < cmd_buff.argc; i++) {
            free(cmd_buff.argv[i]);
        }
    }

    free(cmd_line);  // Free the cmd_line buffer when done
    return OK;
}

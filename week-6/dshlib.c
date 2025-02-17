#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
*/
#include <errno.h>
int last_status = 0; // To store the last command's return code
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

// Function to build command buffer from input line
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
        printf("%d\n", last_status);  // Print the last exit status
        return ERR_CMD_ARGS_BAD;  // You can return any built-in exit constant (like BI_CMD_EXIT)
    }

    // Handle other built-in commands (like 'which', etc.)
    return BI_NOT_BI; // Return this for non-built-in commands
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

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


// Command structure
typedef struct {
    char **args;
} Command;

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

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    cmd_buff->input_redirect = 0;
    cmd_buff->output_redirect = 0;
    cmd_buff->input_fd = -1;
    cmd_buff->output_fd = -1;

    char *token = cmd_line;
    int in_quotes = 0;  // Flag to track if we're inside quotes
    char *start = NULL;

    // Trim leading whitespace
    trim_whitespace(token);

    while (*token != '\0' && cmd_buff->argc < CMD_ARGV_MAX) {
        // Skip leading whitespace unless we're inside quotes
        if (isspace((unsigned char)*token) && !in_quotes) {
            token++;
            continue;
        }

        // Handle input redirection
        if (*token == '<' && !in_quotes) {
            token++;  // Skip the '<' symbol
            trim_whitespace(token);
            if (*token == '\0') {
                return ERR_CMD_ARGS_BAD;  // No file provided after '<'
            }

            cmd_buff->input_redirect = 1;
            char *file = token;
            // Get the filename
            while (*token != '\0' && !isspace((unsigned char)*token)) {
                token++;
            }
            int file_len = token - file;
            char *input_file = malloc(file_len + 1);
            strncpy(input_file, file, file_len);
            input_file[file_len] = '\0';

            cmd_buff->input_fd = open(input_file, O_RDONLY);
            if (cmd_buff->input_fd == -1) {
                perror("Failed to open input file");
                return ERR_EXEC_CMD;
            }

            free(input_file);
            continue;
        }

        // Handle output redirection
        if (*token == '>' && !in_quotes) {
            token++;  // Skip the '>' symbol
            if (*token == '>') {  // Check for append redirection '>>'
                cmd_buff->output_redirect = 2;  // 2 means append mode
                token++;  // Skip the second '>'
            } else {
                cmd_buff->output_redirect = 1;  // 1 means overwrite mode
            }
            token++;  // Skip space
            trim_whitespace(token);
            if (*token == '\0') {
                return ERR_CMD_ARGS_BAD;  // No file provided after '>'
            }

            char *file = token;
            // Get the filename
            while (*token != '\0' && !isspace((unsigned char)*token)) {
                token++;
            }
            int file_len = token - file;
            char *output_file = malloc(file_len + 1);
            strncpy(output_file, file, file_len);
            output_file[file_len] = '\0';

            // Trim any additional spaces just in case
            trim_whitespace(output_file);

            if (cmd_buff->output_redirect == 1) {
                cmd_buff->output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            } else {
                cmd_buff->output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            }

            if (cmd_buff->output_fd == -1) {
                perror("Failed to open output file");
                return ERR_EXEC_CMD;
            }

            free(output_file);
            continue;
        }

        // Process the token (start of a new token)
        if (start == NULL) {
            start = token;
        }

        // Move token to the next space or quote, but keep inside quoted sections
        while (*token != '\0' && (in_quotes || !isspace((unsigned char)*token))) {
            if (*token == '"') {
                in_quotes = !in_quotes;  // Toggle the in_quotes flag when encountering a quote
            }
            token++;
        }

        // If we have a complete token (non-empty)
        if (start != NULL && token != start) {
            int token_length = token - start;

            // If the token ends with a quote (and we're outside quotes), remove it
            if (token_length > 0 && *(token - 1) == '"') {
                token_length--;  // Remove the quote at the end
            }

            // If the token starts with a quote, remove it
            if (token_length > 0 && *start == '"') {
                start++;  // Skip the starting quote
                token_length--;  // Decrease the token length
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
        printf("exiting...\n");
        return BI_CMD_EXIT;
    }

    // Handle the cd command
    if (strcmp(cmd->argv[0], "cd") == 0) {
        char *dir = cmd->argv[1];

        // If no argument provided, default to /tmp
        if (dir == NULL) {
            dir = "/tmp";  
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
        return ERR_CMD_ARGS_BAD;  
    }

    // Handle other built-in commands (like 'which', etc.)
    return BI_NOT_BI; // Return this for non-built-in commands
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    memset(clist->commands, 0, sizeof(clist->commands));
    char *cmd_token;
    char *command_copy = strdup(cmd_line);  // Make a copy of the input command line for strtok
    int cmd_count = 0;

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
            return ERR_TOO_MANY_COMMANDS;
        }

        // Trim whitespace from the command
        trim_whitespace(cmd_token);

        if (strlen(cmd_token) == 0) {
            printf("Error: Empty command in pipeline!\n");
            free(command_copy);
            return ERR_MEMORY;
        }

        // Tokenize the current command into executable and arguments
        char *exe_token = cmd_token;
        char *args_token = NULL;

        // Move exe_token to the first space or end of string
        while (*exe_token != '\0' && !isspace(*exe_token)) {
            exe_token++;  
        }

        if (*exe_token == '\0') {
            // If no space is found, the entire string is the executable
            clist->commands[cmd_count].argv[0] = strdup(cmd_token);
            clist->commands[cmd_count].argc = 1;
        } else {
            // Otherwise, split the string into executable and arguments
            *exe_token = '\0';  // Null-terminate the executable
            clist->commands[cmd_count].argv[0] = strdup(cmd_token);
            clist->commands[cmd_count].argc = 1;

            // Move to the argument string (skipping spaces)
            args_token = exe_token + 1;
            trim_whitespace(args_token);

            if (args_token[0] != '\0') {
                // Tokenize the remaining arguments by spaces
                char *arg_token = strtok(args_token, " ");
                while (arg_token != NULL) {
                    if (clist->commands[cmd_count].argc < CMD_ARGV_MAX) {
                        clist->commands[cmd_count].argv[clist->commands[cmd_count].argc] = strdup(arg_token);
                        clist->commands[cmd_count].argc++;
                    }
                    arg_token = strtok(NULL, " ");
                }
            }
        }

        // NULL terminate the argv array
        clist->commands[cmd_count].argv[clist->commands[cmd_count].argc] = NULL;


        cmd_count++;

        // Move to the next command segment after the pipe
        cmd_token = strtok(NULL, PIPE_STRING);

        // After strtok, skip any leading spaces in the next segment
        if (cmd_token != NULL) {
            while (*cmd_token == SPACE_CHAR) {
                cmd_token++;  
            }
        }
    }

    clist->num = cmd_count;

    if (clist->num == 0) {
        printf("Warning: No commands provided\n");
        free(command_copy);
        return WARN_NO_CMDS;
    }

    free(command_copy);
    return OK;
}

int execute_pipeline(command_list_t *clist) {
    if (clist->num < 1) return 1;

    int pipes[clist->num - 1][2];  
    pid_t pids[clist->num];

    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_MEMORY;
        }
    }

    for (int i = 0; i < clist->num; i++) {

        if (clist->commands[i].argv[0] == NULL) {
            fprintf(stderr, "Error: NULL command detected at index %d!\n", i);
            return ERR_MEMORY;
        }

        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            return ERR_MEMORY;
        }

        if (pids[i] == 0) {  // Child process
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);  
            }

            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);  
            }

            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], NULL, 0);
    }

    return OK;
}

int exec_cmd(cmd_buff_t *cmd_buff) {
    if (cmd_buff->input_redirect) {
        if (dup2(cmd_buff->input_fd, STDIN_FILENO) == -1) {
            perror("dup2 input failed");
            return ERR_EXEC_CMD;
        }
    }

    if (cmd_buff->output_redirect) {
        if (dup2(cmd_buff->output_fd, STDOUT_FILENO) == -1) {
            perror("dup2 output failed");
            return ERR_EXEC_CMD;
        }
    }

    // Close any open file descriptors after redirection
    if (cmd_buff->input_fd != -1) {
        close(cmd_buff->input_fd);
    }
    if (cmd_buff->output_fd != -1) {
        close(cmd_buff->output_fd);
    }

    execvp(cmd_buff->argv[0], cmd_buff->argv);

    // If execvp fails
    perror("execvp failed");
    return ERR_EXEC_CMD;
}



// Main execution loop to read commands and process them
int exec_local_cmd_loop() {
    char *cmd_line = NULL;
    size_t len = 0;
    ssize_t read;
    cmd_buff_t cmd_buff;
    command_list_t clist;

    while (1) {
        printf(SH_PROMPT);
        read = getline(&cmd_line, &len, stdin);
        if (read == -1) {
            if (feof(stdin)) {
                break;
            } else {
                perror("getline failed");
                free(cmd_line);
                return ERR_MEMORY;
            }
        }
        cmd_line[strcspn(cmd_line, "\n")] = 0;
        trim_whitespace(cmd_line);
        if (strlen(cmd_line) == 0) {
            continue;
        }

        clear_cmd_buff(&cmd_buff);
        if (build_cmd_buff(cmd_line, &cmd_buff) != OK) {
            free(cmd_line);
            return ERR_MEMORY;
        }

        Built_In_Cmds built_in_cmd = exec_built_in_cmd(&cmd_buff);
        if (built_in_cmd != BI_NOT_BI) {
            if (built_in_cmd == BI_CMD_EXIT) {
                break;
            }
            last_status = OK;
            continue; // Skip external command/pipeline processing
        }

        if (strchr(cmd_line, '|')) {
            if (build_cmd_list(cmd_line, &clist) != OK) {
                free(cmd_line);
                return ERR_MEMORY;
            }
            execute_pipeline(&clist);
        } else {
            pid_t pid = fork();
            if (pid == 0) {  // Child process
                // Special handling for echo command: remove quotes if present
                if (strcmp(cmd_buff.argv[0], "echo") == 0) {
                    // Check if the first argument has quotes around it
                    char *arg = cmd_buff.argv[1];
                    if (arg && arg[0] == '"' && arg[strlen(arg) - 1] == '"') {
                        // Remove the closing quote
                        arg[strlen(arg) - 1] = '\0';
                        // Shift the string to remove the opening quote
                        memmove(arg, arg + 1, strlen(arg));  
                    }
                }

                // Handle input redirection
                if (cmd_buff.input_redirect) {
                    if (dup2(cmd_buff.input_fd, STDIN_FILENO) == -1) {
                        perror("dup2 input");
                        exit(EXIT_FAILURE);
                    }
                    close(cmd_buff.input_fd);
                }

                // Handle output redirection
                if (cmd_buff.output_redirect) {
                    if (cmd_buff.output_redirect == 1) {  // Overwrite
                        if (dup2(cmd_buff.output_fd, STDOUT_FILENO) == -1) {
                            perror("dup2 output");
                            exit(EXIT_FAILURE);
                        }
                    } else if (cmd_buff.output_redirect == 2) {  // Append
                        if (dup2(cmd_buff.output_fd, STDOUT_FILENO) == -1) {
                            perror("dup2 append output");
                            exit(EXIT_FAILURE);
                        }
                    }
                    close(cmd_buff.output_fd);
                }

                // Execute the command
                if (execvp(cmd_buff.argv[0], cmd_buff.argv) == -1) {
                    switch (errno) {
                        case ENOENT:
                            fprintf(stderr, "Command not found in PATH\n");
                            exit(2);
                        case EACCES:
                            fprintf(stderr, "Permission denied\n");
                            exit(13);
                        default:
                            fprintf(stderr, "Failed to execute command: %s\n", strerror(errno));
                            exit(1);
                    }
                }
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    last_status = WEXITSTATUS(status);
                } else {
                    last_status = -1;
                }
            } else {
                perror("fork failed");
                free(cmd_line);
                return ERR_MEMORY;
            }
        }

        // Clean up allocated memory for arguments
        for (int i = 0; i < cmd_buff.argc; i++) {
            free(cmd_buff.argv[i]);
        }
    }

    free(cmd_line);
    return OK;
}




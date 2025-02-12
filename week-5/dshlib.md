#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dshlib.h"

/*
 * build_cmd_list
 *     cmd_line:     the command line from the user
 *     clist *:       pointer to clist structure to be populated
 *
 * This function builds the command_list_t structure passed by the caller
 * It does this by first splitting the cmd_line into commands by splitting
 * the string based on any pipe characters '|'. It then traverses each
 * command. For each command (a substring of cmd_line), it then parses
 * that command by taking the first token as the executable name, and
 * then the remaining tokens as the arguments.
 *
 * NOTE your implementation should be able to handle properly removing
 * leading and trailing spaces!
 *
 * errors returned:
 *
 *      OK:                 No Error
 *      ERR_TOO_MANY_COMMANDS: There is a limit of CMD_MAX (see dshlib.h)
 *                          commands.
 *      ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                           was larger than allowed, either the
 *                           executable name, or the arg string.
 */
void trim_whitespace(char *str) {
    // Check if the string is empty or only contains spaces
    if (str == NULL || *str == '\0') {
        return;  // If the string is empty, there's nothing to trim
    }

    // Skip leading spaces
    char *start = str;

    while (*start && isspace(*start)) {
        //printf("%s\n", start);
        start++;
    }

    // If the string consists of only spaces, return empty string
    if (*start == '\0') {
        *str = '\0';  // Set the string to an empty string
        return;
    }

    // Find the last non-space character
    //char *end = str + strlen(str) - 1;
    //while (*end == SPACE_CHAR) end--;

    char *end = str + strlen(str) - 1;
        while ((end > str) && (isspace(*end))){
            *end = '\0';
            end--;
        }

    // Null-terminate the string after trimming
    *(end + 1) = '\0';
}

/*void trim_whitespace(char *str) {
    char *start = str;
    while (*start == SPACE_CHAR) start++;  // Skip leading spaces
    char *end = str + strlen(str) - 1;
    while (*end == SPACE_CHAR) end--;  // Skip trailing spaces
    *(end + 1) = '\0';  // Null-terminate the string
}*/

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    
    memset(clist->commands, 0, sizeof(clist->commands)); //to remove any extra memory before the start of the program

    char *cmd_token;
    char *command_copy = strdup(cmd_line);  // Making a copy of the input command line for strtok
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

        // Start iterating over the command string and split into exe and args
        char *exe_token = cmd_token;
        char *args_token = NULL;

        while (*exe_token != '\0' && !isspace(*exe_token)) {
            exe_token++;  // Move to the first space or end of the string
        }

        if (*exe_token == '\0') {
            // If no space is found, the entire string is the executable
            strncpy(clist->commands[cmd_count].exe, cmd_token, EXE_MAX - 1);
            clist->commands[cmd_count].exe[EXE_MAX - 1] = '\0';
        } else {
            // Otherwise, split the string into the executable and arguments
            *exe_token = '\0';  // Null-terminate the executable
            strncpy(clist->commands[cmd_count].exe, cmd_token, EXE_MAX - 1);
            clist->commands[cmd_count].exe[EXE_MAX - 1] = '\0';

            // Move to the argument string (skipping spaces)
            args_token = exe_token + 1;
            trim_whitespace(args_token);

            if (args_token[0] != '\0') {
                strncpy(clist->commands[cmd_count].args, args_token, ARG_MAX - 1);
                clist->commands[cmd_count].args[ARG_MAX - 1] = '\0';  // null-terminate
            } else {
                clist->commands[cmd_count].args[0] = '\0';  // No arguments
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

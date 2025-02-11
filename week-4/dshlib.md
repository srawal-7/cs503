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
    char *start = str;
    while (*start == SPACE_CHAR) start++;  // Skip leading spaces
    char *end = str + strlen(str) - 1;
    while (*end == SPACE_CHAR) end--;  // Skip trailing spaces
    *(end + 1) = '\0';  // Null-terminate the string
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
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
            // Too many commands in a single line
            printf("warning: too many commands (maximum is %d)\n", CMD_MAX); 
            //free(command_copy);
            return ERR_TOO_MANY_COMMANDS;
        }

        // Trim whitespace from the command
        trim_whitespace(cmd_token);

        // Handle the executable name 
        char *exe_token = strtok(cmd_token, " ");  // First token is the executable name
        if (exe_token == NULL) {
            // If no executable found in the command, skip this part
            cmd_token = strtok(NULL, PIPE_STRING);
            continue;
        }

        // Save executable name
        strncpy(clist->commands[cmd_count].exe, exe_token, EXE_MAX - 1);
        clist->commands[cmd_count].exe[EXE_MAX - 1] = '\0'; // null-termination

        // Parse arguments for this command (remaining part after the executable)
        char *args_token = strtok(NULL, ""); // Get the rest of the string as arguments
        if (args_token != NULL) {
            trim_whitespace(args_token);  // Remove spaces around arguments
            strncpy(clist->commands[cmd_count].args, args_token, ARG_MAX - 1);
            clist->commands[cmd_count].args[ARG_MAX - 1] = '\0'; // null-termination
        } else {
            clist->commands[cmd_count].args[0] = '\0';  // No arguments for this command
        }

        cmd_count++;
        cmd_token = strtok(NULL, PIPE_STRING);  // Move to the next command
    }

    // Update the number of commands in the list
    clist->num = cmd_count;

    // Check if no commands were parsed
    if (clist->num == 0) {
        printf("warning: no commands provided\n");
        free(command_copy);
        return WARN_NO_CMDS;
    }

    free(command_copy); //free space
    return OK;
}

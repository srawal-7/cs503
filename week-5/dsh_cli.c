#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dshlib.h"

/*
 * Implement your main function by building a loop that prompts the
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.  Since we want fgets to also handle
 * end of file so we can run this headless for testing we need to check
 * the return code of fgets.  I have provided an example below of how
 * to do this assuming you are storing user input inside of the cmd_buff
 * variable.
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
 *
 *   Expected output:
 *
 *      CMD_OK_HEADER      if the command parses properly. You will
 *                         follow this by the command details
 *
 *      CMD_WARN_NO_CMD    if the user entered a blank command
 *      CMD_ERR_PIPE_LIMIT if the user entered too many commands using
 *                         the pipe feature, e.g., cmd1 | cmd2 | ... |
 *
 *  See the provided test cases for output expectations.
 */

int main() {
    char cmd_buff[SH_CMD_MAX];  // Buffer to store the user input
    int rc = 0;  // Return code for error checking
    command_list_t clist;

    // Main loop to accept commands from the user
    while (1) {
        printf("%s", SH_PROMPT);  // Print the prompt
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;  // Exit if there's an input error
        }

        // Remove the trailing newline character from cmd_buff
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Trim leading and trailing whitespace (if any) from the user input
        trim_whitespace(cmd_buff);

        // Handle empty input
        if (strlen(cmd_buff) == 0) {
            printf(CMD_WARN_NO_CMD);  // Use the defined constant for empty input warning
            continue;  // Skip processing and go back to the prompt
        }

        // If the user entered "exit", break the loop
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            break;
        }

        // Build the command list
        rc = build_cmd_list(cmd_buff, &clist);

        // Handle too many commands after the parsing
        if (clist.num > CMD_MAX) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);  // Error: Too many commands (limit is CMD_MAX)
            continue;
        }

        // Handle different results based on return code from build_cmd_list
        if (rc == OK) {
            // Print the header with the number of commands parsed
            printf(CMD_OK_HEADER, clist.num);

            // Print each command and its arguments
            for (int i = 0; i < clist.num; ++i) {
                // Print the command name
                printf("<%d>%s", i + 1, clist.commands[i].exe);

                // If the command has arguments, print them inside square brackets
                if (clist.commands[i].args[0] != '\0') {
                    // In this new version, `args` is a single concatenated string.
                    // This should print all the arguments space-separated.
                    printf("[%s]", clist.commands[i].args);
                }
                printf("\n");
            }

        } else if (rc == WARN_NO_CMDS) {
            printf(CMD_WARN_NO_CMD);  // Print warning if no commands were provided
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);  // Print error if there are too many commands
        } else if (rc == ERR_CMD_OR_ARGS_TOO_BIG) {
            printf("error: command or argument too big\n");  // Handle the case of too large commands
        }
    }

    return 0;  // Exit the shell
}

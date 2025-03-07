
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h> // Include unistd.h for sleep()

//INCLUDES for extra credit
//#include <signal.h>
//#include <pthread.h>
//-------------------------

#include "dshlib.h"
#include "rshlib.h"


/*
 * start_server(ifaces, port, is_threaded)
 *      ifaces:  a string in ip address format, indicating the interface
 *              where the server will bind.  In almost all cases it will
 *              be the default "0.0.0.0" which binds to all interfaces.
 *              note the constant RDSH_DEF_SVR_INTFACE in rshlib.h
 * 
 *      port:   The port the server will use.  Note the constant 
 *              RDSH_DEF_PORT which is 1234 in rshlib.h.  If you are using
 *              tux you may need to change this to your own default, or even
 *              better use the command line override -s implemented in dsh_cli.c
 *              For example ./dsh -s 0.0.0.0:5678 where 5678 is the new port  
 * 
 *      is_threded:  Used for extra credit to indicate the server should implement
 *                   per thread connections for clients  
 * 
 *      This function basically runs the server by: 
 *          1. Booting up the server
 *          2. Processing client requests until the client requests the
 *             server to stop by running the `stop-server` command
 *          3. Stopping the server. 
 * 
 *      This function is fully implemented for you and should not require
 *      any changes for basic functionality.  
 * 
 *      IF YOU IMPLEMENT THE MULTI-THREADED SERVER FOR EXTRA CREDIT YOU NEED
 *      TO DO SOMETHING WITH THE is_threaded ARGUMENT HOWEVER.  
 */
int start_server(char *ifaces, int port){
    int svr_socket;
    int rc;

    //
    //TODO:  If you are implementing the extra credit, please add logic
    //       to keep track of is_threaded to handle this feature
    //

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0){
        int err_code = svr_socket;  //server socket will carry error code
        return err_code;
    }

    rc = process_cli_requests(svr_socket);

    stop_server(svr_socket);

    return rc;
}
/*
 * stop_server(svr_socket)
 *      svr_socket: The socket that was created in the boot_server()
 *                  function. 
 * 
 *      This function simply returns the value of close() when closing
 *      the socket.  
 */
int stop_server(int svr_socket){
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 *      ifaces & port:  see start_server for description.  They are passed
 *                      as is to this function.   
 * 
 *      This function "boots" the rsh server.  It is responsible for all
 *      socket operations prior to accepting client connections.  Specifically: 
 * 
 *      1. Create the server socket using the socket() function. 
 *      2. Calling bind to "bind" the server to the interface and port
 *      3. Calling listen to get the server ready to listen for connections.
 * 
 *      after creating the socket and prior to calling bind you might want to 
 *      include the following code:
 * 
 *      int enable=1;
 *      setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
 * 
 *      when doing development you often run into issues where you hold onto
 *      the port and then need to wait for linux to detect this issue and free
 *      the port up.  The code above tells linux to force allowing this process
 *      to use the specified port making your life a lot easier.
 * 
 *  Returns:
 * 
 *      server_socket:  Sockets are just file descriptors, if this function is
 *                      successful, it returns the server socket descriptor, 
 *                      which is just an integer.
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code is returned if the socket(),
 *                               bind(), or listen() call fails. 
 *
 */

 int boot_server(char *ifaces, int port) {
    int svr_socket;
    struct sockaddr_in server_addr;
    int opt = 1;

    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("Error creating socket");
        return ERR_RDSH_SERVER;
    }

    // Allow socket reuse
    if (setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error setting socket options");
        close(svr_socket);
        return ERR_RDSH_SERVER;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ifaces, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(svr_socket);
        return ERR_RDSH_SERVER;
    }

    if (bind(svr_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(svr_socket);
        return ERR_RDSH_SERVER;
    }

    if (listen(svr_socket, 5) < 0) {
        perror("Error listening on socket");
        close(svr_socket);
        return ERR_RDSH_SERVER;
    }

    return svr_socket;
}


/*
 * process_cli_requests(svr_socket)
 *      svr_socket:  The server socket that was obtained from boot_server()
 *   
 *  This function handles managing client connections.  It does this using
 *  the following logic
 * 
 *      1.  Starts a while(1) loop:
 *  
 *          a. Calls accept() to wait for a client connection. Recall that 
 *             the accept() function returns another socket specifically
 *             bound to a client connection. 
 *          b. Calls exec_client_requests() to handle executing commands
 *             sent by the client. It will use the socket returned from
 *             accept().
 *          c. Loops back to the top (step 2) to accept connecting another
 *             client.  
 * 
 *          note that the exec_client_requests() return code should be
 *          negative if the client requested the server to stop by sending
 *          the `stop-server` command.  If this is the case step 2b breaks
 *          out of the while(1) loop. 
 * 
 *      2.  After we exit the loop, we need to cleanup.  Dont forget to 
 *          free the buffer you allocated in step #1.  Then call stop_server()
 *          to close the server socket. 
 * 
 *  Returns:
 * 
 *      OK_EXIT:  When the client sends the `stop-server` command this function
 *                should return OK_EXIT. 
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code terminates the loop and is
 *                returned from this function in the case of the accept() 
 *                function failing. 
 * 
 *      OTHERS:   See exec_client_requests() for return codes.  Note that positive
 *                values will keep the loop running to accept additional client
 *                connections, and negative values terminate the server. 
 * 
 */


int process_cli_requests(int svr_socket) {
    int cli_socket;
    int     rc = OK;    

    while (1) {
        printf("Server: Waiting for client data...\n");
        cli_socket = accept(svr_socket, NULL, NULL); //(struct sockaddr *)&client_addr, &client_len);
        if (cli_socket < 0) {
            perror("Error accepting client connection");
            continue; 
        }

        printf("Client connected\n");

        rc = exec_client_requests(cli_socket);
        if (rc < 0){
            break;
        }
    }

    stop_server(cli_socket);
    return rc;
}

/*
 * exec_client_requests(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client
 *   
 *  This function handles accepting remote client commands. The function will
 *  loop and continue to accept and execute client commands.  There are 2 ways
 *  that this ongoing loop accepting client commands ends:
 * 
 *      1.  When the client executes the `exit` command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection. 
 *      2.  When the client executes the `stop-server` command this function
 *          returns to process_cli_requests() with a return code of OK_EXIT
 *          indicating that the server should stop. 
 * 
 *  Note that this function largely follows the implementation of the
 *  exec_local_cmd_loop() function that you implemented in the last 
 *  shell program deliverable. The main difference is that the command will
 *  arrive over the recv() socket call rather than reading a string from the
 *  keyboard. 
 * 
 *  This function also must send the EOF character after a command is
 *  successfully executed to let the client know that the output from the
 *  command it sent is finished.  Use the send_message_eof() to accomplish 
 *  this. 
 * 
 *  Of final note, this function must allocate a buffer for storage to 
 *  store the data received by the client. For example:
 *     io_buff = malloc(RDSH_COMM_BUFF_SZ);
 *  And since it is allocating storage, it must also properly clean it up
 *  prior to exiting.
 * 
 *  Returns:
 * 
 *      OK:       The client sent the `exit` command.  Get ready to connect
 *                another client. 
 *      OK_EXIT:  The client sent `stop-server` command to terminate the server
 * 
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors. 
 */

 int exec_client_requests(int cli_socket) {
    int io_size;
    command_list_t cmd_list;
    int rc;
    int last_rc;
    int cmd_rc;
    char *io_buff;
    io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (io_buff == NULL) {
        return ERR_RDSH_SERVER;
    }

    while (1) {
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);
        printf("memset\n");

        io_size = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ, 0);
        if (io_size == -1) {
            perror("recv");
            free(io_buff);
            close(cli_socket);
            return ERR_RDSH_COMMUNICATION;
        }

        if (io_size == 0) {
            printf(RCMD_MSG_CLIENT_EXITED);
            break;      //leave loop, close connection
        }

        rc = build_cmd_list((char *)io_buff, &cmd_list);
        switch (rc) {
            case ERR_MEMORY:
                sprintf((char *)io_buff, CMD_ERR_RDSH_ITRNL, ERR_MEMORY);
                send_message_string(cli_socket, (char *)io_buff);
                continue;
            case WARN_NO_CMDS:
                sprintf((char *)io_buff, CMD_ERR_RDSH_ITRNL, WARN_NO_CMDS);
                send_message_string(cli_socket, (char *)io_buff);
                continue;
            case ERR_CMD_OR_ARGS_TOO_BIG:
                sprintf((char *)io_buff, CMD_ERR_PIPE_LIMIT, CMD_MAX);
                send_message_string(cli_socket, (char *)io_buff);
                continue;
            default:
                break;
        }
        // Check for built-in command 
        Built_In_Cmds built_in_cmd = rsh_built_in_cmd(&(cmd_list.commands[0]));
        //printf("exec_client_requests: rsh_built_in_cmd returned %d\n", built_in_cmd);
        if (built_in_cmd != BI_NOT_BI) {
            // Handle built-in command directly
            switch (built_in_cmd) {
                case BI_CMD_EXIT:
                    printf(RCMD_MSG_CLIENT_EXITED);
                    free(io_buff);
                    close(cli_socket);
                    return OK;
                case BI_CMD_STOP_SVR:
                    printf(RCMD_MSG_SVR_STOP_REQ);
                    free(io_buff);
                    close(cli_socket);
                    return OK_EXIT;
                case BI_EXECUTED:
                    send_message_eof(cli_socket);
                    continue;
                default:
                    break;
            }

        } else {
            // Execute pipeline for external commands
            last_rc = cmd_rc;
            cmd_rc = rsh_execute_pipeline(cli_socket, &cmd_list);

            switch (cmd_rc) {
                case RC_SC:
                    sprintf((char *)io_buff, RCMD_MSG_SVR_RC_CMD, last_rc);
                    send_message_string(cli_socket, (char *)io_buff);
                    continue;
                case EXIT_SC:
                    printf(RCMD_MSG_CLIENT_EXITED);
                    free(io_buff);
                    close(cli_socket);
                    return OK;
                case STOP_SERVER_SC:
                    printf(RCMD_MSG_SVR_STOP_REQ);
                    free(io_buff);
                    close(cli_socket);
                    return OK_EXIT;
                default:
                    break;
            }
        }

        rc = send_message_eof(cli_socket);
        if (rc != OK) {
            printf(CMD_ERR_RDSH_COMM);
            free(io_buff);
            close(cli_socket);
            return ERR_RDSH_COMMUNICATION;
        }

        printf(RCMD_MSG_SVR_EXEC_REQ, io_buff);

    }

    free(io_buff);
    close(cli_socket);
    return OK;
}


int execute_command(int cli_socket, cmd_buff_t *cmd) {
    int pipe_fd[2];
    char rsp_buff[RDSH_COMM_BUFF_SZ];
    pid_t pid;
    int status;
    ssize_t bytes_read;
    ssize_t bytes_sent;

    if (pipe(pipe_fd) == -1) {
        perror("Pipe error");
        return ERR_RDSH_COMMUNICATION;
    }

    pid = fork();

    if (pid < 0) {
        perror("Fork error");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return ERR_RDSH_COMMUNICATION;
    }

    if (pid == 0) { // Child process
        for (int i = 0; cmd->argv[i] != NULL; i++) {
        }

        if (close(pipe_fd[0]) == -1) perror("child close read end");
        if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) perror("child dup2 stdout");
        if (dup2(pipe_fd[1], STDERR_FILENO) == -1) perror("child dup2 stderr");
        if (close(pipe_fd[1]) == -1) perror("child close write end");

        execvp(cmd->argv[0], cmd->argv);

        perror("execvp failed"); // Only runs if exec fails
        //printf("Server: execute_command - errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    // Parent process
    if (close(pipe_fd[1]) == -1) perror("parent close write end");

    printf("Server: execute_command - Parent process command: %s\n", cmd->argv[0]);

    memset(rsp_buff, 0, sizeof(rsp_buff));
    while ((bytes_read = read(pipe_fd[0], rsp_buff, sizeof(rsp_buff) - 1)) > 0) {
        bytes_sent = send(cli_socket, rsp_buff, bytes_read, 0);
        if (bytes_sent == -1) {
            perror("send failed");
            break; // Exit the loop on send error
        }
    }

    if (close(pipe_fd[0]) == -1) perror("parent close read end");

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid failed");
    } else {
        printf("Server: execute_command - child process exited with status: %d\n", status);
    }

    send_message_eof(cli_socket); // Ensure EOF is sent
    return 0;
}


/*
 * send_message_eof(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client

 *  Sends the EOF character to the client to indicate that the server is
 *  finished executing the command that it sent. 
 * 
 *  Returns:
 * 
 *      OK:  The EOF character was sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the EOF character. 
 */

int send_message_eof(int cli_socket) {
    printf("Server: send_message_eof - Sending EOF\n");
    // Sending the EOF character (0x04) to the client
    char eof_char = RDSH_EOF_CHAR;

    // Send the EOF character through the socket
    if (send(cli_socket, &eof_char, sizeof(eof_char), 0) == -1) {
        perror("send_message_eof: Error sending EOF");
        return ERR_RDSH_COMMUNICATION;  // Return error code if sending fails
    }

    return 0;  // Return 0 to indicate success
}
/*
 * send_message_string(cli_socket, char *buff)
 *      cli_socket:  The server-side socket that is connected to the client
 *      buff:        A C string (aka null terminated) of a message we want
 *                   to send to the client. 
 *   
 *  Sends a message to the client.  Note this command executes both a send()
 *  to send the message and a send_message_eof() to send the EOF character to
 *  the client to indicate command execution terminated. 
 * 
 *  Returns:
 * 
 *      OK:  The message in buff followed by the EOF character was 
 *           sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the message followed by the EOF character. 
 */


int send_message_string(int cli_socket, char *buff) {
    printf("Server: send_message_string - Sending: %s\n", buff);
    size_t msg_length = strlen(buff);

    // Send the message buffer to the client
    if (send(cli_socket, buff, msg_length, 0) == -1) {
        perror("send_message_string: Error sending message");
        return ERR_RDSH_COMMUNICATION; // Return error code if sending fails
    }
    return send_message_eof(cli_socket); // Ensure EOF is sent
}


/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *      cli_sock:    The server-side socket that is connected to the client
 *      clist:       The command_list_t structure that we implemented in
 *                   the last shell. 
 *   
 *  This function executes the command pipeline.  It should basically be a
 *  replica of the execute_pipeline() function from the last deliverable. 
 *  The only thing different is that you will be using the cli_sock as the
 *  main file descriptor on the first executable in the pipeline for STDIN,
 *  and the cli_sock for the file descriptor for STDOUT, and STDERR for the
 *  last executable in the pipeline.  See picture below:  
 * 
 *      
 *┌───────────┐                                                    ┌───────────┐
 *│ cli_sock  │                                                    │ cli_sock  │
 *└─────┬─────┘                                                    └────▲──▲───┘
 *      │   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐  │  │    
 *      │   │   Process 1  │     │   Process 2  │     │   Process N  │  │  │    
 *      │   │              │     │              │     │              │  │  │    
 *      └───▶stdin   stdout├─┬──▶│stdin   stdout├─┬──▶│stdin   stdout├──┘  │    
 *          │              │ │   │              │ │   │              │     │    
 *          │        stderr├─┘   │        stderr├─┘   │        stderr├─────┘    
 *          └──────────────┘     └──────────────┘     └──────────────┘   
 *                                                      WEXITSTATUS()
 *                                                      of this last
 *                                                      process to get
 *                                                      the return code
 *                                                      for this function       
 * 
 *  Returns:
 * 
 *      EXIT_CODE:  This function returns the exit code of the last command
 *                  executed in the pipeline.  If only one command is executed
 *                  that value is returned.  Remember, use the WEXITSTATUS()
 *                  macro that we discussed during our fork/exec lecture to
 *                  get this value. 
 */

 int rsh_execute_pipeline(int socket_fd, command_list_t *clist) {
    int num_cmds = clist->num;
    int pipe_fds[2 * (num_cmds - 1)];
    pid_t pid;
    int i, status;
    int last_pid = -1;

    // Create pipes for inter-process communication
    for (i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipe_fds + i * 2) == -1) {
            perror("Error creating pipe");
            return ERR_RDSH_CMD_EXEC;
        }
    }

    // Iterate over each command in the pipeline
    for (i = 0; i < num_cmds; i++) {
        pid = fork();
        if (pid == -1) {
            perror("Fork failed");
            return ERR_RDSH_CMD_EXEC;
        }

        if (pid == 0) {  // Child process
            // If first command, set input from socket_fd
            if (i == 0) {
                if (dup2(socket_fd, STDIN_FILENO) == -1) {
                    perror("dup2 failed (STDIN for first command)");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Otherwise, set input from previous pipe
                if (dup2(pipe_fds[(i - 1) * 2], STDIN_FILENO) == -1) {
                    perror("dup2 failed (STDIN)");
                    exit(EXIT_FAILURE);
                }
            }

            // If last command, output to socket_fd
            if (i == num_cmds - 1) {
                if (dup2(socket_fd, STDOUT_FILENO) == -1) {
                    perror("dup2 failed (STDOUT for last command)");
                    exit(EXIT_FAILURE);
                }
                if (dup2(socket_fd, STDERR_FILENO) == -1) {
                    perror("dup2 failed (STDERR for last command)");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Otherwise, set output to next pipe
                if (dup2(pipe_fds[i * 2 + 1], STDOUT_FILENO) == -1) {
                    perror("dup2 failed (STDOUT)");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipes
            for (int j = 0; j < 2 * (num_cmds - 1); j++) {
                close(pipe_fds[j]);
            }

            // Execute the command with arguments
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }

        last_pid = pid;  // Track the last process ID
    }

    // Parent process: close all pipes
    for (i = 0; i < 2 * (num_cmds - 1); i++) {
        close(pipe_fds[i]);
    }

    // Wait for all child processes and get exit status of the last one
    for (i = 0; i < num_cmds; i++) {
        if (waitpid(last_pid, &status, 0) == last_pid) {
            return WEXITSTATUS(status);  // Return exit status of last command
        }
    }

    return 0;
}


/*
 * rsh_match_command(const char *input)
 *      cli_socket:  The string command for a built-in command, e.g., dragon,
 *                   cd, exit-server
 *   
 *  This optional function accepts a command string as input and returns
 *  one of the enumerated values from the BuiltInCmds enum as output. For
 *  example:
 * 
 *      Input             Output
 *      exit              BI_CMD_EXIT
 *      dragon            BI_CMD_DRAGON
 * 
 *  This function is entirely optional to implement if you want to handle
 *  processing built-in commands differently in your implementation. 
 * 
 *  Returns:
 * 
 *      BI_CMD_*:   If the command is built-in returns one of the enumeration
 *                  options, for example "cd" returns BI_CMD_CD
 * 
 *      BI_NOT_BI:  If the command is not "built-in" the BI_NOT_BI value is
 *                  returned. 
 */

Built_In_Cmds rsh_match_command(const char *input)
{
    // Compare the input with various built-in commands
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    else if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    else if (strcmp(input, "stop-server") == 0)
        return BI_CMD_STOP_SVR;
    else
        return BI_NOT_BI;  // Not a built-in command
}
/*
 * rsh_built_in_cmd(cmd_buff_t *cmd)
 *      cmd:  The cmd_buff_t of the command, remember, this is the 
 *            parsed version fo the command
 *   
 *  This optional function accepts a parsed cmd and then checks to see if
 *  the cmd is built in or not.  It calls rsh_match_command to see if the 
 *  cmd is built in or not.  Note that rsh_match_command returns BI_NOT_BI
 *  if the command is not built in. If the command is built in this function
 *  uses a switch statement to handle execution if appropriate.   
 * 
 *  Again, using this function is entirely optional if you are using a different
 *  strategy to handle built-in commands.  
 * 
 *  Returns:
 * 
 *      BI_NOT_BI:   Indicates that the cmd provided as input is not built
 *                   in so it should be sent to your fork/exec logic
 *      BI_EXECUTED: Indicates that this function handled the direct execution
 *                   of the command and there is nothing else to do, consider
 *                   it executed.  For example the cmd of "cd" gets the value of
 *                   BI_CMD_CD from rsh_match_command().  It then makes the libc
 *                   call to chdir(cmd->argv[1]); and finally returns BI_EXECUTED
 *      BI_CMD_*     Indicates that a built-in command was matched and the caller
 *                   is responsible for executing it.  For example if this function
 *                   returns BI_CMD_STOP_SVR the caller of this function is
 *                   responsible for stopping the server.  If BI_CMD_EXIT is returned
 *                   the caller is responsible for closing the client connection.
 * 
 *   AGAIN - THIS IS TOTALLY OPTIONAL IF YOU HAVE OR WANT TO HANDLE BUILT-IN
 *   COMMANDS DIFFERENTLY. 
 */

 Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd)
 {
     Built_In_Cmds cmd_type = rsh_match_command(cmd->argv[0]);
     
     switch (cmd_type) {
         case BI_CMD_EXIT:
             return BI_CMD_EXIT;  // Handle client disconnection
         
         case BI_CMD_CD:
             if (cmd->argc > 1) {
                 if (chdir(cmd->argv[1]) < 0) {
                     perror("cd failed");
                     return BI_NOT_BI;  // Indicate failure
                 }
             } else {
                 fprintf(stderr, "cd: missing argument\n");
                 return BI_NOT_BI;
             }
             return BI_EXECUTED;  // Indicate successful execution
 
         case BI_CMD_STOP_SVR:
             return BI_CMD_STOP_SVR;  // Caller should handle stopping the server
 
         default:
             return BI_NOT_BI;  // Command is not built-in
     }
 }
 

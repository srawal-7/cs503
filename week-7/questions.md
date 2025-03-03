1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

My implementation ensures all child processes complete before accepting new input by calling waitpid() for each child in the execute_pipeline function. If waitpid() were omitted, the shell would not wait for child processes to finish, potentially causing orphaned child processes (zombies) and allowing the shell to accept new input prematurely.

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

It is necessary to close unused pipe ends after calling dup2() to prevent file descriptor leakage and ensure proper communication between processes. Unclosed pipes can lead to resource leaks, processes hanging, and unexpected behavior because the system doesn't know the communication is finished. Ex. Incorrect read/write behavior or unintended data flow.

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

The cd command is implemented as a built-in command because it needs to modify the shell's current working directory, which directly affects the parent shell process. If cd were implemented as an external command, it would run in a child process, and any changes to the working directory would be lost once the child process terminates. This would make it difficult for the user to effectively navigate the filesystem using cd in an external process.

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

To allow an arbitrary number of piped commands, I would modify the implementation to use dynamic memory allocation instead of the fixed CMD_MAX array. By using malloc() initially and then realloc() as more commands are added, the shell can dynamically adjust the size of the command list as needed. This would enable support for any number of piped commands without being restricted by a predefined limit.

Potential trade-offs:
1. Memory usage could increase significantly if users frequently issue commands with many pipes
2. Reallocating memory repeatedly can introduce performance issues, especially with a large number of commands
3. Using dynamic memory can add more complexity to the code, which will require additional handling to prevent any memory leaks or segmentation faults
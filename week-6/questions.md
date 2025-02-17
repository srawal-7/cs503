1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  Using fork() and execvp() together lets the shell run commands in a separate child process, so the shell itself stays running and can continue accepting user input. Without fork(), the shell would be replaced by the command, making it difficult to issue new commands until the current one finishes. fork() creates a new process, and execvp() runs the command in that process, allowing the shell to keep working while the command executes in the background.

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  If the fork() system call fails in my code, it returns -1, and the child process is not created. In this case, the shell will print an error message using perror("fork failed") and then return an error code (ERR_MEMORY) to handle the failure gracefully, ensuring that the program doesn't crash and gives feedback on what went wrong.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**:  execvp() finds the command to execute by searching through directories listed in the PATH environment variable. If it finds the command in one of these directories, it executes it; otherwise, it returns an error. The PATH variable plays an important role in this process by defining the locations where execvp() should search for the command.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  The purpose of calling wait() in the parent process after forking is to make the parent wait for the child process to finish executing before it continues. This ensures that the parent process can retrieve the exit status of the child process and perform any necessary clean-up. If we didn’t call wait(), the parent could continue executing immediately, potentially leading to child processes that have finished executing but still exist in the system. Additionally, without wait(), the parent wouldn't know when the child has finished or what its exit status was.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**:  WEXITSTATUS() is used to extract the exit status of a child process after it has finished executing. It provides the return code of the child process, which was set when the child called exit() or when it finished its execution. It is important because it allows the parent process to know if the child process executed successfully or encountered an error. 

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**: Quoted arguments are handled by using a flag (in_quotes) to track whether the current token is inside double quotes. When the function encounters a quote ("), it toggles the in_quotes flag, making the function treat the content inside quotes as a single argument, even if it contains spaces. The function then continues processing the characters between quotes as part of the same token, and once a closing quote is found, it stops including characters in the argument. This is necessary to handle arguments with spaces properly in shell commands, or they will be split incorrectly.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:  Compared to the previous assignment, I broke the code down into smaller functions as per the guidelines. I included multiple helper functions to properly trim whitespaces as well as to correctly allocate and free the memory. It includes better validation and exception handling mechanisms as well. Most of my challenges came from handling quoted spaces because of which I intriduced additional helper functions because i was having trouble freeing the memory. 

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**: Signals are used to notify processes about events or conditions, such as stopping, resuming, or terminating a process. Signals are asynchronous and lightweight, typically conveying simple commands like "terminate" or "pause." Unlike other IPC mechanisms, such as message queues or shared memory, signals are focused on basic process control and error handling. They differ in that they don’t transfer large data and cannot be blocked unless explicitly handled by the process.

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  SIGINT is typically used to interrupt a running process by the user, SIGTERM is used to gracefully terminate a process, allowing it to perform cleanup operations, and SIGKILL is used to forcefully terminate a process immediately, without any chance for cleanup. 

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  When a process receives SIGSTOP, it is immediately stopped (paused) and cannot continue execution until it is resumed. Unlike SIGINT, SIGSTOP cannot be caught, ignored, or handled by the process. This is because SIGSTOP is a signal designed to control process execution at the system level and is used by the kernel to temporarily stop a process.
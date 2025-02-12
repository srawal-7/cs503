1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**:  fgets() is a good choice for reading user input in a shell because it reads input line by line, ensuring that commands with spaces are captured correctly. It prevents buffer overflow by allowing us to specify a maximum input size, and it handles newlines, which makes it easy to process user input. It can detect end-of-file (EOF) and return NULL, allowing the shell to handle EOF gracefully. This makes it a robust and secure method for gathering user input.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  Using malloc() to allocate memory for cmd_buff instead of a fixed-size array allows for more flexible memory management. A fixed-size array is limited to a predefined buffer size, which can lead to inefficiency or failure if the user enters a command that exceeds this fixed size. With malloc(), memory is dynamically allocated based on the actual size needed for the command input. This ensures that your shell can handle varying amounts of input without wasting memory or risking buffer overflow. Additionally, dynamically allocated memory can be freed after use, providing better memory management in the long run, especially when handling potentially large or varying input sizes.


3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**:  Trimming leading and trailing spaces is necessary to ensure that commands and arguments are correctly parsed and executed. If spaces are not removed, the shell might misinterpret or fail to recognize the command, leading to errors. Extra spaces could also cause issues when handling arguments, potentially resulting in incorrect execution or failure to execute commands properly. Trimming spaces helps ensure accurate parsing and smooth operation of the shell.
    
4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  We can include output redirection (> file.txt) to write to a file. We can also include input redirection to import information from a file (< file.txt) and lastly, we can use append redirection (>> file.txt) to append additional information to an already existing file without overwriting its contents. Challenges for redirection in our shell can include properly handling file operations like opening, reading, writing, and closing files, while managing errors such as missing files or permission issues. Additionally, the shell must correctly parse commands to distinguish between executables, arguments, and redirection operators. Error handling will be important for scenarios like file not found, permission denied, or disk space issues.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  Redirection involves changing where data goes, either from stdin or to stdout, often using files. Piping, on the other hand, connects the output of one command directly to the input of another, forming chains of commands. Redirection focuses on file input/output, while piping connects commands directly.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  Separating STDOUT and STDERR allows users to distinguish between regular output and error messages. This helps in debugging and offers flexibility for redirection. For example, you can redirect only errors to a file or pipe only standard output to another command, making it easier to manage outputs effectively.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**: Our shell should check the return code of commands, reporting errors to STDERR even if STDOUT has output.  A mechanism to merge STDOUT and STDERR (command &> file) is useful for unified logging. This can be done by redirecting STDERR to STDOUT's file descriptor before command execution.  This ensures all output, including errors, is captured in one stream.
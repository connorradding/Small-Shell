small-shell is a custom shell written in C that mimics some functionalities of well-known shells like bash.<br />
This shell was developed as part of an educational assignment to deepen understanding of Unix processes, signals, and I/O redirection.<br /><br />

Features<br />
Command Prompt with Colon Symbol (:): small-shell uses a colon as the prompt symbol for command input.<br />
Command Execution: Supports executing commands with arguments, handling input/output redirection, and background process execution.<br />
Built-in Commands: Includes three built-in commands: exit, cd, and status.<br />
Signal Handling: Implements custom handlers for SIGINT and SIGTSTP.<br />
Variable Expansion: Supports expansion of $$ to the process ID of the shell.<br />
Comment and Blank Line Handling: Ignores lines starting with # and blank lines.<br /><br />

Prerequisites<br />
A Unix-like environment<br />
GCC compiler<br /><br />

Compile<br />
compile code with: gcc --std=gnu99 -o small-shell small-shell.c<br />
run code with: ./small-shell<br />

small-shell is a custom shell written in C that mimics some functionalities of well-known shells like bash. <br />
This shell was developed as part of an educational assignment to deepen understanding of Unix processes, signals, and I/O redirection.<br /><br />

Features<br />
Command Prompt with Colon Symbol (:): small-shell uses a colon as the prompt symbol for command input.<br />
Command Execution: Supports executing commands with arguments, handling input/output redirection, and background process execution.<br />
Built-in Commands: Includes three built-in commands: exit, cd, and status.<br />
Signal Handling: Implements custom handlers for SIGINT and SIGTSTP.<br />
Variable Expansion: Supports expansion of $$ to the process ID of the shell.<br />
Comment and Blank Line Handling: Ignores lines starting with # and blank lines.<br />
Getting Started<br />
Prerequisites<br />
A Unix-like environment<br />
GCC compiler<br /><br />

Usage<br />
Start the shell and enter commands after the : prompt.<br />
To execute a command in the background, append & at the end of the command.<br />
Redirect input and output using < and >.<br />
Use exit to quit the shell, cd to change directories, and status to print the exit status or terminating signal of the last foreground process.<br />
Built-in Commands<br />
exit: Exits the shell.<br />
cd [path]: Changes the current directory to [path]. If no path is given, changes to the directory specified in the HOME environment variable.<br />
status: Prints the exit status or terminating signal of the last foreground process run by smallsh.<br /><br />

Signal Handling<br />
SIGINT: Ignored by smallsh itself; causes foreground child processes to terminate.<br />
SIGTSTP: Toggles the ability of smallsh to run processes in the background.<br />
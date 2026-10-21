# OSSP Practical and Skill Programs

C programs for Operating Systems and Systems Programming, written and tested on Ubuntu (WSL2) with gcc.

## Build and run

```
sudo apt install build-essential
make
./bin/FCFS
```

`make` compiles every program in `Practical` and `Skill` into `bin`. Each program can also be built on its own, for example `gcc Practical/paging.c -o paging`.

The record for each program (aim, algorithm, code and output) is in `Practical/Practical_Programs.docx` and `Skill/Skill_Programs.docx`.

## Practical

| File | Program |
|---|---|
| FCFS.c | FCFS CPU scheduling |
| SJF.c | SJF CPU scheduling (non-preemptive) |
| Priority.c | Priority CPU scheduling (non-preemptive) |
| RoundRobin.c | Round Robin CPU scheduling |
| pipes.c | IPC with unnamed pipes |
| domain_socket_server.c, domain_socket_client.c | IPC with Unix domain sockets |
| tcp_server.c, tcp_client.c | Client and server over TCP sockets |
| file_ops.c | File operations with system calls |
| parent-and-child.c | Process creation with fork() |
| virtual_address.c | Virtual addresses and copy-on-write after fork() |
| memory_layout.c | Text, data, BSS, heap and stack of a process |
| dynamic_memory.c | malloc(), calloc(), realloc() and free() |
| null_pointer.c | NULL pointer dereference and SIGSEGV handling |
| paging.c | Logical to physical address translation |
| page_fault.c | FIFO page replacement and real page faults |
| fifo_create.c, fifo_writer.c, fifo_reader.c | IPC with named pipes |
| command_executor.c | Running a command with fork() and execvp() |
| file_copy.c | Copying note.txt to note1.txt with system calls |
| zombie_process.c | Zombie process and reaping it |

Run `fifo_reader` and `fifo_writer` in two terminals. `file_copy` reads `note.txt` from the current directory.

## Skill (building a shell)

| File | Program |
|---|---|
| tokenizer.c | Command-line tokenizer |
| simple_shell.c | Simple shell command loop |
| shell_history.c | Shell with command history |
| keyboard_input.c | Raw keyboard input, Backspace and Enter |
| history_navigation.c | Up and Down arrow history with escape sequences |
| dynamic_buffer.c | Growing input buffer and linked list of lines |
| token_parser.c | Tokens for words and shell operators |
| syntax_parser.c | Recursive descent parser and parse tree |
| single_quote_parser.c | Single quotes |
| double_quote_parser.c | Double quotes and variable expansion |
| escape_parser.c | Backslash escapes and echo -e sequences |
| process_launcher.c | Launching programs and handling exec errors |
| process_sync.c | Monitoring children with waitpid() |
| path_resolver.c | Finding executables through PATH |
| variable_expander.c | $NAME, ${NAME}, ${NAME:-default}, $$ and $? |
| builtin_dispatch.c | Built-in dispatch table |
| directory_changer.c | cd with ~, - and error handling |
| extended_builtins.c | More built-ins with aliases, history and $? |
| current_directory.c | Current directory, exit codes and cleanup |
| export_parser.c | export, unset and child environments |
| history_buffer.c | Circular history buffer with !!, !n, !-n and !prefix |
| pipeline_structure.c | Pipeline structure and execution |
| pipe_redirection.c | Pipes with <, > and >> redirection |
| mini_shell.c | Small shell combining all of the above |

# Mini Shell — Operating Systems Lab 3

A lightweight interactive Unix-style command-line shell written in **C++17** for **CC373: Operating Systems** at Alexandria University, Faculty of Engineering.

The project extends a provided shell skeleton with command parsing, process creation, pipelines, file redirection, background execution, built-in commands, signal handling, process termination logging, wildcard expansion, and interactive command history.

## Features

### Command parsing and execution
- Tokenizes user input into commands, arguments, and shell operators.
- Executes external programs using `fork()` and `execvp()`.
- Handles multiple commands connected by pipelines (`|`).
- Reports invalid commands, missing redirection filenames, duplicate redirections, malformed pipelines, and unclosed quotes.

### File redirection

| Operator | Purpose | Example |
| --- | --- | --- |
| `>` | Redirect standard output, overwriting the destination | `echo hello > output.txt` |
| `>>` | Append standard output to a file | `echo world >> output.txt` |
| `<` | Read standard input from a file | `cat < input.txt` |
| `2>` | Redirect standard error to a file | `ls /missing 2> errors.txt` |
| `>>&` | Append standard output and standard error to the same file | `ls /etc /missing >>& combined.txt` |

Redirection uses `open()`, `dup2()`, and `close()`.

### Pipelines

Connect the standard output of one process to the standard input of the next:

```sh
echo hello | wc -c
ls | grep cc | wc -l
yes | head -n 5
```

The shell uses `pipe()` and forks a separate child for each pipeline stage.

### Background commands

Append `&` to run a command without waiting for it to finish:

```sh
sleep 3 &
echo still_alive
```

The shell tracks background child processes and checks for completion as it returns to the prompt.

### Built-in commands

| Command | Description |
| --- | --- |
| `cd <directory>` | Change the shell's working directory |
| `cd` | Change to the directory specified by `HOME` |
| `exit` | Exit the shell with `Good bye!!` |

Built-ins run in the shell process itself, rather than in a child process.

### Signal handling and process logs
- Ignores `SIGINT` (`Ctrl+C`) in the main shell; child programs restore the default `SIGINT` behavior.
- Uses `SIGCHLD` notification and `waitpid()` to collect completed child processes.
- Records observed child termination events in `process.log`, including PID and exit code or terminating signal.

Example log:

```text
Time: Wed Oct 21 17:40:38 2026
PID: 14782 | Signal: 13
Time: Wed Oct 21 17:40:38 2026
PID: 14783 | Exit code: 0
```

### Wildcard expansion (bonus)

Supports filename matching with:
- `*` — zero or more characters.
- `?` — exactly one character.

Examples:

```sh
echo *.cc
echo command.?
echo /tmp/*
```

Matching filenames are expanded into individual command arguments using `glob()`.

### Interactive command history (extra)

GNU Readline supports:
- **Up arrow**: recall earlier commands.
- **Down arrow**: navigate toward newer commands.
- Editing a command before pressing Enter.

History is stored for the current shell session; it is not persisted between launches.

## Requirements

- Linux (developed and tested on Ubuntu)
- `g++` with C++17 support
- `make`
- GNU Readline development library (`libreadline-dev`)

On Ubuntu, install the build dependencies if needed:

```sh
sudo apt update
sudo apt install build-essential libreadline-dev
```

## Build and run

From the project directory:

```sh
make
./myshell
```

Clean the compiled executable:

```sh
make clean
```

The Makefile must link with GNU Readline (`-lreadline`). `make clean` removes the executable, **not** `process.log`.

## Example session

```text
$ ./myshell
myshell> pwd
/home/user/OS/lab3
myshell> echo hello | wc -c
6
myshell> echo first > notes.txt
myshell> echo second >> notes.txt
myshell> cat notes.txt
first
second
myshell> sleep 3 &
myshell> echo still_alive
still_alive
myshell> echo *.cc
command.cc tokenizer.cc
myshell> cd ..
myshell> pwd
/home/user/OS
myshell> exit
Good bye!!
```

Paths and filename matches depend on the directory and computer.

## Project structure

```text
lab3/
├── Makefile          # Build rules
├── command.cc        # Parsing, execution, built-ins, signals, and logging
├── command.h         # Command and SimpleCommand declarations
├── tokenizer.cc      # Input tokenization
├── tokenizer.h       # Token types and tokenizer declarations
├── myshell           # Generated executable (after make)
└── process.log       # Runtime log (created in the working directory)
```

The executable and log file are generated at runtime; they are not required source files. Because `process.log` is opened with a relative path, changing directories within the shell can result in separate logs in different directories.

## Implementation overview

1. **Read input:** GNU Readline displays `myshell>` and reads a line, with command history support.
2. **Tokenize:** `tokenizer.cc` identifies arguments and operators.
3. **Parse:** `command.cc` builds a `Command` composed of one or more `SimpleCommand` objects.
4. **Expand wildcards:** Unquoted or quoted patterns are currently passed to `glob()` according to their resulting token text.
5. **Execute:** Built-ins run in the parent; external programs run in forked children with optional pipes and redirection.
6. **Wait or return:** Foreground commands are waited for; background commands return control immediately.
7. **Log completion:** Child termination statuses collected by the shell are appended to `process.log`.

## Known limitations

The program is a **teaching shell**, not a complete Bash replacement:

- **Quoted wildcards:** Quote metadata is not retained through tokenization, so patterns such as `echo "*.cc"` may still expand; unlike Bash, quotation does not always suppress globbing.
- **Background child cleanup:** Completed background children are checked during prompt-loop activity. A terminated child may remain temporarily as a zombie while the shell is idle waiting for input.
- **Command history:** History is in memory for the current session only.
- **Job control:** Full terminal job control, process-group management, and commands such as `fg`/`bg` are not implemented.
- **Shell language:** This is not a full POSIX shell and does not implement every Bash quoting, expansion, or scripting feature.

## Validation

Manual tests have covered basic external commands, malformed inputs, all listed redirection operators, multi-stage pipelines, foreground and background execution, `cd`, `exit`, `Ctrl+C`, termination logs, `*`/`?` wildcard expansion, and Readline command history.

A final regression and memory-safety testing pass is recommended before submission. Passing manual examples does not imply exhaustive verification.

## Academic context

Developed as a lab exercise for **CC373 — Operating Systems, Lab 3: Mini Shell**, Faculty of Engineering, Alexandria University.

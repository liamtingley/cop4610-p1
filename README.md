# Shell

A Unix shell written in C for COP4610 Project 1 (Group 32). It supports a
custom prompt, environment variable and tilde expansion, `$PATH` search,
external command execution, I/O redirection, any number of pipes (combined
with redirection), background processing, and the internal commands `exit`,
`cd` and `jobs`.

## Group Members
- **Pragun**
- **Upi**
- **Liam**

## Division of Labor

### Part 1: Prompt
- **Responsibilities**: Print `USER@MACHINE:PWD>` before every command, using
  `getenv("USER")`, `gethostname()` and `getcwd()` (`main.c`).
- **Assigned to**: Liam, Pragun

### Part 2: Environment Variables
- **Responsibilities**: Replace tokens starting with `$` with the value of the
  environment variable (an unset variable expands to an empty string). Text
  after the variable name is kept, so `$HOME/dir` works (`expand.c`).
- **Assigned to**: Liam, Pragun

### Part 3: Tilde Expansion
- **Responsibilities**: Replace a leading `~` or `~/` with `$HOME` (`expand.c`).
- **Assigned to**: Liam, Pragun

### Part 4: $PATH Search
- **Responsibilities**: Split `$PATH` on `:` and search each directory for an
  executable regular file matching the command name. Commands that already
  contain a `/` are checked directly (`execute.c`).
- **Assigned to**: Liam, Pragun

### Part 5: External Command Execution
- **Responsibilities**: `fork()` a child, `execv()` the resolved path with the
  argument list, and `waitpid()` for it in the parent (`execute.c`).
- **Assigned to**: Upi, Pragun

### Part 6: I/O Redirection
- **Responsibilities**: Handle `cmd > file`, `cmd < file`, and both together in
  either order. Output files are created or truncated with `-rw-------`
  permissions; input files must be existing regular files (`redirect.c`).
- **Assigned to**: Liam, Pragun

### Part 7: Piping
- **Responsibilities**: Connect commands separated by `|` with `pipe()` and
  `dup2()`, closing all unused pipe ends in the parent and children
  (`pipeline.c`).
- **Assigned to**: Upi, Liam

### Part 8: Background Processing
- **Responsibilities**: Run commands (and pipelines) ending in `&` without
  waiting, print `[job number] pid`, keep up to 10 jobs, and print
  `[job number] + done command` once every process in the job has finished
  (`jobs.c`).
- **Assigned to**: Upi, Pragun

### Part 9: Internal Command Execution
- **Responsibilities**: `exit` (waits for background jobs, then prints the last
  three valid commands), `cd` (with no argument goes to `$HOME`, errors on too
  many arguments or a bad directory) and `jobs` (lists active background jobs)
  (`builtins.c`, `history.c`, `main.c`).
- **Assigned to**: Liam, Pragun

### AFTERWARDS DIVISION OF LABOR
- Liam : Implemented the shell’s prompt, command execution, expansions, redirection, piping, background jobs, built-ins, and command history. Tested it on macOS and linprog, fixed a duplicate pipeline call, and set up the shared GitHub repository.

- Pragun : Debugged the shell and created the documentation for project  

### Extra Credit
- **Responsibilities**: Unlimited number of pipes, piping combined with I/O
  redirection, and running the shell from within itself (shell-ception)
  (`pipeline.c`, `jobs.c`, `redirect.c`).
- **Assigned to**: Upi, Liam, Pragun

## File Listing
```
root/
├── bin/                  -- executables (created by make)
├── include/
│   ├── builtins.h        -- internal command interface
│   ├── execute.h         -- $PATH search and external command execution
│   ├── expand.h          -- environment variable and tilde expansion
│   ├── history.h         -- record of the last three valid commands
│   ├── jobs.h            -- background job table
│   ├── lexer.h           -- input reading and tokenizing
│   ├── pipeline.h        -- piped command execution
│   └── redirect.h        -- I/O redirection parsing and setup
├── obj/                  -- object files (created by make)
├── src/
│   ├── builtins.c        -- cd and jobs
│   ├── execute.c         -- $PATH search, fork/exec/wait for single commands
│   ├── expand.c          -- $VAR and ~ expansion
│   ├── history.c         -- stores and prints valid commands for exit
│   ├── jobs.c            -- adds, checks, lists and waits on background jobs
│   ├── lexer.c           -- reads a line and splits it into tokens
│   ├── main.c            -- prompt and main read/expand/execute loop, exit
│   ├── pipeline.c        -- runs any number of commands joined by pipes
│   └── redirect.c        -- parses < and > and applies them in the child
├── .gitignore
├── Makefile
└── README.md
```

## How to Compile & Execute

### Requirements
- **Compiler**: `gcc` (C99), GNU `make`. Tested on linprog.
- **Dependencies**: none beyond the standard C library.

### Compilation
```bash
make
```
This builds the executable `bin/shell`. Object files are placed in `obj/`.
`make clean` removes all build output.

### Execution
```bash
make run
```
This runs the shell. It can also be started directly with `./bin/shell`.
Exit with `exit` or Ctrl-D.

## Development Log
Each member records their contributions here.

### Pragun

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| 2026-09-28 | Moved redirection into `redirect.c`; unlimited pipes and pipes with redirection (extra credit); `jobs` output format; Makefile cleanup; README |

### Upi

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |

### Liam

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-09-28 | Implemented the shell’s prompt, command execution, expansions, redirection, piping, background jobs, built-ins, and command history.  |

## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees         | Topics Discussed | Outcomes / Decisions |
|------------|-------------------|------------------|----------------------|
| 2026-09-28 | Pragun, Upi, Liam | Debugging the shell|Testing in linprog |


## Bugs
- **Bug 1 – `&` must be its own token** (runtime): A command is only run in the
  background when `&` is separated by whitespace, e.g. `sleep 5 &`. Writing
  `sleep 5&` passes `5&` to `sleep`, which reports an invalid time interval.
  This has existed since background processing was added, because the shell
  checks whether the last token is exactly `&`. We decided not to change the
  provided tokenizer to split on `&`.
- **Bug 2 – no quoting** (runtime): The tokenizer splits on spaces and tabs
  only, so `echo "a b"` passes the two arguments `"a` and `b"`. This comes from
  the provided `lexer.c`; quoting was not part of the requirements.
- **Bug 3 – built-ins ignore `&`, pipes and redirection** (runtime): `cd`,
  `jobs` and `exit` always run in the shell process itself. `cd /tmp &` changes
  directory in the foreground, and `jobs > file` is not redirected. Running
  built-ins in a child would make `cd` useless, so this was left as-is.
- **Bug 4 – `~user` is not expanded** (runtime): Only `~` and `~/...` are
  expanded to `$HOME`; `~name` is passed through unchanged.

## Extra Credit
- **Extra Credit 1**: Unlimited number of pipes. `pipeline.c` splits the command
  line at every `|` and allocates the commands, pipes and child PIDs at run
  time, so any number of commands can be chained, e.g.
  `ls -l | grep a | sort | uniq | wc -l`. Background jobs store their PIDs in a
  dynamic array for the same reason.
- **Extra Credit 2**: Piping and I/O redirection in a single command. Each
  command in a pipeline may have its own `<` and `>`, e.g.
  `sort < in.txt | uniq | wc -l > out.txt`. The redirection is applied after
  the pipe is connected, so an explicit `<` or `>` takes priority.
- **Extra Credit 3**: Shell-ception. The shell can run itself, e.g.
  `./bin/shell` from inside `bin/shell`, repeatedly. Each inner shell has its
  own job table and history, and `exit` returns to the shell that started it.

## Considerations
- `jobs` prints each active job as `[job number]+ pid command`, where `pid` is
  the PID of the last command in the job (matching the PID printed when the
  job started).
- Environment variables are expanded as whole arguments as required, and
  also when followed by other text, e.g. `$HOME/dir`.
- A command is counted as a valid command for the `exit` history when it
  starts successfully (it is a built-in that succeeded, or it was found and
  forked). Commands that are not found, or that have a syntax error, are not
  recorded.
- At most 10 background jobs can be active. If the table is full, the command
  runs in the foreground instead.

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

struct command {
    char **args;
    char *_stdin;
    char *_stdout;
    char *_stderr;
    int (*built_in_func)(char **args);
};

char *read_line(FILE *input) {
    int sz = 1024;
    int pos = 0;
    char *buffer = malloc(sz);

    bool prev_was_backslash = false;

    for (;;) {
        errno = 0;
        int c = fgetc(input);

        // Means user pressed ^C, so we should go to the next line
        if (c == EOF && errno == EINTR) {
            buffer[0] = '\n';
            buffer[1] = '\0';
            printf("%c", '\n');
            return buffer;
        }

        /* In a comment */
        if (c == '#' && (pos == 0 || isspace(buffer[pos - 1]))) {
            c = getc(input);
            while (c != EOF && c != '\n') {
                c = fgetc(input);
            }

            buffer[pos] = '\n';
            buffer[pos + 1] = '\0';
            return buffer;
        }

        if (c == EOF && pos == 0) {
            return NULL;
        }

        if (c == '\n' && prev_was_backslash) {
            buffer[--pos] = '\0';
            prev_was_backslash = false;

            if (input == stdin && isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) {
                printf("> ");
                fflush(stdout);
            }

            continue;
        }

        if (c == '\\') {
            prev_was_backslash = true;
        }

        if (c == EOF || c == '\n') {
            buffer[pos] = '\n';
            buffer[pos + 1] = '\0';
            return buffer;
        }

        buffer[pos++] = c;

        if (pos + 1 >= sz) {
            sz *= 2;
            buffer = realloc(buffer, sz);
        }
    }
}

void free_commands(struct command **commands) {
    size_t i = 0;
    struct command *command = commands[i++];
    while (command != NULL) {
        free(command->args);
        free(command);

        command = commands[i++];
    }

    free(commands);
}

size_t get_num_commands(struct command **commands) {
    size_t i = 0;
    struct command *command = commands[i++];
    while (command != NULL) {
        command = commands[i++];
    }

    return i - 1;
}

struct command **split_line(char *line) {
    size_t max_commands = 10;
    struct command **commands = calloc(max_commands, sizeof(struct command*));
    struct command *command;

    bool in_quotes = false;
    char *token_start = line;
    size_t i = 0;
    size_t j = 0;
    while (line[i] != '\0') {
        int sz = 1024;
        int pos = 0;
        char **tokens = malloc(sz * sizeof(char*));

        command = malloc(sizeof(struct command));
        command->_stderr = NULL;
        command->_stdout = NULL;
        command->_stdin = NULL;
        command->built_in_func = NULL;

        if (j >= max_commands - 1) {
            max_commands *= 2;
            commands = realloc(commands, max_commands * sizeof(struct command*));
        }

        commands[j++] = command;

        while (line[i] != '\0') {
            if (!in_quotes && (isspace(line[i]))) {
                goto add_token;
            }

            /* Handle pipes */
            else if (!in_quotes && line[i] == '|') {
                while (isspace(line[++i]));
                token_start = line + i;
                break;
            }

            /* Handle output redirection */
            else if (!in_quotes && line[i] == '>') {
                while (isspace(line[++i]));
                command->_stdout = line + i;
                while (!isspace(line[i])) { i++; }
                line[i++] = '\0';
                token_start = line + i;
                continue;
            }

            /* Handles input redirection */
            else if (!in_quotes && line[i] == '<') {
                while (isspace(line[++i]));
                command->_stdin = line + i;
                while (!isspace(line[i])) { i++; }
                line[i++] = '\0';
                token_start = line + i;
                continue;
            }

            /* Assumes quote is at beginning of token */
            else if (!in_quotes && line[i] == '"') {
                in_quotes = true;
                token_start++;
                i++;
                continue;
            }

            else if (in_quotes && line[i] == '"') {
                in_quotes = false;
                goto add_token;
            }

            else {
                i++;
                continue;
            }

        add_token:
            line[i++] = '\0';
            tokens[pos++] = token_start;
            while (isspace(line[i])) { i++; }
            token_start = line + i;

            if (pos + 1 >= sz) {
                sz *= 2;
                tokens = realloc(tokens, sz * sizeof(char*));
            }
        }

        if (in_quotes) {
            pos = 0;
            fprintf(stderr, "Shell: %s\n", "Invalid string format");
        }

        tokens[pos] = NULL;
        command->args = tokens;
    }

    commands[j] = NULL;
    return commands;
}

#define SHELL_EXIT 1
#define SHELL_CONTINUE 0

static int op_exit(char **args) {
    if (args[1] != NULL) {
        printf("Usage: %s\n", args[0]);
        return SHELL_CONTINUE;
    }

    /* Exit */
    return SHELL_EXIT;
}

static int op_cd(char **args) {
    if (!args[1] || args[2]) {
        printf("Usage: %s <dir>\n", args[0]);
        return SHELL_CONTINUE;
    }

    int ret = chdir(args[1]);
    if (ret != 0) {
        perror("Shell");
    }

    return SHELL_CONTINUE;
}

static int op_echo(char **args) {
    if (!args[1]) {
        printf("%c", '\n');
        return SHELL_CONTINUE;
    }

    size_t i = 1;
    for (;;) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) {
            printf("%c", ' ');
            i++;
        } else {
            break;
        }
    }

    printf("%c", '\n');
    return SHELL_CONTINUE;
}

struct builtin_op {
    char name[16];
    int (*op)(char **args);
    bool run_immediately;
};

#define NUM_BUILTINS 3

static struct builtin_op builtin_ops[NUM_BUILTINS] = {
    { "exit", op_exit, true },
    { "cd", op_cd, true },
    { "echo", op_echo, false }
};

int run_commands(struct command **commands) {
    size_t num_commands = get_num_commands(commands);

    int *pipes = num_commands > 1 ? calloc(num_commands - 1, 2 * sizeof(int)) : NULL;
    for (size_t j = 0; j < num_commands - 1; j++) {
        if (pipe(pipes + j * 2) == -1) {
            perror("Shell");
            return SHELL_CONTINUE;
        }
    }

    pid_t save_pgid = getpid();
    size_t i = 0;
    struct command *command = commands[i];
    while (command != NULL) {
        char **args = command->args;
        for (size_t j = 0; j < NUM_BUILTINS; j++) {
            if (strcmp(args[0], builtin_ops[j].name) == 0) {
                if (builtin_ops[j].run_immediately) {
                    return builtin_ops[j].op(args);
                }

                command->built_in_func = builtin_ops[j].op;
            }
        }

        pid_t pid = fork();

        /* Child */
        if (pid == 0) {
            if (isatty(STDOUT_FILENO)) {
                setpgid(0, 0);
                tcsetpgrp(STDOUT_FILENO, getpid());
            }

            if (command->_stdout != NULL && i == num_commands - 1) {
                int fd = open(command->_stdout, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                if (fd == -1) {
                    goto abort_command;
                }
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    goto abort_command;
                }
            }

            if (command->_stdin != NULL && i == 0) {
                int fd = open(command->_stdin, O_RDONLY);
                if (fd == -1) {
                    goto abort_command;
                }
                if (dup2(fd, STDIN_FILENO) == -1) {
                    goto abort_command;
                }
            }

            /* First program in chain */
            if (num_commands > 1 && i == 0) {
                if (dup2(pipes[i + 1], STDOUT_FILENO) == -1) {
                    goto abort_command;
                }
            }

            /* Last program in chain */
            else if (num_commands > 1 && i == num_commands - 1) {
                if (dup2(pipes[(i - 1) * 2], STDIN_FILENO) == -1) {
                    goto abort_command;
                }
            }

            /* Any other program in chain */
            else if (num_commands > 1) {
                if (dup2(pipes[i * 2 + 1], STDOUT_FILENO) == -1) {
                    goto abort_command;
                }

                if (dup2(pipes[(i - 1) * 2], STDIN_FILENO) == -1) {
                    goto abort_command;
                }
            }

            if (command->built_in_func != NULL) {
                exit(command->built_in_func(args));
            }
            execvp(args[0], args);

        abort_command:
            perror("Shell");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("Shell");
        }

        /* Parent */
        else {
            if (isatty(STDOUT_FILENO)) {
                setpgid(pid, pid);
                tcsetpgrp(STDOUT_FILENO, pid);
            }

            /* Close write pipe for the last process */
            if (num_commands > 1 && i == 0) {
                close(pipes[i * 2 + 1]);
            }

            else if (num_commands > 1 && i == num_commands - 1) {
                close(pipes[(i - 1) * 2]);
            }

            else if (num_commands > 1) {
                close(pipes[i * 2 + 1]);
                close(pipes[(i - 1) * 2]);
            }

            int status;
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSTOPPED(status) && !WIFSIGNALED(status));
        }

        command = commands[++i];
    }

    if (isatty(STDOUT_FILENO)) {
        tcsetpgrp(STDOUT_FILENO, save_pgid);
    }

    free(pipes);
    return SHELL_CONTINUE;
}

static char *__getcwd() {
    size_t size = 50;
    char *buffer = malloc(size);
    char *cwd = getcwd(buffer, size);
    
    while (cwd == NULL) {
        free(buffer);
        size *= 2;
        buffer = malloc(size);
        cwd = getcwd(buffer, size);
    }

    return cwd;
}

int main(int argc, char **argv) {
    FILE *input = stdin;
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            perror("Shell");
            return EXIT_FAILURE;
        }
    } else if (argc > 2) {
        printf("Usage: %s [script]\n", argv[0]);
        return EXIT_SUCCESS;
    }

    if (isatty(STDOUT_FILENO)) {
        struct sigaction to_set;
        to_set.sa_handler = SIG_IGN;
        to_set.sa_flags = 0;
        sigaction(SIGINT, &to_set, NULL);
    }

    for (;;) {
        if (input == stdin && isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) {
            char *cwd = __getcwd();
            printf("\033[32m%s\033[37m:\033[36m%s\033[37m$ ", "root@os_2", cwd);
            free(cwd);
        }
        fflush(stdout);

        char *line = read_line(input);

        /* Check if we reached EOF */
        if (line == NULL) {
            free(line);
            break;
        }

        /* Check If The Line Was Empty */
        if (line[0] == '\n') {
            free(line);
            continue;
        }

        struct command **commands = split_line(line);

        if (commands == NULL || commands[0] == NULL || commands[0]->args[0] == NULL) {
            free(line);
            free_commands(commands);
            continue;
        }

        int status = run_commands(commands);

        free(line);
        free_commands(commands);

        if (status == SHELL_EXIT) {
            break;
        }
    }

    if (input != stdin) {
        fclose(input);
    }

    return EXIT_SUCCESS;
}
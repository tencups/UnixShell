/*
118985251
*/

#include "executor.h"
#include "command.h"
#include <stdio.h>

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

#define OPEN_FLAGS (O_WRONLY | O_TRUNC | O_CREAT)
#define DEF_MODE 0664

static int execute_aux(struct tree *t, int p_input_fd, int p_output_fd);
static void error_check(int status, const char *error_message);
static void print_tree(struct tree *t);

int execute(struct tree *t) {
    return execute_aux(t, STDIN_FILENO, STDOUT_FILENO);
}

static int execute_aux(struct tree *t, int p_input_fd, int p_output_fd) {

    if (t->conjunction == AND) {
        if (execute_aux(t->left, p_input_fd, p_output_fd) == 0) {
            execute_aux(t->right, p_input_fd, p_output_fd);
        }
    }

    if (t->conjunction == PIPE) {
        int pipe_fd[2]; /* pipe array */
        pid_t pid;

        if ((t->left->output)) {
            printf("Ambiguous output redirect.\n");
            return -1;
        } else if (t->right->input) {
            printf("Ambiguous input redirect.\n");
            return -1;
        }

        error_check(pipe(pipe_fd), "pipe");
        pid = fork();
        error_check(pid, "fork PIPE pid");

        if (pid) {
            int status;

            status = close(pipe_fd[1]);
            error_check(status, "closing pipe");

            execute_aux(t->right, pipe_fd[0], p_output_fd);
            status = close(pipe_fd[0]);
            error_check(status, "closing pipe");
            waitpid(pid, &status, 0);

        } else {
            int status;

            status = close(pipe_fd[0]);
            error_check(status, "closing pipe");

            execute_aux(t->left, p_input_fd, pipe_fd[1]);
            status = close(pipe_fd[1]);
            error_check(status, "closing pipe");
        }
    }
    if (t->conjunction == SUBSHELL) {
        pid_t pid;
        int pid_status;
        pid = fork();
        error_check(pid, "pid");

        if (pid == 0) {
            int status;
            status = execute_aux(t->left, p_input_fd, p_output_fd);
            if (t->input) {
                close(p_input_fd);
            }

            if (t->output) {
                close(p_output_fd);
            }

            exit(status);
        } else {
            waitpid(pid, &pid_status, 0);
        }
    }

    if (t->conjunction == NONE) {
        if (!strcmp(t->argv[0], "cd")) {
            if (t->argv[1]) {
                if (chdir(t->argv[1]) != 0) {
                    perror(t->argv[1]);
                }
            } else {
                if (chdir(getenv("HOME")) != 0) {
                    perror(getenv("HOME"));
                };
            }

        } else if (!strcmp(t->argv[0], "exit")) {
            exit(EXIT_SUCCESS);
        } else {
            pid_t pid;
            int fd, status;
            pid = fork();
            error_check(pid, "pid");

            if (pid == 0) {
                /* Child */
                if (t->input) {
                    status = fd = open(t->input, O_RDONLY);
                    error_check(status, "open");

                    status = dup2(fd, STDIN_FILENO);
                    error_check(status, "dup2");

                    status = close(fd);
                    error_check(status, "close");
                } else {
                    status = dup2(p_input_fd, STDIN_FILENO);
                    error_check(status, "dup2");
                }

                if (t->output) {
                    status = fd = open(t->output, OPEN_FLAGS, DEF_MODE);
                    error_check(status, "open");

                    status = dup2(fd, STDOUT_FILENO);
                    error_check(status, "dup2");

                    status = close(fd);
                    error_check(status, "close");
                } else {
                    status = dup2(p_output_fd, STDOUT_FILENO);
                    error_check(status, "dup2");
                }

                if (execvp(t->argv[0], t->argv) == -1) {
                    fprintf(stderr, "Failed to execute %s\n", t->argv[0]);
                    fflush(stdout);
                    exit(EX_OSERR);
                };

            } else {
                /* Parent */
                waitpid(pid, &status, 0);
                /*print_tree(t);*/
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    return 0;
                } else {
                    return -1;
                }
            }
        }
    }
    /*print_tree(t);*/

    return 0;
}

static void print_tree(struct tree *t) {
    if (t != NULL) {
        print_tree(t->left);

        if (t->conjunction == NONE) {
            printf("NONE: %s, ", t->argv[0]);
        } else {
            printf("%s, ", conj[t->conjunction]);
        }
        printf("IR: %s, ", t->input);
        printf("OR: %s\n", t->output);

        print_tree(t->right);
    }
}

static void error_check(int status, const char *error_message) {
    if (status < 0) {
        perror("ERROR");
        err(EX_OSERR, error_message);
    }
}
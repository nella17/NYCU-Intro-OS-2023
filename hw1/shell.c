#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

void cleanup(int /*s*/) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

size_t arylen(char** ptr) {
    size_t cnt = 0;
    while (ptr[cnt] != NULL)
        cnt++;
    return cnt;
}

char** split(char* line, const char* delim) {
    size_t cnt = 1;
    for (size_t i = 0, n = strlen(line); i < n; i++)
        if (strchr(delim, line[i]) != NULL)
            cnt++;

    char **ary = calloc(cnt+1, sizeof(char*));
    char *s = line, *saved_ptr;
    for (int i = 0; ; s = NULL) {
        char* token = strtok_r(s, delim, &saved_ptr);
        if (token == NULL) break;
        if (!strlen(token)) continue;
        ary[i++] = token;
    }
    return ary;
}

pid_t exec(char* line, int fd_in, int fd_out) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        char** argv = split(line, " ");
        if (argv[0] == NULL)
            exit(EXIT_SUCCESS);
        if (dup2(fd_in, STDIN_FILENO) < 0)
            perror("dup2(stdin)");
        if (dup2(fd_out, STDOUT_FILENO) < 0)
            perror("dup2(stdout)");
        execvp(argv[0], argv);
        perror("execve");
        exit(EXIT_FAILURE);
    }

    return pid;
}

int main(void) {
    signal(SIGCHLD, cleanup);

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    for (;;) {
        printf("> ");
        if ((nread = getline(&line, &len, stdin)) < 0)
            break;

        while (nread > 0 && line[nread-1] == '\n')
            line[--nread] = '\0';
        int background = 0;
        if (nread > 0 && line[nread-1] == '&') {
            background = 1;
            line[--nread] = '\0';
            if (nread > 0 && line[nread-1] == ' ')
                line[--nread] = '\0';
        }

        if (!nread) continue;

        char** cmds = split(line, "|");
        size_t cnt = arylen(cmds);

        int pipefd[cnt-1][2];
        int fds[cnt][2];
        fds[0][0] = dup(STDIN_FILENO);
        fds[cnt-1][1] = dup(STDOUT_FILENO);

        for (size_t i = 0; i < cnt-1; i++) {
            if (pipe(pipefd[i]) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            fds[i][1] = pipefd[i][1];
            fds[i+1][0] = pipefd[i][0];
        }

        for (size_t i = 0; i < cnt; i++)
            for (int j = 0; j < 2; j++)
                if (fcntl(fds[i][j], F_SETFD, FD_CLOEXEC) < 0)
                    perror("fcntl");

        pid_t pid[cnt];
        for (size_t i = 0; i < cnt; i++)
            pid[i] = exec(cmds[i], fds[i][0], fds[i][1]);
        for (size_t i = 0; i < cnt; i++)
            for (int j = 0; j < 2; j++)
                if (close(fds[i][j]) < 0)
                    perror("close");

        if (!background)
            waitpid(pid[cnt-1], NULL, 0);
    }

    return 0;
}

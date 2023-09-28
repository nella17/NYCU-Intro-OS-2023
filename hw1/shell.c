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

char** split(char* line, const char* delim, size_t* pcnt) {
    size_t cnt = 1;
    for (size_t i = 0, n = strlen(line); i < n; i++)
        if (strchr(delim, line[i]) != NULL)
            cnt++;
    *pcnt = cnt;

    char **ary = malloc(sizeof(char*) * cnt);
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
        size_t cnt;
        char** argv = split(line, " ", &cnt);
        if (cnt <= 0)
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

        size_t cnt;
        char** cmds = split(line, "|", &cnt);

        int pipefd[cnt-1][2];
        int fd_in[cnt], fd_out[cnt];
        fd_in[0] = STDIN_FILENO;
        fd_out[cnt-1] = STDOUT_FILENO;

        for (size_t i = 0; i < cnt-1; i++) {
            if (pipe(pipefd[i]) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            for (int j = 0; j < 2; j++)
                if (fcntl(pipefd[i][j], F_SETFD, FD_CLOEXEC) < 0)
                    perror("fcntl");
            fd_out[i] = pipefd[i][1];
            fd_in[i+1] = pipefd[i][0];
        }

        pid_t pid[cnt];
        for (size_t i = 0; i < cnt; i++)
            pid[i] = exec(cmds[i], fd_in[i], fd_out[i]);
        for (size_t i = 0; i < cnt-1; i++)
            for (int j = 0; j < 2; j++)
                if (close(pipefd[i][j]) < 0)
                    perror("close");
        if (!background)
            waitpid(pid[cnt-1], NULL, 0);
    }

    return 0;
}

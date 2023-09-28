#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
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
    for (int i = 0; ; i++, s = NULL) {
        char* token = strtok_r(s, " ", &saved_ptr);
        if (token == NULL) break;
        ary[i] = token;
    }
    return ary;
}

pid_t exec(char* line) {
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
        if (nread) {
            pid_t pid = exec(line);
            int wstatus;
            waitpid(pid, &wstatus, 0);
        }
    }

    return 0;
}

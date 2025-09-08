#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>

int main() {
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }
    if (pid == 0) _exit(42);    // child
    int status = 0;
    pid_t w = waitpid(pid, &status, 0);
    if (w == -1) { perror("waitpid"); return 1; }
    if (WIFEXITED(status)) std::cout << "child exited with " << WEXITSTATUS(status) << "\\n";
    return 0;
}

#define _GNU_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t shutdown_requested = 0;

void request_shutdown(int sig) {
    (void)sig;
    shutdown_requested = 1;
}

void install_sigint_handler(void (*handler)(int)) {
    struct sigaction act = {0};
    act.sa_handler = handler;

    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("Sigaction error");
        exit(1);
    }

    return;
}
#pragma once

void request_shutdown(int sig);

void install_sigint_handler(void (*handler)(int));
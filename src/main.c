#include "html.h"
#include "server.h"
#include "syscall_wrappers.h"
#include "threadpool.h"

int main() {
    Server s;
    server_init(&s, PORT);
    server_run(&s);

    return 0;
}
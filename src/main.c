#include "server.h"
#include "threadpool.h"

int main() {
    Server s;
    server_init(&s, PORT);
    server_run(&s);

    return 0;
}
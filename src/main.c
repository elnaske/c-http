#include "html.h"
#include "syscall_wrappers.h"
#include "threadpool.h"
#include "server.h"

int main() {
    Server s;
    server_init(&s, PORT);
    server_run(&s);
    
    return 0;
}
#include <iostream>
#include "uthreads.h"

void f(void)
{
    int i = 0;
    while(1){
        ++i;
        printf("in f (%d)\n",i);
        if (i % 3 == 0) {
            printf("f: switching\n");
        }
    }
}

void g(void)
{
    int i = 0;
    while(1){
        ++i;
        printf("in g (%d)\n",i);
        if (i % 5 == 0) {
            printf("g: switching\n");
        }
    }
}

int main() {
    uthread_init(1);
    uthread_spawn(f);
    uthread_spawn(g);
    uthread_terminate(0);
    return 0;
}


#include <iostream>
#include "uthreads.h"

#define JB 10000000

void f(){
    static int id = 0;
    int id_f = id++;
    for(int i = 0; i < 20; ++i){
        for(int j = 0; j < JB; ++j);
        printf("Hello from %d, %d\n", id_f, i);
    }
    printf("Blocking myself %d\n", id_f);
    uthread_block(uthread_get_tid());
    printf("I am alive !\n");
    for(int i = 0; i < 10; ++i){
        for(int j = 0; j < JB; ++j);
        printf("Hello from the same side, %d\n", i);
    }
    uthread_terminate(uthread_get_tid());
}

int main() {
    uthread_init(1000);
    printf("Hello\n");
    int id = uthread_spawn(f);

    for(int i = 0; i < 10; ++i){
        for(int j = 0; j < JB; ++j);
        printf("Hello from the other side, %d\n", i);
    }
    printf("BLOCKED !\n");
    uthread_block(id);
    for(int i = 0; i < 10; ++i){
        for(int j = 0; j < JB; ++j);
        printf("Hello from the other SIDE, %d\n", i);
    }
    uthread_resume(id);
    for(int i = 0; i < 20; ++i){
        for(int j = 0; j < JB; ++j);
        printf("Hello from the other SIDE, %d\n", i);
    }
    uthread_resume(id);
    for(int i = 0; i < 20; ++i){
        for(int j = 0; j < JB; ++j);
        printf("Hello from the other SIDE, %d\n", i);
    }
    uthread_terminate(0);
    return 0;
}


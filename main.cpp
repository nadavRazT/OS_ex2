#include <iostream>
#include "thread.h"
#include "uthreads.h"
#include "uthreads.cpp"

void f(void)
{
    int i = 0;
    while(1){
        ++i;
        printf("in f (%d)\n",i);
        if (i % 3 == 0) {
            printf("f: switching\n");
            switchThreads();
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
            switch_threads();
        }
    }
}

int main() {
    thread *t1 = new thread(f, 1);
    thread *t2 = new thread(g, 2)
    std::cout << "Hello, World!" << std::endl;
    return 0;
}


//
// Created by nadav on 19/05/2021.
//
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

#define RUNNING 0
#define READY 1
#define BLOCKED 2

#define STACK_SIZE 4096

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr) {
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

class thread {
private:
    int id;


    bool mutex;
    int state;
    int num_quantum = 0;
    sigjmp_buf env;
    char *stack = new char[STACK_SIZE];

    void setup_env() {
        address_t sp, pc;
        sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
        pc = (address_t) f;
        sigsetjmp(env, 1);
        (env->__jmpbuf)[JB_SP] = translate_address(sp);
        (env->__jmpbuf)[JB_PC] = translate_address(pc);
        sigemptyset(&env->__saved_mask);

    }

public:
    void (*f)(void);

    __jmp_buf_tag *get_env();


    int get_id();

    void terminate();

    void increment_num_quantum() {
        this->num_quantum += 1;
    }

    int get_num_quantum()
    {
        return this->num_quantum;
    }

    thread(void (*f)(void), int id) {
        this->f = f;
        this->id = id;
        setup_env();
    }

    thread() {
        this->f = nullptr;
        this->id = -1;

    }

};

__jmp_buf_tag *thread::get_env() {
    return env;
}


int thread::get_id() {
    return id;
}

void thread::terminate() {
    delete stack;
}


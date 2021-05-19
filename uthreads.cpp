#include "uthreads.h"
#include "thread.cpp"
#include <list>
#include <signal.h>
#include <sys/time.h>
#include <iostream>

using namespace std;

int i = 0;
list<thread> readyList;
list<thread> blockedList;
thread runningThread = new thread();
int curr_thread_num = 1;
int total_num_quantum = 0;

void on_click(int sig) {
    total_num_quantum += 1;
    runningThread.increment_num_quantum();
    readyList.push_back(runningThread);
    runningThread = readyList.pop_front();

}

void set_timer(int quantum_usecs) {
    // set timer sigaction
    struct sigaction timer_sa = {0};
    struct itimerval timer;
    timer_sa.sa_handler = &on_click;
    if (sigaction(SIGVTALRM, &timer_sa, NULL) < 0) {
        cerr << ("sigaction error.") << endl;
    }
    // Configure the timer to expire after 1 sec... */
    timer.it_value.tv_sec = 3;        // first time interval, seconds part
    timer.it_value.tv_usec = quantum_usecs;        // first time interval, microseconds part


    // configure the timer to expire every 3 sec after that.
    timer.it_interval.tv_sec = 3;    // following time intervals, seconds part
    timer.it_interval.tv_usec = quantum_usecs;    // following time intervals, microseconds part

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL)) {
        printf("setitimer error.");
    }

}


void switch_threads(thread &th) {
    int ret_val = sigsetjmp(runningThread.get_env(), 1); //todo check ret_val if relevant
    siglongjmp(th.get_env(), 1);
    runningThread = th;
}


/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {
    if (quantum_usecs <= 0) return -1;
    set_timer(quantum_usecs);
    return 0;
}


int find_next_id() {
    for (int j = 1; j < MAX_THREAD_NUM; j++) {
        bool used = false;
        for (thread th : readyList) {
            if (th.get_id() == j) {
                used = true;
            }
        }
        for (thread th : blockedList) {
            if (th.get_id() == j) {
                used = true;
            }
        }
        if (runningThread.get_id() == j) {
            used = true;
        }
        if (!used) {
            return j;
        }
    }
    return -1;
}


/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void)) {
    if (curr_thread_num >= MAX_THREAD_NUM) return -1;
    thread *toAdd = new thread(f, find_next_id());
    readyList.push_back(*toAdd);
    return 0;
}


thread find_thread_by_id(int tid) {

    for (auto th : readyList) {
        if (th.get_id() == tid) {
            return th;
        }
    }
    for (auto th : blockedList) {
        if (th.get_id() == tid) {
            return th;
        }
    }
    if (runningThread.get_id() == tid) {
        return runningThread;
    }
    return thread();
}


int find_and_delete(int tid) {
    bool flag = false;
    thread th_to_del;
    for (auto th : readyList) {
        if (th.get_id() == tid) {
            flag = true;
            th_to_del = th;
        }
    }

    if (flag) {
        readyList.remove(th_to_del);
        th_to_del.terminate();
        delete &th_to_del;
        return 1;
    }
    for (auto th : blockedList) {
        if (th.get_id() == tid) {
            flag = true;
            th_to_del = th;
        }
    }
    if (flag) {
        readyList.remove(th_to_del);
        th_to_del.terminate();
        delete &th_to_del;
        return 1;

    }
    if (runningThread.get_id() == tid) {
        runningThread.terminate();
    }
    return -1;
}


void terminate_all() {
    for (auto th: readyList) {
        th.terminate();
    }
    for (auto th:blockedList) {
        th.terminate();
    }
    runningThread.terminate();
}


/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
    if (tid == 0 || tid == runningThread.get_id()) {
        terminate_all();
        exit(0); //todo free memory
    }

    int ret = find_and_delete(tid);
    if (ret == -1) {
        return -1;
    }

}


/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid) {
    if (tid == 0 || find_thread_by_id(tid).get_id() == -1) {
        return -1; //todo check error or failure
    }
    if (tid == runningThread.get_id()) {
        if (readyList.empty()) {
            exit(0); //todo free memory
        }
        switch_threads(readyList.pop_front());
        blockedList.push_back(runningThread);
    }
    for (auto th: readyList) {
        if (th.get_id() == tid) {
            readyList.remove(th);
            blockedList.push_back(th);
            break;
        }
    }
    return 0;
}


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state if it's not synced. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid) {
    if(find_thread_by_id(tid).get_id() == -1)
    {
        return -1;
    }
    for (auto th: blockedList) {
        if (th.get_id() == tid) {
            blockedList.remove(th);
            readyList.push_back(th);
            break;
        }
    }
    return 0;
}


/*
 * Description: This function tries to acquire a mutex.
 * If the mutex is unlocked, it locks it and returns.
 * If the mutex is already locked by different thread, the thread moves to BLOCK state.
 * In the future when this thread will be back to RUNNING state,
 * it will try again to acquire the mutex.
 * If the mutex is already locked by this thread, it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_mutex_lock() {
    return 0;
}


/*
 * Description: This function releases a mutex.
 * If there are blocked threads waiting for this mutex,
 * one of them (no matter which one) moves to READY state.
 * If the mutex is already unlocked, it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_mutex_unlock() {
    return 0;
}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid() {
    return runningThread.get_id();
}


/*
 * Description: This function returns the total number of quantums since
 * the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums() {
    return total_num_quantum;
}


/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered an error.
 * Return value: On success, return the number of quantums of the thread with ID tid.
 * 			     On failure, return -1.
*/
int uthread_get_quantums(int tid) {
    return find_thread_by_id(tid).get_num_quantum();
}

//int main()
//{
//    uthread_init(10);
//    for(;;){}
//    return 0;
//}
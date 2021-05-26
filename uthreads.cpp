#include "uthreads.h"
#include "thread.h"
#include <list>
#include "mutex.h"
#include <signal.h>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>

#define THREAD_LIBRARY_ERROR "thread library error: "
using namespace std;
int i = 0;
list<thread *> readyList;
list<thread *> blockedList;
mutex *mut = nullptr;
thread *runningThread = nullptr;
int curr_thread_num = 1;
int total_num_quantum = 1;
thread *thread_array[MAX_THREAD_NUM];
sigset_t set;
struct sigaction timer_sa = {0};
struct itimerval timer;
thread* mutexHolder = nullptr;


void block_signal() {
    sigprocmask(SIG_BLOCK, &set, NULL);
}

void unblock_signal() {
    sigprocmask(SIG_UNBLOCK, &set, NULL);
//    cout<<"UNBLOCKED" <<endl;
//    if ( == -1) {
//
//        //todo printerror
//        exit(0);
//    }
}


thread *real_pop_front(list<thread *> &l) {
    thread *th = l.front();
    l.remove(l.front());
    return th;
}

void reset_timer() {
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL)) {
        printf("setitimer error.");
    }
}

void switch_threads() {
    block_signal();
//    cout<<"jumping from: "<< runningThread->get_id()<<endl;

    thread *th = real_pop_front(readyList);
    thread *cur = runningThread;
    runningThread = th;
    runningThread->increment_num_quantum();
    total_num_quantum++;
    reset_timer();
    unblock_signal();
    int ret_val = sigsetjmp(cur->get_env(), 1);
    if (ret_val != 0) {
        block_signal();
        return;
    }//todo check ret_val if relevant
//    cout<<"jumping to: "<<th->get_id()<<endl;//<< " has "<< th->get_num_quantum() <<" runs" <<endl;
//   cout<< "total quantum: "<< uthread_get_total_quantums() << endl;
//    cout<< "num of ready: "<< readyList.size()<<endl;
    if(th->get_id() == 0)
    {
        int b= 0;
    }
    siglongjmp(th->get_env(), 0);
}


void on_click(int sig) {
    //total_num_quantum += 1;
//    cout<<"SWITCH" <<endl;
    block_signal();
//    sigprocmask(SIG_SETMASK, &set, NULL);
    if (runningThread == nullptr) {
        if (readyList.empty()) {
            return;
        }
        runningThread = real_pop_front(readyList);
        return;
    }
    readyList.push_back(runningThread);
    unblock_signal();
    switch_threads();
//    unblock_signal();
}


void set_timer(int quantum_usecs) {
    // set timer sigaction

    timer_sa.sa_handler = &on_click;
    if (sigaction(SIGVTALRM, &timer_sa, NULL) < 0) {
        cerr << ("sigaction error.") << endl;
    }
    // Configure the timer to expire after 1 sec... */
    timer.it_value.tv_sec = quantum_usecs / 1000000;       // first time interval, seconds part
    timer.it_value.tv_usec = quantum_usecs % 1000000;        // first time interval, microseconds part


    // configure the timer to expire every 3 sec after that.
    timer.it_interval.tv_sec = quantum_usecs / 1000000;    // following time intervals, seconds part
    timer.it_interval.tv_usec = quantum_usecs % 1000000;    // following time intervals, microseconds part

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL)) {
        printf("setitimer error.");
    }

}


thread *find_thread_by_id(int tid) {
//    std::cout << "A "<< uthread_get_tid << " "<< readyList.size() << " ";
//    std::cout.flush();
    if (!readyList.empty()) {
//        std::cout << "B "<< uthread_get_tid<< " " << readyList.size() << " ";
//        std::cout.flush();
        for (auto th = readyList.begin(); th != readyList.end(); ++th) {
//            std::cout << "C "<< uthread_get_tid<< " " << readyList.size() << std::endl;
//            std::cout.flush();
            if ((*th)->get_id() == tid) {
                return *th;
            }
        }
    }
    if(!blockedList.empty()) {
        for (auto th : blockedList) {
            if (th->get_id() == tid) {
                return th;
            }
        }
    }
    if (runningThread->get_id() == tid) {
        return runningThread;
    }
    return nullptr;
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
    if (quantum_usecs <= 0) {
        cerr << THREAD_LIBRARY_ERROR << "invalid quantum usecs value" << endl;
        return -1;
    }
    thread_array[0] = new thread();
    runningThread = thread_array[0];
    sigemptyset(&set);
    sigaddset(&set, SIGVTALRM);
    curr_thread_num++;
    runningThread->increment_num_quantum();
    mut = mut->get_instance();
    set_timer(quantum_usecs);

    return 0;
}


int find_next_id() {
    for (int j = 0; j < MAX_THREAD_NUM; j++) {
        bool used = false;
        for (auto th : readyList) {
            if (th->get_id() == j) {
                used = true;
            }
        }
        for (auto th : blockedList) {
            if (th->get_id() == j) {
                used = true;
            }
        }
        if (runningThread && runningThread->get_id() == j) {
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
    block_signal();
    if (curr_thread_num > MAX_THREAD_NUM) {
        unblock_signal();
        cerr << THREAD_LIBRARY_ERROR << "exceeded max thread num" << endl;
        return -1;
    }
    curr_thread_num++;
    thread *toAdd = new thread(f, find_next_id());
    readyList.push_back(toAdd);
    int ret = toAdd->get_id();
    thread_array[ret] = toAdd;

    unblock_signal();


    return ret;
}


int find_and_delete(int tid) {
    bool flag = false;
    thread *th_to_del;
    for (auto &th : readyList) {
        if (th->get_id() == tid) {
            flag = true;
            th_to_del = th;
        }
    }

    if (flag) {
        readyList.remove(th_to_del);
        thread_array[th_to_del->get_id()] = nullptr;
        th_to_del->terminate();
        curr_thread_num--;
        th_to_del = nullptr;

        return 1;
    }
    for (auto &th : blockedList) {
        if (th->get_id() == tid) {
            flag = true;
            th_to_del = th;
        }
    }
    if (flag) {
        blockedList.remove(th_to_del);
        thread_array[th_to_del->get_id()] = nullptr;
        th_to_del->terminate();
        curr_thread_num--;
        th_to_del = nullptr;
        return 1;

    }
    if (runningThread->get_id() == tid) {
        thread_array[runningThread->get_id()] = nullptr;
        runningThread->terminate();
        curr_thread_num--;
        return 1;
    }
    return -1;
}


void terminate_all() {
    for (auto &th: readyList) {
        std::cout << th->get_id() << std::endl;
        thread_array[th->get_id()] = nullptr;
        th->terminate();
        curr_thread_num--;
    }
    for (auto &th:blockedList) {
        std::cout << th->get_id() << std::endl;
        thread_array[th->get_id()] = nullptr;
        th->terminate();
        curr_thread_num--;
    }
    thread_array[runningThread->get_id()] = nullptr;
    runningThread->terminate();
    curr_thread_num--;
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

    block_signal();
    if (tid == 0) {
        terminate_all();
        exit(0);
    }
    if (readyList.empty() && runningThread->get_id() != 0) {
        terminate_all();
        exit(0); //todo free memory
    }
    int ret = find_and_delete(tid);

    if(tid == runningThread->get_id())
    {
        uthread_mutex_unlock();
        runningThread = real_pop_front(readyList);
        runningThread->increment_num_quantum();
        total_num_quantum++;
        siglongjmp(runningThread->get_env(), 0);

    }
    if(mutexHolder != nullptr && mutexHolder->get_id() == tid){
        mutexHolder = nullptr;
        thread *to_change = nullptr;
        for (auto &th : blockedList) {
            if (th->is_waiting_for_mutex()) {
                to_change = th;
                break;
            }
        }
        if (to_change != nullptr) {
            to_change->got_mutex();
            uthread_resume(to_change->get_id());
        }
    }
    if (ret == -1) {
        cerr << THREAD_LIBRARY_ERROR << "couldn't find the requested thread" << endl;
        unblock_signal();
        return -1;
    }

    //todo reset timer
    unblock_signal();

//    switch_threads();
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
    block_signal();
    if (tid == 0) {
        cerr << THREAD_LIBRARY_ERROR << "can't block main thread" << endl;
        unblock_signal();
        return -1; //todo check error or failure
    } else if (find_thread_by_id(tid) == nullptr) {
        cerr << THREAD_LIBRARY_ERROR << "thread not found" << endl;
        unblock_signal();
        return -1;
    }
    if (tid == runningThread->get_id()) {
        if (readyList.empty()) {
            exit(0); //todo free memory
        }
        blockedList.push_back(runningThread);
        unblock_signal();
        switch_threads();
        return 0;
    }
    for (auto &th: readyList) {
        if (th->get_id() == tid) {
            readyList.remove(th);
            blockedList.push_back(th);
            break;
        }
    }
    unblock_signal();
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
    block_signal();
    if (find_thread_by_id(tid) == nullptr) {
        cerr << THREAD_LIBRARY_ERROR << "couldn't find the requested thread" << endl;
        unblock_signal();
        return -1;
    }
    for (auto &th: blockedList) {
        if (th->get_id() == tid) {
            blockedList.remove(th);
            readyList.push_back(th);
            break;
        }
    }
    unblock_signal();
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
    start:
    block_signal();
    if (mutexHolder == nullptr) {
        mutexHolder = runningThread;
        unblock_signal();
        return 0;
    }
    if (mutexHolder->get_id() == runningThread->get_id()) //todo possible voodo
    {
        unblock_signal();
        return -1; //todo possible error
    }
    runningThread->wait_for_mutex();
    uthread_block(runningThread->get_id());
    unblock_signal();
    goto start;
}


/*
 * Description: This function releases a mutex.
 * If there are blocked threads waiting for this mutex,
 * one of them (no matter which one) moves to READY state.
 * If the mutex is already unlocked, it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_mutex_unlock() {
    block_signal();
    if (mutexHolder == nullptr) {
        return -1; // todo raise error
    }
    if(mutexHolder->get_id() != runningThread->get_id())
    {
        return -1; //TODO: RAISE ERROR
    }
    mutexHolder = nullptr;
    thread *to_change = nullptr;
    for (auto &th : blockedList) {
        if (th->is_waiting_for_mutex()) {
            to_change = th;
            break;
        }
    }
    if (to_change != nullptr) {
        to_change->got_mutex();
        uthread_resume(to_change->get_id());
    }
    unblock_signal();
    return 0;
}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid() {
    return runningThread->get_id();
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
//    block_signal();
//    if (find_thread_by_id(tid) == nullptr) {
//        cerr << THREAD_LIBRARY_ERROR << "couldn't find the requested thread" << endl;
//        return -1;
//    }
//    int ret = find_thread_by_id(tid)->get_num_quantum();
//    unblock_signal();
    if(thread_array[tid] == nullptr)
    {
        cerr << THREAD_LIBRARY_ERROR << "couldn't find the requested thread" << endl;
        return -1;
    }
    return thread_array[tid]->get_num_quantum();
}


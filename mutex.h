//
// Created by nadav on 23/05/2021.
//

#ifndef OS_EX2_MUTEX_H
#define OS_EX2_MUTEX_H


class mutex {
private:
    static mutex *inst;
    thread *th = nullptr;
    bool acquired = false;

    mutex() {

    }

public:
    static mutex *get_instance() {
        if (inst == nullptr) {
            inst = new mutex();
        }
        return inst;
    }

    static bool is_acquired() {
        return inst->acquired;
    }

    static void release_mutex() {
        inst->acquired = false;
    }

    static bool acquire(thread &th) {
        if (is_acquired()) {
            return false;
        }
        inst->th = &th;
        inst->acquired = true;
        return true;
    }

    static thread *get_thread() {
        return inst->th;
    }


};

mutex* mutex::inst = nullptr;
#endif //OS_EX2_MUTEX_H

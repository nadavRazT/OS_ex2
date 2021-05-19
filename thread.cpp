//
// Created by nadav on 19/05/2021.
//
#define RUNNING 0
#define READY 1
#define BLOCKED 2

class thread
{
private:
    int id;
    void (*f)(void);
    bool mutex;
    int state;

public:

    thread(void (*f)(void), int id)
    {
        this->f = f;
        this->id = id;
    }
};
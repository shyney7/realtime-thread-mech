#include <iostream>
#include <thread>           
#include <pthread.h>            //POSIX threads lib
#include <cstring>              //std::strerror
#include <chrono>               //timing lib
#include <mutex>                //mutex is needed for cond_var
#include <condition_variable>
#include <sys/prctl.h>          //prctl set timerslack for nanosleep
#include <atomic>               //atomic bool for predefined behavior /non copiable non chacheable etc. (prevent one thread to read a old value from a copy / chached state)


//global variables / objects
std::atomic_bool task_finished;
std::mutex cmu, mu;
std::condition_variable cv;
unsigned int k(0);
std::thread taskt, contrt;


//posix sched func
void setScheduling(std::thread& th, int policy, int priority) {
    sched_param sch_params;
    sch_params.sched_priority = priority;
    if(pthread_setschedparam(th.native_handle(), policy, &sch_params)) {
        std::cout << "Failed to set Thread scheduling : " << std::strerror(errno) << '\n';
    }
}

//task func prototype
void task_func();

//control thread
void cont_func() {
    
    //prctl (PR_SET_TIMERSLACK, 10000U, 0, 0, 0); //not sure if this is needed when execution is done with high prio and fifo sched on RT kernel
    
    taskt = std::thread{task_func};
    setScheduling(taskt, SCHED_FIFO, 30);
    mu.lock();
    std::cout << "Task started\n";
    mu.unlock();
    pthread_t nativeID = taskt.native_handle();
    taskt.detach();


    while (1) {
        auto start = std::chrono::high_resolution_clock::now();
        ++k;
        if (k > 200) {
            exit(0);
        }
        mu.lock();
        std::cout << "Loop No.: " << k << '\n';
        mu.unlock();

        std::this_thread::sleep_until(start+std::chrono::nanoseconds(500000000));

        if(task_finished) {
            //wake up task with cond var
            {
            std::unique_lock<std::mutex> locker(cmu);
            task_finished = false;
            }
            //locker.unlock();
            cv.notify_one();
        }
        else {
            mu.lock();
            std::cout << "Task time violation restarting thread...\n";
            mu.unlock();
            //taskt.std::thread::~thread();         //calling the destructor doesnt work even if the thread is either joined nor detached
                                                    //std is defined to abort programm if a thread runs out of scope
            //std::thread taskt(task_func);
            //setScheduling(taskt, SCHED_FIFO, 30);

            pthread_cancel(nativeID);
            pthread_join(nativeID, NULL);
            taskt = std::thread(task_func);
            setScheduling(taskt, SCHED_FIFO, 30);
            nativeID = taskt.native_handle();
            taskt.detach();

            
            /*
            pthread_cancel(taskt.native_handle());
            pthread_join(taskt.native_handle(), NULL);
            taskt = std::thread{task_func};
            setScheduling(taskt, SCHED_FIFO, 30);
            */
        }
    }

}

//task func
void task_func() {

    int simulation_time(100000000);

    while(1) {
        if (k%50 == 0) {
            simulation_time += 900000000;
        }
        
        std::this_thread::sleep_for(std::chrono::nanoseconds(simulation_time));
        
        std::unique_lock<std::mutex> locker(cmu);
        task_finished = true;
        cv.wait(locker, [](){ return !task_finished; }); //lambda to prevent spurious wake


    }

}


int main() {
    
    std::cout << "Starting Controller Thread...\n";
    contrt = std::thread{cont_func};
    setScheduling(contrt, SCHED_FIFO, 30);
    contrt.join();

    std::cout << "Loop cycle finished!\n";
    
    return 0;
}
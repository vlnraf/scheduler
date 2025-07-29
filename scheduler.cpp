#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <chrono>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#define MAX_THREADS 8

class Task{
    public:
    uint32_t waitingTime;
    std::function<void()> fun = nullptr;
    std::chrono::time_point<std::chrono::steady_clock> start;
    
    Task(){
        this->fun = nullptr;
    }
    Task(const std::function<void()> function, uint32_t waitingTime){
        this->fun = function;
        this->waitingTime = waitingTime;
        this->start = std::chrono::steady_clock::now();
    };

    void runTask(){
        this->fun();
    }

    ~Task(){

    };
};

class ThreadPool{
    private:
    std::thread threads[MAX_THREADS];
    size_t currentThreads;
    std::queue<Task> tasks;
    size_t counter = 0;
    std::mutex queueMutex;
    std::condition_variable cv;

    public:
    ThreadPool(size_t numThreads){
        if(numThreads > MAX_THREADS){
            printf("Numero di thread eccessivo, numero di threads massimo = 8");
            return;
        }
        this->currentThreads = numThreads;
        for(size_t i = 0; i < numThreads; i++){
            this->threads[i] = std::thread(&ThreadPool::runPool, this);
        }
    }

    void enqueueTask(Task& t){
        //tasks[counter++] = t;
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.push(t);
        cv.notify_one();
    }

    void runPool(){
        while(true){
            std::unique_lock<std::mutex> lock(queueMutex);

            cv.wait(lock, [this]{ return !tasks.empty();});
            
            if(tasks.empty()){
                return;
            }

            Task t = tasks.front();
            t.runTask();
            tasks.pop();
        }
    }

    ~ThreadPool(){
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.notify_all();
        for(size_t i = 0; i < currentThreads; i++){
            threads[i].join();
        }
    }
};

class Scheduler{
    Task tasks[1000];
    size_t counter = 0;
    ThreadPool pool;

    public:
    Scheduler(size_t numThreads) : pool(numThreads){
        //for(int i = 0; i < 1000; i++){
        //    this->tasks[i] = {};
        //}
    };
    ~Scheduler(){};

    void addTask(Task& t){
        this->tasks[counter++] = t;
        pool.enqueueTask(t);
    }

    void loop(){
        while(true){
            for(size_t i = 0; i < counter; i++){
                Task& t = tasks[i];
                const auto time = std::chrono::steady_clock::now();
                const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(time - t.start);
                if(t.fun && diff.count() >= t.waitingTime){
                    //t.runTask();
                    //this->addTask(t);
                    t.start = std::chrono::steady_clock::now();
                    pool.enqueueTask(t);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};

void printCiao(){
    printf("ciao\n");
}

void printRaffo(){
    printf("raffo\n");
}

int main(void){
    Scheduler s(2);
    Task t = Task(printCiao, 5000);
    Task t2 = Task(printRaffo, 10000);
    s.addTask(t);
    s.addTask(t2);
    s.loop();

    return 0;
}
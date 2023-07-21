#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <vector>
#include <queue>
#include <string>
#include <atomic>

class TaskManager {
public:
    TaskManager();
    ~TaskManager();

    void start();
    void queueJob(const std::string& source, const std::string& destination);
    void stop();
    bool busy() const;
    void wait();
    void printProgress() const;

private:
    void loop();
    bool loopCondition() const;
    bool waitCondition() const;
    static bool job(const std::string& source, const std::string& destination);

private:
    std::atomic<uint32_t> busyThreads;
    std::atomic<uint32_t> maxTasks;
    std::atomic<uint32_t> completeTasks;
    bool terminate;
    mutable std::mutex queueMutex;
    std::mutex busyMutex;
    std::condition_variable loopConditional;
    std::condition_variable waitConditional;
    std::vector<std::thread> threads;
    std::queue<std::pair<std::string, std::string>> jobs;
    std::function<bool()> boundLoopCondition;
    std::function<bool()> boundWaitCondition;

};


#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <vector>
#include <list>
#include <queue>
#include <string>
#include <atomic>
#include <iostream>
#include <array>

#include "loggable.h"

class TaskManager {
    typedef std::pair<bool, std::list<Loggable::Message>> JobResult;
public:
    TaskManager();
    ~TaskManager();

    void start();
    void queueJob(const std::string& source, const std::string& destination);
    void stop();
    bool busy() const;
    void wait();
    void printProgress() const;
    void printProgress(const JobResult& result, const std::string& source, const std::string& destination) const;

private:
    void loop();
    bool loopCondition() const;
    bool waitCondition() const;
    static JobResult job(const std::string& source, const std::string& destination);
    static void printLog(const JobResult& result, const std::string& source, const std::string& destination);
private:
    std::atomic<uint32_t> busyThreads;
    std::atomic<uint32_t> maxTasks;
    std::atomic<uint32_t> completeTasks;
    bool terminate;
    mutable std::mutex printMutex;
    mutable std::mutex queueMutex;
    std::mutex busyMutex;
    std::condition_variable loopConditional;
    std::condition_variable waitConditional;
    std::vector<std::thread> threads;
    std::queue<std::pair<std::string, std::string>> jobs;
    std::function<bool()> boundLoopCondition;
    std::function<bool()> boundWaitCondition;

};


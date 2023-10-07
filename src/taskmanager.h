#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <filesystem>
#include <vector>
#include <list>
#include <queue>
#include <string>
#include <atomic>
#include <iostream>
#include <array>
#include <memory>

#include "loggable.h"
#include "settings.h"

class TaskManager {
    typedef std::pair<bool, std::list<Loggable::Message>> JobResult;
public:
    TaskManager(const std::shared_ptr<Settings>& settings);
    ~TaskManager();

    void start();
    void queueJob(const std::filesystem::path& source, const std::filesystem::path& destination);
    void stop();
    bool busy() const;
    void wait();
    void printProgress() const;
    void printProgress(const JobResult& result, const std::filesystem::path& source, const std::filesystem::path& destination) const;

private:
    void loop();
    bool loopCondition() const;
    bool waitCondition() const;
    static JobResult mp3Job(const std::filesystem::path& source, const std::filesystem::path& destination);
    static void printLog(const JobResult& result, const std::filesystem::path& source, const std::filesystem::path& destination);
private:
    std::shared_ptr<Settings> settings;
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
    std::queue<std::pair<std::filesystem::path, std::filesystem::path>> jobs;
    std::function<bool()> boundLoopCondition;
    std::function<bool()> boundWaitCondition;

};


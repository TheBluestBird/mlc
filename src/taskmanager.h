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

#include "settings.h"
#include "logger/printer.h"

class TaskManager {
    typedef std::pair<bool, std::list<Logger::Message>> JobResult;
public:
    TaskManager(const std::shared_ptr<Settings>& settings, const std::shared_ptr<Printer>& logger);
    ~TaskManager();

    void start();
    void queueJob(const std::filesystem::path& source, const std::filesystem::path& destination);
    void stop();
    bool busy() const;
    void wait();

    unsigned int getCompleteTasks() const;

private:
    void loop();
    static JobResult mp3Job(const std::filesystem::path& source, const std::filesystem::path& destination, Logger::Severity logLevel);
private:
    std::shared_ptr<Settings> settings;
    std::shared_ptr<Printer> logger;
    unsigned int busyThreads;
    unsigned int maxTasks;
    unsigned int completeTasks;
    bool terminate;
    bool running;
    mutable std::mutex queueMutex;
    std::condition_variable loopConditional;
    std::condition_variable waitConditional;
    std::vector<std::thread> threads;
    std::queue<std::pair<std::filesystem::path, std::filesystem::path>> jobs;

};


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
    using JobResult = std::pair<bool, std::list<Logger::Message>>;
    struct Job;
public:
    TaskManager(const std::shared_ptr<Settings>& settings, const std::shared_ptr<Printer>& logger);
    ~TaskManager();

    void start();
    void queueConvert(const std::filesystem::path& source, const std::filesystem::path& destination);
    void queueCopy(const std::filesystem::path& source, const std::filesystem::path& destination);
    void stop();
    bool busy() const;
    void wait();

    unsigned int getCompleteTasks() const;

private:
    void loop();
    JobResult execute(Job& job);
    void printResult(const Job& job, const JobResult& result);
    static JobResult mp3Job(const Job& job, const std::shared_ptr<Settings>& settings);
    static JobResult copyJob(const Job& job, const std::shared_ptr<Settings>& settings);

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
    std::queue<Job> jobs;

};

struct TaskManager::Job {
    enum Type {
        copy,
        convert
    };
    Job(Type type, const std::filesystem::path& source, std::filesystem::path destination);
    Type type;
    std::filesystem::path source;
    std::filesystem::path destination;
};

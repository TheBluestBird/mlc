#include "taskmanager.h"

#include "flactomp3.h"

TaskManager::TaskManager():
    busyThreads(0),
    maxTasks(0),
    completeTasks(0),
    terminate(false),
    queueMutex(),
    busyMutex(),
    loopConditional(),
    waitConditional(),
    threads(),
    jobs(),
    boundLoopCondition(std::bind(&TaskManager::loopCondition, this)),
    boundWaitCondition(std::bind(&TaskManager::waitCondition, this))
{
}

TaskManager::~TaskManager() {
}

void TaskManager::queueJob(const std::string& source, const std::string& destination) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        jobs.emplace(source, destination);
    }
    ++maxTasks;
    loopConditional.notify_one();
    waitConditional.notify_all();
}

bool TaskManager::busy() const {
    bool result;
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        result = !jobs.empty();
    }

    return result;
}

void TaskManager::start() {
    const uint32_t num_threads = std::thread::hardware_concurrency();
    for (uint32_t ii = 0; ii < num_threads; ++ii)
        threads.emplace_back(std::thread(&TaskManager::loop, this));
}

void TaskManager::loop() {
    while (true) {
        std::pair<std::string, std::string> pair;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            loopConditional.wait(lock, boundLoopCondition);
            if (terminate)
                return;

            pair = jobs.front();
            ++busyThreads;
            waitConditional.notify_all();
            jobs.pop();
        }

        job(pair.first, pair.second);
        ++completeTasks;
        printProgress();
        --busyThreads;
        waitConditional.notify_all();
    }
}

bool TaskManager::loopCondition() const {
    return !jobs.empty() || terminate;
}

bool TaskManager::waitCondition() const {
    return busyThreads == 0 && !busy();
}

void TaskManager::stop() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        terminate = true;
    }

    loopConditional.notify_all();
    for (std::thread& thread : threads)
        thread.join();

    threads.clear();
}

void TaskManager::wait() {
    std::unique_lock<std::mutex> lock(busyMutex);
    waitConditional.wait(lock, boundWaitCondition);
}

bool TaskManager::job(const std::string& source, const std::string& destination) {
    FLACtoMP3 convertor;
    convertor.setInputFile(source);
    convertor.setOutputFile(destination);
    return convertor.run();
}

void TaskManager::printProgress() const {
    std::cout << "\r" << completeTasks << "/" << maxTasks << std::flush;
}

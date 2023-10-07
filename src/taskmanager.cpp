#include "taskmanager.h"

#include "flactomp3.h"

constexpr const std::array<std::string_view, Loggable::fatal + 1> logSettings({
    /*debug*/   "\e[90m",
    /*info*/    "\e[32m",
    /*minor*/   "\e[34m",
    /*major*/   "\e[94m",
    /*warning*/ "\e[33m",
    /*error*/   "\e[31m",
    /*fatal*/   "\e[91m"
});

constexpr const std::array<std::string_view, Loggable::fatal + 1> logHeaders({
    /*debug*/   "DEBUG: ",
    /*info*/    "INFO: ",
    /*minor*/   "MINOR: ",
    /*major*/   "MAJOR: ",
    /*warning*/ "WARNING: ",
    /*error*/   "ERROR: ",
    /*fatal*/   "FATAL: "
});

TaskManager::TaskManager(const std::shared_ptr<Settings>& settings):
    settings(settings),
    busyThreads(0),
    maxTasks(0),
    completeTasks(0),
    terminate(false),
    printMutex(),
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

void TaskManager::queueJob(const std::filesystem::path& source, const std::filesystem::path& destination) {
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

        JobResult result;
        switch (settings->getType()) {
            case Settings::mp3:
                result = mp3Job(pair.first, pair.second + ".mp3");
                break;
        }

        ++completeTasks;
        printProgress(result, pair.first, pair.second);
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

TaskManager::JobResult TaskManager::mp3Job(const std::filesystem::path& source, const std::filesystem::path& destination) {
    FLACtoMP3 convertor(Loggable::debug);
    convertor.setInputFile(source);
    convertor.setOutputFile(destination);
    bool result = convertor.run();
    return {result, convertor.getHistory()};
}

void TaskManager::printProgress() const {
    std::unique_lock<std::mutex> lock(printMutex);
    std::cout << "\r" << completeTasks << "/" << maxTasks << std::flush;
}

void TaskManager::printProgress(
    const TaskManager::JobResult& result,
    const std::filesystem::path& source,
    const std::filesystem::path& destination) const
{
    std::unique_lock<std::mutex> lock(printMutex);
    if (result.first) {
        if (result.second.size() > 0) {
            std::cout << "\r\e[1m" << "Encoding complete but there are messages about it" << "\e[0m" << std::endl;
            printLog(result, source, destination);
        }
    } else {
        std::cout << "\r\e[1m" << "Encoding failed!" << "\e[0m" << std::endl;
        printLog(result, source, destination);
    }
    std::cout << "\r" << completeTasks << "/" << maxTasks << std::flush;
}

void TaskManager::printLog(
    const TaskManager::JobResult& result,
    const std::filesystem::path& source,
    const std::filesystem::path& destination)
{
    std::cout << "Source: \t" << source << std::endl;
    std::cout << "Destination: \t" << destination << std::endl;
    for (const Loggable::Message& msg : result.second) {
        std::cout << "\t" << logSettings[msg.first] << "\e[1m" << logHeaders[msg.first] << "\e[22m" << msg.second << std::endl;
    }
    std::cout << "\e[0m" << std::endl;
}

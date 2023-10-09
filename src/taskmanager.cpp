#include "taskmanager.h"

#include "flactomp3.h"

TaskManager::TaskManager(const std::shared_ptr<Settings>& settings, const std::shared_ptr<Printer>& logger):
    settings(settings),
    logger(logger),
    busyThreads(0),
    maxTasks(0),
    completeTasks(0),
    terminate(false),
    running(false),
    queueMutex(),
    loopConditional(),
    waitConditional(),
    threads(),
    jobs()
{
}

TaskManager::~TaskManager() {
}

void TaskManager::queueJob(const std::filesystem::path& source, const std::filesystem::path& destination) {
    std::unique_lock<std::mutex> lock(queueMutex);
    jobs.emplace(source, destination);

    ++maxTasks;
    logger->setStatusMessage(std::to_string(completeTasks) + "/" + std::to_string(maxTasks));

    lock.unlock();
    loopConditional.notify_one();
}

bool TaskManager::busy() const {
    std::lock_guard lock(queueMutex);
    return !jobs.empty();
}

void TaskManager::start() {
    std::lock_guard lock(queueMutex);
    if (running)
        return;

    unsigned int amount = settings->getThreads();
    if (amount == 0)
        amount = std::thread::hardware_concurrency();

    for (uint32_t i = 0; i < amount; ++i)
        threads.emplace_back(std::thread(&TaskManager::loop, this));

    running = true;
}

void TaskManager::loop() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        while (!terminate && jobs.empty())
            loopConditional.wait(lock);

        if (terminate)
            return;

        std::pair<std::string, std::string> pair = jobs.front();
        ++busyThreads;
        jobs.pop();
        lock.unlock();

        JobResult result;
        switch (settings->getType()) {
            case Settings::mp3:
                result = mp3Job(pair.first, pair.second + ".mp3", settings->getLogLevel());
                break;
            default:
                break;
        }

        lock.lock();
        ++completeTasks;
        logger->printNested(
            result.first ? "Encoding complete but there are messages about it" : "Encoding failed!",
            {"Source: \t" + pair.first, "Destination: \t" + pair.second},
            result.second,
            std::to_string(completeTasks) + "/" + std::to_string(maxTasks)
        );
        --busyThreads;
        lock.unlock();
        waitConditional.notify_all();
    }
}

void TaskManager::stop() {
    std::unique_lock lock(queueMutex);
    if (!running)
        return;

    terminate = true;

    lock.unlock();
    loopConditional.notify_all();
    for (std::thread& thread : threads)
        thread.join();

    lock.lock();
    threads.clear();
    running = false;
    logger->clearStatusMessage();
}

void TaskManager::wait() {
    std::unique_lock lock(queueMutex);
    while (busyThreads != 0 || !jobs.empty())
        waitConditional.wait(lock);
}

unsigned int TaskManager::getCompleteTasks() const {
    std::lock_guard lock(queueMutex);
    return completeTasks;
}

TaskManager::JobResult TaskManager::mp3Job(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    Logger::Severity logLevel)
{
    FLACtoMP3 convertor(logLevel);
    convertor.setInputFile(source);
    convertor.setOutputFile(destination);
    bool result = convertor.run();
    return {result, convertor.getHistory()};
}

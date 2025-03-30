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

void TaskManager::queueConvert(const std::filesystem::path& source, const std::filesystem::path& destination) {
    if (settings->isExcluded(source))
        return;

    std::unique_lock<std::mutex> lock(queueMutex);
    jobs.emplace(Job::convert, source, destination);

    ++maxTasks;
    logger->setStatusMessage(std::to_string(completeTasks) + "/" + std::to_string(maxTasks));

    lock.unlock();
    loopConditional.notify_one();
}

void TaskManager::queueCopy(const std::filesystem::path& source, const std::filesystem::path& destination) {
    if (!settings->matchNonMusic(source.filename()))
        return;

    std::unique_lock<std::mutex> lock(queueMutex);
    jobs.emplace(Job::copy, source, destination);
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

        Job job = jobs.front();
        ++busyThreads;
        jobs.pop();
        lock.unlock();

        JobResult result = execute(job);

        lock.lock();
        ++completeTasks;
        printResult(job, result);
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

TaskManager::JobResult TaskManager::execute(Job& job) {
    switch (job.type) {
        case Job::copy:
            return copyJob(job, settings);
        case Job::convert:
            switch (settings->getType()) {
                case Settings::mp3:
                    job.destination.replace_extension("mp3");
                    return mp3Job(job, settings);
                default:
                    break;
            }
    }

    return {false, {
        {Logger::Severity::error, "Unknown job type: " + std::to_string(job.type)}
    }};
}

void TaskManager::printResult(const TaskManager::Job& job, const TaskManager::JobResult& result) {
    std::string msg;
    switch (job.type) {
        case Job::copy:
            if (result.first)
                msg = "File copy complete, but there are messages about it:";
            else
                msg = "File copy failed!";
            break;
        case Job::convert:
            if (result.first)
                msg = "Encoding complete but there are messages about it:";
            else
                msg = "Encoding failed!";
            break;
    }

    logger->printNested(
        msg,
        {"Source: \t" + job.source.string(), "Destination: \t" + job.destination.string()},
        result.second,
        std::to_string(completeTasks) + "/" + std::to_string(maxTasks)
    );
}

TaskManager::JobResult TaskManager::mp3Job(const TaskManager::Job& job, const std::shared_ptr<Settings>& settings) {
    FLACtoMP3 convertor(settings->getLogLevel());
    convertor.setInputFile(job.source);
    convertor.setOutputFile(job.destination);
    convertor.setParameters(settings->getEncodingQuality(), settings->getOutputQuality(), settings->getVBR());
    bool result = convertor.run();

    return {result, convertor.getHistory()};
}

TaskManager::JobResult TaskManager::copyJob(const TaskManager::Job& job, const std::shared_ptr<Settings>& settings) {
    (void)(settings);
    bool success = std::filesystem::copy_file(
        job.source,
        job.destination,
        std::filesystem::copy_options::overwrite_existing
    );

    return {success, {}};
}

TaskManager::Job::Job(Type type, const std::filesystem::path& source, std::filesystem::path destination):
    type(type),
    source(source),
    destination(destination) {}

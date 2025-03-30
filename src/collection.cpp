#include "collection.h"

#include "taskmanager.h"

namespace fs = std::filesystem;

static const std::string flac(".flac");

Collection::Collection(const std::filesystem::path& path, const std::shared_ptr<TaskManager>& tm, const std::shared_ptr<Settings>& st):
    path(path),
    countMusical(0),
    counted(false),
    taskManager(tm),
    settings(st)
{}

Collection::~Collection()
{}

void Collection::list() const {
    if (fs::is_regular_file(path))
        return;

    for (const fs::directory_entry & entry : fs::directory_iterator(path))
        std::cout << entry.path() << std::endl;
}

uint32_t Collection::countMusicFiles() const {
    if (counted)
        return countMusical;

    if (fs::is_regular_file(path)) {
        if (isMusic(path))
            ++countMusical;
    } else if (fs::is_directory(path)) {
        for (const fs::directory_entry& entry : fs::directory_iterator(path)) {
            if (settings->isExcluded(entry.path()))
                continue;

            switch (entry.status().type()) {
                case fs::file_type::regular:
                    if (isMusic(entry.path()))
                        ++countMusical;
                break;
                case fs::file_type::directory: {
                    Collection collection(entry.path(), taskManager, settings);
                    countMusical += collection.countMusicFiles();
                }   break;
                default:
                    break;
            }
        }
    }

    counted = true;
    return countMusical;
}

void Collection::convert(const std::string& outPath) {
    fs::path out = fs::absolute(outPath);

    fs::create_directories(out);
    out = fs::canonical(outPath);
    for (const fs::directory_entry& entry : fs::directory_iterator(path)) {
        fs::path sourcePath = entry.path();
        if (settings->isExcluded(sourcePath))
            continue;

        switch (entry.status().type()) {
            case fs::file_type::regular: {
                if (isMusic(sourcePath))
                    taskManager->queueConvert(sourcePath, out / sourcePath.stem());
                else
                    taskManager->queueCopy(sourcePath, out / sourcePath.filename());
            }   break;
            case fs::file_type::directory: {
                Collection collection(sourcePath, taskManager, settings);
                fs::path::iterator itr = sourcePath.end();
                --itr;
                collection.convert(std::string(out / *itr));
            }   break;
            default:
                break;
        }
    }
}

bool Collection::isMusic(const std::filesystem::path& path) {
    return path.extension() == flac;    //I know, it's primitive yet, but it's the fastest
}


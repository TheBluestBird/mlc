#include "collection.h"

#include "taskmanager.h"

namespace fs = std::filesystem;

static const std::string flac(".flac");

Collection::Collection(const std::filesystem::path& path, TaskManager* tm) :
    path(path),
    countMusical(0),
    counted(false),
    taskManager(tm)
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
            switch (entry.status().type()) {
                case fs::file_type::regular:
                    if (isMusic(entry.path()))
                        ++countMusical;
                break;
                case fs::file_type::directory: {
                    Collection collection(entry.path());
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
    if (taskManager == nullptr)
        throw 6;

    fs::path out = fs::absolute(outPath);

    fs::create_directories(out);
    out = fs::canonical(outPath);
    for (const fs::directory_entry& entry : fs::directory_iterator(path)) {
        switch (entry.status().type()) {
            case fs::file_type::regular: {
                fs::path sourcePath = entry.path();
                if (isMusic(sourcePath))
                    taskManager->queueConvert(sourcePath, out / sourcePath.stem());
                else
                    taskManager->queueCopy(sourcePath, out / sourcePath.filename());
            }   break;
            case fs::file_type::directory: {
                fs::path sourcePath = entry.path();
                Collection collection(sourcePath, taskManager);
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


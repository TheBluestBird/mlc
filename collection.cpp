#include "collection.h"

namespace fs = std::filesystem;

static const std::string flac(".flac");

Collection::Collection(const std::string& path) :
    path(fs::canonical(path)),
    countMusical(0),
    counted(false),
    convertedSoFar(0)
{
}

Collection::Collection(const std::filesystem::path& path):
    path(fs::canonical(path)),
    countMusical(0),
    counted(false),
    convertedSoFar(0)
{
}


Collection::~Collection() {
}

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

void Collection::convert(const std::string& outPath, std::function<void()> progress) {
    convertedSoFar = 0;
    fs::path out = fs::absolute(outPath);
    if (progress == nullptr) {
        progress = std::bind(&Collection::prg, this);
    }

    fs::create_directories(out);
    out = fs::canonical(outPath);
    for (const fs::directory_entry& entry : fs::directory_iterator(path)) {
        switch (entry.status().type()) {
            case fs::file_type::regular: {
                fs::path sourcePath = entry.path();
                fs::path dstPath = out / sourcePath.filename();
                if (isMusic(sourcePath)) {
                    dstPath.replace_extension(".mp3");
                    FLACtoMP3 convertor;
                    convertor.setInputFile(sourcePath);
                    convertor.setOutputFile(dstPath);
                    convertor.run();
                    progress();
                } else {
                    fs::copy_file(sourcePath, dstPath);
                }
                //std::cout << sourcePath << " => " << dstPath << std::endl;
            }   break;
            case fs::file_type::directory: {
                fs::path sourcePath = entry.path();
                Collection collection(sourcePath);
                fs::path::iterator itr = sourcePath.end();
                --itr;
                collection.convert(std::string(out / *itr), progress);
            }   break;
            default:
                break;
        }
    }
}

bool Collection::isMusic(const std::filesystem::path& path) {
    return path.extension() == flac;    //I know, it's primitive yet, but it's the fastest
}

void Collection::prg() const {
    std::cout << "\r" << ++convertedSoFar << "/" << countMusicFiles() << std::flush;
}


#pragma once

#include <string>
#include <iostream>
#include <filesystem>
#include <memory>

#include "settings.h"
#include "flactomp3.h"

class TaskManager;

class Collection {
public:
    Collection(const std::filesystem::path& path, const std::shared_ptr<TaskManager>& tm, const std::shared_ptr<Settings>& st);
    ~Collection();

    void list() const;
    uint32_t countMusicFiles() const;
    void convert(const std::string& outPath);

private:
    static bool isMusic(const std::filesystem::path& path);

private:
    std::filesystem::path path;
    mutable uint32_t countMusical;
    mutable bool counted;
    std::shared_ptr<TaskManager> taskManager;
    std::shared_ptr<Settings> settings; 
};


#pragma once

#include <string>
#include <iostream>
#include <filesystem>

#include "flactomp3.h"

class TaskManager;

class Collection {
public:
    Collection(const std::string& path, TaskManager* tm = nullptr);
    Collection(const std::filesystem::path& path, TaskManager* tm = nullptr);
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
    TaskManager* taskManager;
};


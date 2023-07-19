#pragma once

#include <string>
#include <iostream>
#include <filesystem>
#include <functional>

#include "flactomp3.h"

class Collection {
public:
    Collection(const std::string& path);
    Collection(const std::filesystem::path& path);
    ~Collection();

    void list() const;
    uint32_t countMusicFiles() const;
    void convert(const std::string& outPath, std::function<void()> progress = nullptr);

private:
    static bool isMusic(const std::filesystem::path& path);
    void prg() const;

private:
    std::filesystem::path path;
    mutable uint32_t countMusical;
    mutable bool counted;
    mutable uint32_t convertedSoFar;
};


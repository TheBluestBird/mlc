#pragma once

#include <stdint.h>
#include <string_view>
#include "FLAC/stream_decoder.h"

class Decoded {
public:
    Decoded();
    ~Decoded();

    void initializeStream(uint64_t total, uint32_t rate, uint16_t channels, uint16_t depth);

    bool streamInitialized() const;
    void printInfo() const;
    uint64_t pcmSize() const;
    uint64_t waveStremSize() const;
    void write(uint16_t data);
    void write(uint32_t data);
    void write(std::string_view data);
    void writeWaveHeader();
    void saveWaveToFile(const std::string& path);

    static Decoded* fromFLACFile(const std::string& path);

    static void FLACerror(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
    static void FLACmetadata(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
    static FLAC__StreamDecoderWriteStatus FLACwrite(
        const FLAC__StreamDecoder *decoder,
        const FLAC__Frame *frame,
        const FLAC__int32 * const buffer[],
        void *client_data
    );

private:
    uint64_t totalSamples;
    uint32_t sampleRate;
    uint16_t channels;
    uint16_t depth;
    uint64_t streamPosition;
    uint8_t* stream;
    uint64_t streamSize;
};


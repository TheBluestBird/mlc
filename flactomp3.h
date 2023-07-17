#pragma once

#include "FLAC/stream_decoder.h"
#include <lame/lame.h>

#include <string>
#include <string_view>
#include <iostream>
#include <map>
#include <stdio.h>

class FLACtoMP3 {
public:
    FLACtoMP3(uint8_t size = 4);
    ~FLACtoMP3();

    void setInputFile(const std::string& path);
    void setOutputFile(const std::string& path);
    void run();

private:
    void processTags(const FLAC__StreamMetadata_VorbisComment& tags);
    void processInfo(const FLAC__StreamMetadata_StreamInfo& info);
    bool decodeFrame(const int32_t * const buffer[], uint32_t size);
    bool flush();
    bool initializeOutput();
    bool tryKnownTag(std::string_view source);

    static void error(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
    static void metadata(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
    static FLAC__StreamDecoderWriteStatus write(
        const FLAC__StreamDecoder *decoder,
        const FLAC__Frame *frame,
        const FLAC__int32 * const buffer[],
        void *client_data
    );

private:
    std::string inPath;
    std::string outPath;

    FLAC__StreamDecoder *decoder;
    lame_t encoder;
    FLAC__StreamDecoderInitStatus statusFLAC;

    FILE* output;
    uint32_t pcmCounter;
    uint32_t pcmSize;
    int16_t* pcm;
    uint8_t* outputBuffer;
    bool outputInitilized;
};

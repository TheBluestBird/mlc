#pragma once

#include <stream_decoder.h>
#include <lame.h>
#include <jpeglib.h>
#include <id3v2tag.h>

#include <string>
#include <string_view>
#include <map>
#include <array>
#include <stdio.h>

#include "logger/accumulator.h"

class FLACtoMP3 {
public:
    FLACtoMP3(Logger::Severity severity = Logger::Severity::info, uint8_t size = 4);
    ~FLACtoMP3();

    void setInputFile(const std::string& path);
    void setOutputFile(const std::string& path);
    void setParameters(unsigned char encodingQuality, unsigned char outputQuality, bool vbr);
    bool run();

    std::list<Logger::Message> getHistory() const;

private:
    void processTags(const FLAC__StreamMetadata_VorbisComment& tags);
    void processInfo(const FLAC__StreamMetadata_StreamInfo& info);
    void processPicture(const FLAC__StreamMetadata_Picture& picture);
    bool decodeFrame(const int32_t * const buffer[], uint32_t size);
    bool flush();
    bool initializeOutput();
    bool scaleJPEG(const FLAC__StreamMetadata_Picture& picture);
    void attachPictureFrame(const FLAC__StreamMetadata_Picture& picture, const TagLib::ByteVector& bytes);

    static void error(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
    static void metadata(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
    static FLAC__StreamDecoderWriteStatus write(
        const FLAC__StreamDecoder *decoder,
        const FLAC__Frame *frame,
        const FLAC__int32 * const buffer[],
        void *client_data
    );

private:
    Accumulator logger;
    std::string inPath;
    std::string outPath;

    FLAC__StreamDecoder *decoder;
    lame_t encoder;
    FLAC__StreamDecoderInitStatus statusFLAC;

    FILE* output;
    uint8_t bufferMultiplier;
    uint32_t flacMaxBlockSize;
    uint32_t pcmCounter;
    uint32_t pcmSize;
    int16_t* pcm;
    uint8_t* outputBuffer;
    uint32_t outputBufferSize;
    bool outputInitilized;
    bool downscaleAlbumArt;
    TagLib::ID3v2::Tag id3v2tag;
};

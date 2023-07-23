#pragma once

#include "FLAC/stream_decoder.h"
#include <lame/lame.h>
#include <jpeglib.h>

#include <string>
#include <string_view>
#include <codecvt>
#include <locale>
#include <map>
#include <stdio.h>

#include "loggable.h"

class FLACtoMP3 : public Loggable {
public:
    FLACtoMP3(Severity severity = info, uint8_t size = 4);
    ~FLACtoMP3();

    void setInputFile(const std::string& path);
    void setOutputFile(const std::string& path);
    bool run();

private:
    void processTags(const FLAC__StreamMetadata_VorbisComment& tags);
    void processInfo(const FLAC__StreamMetadata_StreamInfo& info);
    void processPicture(const FLAC__StreamMetadata_Picture& picture);
    bool decodeFrame(const int32_t * const buffer[], uint32_t size);
    bool flush();
    bool initializeOutput();
    bool tryKnownTag(std::string_view source);
    bool scaleJPEG(const FLAC__StreamMetadata_Picture& picture);

    void setTagTitle(const std::string_view& title);
    void setTagAlbum(const std::string_view& album);
    void setTagArtist(const std::string_view& artist);

    int setTagUSC2(const std::string_view& field, const std::string_view& value);

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
    uint8_t bufferMultiplier;
    uint32_t flacMaxBlockSize;
    uint32_t pcmCounter;
    uint32_t pcmSize;
    int16_t* pcm;
    uint8_t* outputBuffer;
    uint32_t outputBufferSize;
    bool outputInitilized;
    bool downscaleAlbumArt;
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> usc2convertor;
};

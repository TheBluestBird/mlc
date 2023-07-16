#include "decoded.h"

#include <iostream>
#include <ios>
#include <fstream>

constexpr uint8_t waveDeclarationSize = 8;
constexpr uint8_t waveDescriptionSize = 36;
constexpr uint8_t waveHeaderSize = waveDeclarationSize + waveDescriptionSize;

Decoded::Decoded():
    totalSamples(0),
    sampleRate(0),
    channels(0),
    depth(0),
    streamPosition(0),
    stream(nullptr),
    streamSize(0)
{}

Decoded::~Decoded() {
    if (streamInitialized())
        delete[] stream;
}

bool Decoded::streamInitialized() const {
    return sampleRate != 0;
}

void Decoded::initializeStream(uint64_t total, uint32_t rate, uint16_t channels, uint16_t depth) {
    if (streamInitialized())
        throw 1;

    totalSamples = total;
    sampleRate = rate;
    Decoded::channels = channels;
    Decoded::depth = depth;
    streamPosition = 0;

    streamSize = pcmSize() + waveHeaderSize;
    stream = new uint8_t[streamSize];
}

void Decoded::printInfo() const {
    std::cout << "sample rate       : " << sampleRate << " Hz" << std::endl;
    std::cout << "channels          : " << channels << std::endl;
    std::cout << "bits per sample   : " << depth << std::endl;
    std::cout << "total samples     : " << totalSamples << std::endl;
}

void Decoded::saveWaveToFile(const std::string& path) {
    std::ofstream fp;
    fp.open(path, std::ios::out | std::ios::binary);
    fp.write((char*)stream, streamSize);
    fp.close();
}

uint64_t Decoded::waveStremSize() const {
    return streamSize;
}


void Decoded::write(uint16_t data) {
    uint16_t* stream16 = (uint16_t*)stream;
    stream16[streamPosition / 2] = data;
    streamPosition += 2;
}

void Decoded::write(std::string_view data) {
    for (const char& letter : data) {
        stream[streamPosition] = (uint8_t)letter;
        ++streamPosition;
    }
}

void Decoded::write(uint32_t data) {
    uint32_t* stream32 = (uint32_t*)stream;
    stream32[streamPosition / 4] = data;
    streamPosition += 4;
}

void Decoded::writeWaveHeader() {
    uint32_t pcm = pcmSize();
    uint8_t bytesPerSample = depth / 8;

    write("RIFF");                                                      //4
    write(pcm + waveDescriptionSize);                                   //8
    write("WAVEfmt ");                                          //8     //16
    write((uint32_t)16);                                        //12    //20
    write((uint16_t)1);                                         //14    //22
    write(channels);                                            //16    //24
    write(sampleRate);                                          //20    //28
    write((uint32_t)(sampleRate * channels * bytesPerSample));  //24    //32
    write((uint16_t)(channels * bytesPerSample));//block align  //26    //34
    write(depth);                                               //28    //36
    write("data");                                              //32    //40
    write(pcm);                                                 //36    //44
}

uint64_t Decoded::pcmSize() const {
    return totalSamples * channels * (depth / 8);
}

void Decoded::FLACerror(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data) {
    (void)decoder, (void)client_data;
    std::cout << "Got error callback: " << FLAC__StreamDecoderErrorStatusString[status] << std::endl;
}

void Decoded::FLACmetadata(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data) {
    (void)(decoder);
    Decoded* decoded = static_cast<Decoded*>(client_data);

    /* print some stats */
    if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        /* save for later */
        decoded->initializeStream(
            metadata->data.stream_info.total_samples,
            metadata->data.stream_info.sample_rate,
            metadata->data.stream_info.channels,
            metadata->data.stream_info.bits_per_sample
        );

        decoded->printInfo();
    }
}

FLAC__StreamDecoderWriteStatus Decoded::FLACwrite(
    const FLAC__StreamDecoder* decoder,
    const FLAC__Frame* frame,
    const FLAC__int32 * const buffer[],
    void* client_data
) {
    Decoded* decoded = static_cast<Decoded*>(client_data);

    if (decoded->totalSamples == 0) {
        std::cout << "ERROR: this example only works for FLAC files that have a total_samples count in STREAMINFO" << std::endl;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if (decoded->channels != 2 || decoded->depth != 16) {
        std::cout << "ERROR: this example only supports 16bit stereo streams" << std::endl;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if (frame->header.channels != 2) {
        std::cout << "ERROR: This frame contains " << frame->header.channels << " channels (should be 2)" << std::endl;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if (buffer[0] == NULL) {
        std::cout << "ERROR: buffer [0] is NULL" << std::endl;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if (buffer[1] == NULL) {
        std::cout << "ERROR: buffer [1] is NULL" << std::endl;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    /* write WAVE header before we write the first frame */
    if (frame->header.number.sample_number == 0)
        decoded->writeWaveHeader();

    /* write decoded PCM samples */
    for (size_t i = 0; i < frame->header.blocksize; i++) {
        decoded->write((uint16_t)buffer[0][i]);
        decoded->write((uint16_t)buffer[1][i]);
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

Decoded* Decoded::fromFLACFile(const std::string& path) {
    FLAC__bool ok = true;
    FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
    FLAC__StreamDecoderInitStatus init_status;

    Decoded* decoded = new Decoded;

    if (decoder == NULL) {
        std::cout << "error allocating flac decoder" << std::endl;
        throw 2;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, true);
    init_status = FLAC__stream_decoder_init_file(decoder, path.c_str(), FLACwrite, FLACmetadata, FLACerror, decoded);

    ok = init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK;
    if (ok) {
        std::cout << "error initializing decoder: " << FLAC__StreamDecoderInitStatusString[init_status] << std::endl;
    } else {
        ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
        std::cout << "decoding: ";
        if (ok)
            std::cout << "succeeded";
        else
            std::cout << "FAILED";

        std::cout << std::endl;
        std::cout << "   state: " << FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)] << std::endl;
    }

    FLAC__stream_decoder_delete(decoder);

    return decoded;
}


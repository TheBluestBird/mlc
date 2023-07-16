#include "flactomp3.h"

constexpr uint16_t flacBlockMaxSize = 4096;

FLACtoMP3::FLACtoMP3(uint8_t size) :
    inPath(),
    outPath(),
    decoder(FLAC__stream_decoder_new()),
    encoder(lame_init()),
    statusFLAC(),
    output(),
    pcmCounter(0),
    pcmSize(size * flacBlockMaxSize),
    pcm(new int16_t[pcmSize * 2]),
    mp3(new uint8_t[pcmSize])
{
}

FLACtoMP3::~FLACtoMP3() {
    lame_close(encoder);
    FLAC__stream_decoder_delete(decoder);
    delete[] pcm;
    delete[] mp3;
}

void FLACtoMP3::run() {
    FLAC__bool ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
    std::cout << "decoding: ";
    if (ok)
        std::cout << "succeeded";
    else
        std::cout << "FAILED";

    std::cout << std::endl;
    std::cout << "   state: " << FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)] << std::endl;

    flush();
    int nwrite = lame_encode_flush(encoder, mp3, pcmSize);
    output.write((char*)mp3, nwrite);

    // // 7. Write INFO tag (OPTIONAL)
    // // lame_mp3_tags_fid(lame, mp3);
    output.close();
}

void FLACtoMP3::setInputFile(const std::string& path) {
    if (inPath.size() > 0)
        throw 1;

    inPath = path;

    FLAC__stream_decoder_set_md5_checking(decoder, true);
    statusFLAC = FLAC__stream_decoder_init_file(decoder, path.c_str(), write, metadata, error, this);
}

void FLACtoMP3::setOutputFile(const std::string& path) {
    if (outPath.size() > 0)
        throw 2;

    outPath = path;

    // 3. Do some settings (OPTIONAL)
    lame_set_in_samplerate(encoder, 44100);
    lame_set_VBR(encoder, vbr_default);
    lame_set_VBR_quality(encoder, 2);

    // 4. Initialize parameters
    int ret = lame_init_params(encoder);
    if (ret < 0) {
        std::cout << "Error occurred during parameters initializing. Code = " << ret << std::endl;
        throw 3;
    }

    output.open(path, std::ios::out | std::ios::binary);
}

void FLACtoMP3::decodeFrame(const int32_t * const buffer[], uint32_t size) {
    for (size_t i = 0; i < size; ++i) {
        pcm[2 * pcmCounter] = (int16_t)buffer[0][i];
        pcm[2 * pcmCounter + 1] = (int16_t)buffer[1][i];
        ++pcmCounter;
    }

    if (pcmCounter == pcmSize)
        flush();
}

void FLACtoMP3::flush() {
    int nwrite = lame_encode_buffer_interleaved(
        encoder,
        pcm,
        pcmCounter,
        mp3,
        pcmSize
    );
    output.write((char*)mp3, nwrite);
    pcmCounter = 0;
}

void FLACtoMP3::metadata(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data) {

}

FLAC__StreamDecoderWriteStatus FLACtoMP3::write(
    const FLAC__StreamDecoder* decoder,
    const FLAC__Frame* frame,
    const FLAC__int32 * const buffer[],
    void* client_data
){
    FLACtoMP3* self = static_cast<FLACtoMP3*>(client_data);
    self->decodeFrame(buffer, frame->header.blocksize);

    // if (decoded->totalSamples == 0) {
    //     std::cout << "ERROR: this example only works for FLAC files that have a total_samples count in STREAMINFO" << std::endl;
    //     return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    // }
    // if (decoded->channels != 2 || decoded->depth != 16) {
    //     std::cout << "ERROR: this example only supports 16bit stereo streams" << std::endl;
    //     return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    // }
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

    // /* write WAVE header before we write the first frame */
    // if (frame->header.number.sample_number == 0)
    //     decoded->writeWaveHeader();
    //
    // /* write decoded PCM samples */
    // for (size_t i = 0; i < frame->header.blocksize; i++) {
    //     decoded->write((uint16_t)buffer[0][i]);
    //     decoded->write((uint16_t)buffer[1][i]);
    // }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACtoMP3::error(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data) {
    (void)decoder, (void)client_data;
    std::cout << "Got error callback: " << FLAC__StreamDecoderErrorStatusString[status] << std::endl;
}

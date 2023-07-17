#include "flactomp3.h"

constexpr uint16_t flacBlockMaxSize = 4096;

static const std::string_view title ("TITLE=");
static const std::string_view artist ("ARTIST=");
static const std::string_view album ("ALBUM=");
static const std::string_view comment ("COMMENT=");
static const std::string_view genre ("GENRE=");
static const std::string_view track ("TRACKNUMBER=");
static const std::string_view date ("DATE=");

static const std::map<std::string, const std::string> knownTags {
    {"ALBUMARTIST=", "TPE2="},
    {"PUBLISHER=", "TPUB="},
    {"LENGTH=", "TLEN="},
    {"ISRC=", "TSRC="},
    {"DISCNUMBER=", "TPOS="},
    {"BPM=", "TBPM="}
};

FLACtoMP3::FLACtoMP3(uint8_t size) :
    inPath(),
    outPath(),
    decoder(FLAC__stream_decoder_new()),
    encoder(lame_init()),
    statusFLAC(),
    output(nullptr),
    pcmCounter(0),
    pcmSize(size * flacBlockMaxSize),
    pcm(new int16_t[pcmSize * 2]),
    outputBuffer(new uint8_t[pcmSize * 2]),
    outputInitilized(false)
{
}

FLACtoMP3::~FLACtoMP3() {
    lame_close(encoder);
    FLAC__stream_decoder_delete(decoder);
    delete[] pcm;
    delete[] outputBuffer;
}

void FLACtoMP3::run() {
    FLAC__bool ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
    std::cout << "decoding: ";
    if (ok) {
        std::cout << "succeeded";

        flush();
        int nwrite = lame_encode_flush(encoder, outputBuffer, pcmSize * 2);
        fwrite((char*)outputBuffer ,nwrite, 1, output);

        // // 7. Write INFO tag (OPTIONAL)
        lame_mp3_tags_fid(encoder, output);

    } else {
        std::cout << "FAILED";
    }

    std::cout << std::endl;
    std::cout << "   state: " << FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)] << std::endl;

    if (outputInitilized) {
        fclose(output);
        output = nullptr;
    }
}

void FLACtoMP3::setInputFile(const std::string& path) {
    if (inPath.size() > 0)
        throw 1;

    inPath = path;

    FLAC__stream_decoder_set_md5_checking(decoder, true);
    FLAC__stream_decoder_set_metadata_respond_all(decoder);
    statusFLAC = FLAC__stream_decoder_init_file(decoder, path.c_str(), write, metadata, error, this);
}

void FLACtoMP3::setOutputFile(const std::string& path) {
    if (outPath.size() > 0)
        throw 2;

    outPath = path;

    lame_set_VBR(encoder, vbr_default);
    lame_set_VBR_quality(encoder, 0);
    lame_set_quality(encoder, 0);
}

bool FLACtoMP3::initializeOutput() {
    if (outputInitilized)
        throw 5;

    output = fopen(outPath.c_str(), "w+b");
    if (output == 0) {
        output = nullptr;
        std::cout << "Error opening file " << outPath << std::endl;
        return false;
    }

    int ret = lame_init_params(encoder);
    if (ret < 0) {
        std::cout << "Error occurred during parameters initializing. Code = " << ret << std::endl;
        fclose(output);
        output = nullptr;
        return false;
    }

    outputInitilized = true;
    return true;
}

void FLACtoMP3::processInfo(const FLAC__StreamMetadata_StreamInfo& info) {
    lame_set_in_samplerate(encoder, info.sample_rate);
    lame_set_num_channels(encoder, info.channels);
}

void FLACtoMP3::processTags(const FLAC__StreamMetadata_VorbisComment& tags) {
    for (FLAC__uint32 i = 0; i < tags.num_comments; ++i) {
        const FLAC__StreamMetadata_VorbisComment_Entry& entry = tags.comments[i];
        std::string_view comm((const char*)entry.entry);
        if (comm.find(title) == 0) {
            id3tag_set_title(encoder, comm.substr(title.size()).data());
        } else if (comm.find(artist) == 0) {
            id3tag_set_artist(encoder, comm.substr(artist.size()).data());
        } else if (comm.find(album) == 0) {
            id3tag_set_album(encoder, comm.substr(album.size()).data());
        } else if (comm.find(comment) == 0) {
            id3tag_set_comment(encoder, comm.substr(comment.size()).data());
        } else if (comm.find(genre) == 0) {
            id3tag_set_genre(encoder, comm.substr(genre.size()).data());
        } else if (comm.find(track) == 0) {
            id3tag_set_track(encoder, comm.substr(track.size()).data());
        } else if (comm.find(date) == 0) {
            std::string_view fullDate = comm.substr(date.size());
            if (fullDate.size() == 10) {
                std::string_view month = fullDate.substr(5, std::size_t(2));
                std::string_view day = fullDate.substr(8);
                std::string md = "TDAT=" + std::string(month) + std::string(day);
                int res = id3tag_set_fieldvalue(encoder, md.c_str());
                if (res != 0)
                    std::cout << "wasn't able to set the date tag (" << md << ")" << std::endl;

                fullDate = fullDate.substr(0, 4);   //year;
            }
            id3tag_set_year(encoder, std::string(fullDate).data());
        } else if (!tryKnownTag(comm)) {
            std::string tag = "TXXX=" + std::string(comm);
            int res = id3tag_set_fieldvalue(encoder, tag.c_str());
            if (res != 0)
                std::cout << "wasn't able to set user tag (" << comm << ")" << std::endl;
        }
    }
}

bool FLACtoMP3::tryKnownTag(std::string_view source) {
    for (const std::pair<const std::string, const std::string>& pair : knownTags) {
        if (source.find(pair.first) == 0) {
            std::string tag = pair.second + std::string(source.substr(pair.first.size()));
            int res = id3tag_set_fieldvalue(encoder, tag.c_str());
            if (res != 0)
                std::cout << "wasn't able to set tag (" << source << ")" << std::endl;

            return true;
        }
    }

    return false;
}


bool FLACtoMP3::decodeFrame(const int32_t * const buffer[], uint32_t size) {
    if (!outputInitilized) {
        bool success = initializeOutput();
        if (!success)
            return false;
    }

    for (size_t i = 0; i < size; ++i) {
        pcm[2 * pcmCounter] = (int16_t)buffer[0][i];
        pcm[2 * pcmCounter + 1] = (int16_t)buffer[1][i];
        ++pcmCounter;
    }

    if (pcmCounter == pcmSize)
        return flush();

    return true;
}

bool FLACtoMP3::flush() {
    int nwrite = lame_encode_buffer_interleaved(
        encoder,
        pcm,
        pcmCounter,
        outputBuffer,
        pcmSize * 2
    );
    int actuallyWritten = fwrite((char*)outputBuffer, nwrite, 1, output);
    pcmCounter = 0;
    return actuallyWritten == 1;
}

void FLACtoMP3::metadata(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data) {
    (void)(decoder);
    FLACtoMP3* self = static_cast<FLACtoMP3*>(client_data);

    switch (metadata->type) {
        case FLAC__METADATA_TYPE_STREAMINFO:
            self->processInfo(metadata->data.stream_info);
            break;
        case FLAC__METADATA_TYPE_VORBIS_COMMENT:
            self->processTags(metadata->data.vorbis_comment);
            break;
        default:
            break;
    }
}

FLAC__StreamDecoderWriteStatus FLACtoMP3::write(
    const FLAC__StreamDecoder* decoder,
    const FLAC__Frame* frame,
    const FLAC__int32 * const buffer[],
    void* client_data
) {
    (void)(decoder);
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

    FLACtoMP3* self = static_cast<FLACtoMP3*>(client_data);
    bool result = self->decodeFrame(buffer, frame->header.blocksize);

    if (result)
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    else
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

void FLACtoMP3::error(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data) {
    (void)decoder, (void)client_data;
    std::cout << "Got error callback: " << FLAC__StreamDecoderErrorStatusString[status] << std::endl;
}

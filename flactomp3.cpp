#include "flactomp3.h"

#include <cmath>

constexpr uint16_t flacDefaultMaxBlockSize = 4096;
constexpr uint16_t id3v1TagSize = 128;
// constexpr std::wstring_view BOM ({(0xFEFF}); //one day...

constexpr std::string_view title ("TITLE=");
constexpr std::string_view artist ("ARTIST=");
constexpr std::string_view album ("ALBUM=");
constexpr std::string_view comment ("COMMENT=");
constexpr std::string_view genre ("GENRE=");
constexpr std::string_view track ("TRACKNUMBER=");
constexpr std::string_view date ("DATE=");

constexpr std::string_view jpeg ("image/jpeg");


static const std::map<std::string, const std::string> knownTags {
    {"ALBUMARTIST=", "TPE2="},
    {"PUBLISHER=", "TPUB="},
    {"LENGTH=", "TLEN="},
    {"ISRC=", "TSRC="},
    {"DISCNUMBER=", "TPOS="},
    {"BPM=", "TBPM="},
    {"LYRICS=", "USLT="}    //but it's not supported in LAME
};

FLACtoMP3::FLACtoMP3(Severity severity, uint8_t size) :
    Loggable(severity),
    inPath(),
    outPath(),
    decoder(FLAC__stream_decoder_new()),
    encoder(lame_init()),
    statusFLAC(),
    output(nullptr),
    bufferMultiplier(size),
    flacMaxBlockSize(0),
    pcmCounter(0),
    pcmSize(0),
    pcm(nullptr),
    outputBuffer(nullptr),
    outputBufferSize(0),
    outputInitilized(false),
    downscaleAlbumArt(false),
    usc2convertor()
{
}

FLACtoMP3::~FLACtoMP3() {
    lame_close(encoder);
    FLAC__stream_decoder_delete(decoder);
}

bool FLACtoMP3::run() {
    FLAC__bool ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
    uint32_t fileSize;
    if (ok) {
        if (pcmCounter > 0)
            flush();

        int nwrite = lame_encode_flush(encoder, outputBuffer, pcmSize * 2);
        fwrite((char*)outputBuffer, nwrite, 1, output);

        if (downscaleAlbumArt) {
            fileSize = ftell(output);
            lame_mp3_tags_fid(encoder, output);
        } else {
            int tag1Size = lame_get_id3v1_tag(encoder, outputBuffer, id3v1TagSize);
            if (tag1Size > id3v1TagSize)
                log(warning, "couldn't write id3v1 tag");
            else
                fwrite((char*)outputBuffer, tag1Size, 1, output);

            fileSize = ftell(output);
            fseek(output, 0, SEEK_SET);
            int tag2Size = lame_get_id3v2_tag(encoder, outputBuffer, outputBufferSize);
            if (tag2Size > outputBufferSize) {
                log(Loggable::error, "couldn't write id3v2 tag");
            } else
                fwrite((char*)outputBuffer, tag2Size, 1, output);

            int vbrTagSize = lame_get_lametag_frame(encoder, outputBuffer, outputBufferSize);
            if (vbrTagSize > outputBufferSize)
                log(Loggable::error, "couldn't write vbr tag");

            fwrite((char*)outputBuffer, vbrTagSize, 1, output);
        }
    }

    // std::cout << "   state: " << FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)] << std::endl;

    if (outputInitilized) {
        fclose(output);
        output = nullptr;

        delete[] pcm;
        delete[] outputBuffer;

        pcm = nullptr;
        outputBuffer = nullptr;
        pcmSize = 0;
        flacMaxBlockSize = 0;
        outputBufferSize = 0;
        if (ok) {
            float MBytes = (float)fileSize / 1024 / 1024;
            std::string strMBytes = std::to_string(MBytes);
            strMBytes = strMBytes.substr(0, strMBytes.find(".") + 3) + " MiB";
            log(info, "resulting file size: " + strMBytes);
        }

        return ok;
    }

    return false;
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
    if (!downscaleAlbumArt)
        lame_set_write_id3tag_automatic(encoder, 0);
}

bool FLACtoMP3::initializeOutput() {
    if (outputInitilized)
        throw 5;

    output = fopen(outPath.c_str(), "w+b");
    if (output == 0) {
        output = nullptr;
        log(fatal, "Error opening file " + outPath);
        return false;
    }

    int ret = lame_init_params(encoder);
    if (ret < 0) {
        log(fatal, "Error initializing LAME parameters. Code = " + std::to_string(ret));
        fclose(output);
        output = nullptr;
        return false;
    }

    if (flacMaxBlockSize == 0)
        flacMaxBlockSize = flacDefaultMaxBlockSize;

    pcmSize = lame_get_num_channels(encoder) * flacMaxBlockSize * bufferMultiplier;
    outputBufferSize = pcmSize / 2;

    if (!downscaleAlbumArt) {
        int tag2Size = lame_get_id3v2_tag(encoder, nullptr, 0);
        int vbrTagSize = lame_get_lametag_frame(encoder, nullptr, 0);

        fseek(output, tag2Size + vbrTagSize, SEEK_SET);  //reserve place for tags
        if (tag2Size > outputBufferSize)
            outputBufferSize = tag2Size;
    }

    pcm = new int16_t[pcmSize];
    outputBuffer = new uint8_t[outputBufferSize];

    outputInitilized = true;

    return true;
}

void FLACtoMP3::processInfo(const FLAC__StreamMetadata_StreamInfo& info) {
    lame_set_in_samplerate(encoder, info.sample_rate);
    lame_set_num_channels(encoder, info.channels);
    flacMaxBlockSize = info.max_blocksize;
    log(Loggable::info, "sample rate: " + std::to_string(info.sample_rate));
    log(Loggable::info, "channels: " + std::to_string(info.channels));
    log(Loggable::info, "bits per sample: " + std::to_string(info.bits_per_sample));
}

void FLACtoMP3::processTags(const FLAC__StreamMetadata_VorbisComment& tags) {
    bool artistSet = false;                                                         //in vorbis comment there could be more then 1 artist tag
    for (FLAC__uint32 i = 0; i < tags.num_comments; ++i) {                          //usualy it's about guested and fetured artists
        const FLAC__StreamMetadata_VorbisComment_Entry& entry = tags.comments[i];   //gonna keep only the first one for now
        std::string_view comm((const char*)entry.entry);
        if (comm.find(title) == 0) {
            setTagTitle(comm.substr(title.size()));
        } else if (comm.find(artist) == 0) {
            if (!artistSet) {
                setTagArtist(comm.substr(artist.size()));
                artistSet = true;
            } else {
                log(minor, "more than one artist tag, ignoring " + std::string(comm.substr(artist.size())));
            }
        } else if (comm.find(album) == 0) {
            setTagAlbum(comm.substr(album.size()));
        } else if (comm.find(comment) == 0) {
            id3tag_set_comment(encoder, comm.substr(comment.size()).data());
        } else if (comm.find(genre) == 0) {
            id3tag_set_genre(encoder, comm.substr(genre.size()).data());
        } else if (comm.find(track) == 0) {
            id3tag_set_track(encoder, comm.substr(track.size()).data());
        } else if (comm.find(date) == 0) {
            std::string_view fullDate = comm.substr(date.size());
            if (fullDate.size() == 10) {                                            //1999-11-11    kind of string
                std::string_view month = fullDate.substr(5, std::size_t(2));
                std::string_view day = fullDate.substr(8);
                std::string md = "TDAT=" + std::string(day) + std::string(month);
                int res = id3tag_set_fieldvalue(encoder, md.c_str());
                if (res != 0)
                    log(warning, "wasn't able to set the date tag (" + md + ")");

                fullDate = fullDate.substr(0, 4);   //year;
            }
            id3tag_set_year(encoder, std::string(fullDate).data());
        } else if (!tryKnownTag(comm)) {
            std::string tag = "TXXX=" + std::string(comm);
            int res = id3tag_set_fieldvalue(encoder, tag.c_str());
            if (res != 0)
                log(warning, "wasn't able to set user tag (" + tag + ")");
        }
    }
}

void FLACtoMP3::processPicture(const FLAC__StreamMetadata_Picture& picture) {
    if (downscaleAlbumArt && picture.data_length > LAME_MAXALBUMART) {
        log(info, "embeded album art is too big (" + std::to_string(picture.data_length) + " bytes), rescaling");
        log(debug, "mime type is " + std::string(picture.mime_type));
        if (picture.mime_type == jpeg) {
            if (scaleJPEG(picture))
                log(debug, "successfully rescaled album art");
            else
                log(warning, "failed to rescale album art");
        }
    } else {
        int result = id3tag_set_albumart(encoder, (const char*)picture.data, picture.data_length);
        if (result != 0)
            log(warning, "couldn't set album art tag, errcode: " + std::to_string(result));
    }
}

bool FLACtoMP3::scaleJPEG(const FLAC__StreamMetadata_Picture& picture) {
    struct jpeg_decompress_struct dinfo;
    struct jpeg_error_mgr derr;

    dinfo.err = jpeg_std_error(&derr);
    jpeg_create_decompress(&dinfo);
    jpeg_mem_src(&dinfo, picture.data, picture.data_length);
    int rc = jpeg_read_header(&dinfo, TRUE);

    if (rc != 1) {
        log(Loggable::error, "error reading jpeg header");
        return false;
    }
    uint64_t mem_size = 0;
    uint8_t *mem = new uint8_t[LAME_MAXALBUMART + 1024 * 4];    //I allocate a little bit more not to corrupt someone else's the memory

    dinfo.scale_num = 2;        //need to tune it, feels like 500 by 500 is a good size
    dinfo.scale_denom = 3;
    jpeg_start_decompress(&dinfo);

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr cerr;
    cinfo.err = jpeg_std_error(&cerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &mem, &mem_size);

    cinfo.image_width = dinfo.output_width;
    cinfo.image_height = dinfo.output_height;
    cinfo.input_components = dinfo.output_components;
    cinfo.in_color_space = dinfo.out_color_space;
    jpeg_set_defaults(&cinfo);

    jpeg_start_compress(&cinfo, TRUE);

    uint32_t rowSize = dinfo.image_width * dinfo.output_components;
    uint8_t* row = new uint8_t[rowSize];

    while (dinfo.output_scanline < dinfo.output_height) {
        jpeg_read_scanlines(&dinfo, &row, 1);
        jpeg_write_scanlines(&cinfo, &row, 1);

        if (mem_size > LAME_MAXALBUMART) {
            delete[] mem;
            log(Loggable::error, "resized album art exceeded reserved buffer size, stopping before memory corrution");
            return false;
        }
    }
    jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    delete[] row;

    int result = id3tag_set_albumart(encoder, (const char*)mem, mem_size);
    delete[] mem;

    return result == 0;
}

bool FLACtoMP3::tryKnownTag(std::string_view source) {
    for (const std::pair<const std::string, const std::string>& pair : knownTags) {
        if (source.find(pair.first) == 0) {
            if (pair.first == "LYRICS=") {
                log(warning, "lyrics tag was not transfered, it is not supported currently");
            } else if (pair.first == "BPM=") {
                float bpm = std::atof(source.substr(pair.first.size()).data());
                bpm = std::round(bpm);                                          //id3 bpm must be integer
                int result = id3tag_set_fieldvalue(encoder, std::string(pair.second + std::to_string((int)bpm)).c_str());
                if (result != 0)
                    log(warning, "wasn't able to set BPM tag. Code = " + std::to_string(result));
            } else if (pair.first == "ALBUMARTIST=") {
                int res = setTagUSC2("TPE2", source.substr(pair.first.size()));
                if (res != 0)
                    log(warning, "wasn't able to album artist tag. Code = " + std::to_string(res));
            } else {
                std::string tag = pair.second + std::string(source.substr(pair.first.size()));
                int res = id3tag_set_fieldvalue(encoder, tag.c_str());
                if (res != 0)
                    log(warning, "wasn't able to set tag (" + std::string(source) + "). Code = " + std::to_string(res));
            }

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
        pcm[pcmCounter++] = (int16_t)buffer[0][i];
        pcm[pcmCounter++] = (int16_t)buffer[1][i];

        if (pcmCounter == pcmSize)
            return flush();
    }

    return true;
}

bool FLACtoMP3::flush() {
    int nwrite = lame_encode_buffer_interleaved(
        encoder,
        pcm,
        pcmCounter / 2,
        outputBuffer,
        outputBufferSize
    );
    while (nwrite == -1) {      //-1 is returned when there was not enough space in the given buffer
        log(major, std::to_string(outputBufferSize) + " bytes in the output buffer wasn't enough");;
        outputBufferSize = outputBufferSize * 2;
        delete[] outputBuffer;
        outputBuffer = new uint8_t[outputBufferSize];
        log(major, "allocating " + std::to_string(outputBufferSize) + " bytes");

        nwrite = lame_encode_buffer_interleaved(
            encoder,
            pcm,
            pcmCounter,
            outputBuffer,
            outputBufferSize
        );
    }

    if (nwrite > 0) {
        int actuallyWritten = fwrite((char*)outputBuffer, nwrite, 1, output);
        pcmCounter = 0;
        return actuallyWritten == 1;
    } else {
        if (nwrite == 0) {
            log(minor, "encoding flush encoded 0 bytes, skipping write");
            return true;
        } else {
            log(fatal, "encoding flush failed. Code = : " + std::to_string(nwrite));
            return false;
        }
    }
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
        case FLAC__METADATA_TYPE_PICTURE:
            self->processPicture(metadata->data.picture);
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
    // if (decoded->channels != 2 || decoded->depth != 16) {
    //     std::cout << "ERROR: this example only supports 16bit stereo streams" << std::endl;
    //     return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    // }
    FLACtoMP3* self = static_cast<FLACtoMP3*>(client_data);
    if (frame->header.channels != 2) {
        self->log(fatal, "ERROR: This frame contains " + std::to_string(frame->header.channels) + " channels (should be 2)");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if (buffer[0] == NULL) {
        self->log(fatal, "ERROR: buffer [0] is NULL");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if (buffer[1] == NULL) {
        self->log(fatal, "ERROR: buffer [1] is NULL");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    bool result = self->decodeFrame(buffer, frame->header.blocksize);

    if (result)
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    else
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

void FLACtoMP3::error(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data) {
    (void)decoder;
    FLACtoMP3* self = static_cast<FLACtoMP3*>(client_data);
    std::string errText(FLAC__StreamDecoderErrorStatusString[status]);
    self->log(Loggable::error, "Got error callback: " + errText);
}

void FLACtoMP3::setTagTitle(const std::string_view& title) {
    id3tag_set_title(encoder, title.data());                //to set at least some possibly encoding broken id3v1 tag
    int res = setTagUSC2("TIT2", title);
    if (res != 0)
        log(warning, "Couldn't set Title tag. Code = " + std::to_string(res));
}

void FLACtoMP3::setTagAlbum(const std::string_view& album) {
    id3tag_set_album(encoder, album.data());                //to set at least some possibly encoding broken id3v1 tag
    int res = setTagUSC2("TALB", album);
    if (res != 0)
        log(warning, "Couldn't set Album tag. Code = " + std::to_string(res));
}

void FLACtoMP3::setTagArtist(const std::string_view& artist) {
    id3tag_set_artist(encoder, artist.data());                //to set at least some possibly encoding broken id3v1 tag
    int res = setTagUSC2("TPE1", artist);
    if (res != 0)
        log(warning, "Couldn't set Artist tag. Code = " + std::to_string(res));
}

int FLACtoMP3::setTagUSC2(const std::string_view& field, const std::string_view& value) {
    std::u16string utf16 = std::u16string({0xFEFF}) + usc2convertor.from_bytes(value.data());
    return id3tag_set_textinfo_utf16(encoder, field.data(), (unsigned short*)utf16.c_str());
}

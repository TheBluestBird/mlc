#include "flactomp3.h"

constexpr uint16_t flacDefaultMaxBlockSize = 4096;

static const std::string_view title ("TITLE=");
static const std::string_view artist ("ARTIST=");
static const std::string_view album ("ALBUM=");
static const std::string_view comment ("COMMENT=");
static const std::string_view genre ("GENRE=");
static const std::string_view track ("TRACKNUMBER=");
static const std::string_view date ("DATE=");

static const std::string_view jpeg ("image/jpeg");

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
    bufferMultiplier(size),
    flacMaxBlockSize(0),
    pcmCounter(0),
    pcmSize(0),
    pcm(nullptr),
    outputBuffer(nullptr),
    outputBufferSize(0),
    outputInitilized(false),
    downscaleAlbumArt(false)
{
}

FLACtoMP3::~FLACtoMP3() {
    lame_close(encoder);
    FLAC__stream_decoder_delete(decoder);
}

void FLACtoMP3::run() {
    FLAC__bool ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
    std::cout << "decoding: ";
    if (ok) {
        std::cout << "succeeded";

        flush();
        int nwrite = lame_encode_flush(encoder, outputBuffer, pcmSize * 2);
        fwrite((char*)outputBuffer, nwrite, 1, output);

        if (downscaleAlbumArt) {
            lame_mp3_tags_fid(encoder, output);
        } else {
            int tag1Size = lame_get_id3v1_tag(encoder, outputBuffer, 128);
            if (tag1Size > 128)
                std::cout << std::endl << "couldn't write id3v1 tag";
            else
                fwrite((char*)outputBuffer, tag1Size, 1, output);

            fseek(output, 0, SEEK_SET);
            int tag2Size = lame_get_id3v2_tag(encoder, outputBuffer, outputBufferSize);
            if (tag2Size > outputBufferSize) {
                std::cout << std::endl << "couldn't write id3v1 tag";
            } else
                fwrite((char*)outputBuffer, tag2Size, 1, output);

            int vbrTagSize = lame_get_lametag_frame(encoder, outputBuffer, outputBufferSize);
            if (vbrTagSize > outputBufferSize)
                std::cout << std::endl << "couldn't write vbr tag";

            fwrite((char*)outputBuffer, vbrTagSize, 1, output);
        }
    } else {
        std::cout << "FAILED";
    }

    std::cout << std::endl;
    std::cout << "   state: " << FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)] << std::endl;

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
    if (!downscaleAlbumArt)
        lame_set_write_id3tag_automatic(encoder, 0);
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
    //std::cout << "bits per sample: " << info.bits_per_sample << std::endl;;
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

void FLACtoMP3::processPicture(const FLAC__StreamMetadata_Picture& picture) {
    if (downscaleAlbumArt && picture.data_length > LAME_MAXALBUMART) {
        std::cout << "embeded picture is too big (" << picture.data_length << " bytes) " << std::endl;
        std::cout << "mime type is " << picture.mime_type << std::endl;
        if (picture.mime_type == jpeg) {
            if (scaleJPEG(picture)) {
                std::cout << "successfully scaled album art" << std::endl;
            } else {
                std::cout << "failed to album art" << std::endl;
            }
        }
    } else {
        int result = id3tag_set_albumart(encoder, (const char*)picture.data, picture.data_length);
        if (result != 0) {
            std::cout << "couldn't set album art tag, errcode: " << result << std::endl;
        }
    }
}

bool FLACtoMP3::scaleJPEG(const FLAC__StreamMetadata_Picture& picture) {
    // Variables for the decompressor itself
    struct jpeg_decompress_struct dinfo;
    struct jpeg_error_mgr derr;

    dinfo.err = jpeg_std_error(&derr);
    jpeg_create_decompress(&dinfo);
    jpeg_mem_src(&dinfo, picture.data, picture.data_length);
    int rc = jpeg_read_header(&dinfo, TRUE);

    if (rc != 1) {
        std::cout << "error reading jpeg header" << std::endl;
        return false;
    }
    uint64_t mem_size = LAME_MAXALBUMART - 1024 * 16;
    uint8_t *mem = new uint8_t[mem_size];

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
	}
	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

	// And free the input buffer
	delete[] row;

    std::cout << "writing " << mem_size << std::endl;
    int result = id3tag_set_albumart(encoder, (const char*)mem, mem_size);

    std::cout << "deallocating" << std::endl;
    delete[] mem;

    return result == 0;
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
        std::cout << outputBufferSize << " bytes in the output buffer wasn't enough" << std::endl;
        outputBufferSize = outputBufferSize * 2;
        delete[] outputBuffer;
        outputBuffer = new uint8_t[outputBufferSize];
        std::cout << "allocating " << outputBufferSize << " bytes" << std::endl;

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
            std::cout << "encoding flush encoded 0 bytes, skipping write" << std::endl;
            return true;
        } else {
            std::cout << "encoding flush failed, error: " << nwrite << std::endl;
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

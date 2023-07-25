#include "flactomp3.h"

#include <cmath>

#include <tpropertymap.h>
#include <attachedpictureframe.h>

constexpr uint16_t flacDefaultMaxBlockSize = 4096;

constexpr std::string_view jpeg ("image/jpeg");

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
    id3v2tag()
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


        fileSize = ftell(output);
        lame_mp3_tags_fid(encoder, output);
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

    TagLib::ByteVector vector = id3v2tag.render();
    fwrite((const char*)vector.data(), vector.size(), 1, output);

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
    TagLib::PropertyMap props;
    for (FLAC__uint32 i = 0; i < tags.num_comments; ++i) {
        const FLAC__StreamMetadata_VorbisComment_Entry& entry = tags.comments[i];
        std::string_view comm((const char*)entry.entry);
        std::string_view::size_type ePos = comm.find("=");
        if (ePos == std::string_view::npos) {
            log(warning, "couldn't understand tag (" + std::string(comm) + "), symbol '=' is missing, skipping");
            continue;
        }
        std::string key(comm.substr(0, ePos));
        std::string value(comm.substr(ePos + 1));

        if (key == "BPM") {                                     //somehow TagLib lets BPM be fractured
            std::string::size_type dotPos = value.find(".");    //but IDv2.3.0 spec requires it to be integer
            if (dotPos != std::string::npos)                    //I don't know better than just to floor it
                value = value.substr(0, dotPos);
        }

        bool success = props.insert(key, TagLib::String(value, TagLib::String::UTF8));

        if (!success)
            log(warning, "couldn't understand tag (" + key + "), skipping");
    }
    id3v2tag.setProperties(props);
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
        //auch, sorry for copying so much, but I haven't found a way around it yet
        TagLib::ByteVector bytes((const char*)picture.data, picture.data_length);
        attachPictureFrame(picture, bytes);
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
    TagLib::ByteVector vector (picture.data_length + 1024 * 4); //I allocate a little bit more not to corrupt someone else's the memory
    uint64_t mem_size = 0;
    uint8_t* data = (uint8_t*)vector.data();

    dinfo.scale_num = 2;        //need to tune it, feels like 500 by 500 is a good size
    dinfo.scale_denom = 3;
    jpeg_start_decompress(&dinfo);

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr cerr;
    cinfo.err = jpeg_std_error(&cerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &data, &mem_size);

    cinfo.image_width = dinfo.output_width;
    cinfo.image_height = dinfo.output_height;
    cinfo.input_components = dinfo.output_components;
    cinfo.in_color_space = dinfo.out_color_space;
    jpeg_set_defaults(&cinfo);

    jpeg_start_compress(&cinfo, TRUE);

    uint32_t rowSize = dinfo.image_width * dinfo.output_components;
    uint8_t* row = new uint8_t[rowSize];

    while (dinfo.output_scanline < dinfo.output_height) {
        if (mem_size + rowSize > vector.size()) {
            vector.resize(vector.size() + rowSize);
            log(Loggable::major, "allocated memory for resising the image wasn't enougth, resising");
        }
        jpeg_read_scanlines(&dinfo, &row, 1);
        jpeg_write_scanlines(&cinfo, &row, 1);
    }
    jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    delete[] row;

    vector.resize(mem_size);    //to shrink if down to the actual size
    attachPictureFrame(picture, vector);

    return true;
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

void FLACtoMP3::attachPictureFrame(const FLAC__StreamMetadata_Picture& picture, const TagLib::ByteVector& bytes) {
    TagLib::ID3v2::AttachedPictureFrame* frame = new TagLib::ID3v2::AttachedPictureFrame();
    frame->setPicture(bytes);
    frame->setType(TagLib::ID3v2::AttachedPictureFrame::Media);
    frame->setMimeType(picture.mime_type);
    frame->setDescription(TagLib::String((const char*)picture.description, TagLib::String::UTF8));
    switch (picture.type) {
        case FLAC__STREAM_METADATA_PICTURE_TYPE_OTHER:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::Other);
            log(info, "attached picture is described as \"other\"");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON_STANDARD:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::FileIcon);
            log(info, "attached picture is a standard file icon");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::OtherFileIcon);
            log(info, "attached picture is apparently not so standard file icon...");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
            log(info, "attached picture is a front album cover");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_BACK_COVER:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::BackCover);
            log(info, "attached picture is an back album cover");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_LEAFLET_PAGE:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::LeafletPage);
            log(info, "attached picture is a leflet from the album");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_MEDIA:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::Media);
            log(info, "attached picture is probably an imadge of an album CD");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_LEAD_ARTIST:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::LeadArtist);
            log(info, "attached picture is an image of the lead artist");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_ARTIST:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::Artist);
            log(info, "attached picture is an image the artist");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_CONDUCTOR:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::Conductor);
            log(info, "attached picture is an image the conductor");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_BAND:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::Band);
            log(info, "attached picture is an image of the band");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_COMPOSER:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::Composer);
            log(info, "attached picture is an image of composer");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_LYRICIST:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::Lyricist);
            log(info, "attached picture is an image of the lyricist");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_RECORDING_LOCATION:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::RecordingLocation);
            log(info, "attached picture is an image recording location");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_DURING_RECORDING:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::DuringRecording);
            log(info, "attached picture is an image of the process of the recording");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_DURING_PERFORMANCE:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::DuringPerformance);
            log(info, "attached picture is an image of process of the performance");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_VIDEO_SCREEN_CAPTURE:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::MovieScreenCapture);
            log(info, "attached picture is an frame from a movie");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_FISH:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::ColouredFish);
            log(info, "attached picture is ... a bright large colo(u?)red fish...? o_O");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_ILLUSTRATION:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::Illustration);
            log(info, "attached picture is an track related illustration");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_BAND_LOGOTYPE:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::BandLogo);
            log(info, "attached picture is a band logo");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_PUBLISHER_LOGOTYPE:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::PublisherLogo);
            log(info, "attached picture is a publisher logo");
            break;
        case FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED:
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::Other);
            log(info, "attached picture is something unknown, so, I would assume it's \"other\"");
            break;
    }
    log(info, "attached picture size: " + std::to_string(frame->picture().size()));
    std::string description = frame->description().to8Bit();
    if (description.size() > 0)
        log(info, "attached picture has a description (b'cuz where else would you ever read it?): " + description);
    id3v2tag.addFrame(frame);
}


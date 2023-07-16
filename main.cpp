#include <iostream>
#include <string>

#include "FLAC/stream_decoder.h"
#include <lame/lame.h>

#include "help.h"
#include "decoded.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Insufficient amount of arguments, launch with \"--help\" argument to see usage" << std::endl;
        return 1;
    }

    const std::string firstArgument(argv[1]);

    if (firstArgument == "--help") {
        printHelp();
        return 0;
    }

    Decoded* decoded = Decoded::fromFLACFile(firstArgument);
    decoded->saveWaveToFile("./out.wav");
    // FILE *mp3 = fopen("output.mp3", "wb");
    // size_t nread;
    // int ret, nwrite;
    //
    // // 1. Get lame version (OPTIONAL)
    // std::cout << "Using LAME v" << get_lame_version() << std::endl;
    //
    // const int PCM_SIZE = 8192;
    // const int MP3_SIZE = 8192;
    //
    // short pcm_buffer[PCM_SIZE * 2];
    // unsigned char mp3_buffer[MP3_SIZE];
    //
    // // 2. Initializing
    // lame_t lame = lame_init();
    //
    // // 3. Do some settings (OPTIONAL)
    // // lame_set_in_samplerate(lame, 48000);
    // lame_set_VBR(lame, vbr_default);
    // // lame_set_VBR_quality(lame, 2);
    //
    // // 4. Initialize parameters
    // ret = lame_init_params(lame);
    // if (ret < 0) {
    //     std::cout << "Error occurred during parameters initializing. Code = " << ret << std::endl;
    //     return 3;
    // }
    //
    // do {
    //     // Read PCM_SIZE of array
    //     nread = fread(pcm_buffer, 2 * sizeof(short), PCM_SIZE, pcm);
    //     if (nread != 0) {
    //         // 5. Encode
    //         int nsamples = nread;
    //         short buffer_l[nsamples];
    //         short buffer_r[nsamples];
    //
    //         printf("nread = %d\n", nread);
    //         printf("pcm_buffer.length = %d\n", sizeof(pcm_buffer)/sizeof(short));
    //
    //         int j = 0;
    //         int i = 0;
    //         for (i = 0; i < nsamples; i++) {
    //             buffer_l[i] = pcm_buffer[j++];
    //             buffer_r[i] = pcm_buffer[j++];
    //
    //         }
    //
    //         nwrite = lame_encode_buffer(lame, buffer_l, buffer_r, nread,
    //                 mp3_buffer, MP3_SIZE);
    //     } else {
    //         // 6. Flush and give some final frames
    //         nwrite = lame_encode_flush(lame, mp3_buffer, MP3_SIZE);
    //     }
    //
    //     if (nwrite < 0) {
    //         printf("Error occurred during encoding. Code = %d\n", nwrite);
    //         return 1;
    //     }
    //
    //     fwrite(mp3_buffer, nwrite, 1, mp3);
    // } while (nread != 0);
    //
    // // 7. Write INFO tag (OPTIONAL)
    // // lame_mp3_tags_fid(lame, mp3);
    //
    // // 8. Free internal data structures
    // lame_close(lame);
    //
    // fclose(mp3);



    delete decoded;

    return 0;
}

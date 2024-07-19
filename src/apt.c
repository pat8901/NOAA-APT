// https://www.sigidwiki.com/wiki/Automatic_Picture_Transmission_(APT)
// https://en.wikipedia.org/wiki/Automatic_picture_transmission
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sndfile.h>
#include <linux/limits.h>
#include "apt.h"
#include "algebra.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    SF_INFO sfinfo;
    SF_INFO *ptr = &sfinfo;
    SNDFILE *sndfile;

    get_path();
    sfinfo.format = 0;
    const char *file_path = "./documentation/test_audio/20210720111842.wav";

    // Opening audio file.
    sndfile = sf_open(file_path, SFM_READ, &sfinfo);
    if (!sndfile)
    {
        printf("Failed to open file: %s\n", sf_strerror(NULL));
        return -1;
    }

    printf("Frame amount: %ld\n", ptr->frames);
    printf("Sample rate: %d Hz\n", ptr->samplerate);
    printf("Format: %d\n", ptr->format);
    printf("Channels: %d\n\n", sfinfo.channels);

    seek(sndfile, ptr);

    sf_close(sndfile);

    return 0;
}

// TODO:
int seek(SNDFILE *sndfile, SF_INFO *sfinfo)
{
    sf_count_t frames = sfinfo->frames;
    sf_count_t count = 0;
    sf_count_t high_frames_amount = 11025;
    sf_count_t low_frame_amount = 4160;
    int sample_rate = sfinfo->samplerate;
    float seek_rate = (float)sample_rate / 4160.0;
    float *buffer_11025 = (float *)malloc(high_frames_amount * sizeof(float));

    // init write output file for 4160Hz downsample
    SF_INFO sfinfo_4160;
    SNDFILE *sndfile_4160;
    sfinfo_4160.samplerate = 4160;
    sfinfo_4160.channels = 1;
    sfinfo_4160.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    const char *file_path_4160 = "./documentation/output/test.wav";
    sndfile_4160 = sf_open(file_path_4160, SFM_WRITE, &sfinfo_4160);
    if (!sndfile_4160)
    {
        printf("Failed to open file: %s\n", sf_strerror(NULL));
        return -1;
    }

    // Downsample from 11025Hz to 4160Hz using Linear Interpolation.
    while (count < frames)
    {

        printf("count: %d\n", count);
        sf_count_t start_frame = sf_seek(sndfile, count, SEEK_SET);
        sf_count_t frames_requested = sf_readf_float(sndfile, buffer_11025, high_frames_amount);

        // indicator that the last chunk is less than required amount of frames to create 2 lines of an APT image.
        // The program has either neared the end of the audio file or the file did not have enough information to begin with.
        if (frames_requested != 11025)
        {
            printf("Remaining chuck-size: %d\n", frames_requested);
        }

        // This is the dynamic length of our down-sampled audio buffer. This is necessary in order to get every last drop of data
        // from our wav audio file.
        int downsample_length = (int)(frames_requested / seek_rate);
        printf("Downsample length: %d\n", downsample_length);
        // Look into the mechanics in how this works more with indexing and overflowing
        float *buffer_4160 = (float *)malloc(downsample_length * sizeof(float));

        for (int i = 0; i < downsample_length; i++)
        {
            // x value
            float input_index = (float)i * seek_rate;
            //  x_0, x_1 values
            sf_count_t position_1 = (sf_count_t)input_index;
            sf_count_t position_2 = position_1 + 1;

            //  y_0, y_1 values
            float sample_1 = buffer_11025[position_1];
            float sample_2 = buffer_11025[position_2];
            // not used - will get rid of later.
            float mu = input_index - position_1;

            float result = linear_interpolate(position_1, position_2, input_index, sample_1, sample_2, mu);
            // printf("================================\n");
            // printf("4160Hz index: %d\n", i);
            // printf("11025Hz indexes: %d(%f), %d(%f)\n", position_1, sample_1, position_2, sample_2);
            // printf("interpolated value: %f\n", result);
            buffer_4160[i] = result;
        }
        // print_buffer_4160(buffer_4160);

        sf_count_t frames_written = sf_writef_float(sndfile_4160, buffer_4160, downsample_length);
        printf("Frames written %d\n", frames_written);
        count = count + frames_requested;
        free(buffer_4160);
    }
    printf("=================\n");
    printf("Finished!\n");
    printf("Count: %d\n", count);
    free(buffer_11025);
    sf_close(sndfile_4160);
    return 0;
}

void print_buffer_4160(float *buffer)
{
    for (int i = 0; i < 4160; i++)
    {
        printf("index %d: %f\n", i, buffer[i]);
    }
}

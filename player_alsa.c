// minimp3 example player application for Linux/ALSA
// this file is public domain -- do with it whatever you want!
#include "minimp3.h"

#include <sys/mman.h>
#include <alsa/asoundlib.h>

#define BUFF_SIZE 4096

size_t strlen(const char *s);
#define out(text) write(1, (const void *) text, strlen(text))

int main(int argc, char *argv[]) {
    mp3_decoder_t mp3;
    mp3_info_t info;
    int fd, pcm;
    void *file_data;
    unsigned char *stream_pos;
    signed short sample_buf[MP3_MAX_SAMPLES_PER_FRAME];
    int bytes_left;
    int frame_size;
    int value;

    out("minimp3 -- a small MPEG-1 Audio Layer III player based on ffmpeg\n\n");
    if (argc < 2) {
        out("Error: no input file specified!\n");
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        out("Error: cannot open `");
        out(argv[1]);
        out("'!\n");
        return 1;
    }
    
    bytes_left = lseek(fd, 0, SEEK_END);    
    file_data = mmap(0, bytes_left, PROT_READ, MAP_PRIVATE, fd, 0);
    stream_pos = (unsigned char *) file_data;
    bytes_left -= 100;
    out("Now Playing: ");
    out(argv[1]);

    mp3 = mp3_create();
    frame_size = mp3_decode(mp3, stream_pos, bytes_left, sample_buf, &info);
    if (!frame_size) {
        out("\nError: not a valid MP3 audio file!\n");
        return 1;
    }
    
    #define FAIL(msg) { \
        out("\nError: " msg "\n"); \
        return 1; \
    }   

	int err;
  	short buf[BUFF_SIZE];
  	int rate = 44100; /* Sample rate */
  	unsigned int exact_rate; /* Sample rate returned by */
  	snd_pcm_t *playback_handle;
  	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

  	snd_pcm_hw_params_t *hw_params;
 
	static char *device = "default"; /* playback device */

	if ((err = snd_pcm_open (&playback_handle, device, stream, 0)) < 0) {
    	fprintf (stderr, "cannot open audio device (%s)\n", snd_strerror (err));
    	exit (1);
	}

	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
	    fprintf (stderr, "cannot allocate hardware parameters (%s)\n", snd_strerror (err));
	    exit(1);
	}

	if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
	    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
	    exit (1);
	}

	/* Set access type. */
	if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    	fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err));
    	exit (1);
	}
 
	/* Set sample format */
	if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
    	fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
    	exit (1);
	}
 
	/* Set sample rate. If the exact rate is not supported by the hardware, use nearest possible rate. */
	exact_rate = rate;
	if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &info.sample_rate, 0)) < 0) {
    	fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
    	exit (1);
	}
 
	if (rate != exact_rate) {
    	fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n ==> Using %d Hz instead.\n", rate, exact_rate);
	}
 
	/* Set number of channels */
	if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, info.channels)) < 0) {
    	fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
    	exit (1);
	}

	/* Apply HW parameter settings to PCM device and prepare device. */
	if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
    	fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
   		exit (1);
	}
 
	snd_pcm_hw_params_free (hw_params);
 
	if ((err = snd_pcm_prepare (playback_handle)) < 0) {
    	fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
    	exit (1);
	}


	/* Write output data from mp3 decoder to audio device. */
	out("\n\nPress Ctrl+C to stop playback.\n");

    while ((bytes_left >= 0) && (frame_size > 0)) {
        stream_pos += frame_size;
        bytes_left -= frame_size;
		snd_pcm_writei(playback_handle, sample_buf, sizeof(sample_buf)/4);
        //write(pcm, (const void *) sample_buf, info.audio_bytes);
        frame_size = mp3_decode(mp3, stream_pos, bytes_left, sample_buf, NULL);
    }

	/* Close the device. */
	snd_pcm_close (playback_handle);

    return 0;
}

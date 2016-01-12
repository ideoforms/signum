#include "soundio.h"

#ifdef HAVE_SOUNDIO

#include "../../graph.h"

#include "../output/soundio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>

namespace signum
{

extern AudioIn_Abstract *shared_in;
    
void read_callback(struct SoundIoInStream *instream,
		int frame_count_min, int frame_count_max)
{
	const struct SoundIoChannelLayout *layout = &instream->layout;
	struct SoundIoChannelArea *areas;
	int frame_count = frame_count_max;
	int frames_left = frame_count_max;
    
    AudioIn_SoundIO *input = (AudioIn_SoundIO *) shared_in;

	/*-----------------------------------------------------------------------*
	 * On some drivers (eg Linux), we cannot write all samples at once.
	 * Keep reading as many as we can until we have cleared the buffer.
	 *-----------------------------------------------------------------------*/
	while (frames_left > 0)
	{
		int err;

		if ((err = soundio_instream_begin_read(instream, &areas, &frame_count)))
			throw std::runtime_error("libsoundio error on begin read: " + std::string(soundio_strerror(err)));

		if (!input)
			throw std::runtime_error("libsoundio error: No global input created");

		for (int frame = 0; frame < frame_count; frame++)
		{
			for (int channel = 0; channel < layout->channel_count; channel += 1)
			{
				float *ptr = (float *)(areas[channel].ptr + areas[channel].step * frame);
				input->buffer->data[channel][input->write_pos] = *ptr;
			}
			input->write_pos = (input->write_pos + 1) % input->buffer->num_frames;
		}

		if ((err = soundio_instream_end_read(instream)))
			throw std::runtime_error("libsoundio error on end read: " + std::string(soundio_strerror(err)));

		frames_left -= frame_count;
	}
}

AudioIn_SoundIO::AudioIn_SoundIO() : AudioIn_Abstract()
{
	// Allocate enough buffer for twice our block size, else
	// we risk overwriting our input buffer from the audio in
	// while it is still being read from.
	// TODO: Bad hardcoded block size
	this->buffer = new Buffer(2, 512 * 2);
	this->read_pos = 0;
	this->write_pos = 512;
	this->init();
}

int AudioIn_SoundIO::init()
{
	int err;

	this->soundio = ((AudioOut_SoundIO *) this->graph->output.get())->soundio;

	if (!this->soundio)
		throw std::runtime_error("libsoundio init error: No output unit found in graph (initialising input before output?)");

	int default_in_device_index = soundio_default_input_device_index(this->soundio);
	if (default_in_device_index < 0)
		throw std::runtime_error("libsoundio init error: no input devices found.");

	this->device = soundio_get_input_device(this->soundio, default_in_device_index);
	if (!device)
		throw std::runtime_error("libsoundio init error: out of memory.");

	this->instream = soundio_instream_create(device);
	this->instream->format = SoundIoFormatFloat32NE;
	this->instream->read_callback = read_callback;
	// this->outstream->software_latency = 512 / 44100.0;
	this->instream->sample_rate = 44100.0;

	if ((err = soundio_instream_open(this->instream)))
		throw std::runtime_error("libsoundio init error: unable to open device: " + std::string(soundio_strerror(err)));

	if (this->instream->layout_error)
		throw std::runtime_error("libsoundio init error: unable to set channel layout: " +
				std::string(soundio_strerror(this->instream->layout_error)));

	if ((err = soundio_instream_start(instream)))
		throw std::runtime_error("libsoundio init error: unable to start device: " + std::string(soundio_strerror(err)));

	return 0;
}
    
int AudioIn_SoundIO::start()
{
    return 0;
}

int AudioIn_SoundIO::close()
{
	soundio_instream_destroy(this->instream);

	return 0;
}

void AudioIn_SoundIO::next(sample **out, int num_frames)
{
	// don't have to do anything as our output is written by the read_callback
	for (int frame = 0; frame < num_frames; frame++)
	{
		for (int channel = 0; channel < channels_out; channel++)
		{
			out[channel][frame] = this->buffer->data[channel][read_pos];
		}
		read_pos = (read_pos + 1) % this->buffer->num_frames;
	}
}

}


#endif
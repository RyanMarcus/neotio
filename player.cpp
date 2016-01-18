
#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>


float* data;
int pos;

static int neotioCallback(const void* inputBuffer,
			  void* outputBuffer,
			  unsigned long framesPerBuffer,
			  const PaStreamCallbackTimeInfo* timeInfo,
			  PaStreamCallbackFlags statusFlags,
			  void* userData) {

	//printf("Frames per buffer: %lu\n", framesPerBuffer);
	
	float* out = (float*) outputBuffer;
	for (int i = 0; i < framesPerBuffer*2; i += 2) {
		// set both the left and right channel 
		out[i] = data[pos++];
		out[i+1] = data[pos++];
	}

	return 0;
}

int main(int argc, char** argv) {
	PaError err = Pa_Initialize();

	if (err != paNoError) {
		printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		exit(-1);
	}


	// read in a file to play
	FILE *f = fopen("output.raw", "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	data = (float*) malloc((fsize / sizeof(float)));
	fread(data, fsize, 1, f);
	fclose(f);
	printf("File read (%ld bytes)\n", fsize);
	
	// open up a stream
	PaStream *stream;

	err = Pa_OpenDefaultStream( &stream,
				    0,          /* no input channels */
				    2,          /* stereo output */
				    paFloat32,  /* 32 bit floating point output */
				    96000, // the sample rate
				    paFramesPerBufferUnspecified,
				    neotioCallback, 
				    &data ); // data passed to callback

	
	if (err != paNoError) {
		printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		exit(-1);
	}

	// start the stream
	err = Pa_StartStream(stream);

	if (err != paNoError) {
		printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		exit(-1);
	}
	
	// wait a few seconds
	Pa_Sleep(50000); 

	// stop the stream (might want to use Pa_StopStream for buffer flushing)
	err = Pa_AbortStream(stream);

	if (err != paNoError) {
		printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		exit(-1);
	}


	//  close the stream
	err = Pa_CloseStream(stream);

	if (err != paNoError) {
		printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		exit(-1);
	}
	
	// once we are done, close up PortAudio
	err = Pa_Terminate();
	
	if (err != paNoError) {
		printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		exit(-1);
	}
	
	
}

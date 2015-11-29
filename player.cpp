#include <portaudio.h>



static int neotioCallback(const void* inputBuffer,
			  void* outputBuffer,
			  unsigned long framesPerBuffer,
			  const PaStreamCallbackTimeInfo* timeInfo,
			  PaStreamCallbackFlags statusFlags,
			  void* userData) {

	float* out = (float*) outputBuffer;
	for (int i = 0; i < framesPerBuffer*2; i += 2) {
		// set both the left and right channel to 0.5f
		out[i] = 0.5f;
		out[i+1] = 0.5f;
	}

	return 0;
}

int main(int argc, char** argv) {
	paError err = Pa_Initialize();

	if (err != paNoError) {
		printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		exit(-1);
	}



	// open up a stream
	PaStream *stream;


	err = Pa_OpenDefaultStream( &stream,
				    0,          /* no input channels */
				    2,          /* stereo output */
				    paFloat32,  /* 32 bit floating point output */
				    SAMPLE_RATE,
				    paFramesperBufferUnspecified,
				    neotioCallback, 
				    NULL ); // data passed to callback

	
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
	
	// wait a second
	Pa_Sleep(1000); 

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

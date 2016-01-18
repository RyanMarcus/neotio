#include <stdio.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
//#include <ffmpeg/swscale.h>

void printError(int err) {
	char buf[300];
	if (av_strerror(err, buf, 300) < 0) {
		printf("unknown error code: %d\n", err);
	} else {
		printf("error: %s\n", buf);
	}
}


void handleSample(void* sample, size_t size) {
	
}


int main(int argc, char** argv) {

	// register all the needed ffmpeg formats and codecs...
	av_register_all();

	AVFormatContext* pFormatCtx = NULL;

	if (avformat_open_input(&pFormatCtx, "output.wav", NULL, NULL) != 0) {
		printf("error: couldn't open file.\n");
		return -1;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) != 0) {
		printf("error: couldn't find stream information\n");
		return -1;
	}

	// dump debugging info about the stream
	av_dump_format(pFormatCtx, 0, "test.m4a", 0);
	
	// figure out which stream is the "best" stream
	AVCodec* codec;
	int firstAudio = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

	if (firstAudio == AVERROR_STREAM_NOT_FOUND) {
		printf("could not find an audio stream\n");
		return -1;
	} else if (firstAudio == AVERROR_DECODER_NOT_FOUND) {
		printf("found an audio stream, but no decoder\n");
		return -1;
	} else if (firstAudio < 0) {
		printf("error finding audio stream and codec\n");
		return -1;
	}

	AVCodecContext* pCodecCtx = pFormatCtx->streams[firstAudio]->codec;

		
	printf("Using codec: %s for stream %d\n", codec->long_name, firstAudio);


	
	if (pCodecCtx == NULL) {
		printf("could not allocate a codec context\n");
		return -1;
	}
	
	int err;
	if ((err = avcodec_open2(pCodecCtx, codec, NULL)) != 0) {
		printf("could not open codec\n");
		printError(err);
		return -1;
	}





	
	int bytesPerSample = av_get_bytes_per_sample(pCodecCtx->sample_fmt);
	if (bytesPerSample <= 0) {
		printf("could not determine proper number of bytes per sample\n");
		return -1;
	}
	printf("Bytes per sample: %d\n", bytesPerSample);
	printf("Internal format: %s\n", av_get_sample_fmt_name(pCodecCtx->sample_fmt));
	int isPlanar = av_sample_fmt_is_planar(pCodecCtx->sample_fmt);
	printf("Planar: %d\n", isPlanar);
	
	AVFrame *pFrame = av_frame_alloc();
	AVPacket packet;
	av_init_packet(&packet);

	int bufferSize = 20480 + FF_INPUT_BUFFER_PADDING_SIZE;
	uint8_t buffer[bufferSize];
	packet.data = buffer;
	packet.size = bufferSize;


	FILE* out = fopen("output.raw", "wb");
	
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		if (packet.stream_index != firstAudio)
			continue;

		// decode a frame
		int totalRead = 0;
		while (totalRead < packet.size) {
			int gotFrame, read = 0;
			read += avcodec_decode_audio4(pCodecCtx, pFrame, &gotFrame, &packet);

			if (read < 0) {
				printf("error during decoding\n");
				printError(read);
				return -1;
			}
			
			if (gotFrame == 0)
				continue;
			totalRead += read;

			// these loops will iterate over the data in packed order
			// regradless of if the sample was planar
			for (int i = 0; i < pFrame->nb_samples; i++) {
				// if the format is planar, each channel is stored
				// seperately. Otherwise, the data is "packed"
				int planes = (isPlanar ? pCodecCtx->channels : 1);
				for (int ch = 0; ch < planes; ch++) {
					for (int b = 0; b < bytesPerSample; b++) {
						fwrite(pFrame->data[ch] + b, 1, 1, out);
						//printf("%d ", pFrame->data[ch][b]);
					}
					//printf("\n");
					      
				}
				
			}
		}
	}

	fclose(out);





	

	



}

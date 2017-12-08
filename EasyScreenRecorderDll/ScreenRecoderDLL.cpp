// ScreenRecoderDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#define EXTERN __declspec(dllexport)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <Windows.h>
#include <Mmsystem.h>
#define snprintf _snprintf
#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
extern "C"
{
#include "ffmpeg\include\libavutil\avassert.h"
#include "ffmpeg\include\libavutil\channel_layout.h"
#include "ffmpeg\include\libavutil\opt.h"
#include "ffmpeg\include\libavutil\mathematics.h"
#include "ffmpeg\include\libavutil\timestamp.h"
#include "ffmpeg\include\libavformat\avformat.h"
#include "ffmpeg\include\libswscale\swscale.h"
#include "ffmpeg\include\libswresample\swresample.h"
#include "ffmpeg\include\libavutil\frame.h"
#include "ffmpeg\include\libavcodec\avcodec.h"
#include "ffmpeg\include\libavdevice\avdevice.h"

}
#endif

#pragma comment(lib, "ffmpeg/lib/avcodec.lib")
#pragma comment(lib, "ffmpeg/lib/avformat.lib")
#pragma comment(lib, "ffmpeg/lib/avutil.lib")
#pragma comment(lib, "ffmpeg/lib/swscale.lib")
#pragma comment(lib, "ffmpeg/lib/avutil.lib")
#pragma comment(lib, "ffmpeg/lib/swresample.lib")
#pragma comment(lib, "ffmpeg/lib/avdevice.lib")
#pragma comment(lib,"winmm.lib")


HANDLE hEvent_BufferReady;
HANDLE hEvent_FinishedPlaying;

int				_iBuf;
int				_iplaying;
unsigned long	result;

HWAVEIN hWaveIn;
WAVEFORMATEX pFormat;

enum { NUM_BUF = 3 };
WAVEHDR _header[NUM_BUF];

//#define STREAM_DURATION   5.0
int		g_captureStopFlag = 0;  // if >0, stop
int		g_capturePauseFlag = 0;  // if >0, pause

int m_audioinput = 0;//MIC
int m_audiovolume = 10;//10
int m_audiofrequency = 44100;//44100Hz
int m_audiobites = 16;//16-bit
int m_audiotype = 0;//Stereo, Mono
int m_audioformat = 0;//AVI, MP4
int m_audioIS = 0;//
int m_videowith = 1440;//1440
int m_videoheight = 900;//900
int m_videoframerates = 25;//25
int m_videoCodec = 0;//WMV, MP4
#define STREAM_FRAME_RATE 25/* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
#define SCALE_FLAGS		SWS_BICUBIC

//'1' Use Dshow 
//'0' Use GDIgrab
#define USE_DSHOW 0

AVFormatContext	*pFormatCtx;
int				videoindex;
AVCodecContext	*pCodecCtx;
AVCodec			*pCodec;
AVPacket		*packet;
AVFrame			*pFrame;

SwsContext		*img_convert_ctx;

HANDLE hThread;
HANDLE hThreadPlay;
// a wrapper around a single output AVStream
typedef struct OutputStream {
	AVStream *st;
	AVCodecContext *enc;

	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;

	AVFrame *frame;
	AVFrame *tmp_frame;

	float t, tincr, tincr2;

	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
} OutputStream;


int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;

	/* Write the compressed frame to the media file. */
	return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
void add_stream(OutputStream *ost, AVFormatContext *oc,
	AVCodec **codec, enum AVCodecID codec_id)
{
	AVCodecContext *c;
	int i;

	/* find the encoder */
	*codec = avcodec_find_encoder(codec_id);
	if (!(*codec)) {
		fprintf(stderr, "Could not find encoder for '%s'\n",
			avcodec_get_name(codec_id));
		exit(1);
	}

	ost->st = avformat_new_stream(oc, NULL);
	if (!ost->st) {
		fprintf(stderr, "Could not allocate stream\n");
		exit(1);
	}
	ost->st->id = oc->nb_streams - 1;
	c = avcodec_alloc_context3(*codec);
	if (!c) {
		fprintf(stderr, "Could not alloc an encoding context\n");
		exit(1);
	}
	ost->enc = c;

	switch ((*codec)->type) {
	case AVMEDIA_TYPE_AUDIO:
		c->sample_fmt = (*codec)->sample_fmts ?
			(*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
		c->bit_rate = 64000;
		c->sample_rate = 44100;
		if ((*codec)->supported_samplerates) {
			c->sample_rate = (*codec)->supported_samplerates[0];
			for (i = 0; (*codec)->supported_samplerates[i]; i++) {
				if ((*codec)->supported_samplerates[i] == 44100)
					c->sample_rate = 44100;
			}
		}
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		c->channel_layout = AV_CH_LAYOUT_STEREO;
		if ((*codec)->channel_layouts) {
			c->channel_layout = (*codec)->channel_layouts[0];
			for (i = 0; (*codec)->channel_layouts[i]; i++) {
				if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
					c->channel_layout = AV_CH_LAYOUT_STEREO;
			}
		}
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		AVRational ar;
		ar.num = 1;
		ar.den = c->sample_rate;
		ost->st->time_base = ar;// (AVRational){ 1, c->sample_rate };
		break;

	case AVMEDIA_TYPE_VIDEO:
		c->codec_id = codec_id;

		c->bit_rate = 400000;
		/* Resolution must be a multiple of two. */
		c->width = m_videowith;// pCodecCtx->width;// 352;
		c->height = m_videoheight;//pCodecCtx->height;// 288;
		/* timebase: This is the fundamental unit of time (in seconds) in terms
		* of which frame timestamps are represented. For fixed-fps content,
		* timebase should be 1/framerate and timestamp increments should be
		* identical to 1. */

		AVRational ar1;
		ar1.num = 1;
		ar1.den = m_videoframerates;//;
		ost->st->time_base = ar1;// (AVRational){ 1, STREAM_FRAME_RATE };
		c->time_base = ost->st->time_base;

		c->gop_size = 300; /* emit one intra frame every twelve frames at most */
		c->pix_fmt = STREAM_PIX_FMT;
		//		if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
		/* just for testing, we also add B-frames */
		c->max_b_frames = 1;
		//		}
		//		if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
		/* Needed to avoid using macroblocks in which some coeffs overflow.
		* This does not happen with normal video, it just happens here as
		* the motion of the chroma plane does not match the luma plane. */
		//			c->mb_decision = 2;
		//		}
		av_opt_set(c->priv_data, "preset", "slow", 0);
		break;

	default:
		break;
	}

	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

/**************************************************************/
/* audio output */

AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
	uint64_t channel_layout,
	int sample_rate, int nb_samples)
{

	AVFrame *frame = av_frame_alloc();
	int ret;

	if (!frame) {
		fprintf(stderr, "Error allocating an audio frame\n");
		exit(1);
	}

	frame->format = sample_fmt;
	frame->channel_layout = channel_layout;
	frame->sample_rate = sample_rate;
	frame->nb_samples = nb_samples;

	if (nb_samples) {
		ret = av_frame_get_buffer(frame, 0);
		if (ret < 0) {
			fprintf(stderr, "Error allocating an audio buffer\n");
			exit(1);
		}
	}

	return frame;
}

void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
	AVCodecContext *c;
	int nb_samples;
	int ret;
	AVDictionary *opt = NULL;

	c = ost->enc;

	/* open it */
	av_dict_copy(&opt, opt_arg, 0);
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		//		fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
		exit(1);
	}

	/* init signal generator */
	ost->t = 0;
	ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
	/* increment frequency by 110 Hz per second */
	ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

	if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = c->frame_size;

	ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
		c->sample_rate, nb_samples);
	ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
		c->sample_rate, nb_samples);

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		fprintf(stderr, "Could not copy the stream parameters\n");
		exit(1);
	}

	/* create resampler context */
	ost->swr_ctx = swr_alloc();
	if (!ost->swr_ctx) {
		fprintf(stderr, "Could not allocate resampler context\n");
		exit(1);
	}

	/* set options */
	av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
	av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
	av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(ost->swr_ctx)) < 0) {
		fprintf(stderr, "Failed to initialize the resampling context\n");
		exit(1);
	}
}

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
* 'nb_channels' channels. */
AVFrame *get_audio_frame(OutputStream *ost)
{
	AVFrame *frame = ost->tmp_frame;
	int j, i, v;
	int16_t *q = (int16_t*)frame->data[0];

	if (g_captureStopFlag > 0) return NULL;

	frame->data[0] = (uint8_t*)malloc(8000);
	memset(frame->data[0], 0, 8000);
	memcpy(frame->data[0], _header[_iplaying].lpData, 8000);
	frame->pts = ost->next_pts;
	ost->next_pts += 500;// frame->nb_samples;

	return frame;
}

/*
* encode one audio frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise
*/
int write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
	AVCodecContext *c;
	AVPacket pkt = { 0 }; // data and size must be 0;
	AVFrame *frame;
	int ret;
	int got_packet;
	int dst_nb_samples;

	av_init_packet(&pkt);
	c = ost->enc;

	frame = get_audio_frame(ost);

	if (frame) {
		/* convert samples from native format to destination codec format, using the resampler */
		/* compute destination number of samples */
		dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
			c->sample_rate, c->sample_rate, AV_ROUND_UP);
		av_assert0(dst_nb_samples == frame->nb_samples);

		/* when we pass a frame to the encoder, it may keep a reference to it
		* internally;
		* make sure we do not overwrite it here
		*/
		ret = av_frame_make_writable(ost->frame);
		if (ret < 0)
			exit(1);

		/* convert to destination format */
		ret = swr_convert(ost->swr_ctx,
			ost->frame->data, dst_nb_samples,
			(const uint8_t **)frame->data, frame->nb_samples);
		if (ret < 0) {
			fprintf(stderr, "Error while converting\n");
			exit(1);
		}
		frame = ost->frame;

		AVRational ar;
		ar.num = 1;
		ar.den = c->sample_rate;

		frame->pts = av_rescale_q(ost->samples_count, ar, c->time_base);
		ost->samples_count += dst_nb_samples;
	}

	ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
	if (ret < 0) {
		fprintf(stderr, "Error encoding audio frame");
		exit(1);
	}

	if (got_packet) {
		ret = write_frame(oc, &c->time_base, ost->st, &pkt);
		if (ret < 0) {
			fprintf(stderr, "Error while writing audio frame");
			exit(1);
		}
	}

	return (frame || got_packet) ? 0 : 1;
}

/**************************************************************/
/* video output */

AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	int ret;

	picture = av_frame_alloc();
	if (!picture)
		return NULL;

	picture->format = pix_fmt;
	picture->width = width;
	picture->height = height;

	/* allocate the buffers for the frame data */
	ret = av_frame_get_buffer(picture, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate frame data.\n");
		exit(1);
	}

	return picture;
}

void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
	int ret;
	AVCodecContext *c = ost->enc;
	AVDictionary *opt = NULL;

	av_dict_copy(&opt, opt_arg, 0);

	/* open the codec */
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		fprintf(stderr, "Could not open video codec");
		exit(1);
	}

	/* allocate and init a re-usable frame */
	ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
	if (!ost->frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
	* picture is needed too. It is then converted to the required
	* output format. */
	ost->tmp_frame = NULL;
	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
		if (!ost->tmp_frame) {
			fprintf(stderr, "Could not allocate temporary picture\n");
			exit(1);
		}
	}

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		fprintf(stderr, "Could not copy the stream parameters\n");
		exit(1);
	}
}

/* Prepare a dummy image. */
void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height)
{
	if (av_read_frame(pFormatCtx, packet) >= 0){
		if (packet->stream_index == videoindex){
			int	got_picture;
			int ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0){
				printf("Decode Error.\n");
				return;
			}

			if (got_picture){
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pict->data, pict->linesize);

			}
		}

	}


}

AVFrame *get_video_frame(OutputStream *ost)
{
	AVCodecContext *c = ost->enc;

	/* check if we want to generate more frames */

	AVRational ar;
	ar.num = 1;
	ar.den = 1;

	if (g_captureStopFlag > 0)
		return NULL;

	/* when we pass a frame to the encoder, it may keep a reference to it
	* internally; make sure we do not overwrite it here */
	if (av_frame_make_writable(ost->frame) < 0)
		exit(1);

	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		/* as we only generate a YUV420P picture, we must convert it
		* to the codec pixel format if needed */
		if (!ost->sws_ctx) {
			ost->sws_ctx = sws_getContext(c->width, c->height,
				AV_PIX_FMT_YUV420P,
				c->width, c->height,
				c->pix_fmt,
				SCALE_FLAGS, NULL, NULL, NULL);
			if (!ost->sws_ctx) {
				fprintf(stderr,
					"Could not initialize the conversion context\n");
				exit(1);
			}
		}

		fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);

		sws_scale(ost->sws_ctx,
			(const uint8_t * const *)ost->tmp_frame->data, ost->tmp_frame->linesize,
			0, c->height, ost->frame->data, ost->frame->linesize);
	}
	else {
		fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
	}

	ost->frame->pts = ost->next_pts++;
	return ost->frame;
}

/*
* encode one video frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise
*/
int write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
	int ret;
	AVCodecContext *c;
	AVFrame *frame;
	int got_packet = 0;
	AVPacket pkt = { 0 };

	c = ost->enc;

	frame = get_video_frame(ost);

	av_init_packet(&pkt);

	/* encode the image */
	ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
	if (ret < 0) {
		fprintf(stderr, "Error encoding video frame");
		exit(1);
	}

	if (got_packet) {
		ret = write_frame(oc, &c->time_base, ost->st, &pkt);
	}
	else {
		ret = 0;
	}

	if (ret < 0) {
		fprintf(stderr, "Error while writing video frame");
		exit(1);
	}

	return (frame || got_packet) ? 0 : 1;
}

void close_stream(AVFormatContext *oc, OutputStream *ost)
{
	avcodec_free_context(&ost->enc);
	av_frame_free(&ost->frame);
	av_frame_free(&ost->tmp_frame);
	sws_freeContext(ost->sws_ctx);
	swr_free(&ost->swr_ctx);
}

DWORD WINAPI RecordingWaitingThread(LPVOID ivalue)
{
//	int jjj = 0;
	while (1)
	{
//		jjj++;
//		if (jjj > 10) jjj = 0;
//		if (jjj == 0){
			WaitForSingleObject(hEvent_BufferReady, INFINITE);

			result = waveInUnprepareHeader(hWaveIn, &_header[_iBuf], sizeof (WAVEHDR));

			++_iBuf;
			if (_iBuf == NUM_BUF)   _iBuf = 0;
			_iplaying = _iBuf;
			result = waveInPrepareHeader(hWaveIn, &_header[_iBuf], sizeof(WAVEHDR));
			result = waveInAddBuffer(hWaveIn, &_header[_iBuf], sizeof (WAVEHDR));

//		}
	}
	return 0;
}

DWORD WINAPI PlayingWaitingThread(LPVOID ivalue)
{
	while (1){
		WaitForSingleObject(hEvent_FinishedPlaying, INFINITE);
	}
}

void CALLBACK myWaveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (uMsg != WIM_DATA)
		return;
	SetEvent(hEvent_BufferReady);
}

extern "C"
{
	EXTERN void startMicrophone(){
		if (m_audioIS==0) return;

		hEvent_BufferReady = CreateEvent(NULL, FALSE, FALSE, NULL);
		hEvent_FinishedPlaying = CreateEvent(NULL, FALSE, FALSE, NULL);


		pFormat.wFormatTag = WAVE_FORMAT_PCM; // simple, uncompressed format
		pFormat.nChannels = 2; // 1=mono, 2=stereo
		pFormat.nSamplesPerSec = 44100;
		pFormat.wBitsPerSample = 16; // 16 for high quality, 8 for telephone-grade
		pFormat.nBlockAlign = pFormat.nChannels*pFormat.wBitsPerSample / 8;
		pFormat.nAvgBytesPerSec = (pFormat.nSamplesPerSec)*(pFormat.nChannels)*(pFormat.wBitsPerSample) / 8;
		pFormat.cbSize = 0;


		short int  *_pBuf;
		size_t bpbuff = 4000;//= (pFormat.nSamplesPerSec) * (pFormat.nChannels) * (pFormat.wBitsPerSample)/8;
		_pBuf = new short int[bpbuff * NUM_BUF];


		result = waveInOpen(&hWaveIn, WAVE_MAPPER, &pFormat, (DWORD)myWaveInProc, 0L, CALLBACK_FUNCTION);
		// initialize all headers in the queue
		for (int i = 0; i < NUM_BUF; i++)
		{
			_header[i].lpData = (LPSTR)&_pBuf[i * bpbuff];
			_header[i].dwBufferLength = bpbuff*sizeof(*_pBuf);
			_header[i].dwFlags = 0L;
			_header[i].dwLoops = 0L;
		}

		DWORD myThreadID;
		DWORD myThreadIDPlay;
		HANDLE hThread;
		HANDLE hThreadPlay;
		hThread = CreateThread(NULL, 0, RecordingWaitingThread, NULL, 0, &myThreadID);
		hThreadPlay = CreateThread(NULL, 0, PlayingWaitingThread, NULL, 0, &myThreadIDPlay);

		_iBuf = 0;

		waveInPrepareHeader(hWaveIn, &_header[_iBuf], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &_header[_iBuf], sizeof (WAVEHDR));

		waveInStart(hWaveIn);

	}
	EXTERN int OnStartCapturing(){
		
		startMicrophone();
		g_captureStopFlag = 0;  // if >0, stop
		g_capturePauseFlag = 0;  // if >0, pause

		OutputStream video_st = { 0 }, audio_st = { 0 };
		char *filename = (char*)malloc(20);
		AVOutputFormat *fmt;
		AVFormatContext *oc;
		AVCodec *audio_codec, *video_codec;
		int ret;
		int have_video = 0, have_audio = 0;
		int encode_video = 0, encode_audio = 0;
		AVDictionary *opt = NULL;
		int i;

		/* Initialize libavcodec, and register all codecs and formats. */
		av_register_all();

		time_t timer;
		struct tm *t;
		timer = time(NULL);
		t = localtime(&timer);
		sprintf(filename, "%d%d%d%d%d%d.mp4", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

		/*		if (m_videoCodec == 0){
		filename = (char*)malloc(20);
		strcpy(filename , "Screen.mp4");
		}
		else if (m_videoCodec == 1){*/
		//			strcpy(filename, "Screen1.wmv");
		//}
		/*		for (i = 2; i + 1 < argc; i += 2) {
		if (!strcmp(argv[i], "-flags") || !strcmp(argv[i], "-fflags"))
		av_dict_set(&opt, argv[i] + 1, argv[i + 1], 0);
		}
		*/

		av_register_all();
		avformat_network_init();
		pFormatCtx = avformat_alloc_context();

		//Register Device
		avdevice_register_all();
		//Windows

#if USE_DSHOW
		//Use dshow
		AVInputFormat *ifmt = av_find_input_format("dshow");
		if (avformat_open_input(&pFormatCtx, "video=screen-capture-recorder", ifmt, NULL) != 0){
			printf("Couldn't open input stream.\n");
			return -1;
		}
#else
		//Use gdigrab
		AVDictionary* options = NULL;
		AVInputFormat *ifmt = av_find_input_format("gdigrab");
		if (avformat_open_input(&pFormatCtx, "desktop", ifmt, &options) != 0){
			printf("Couldn't open input stream.\n");
			return -1;
		}
#endif

		if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
		{
			printf("Couldn't find stream information.\n");
			return -1;
		}
		videoindex = -1;
		for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
		if (videoindex == -1)
		{
			printf("Didn't find a video stream.\n");
			return -1;
		}
		pCodecCtx = pFormatCtx->streams[videoindex]->codec;
		pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if (pCodec == NULL)
		{
			printf("Codec not found.\n");
			return -1;
		}
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		{
			printf("Could not open codec.\n");
			return -1;
		}

		pFrame = av_frame_alloc();

		packet = (AVPacket *)av_malloc(sizeof(AVPacket));

		img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, /* pCodecCtx->width, pCodecCtx->height*/m_videowith, m_videoheight, AV_PIX_FMT_YUV420P, SCALE_FLAGS, NULL, NULL, NULL);

		/* allocate the output media context */


		/*		if (m_videoCodec == 0){
		avformat_alloc_output_context2(&oc, NULL, "mp4", filename);
		}
		else if (m_videoCodec == 1){*/
		avformat_alloc_output_context2(&oc, NULL, "mp4", filename);
		//}

		if (!oc) {
			printf("Could not deduce output format from file extension: using MPEG.\n");
			avformat_alloc_output_context2(&oc, NULL, "mp4", filename);
		}
		if (!oc)
			return 1;

		fmt = oc->oformat;

		/* Add the audio and video streams using the default format codecs
		* and initialize the codecs. */
		if (fmt->video_codec != AV_CODEC_ID_NONE) {
			add_stream(&video_st, oc, &video_codec, fmt->video_codec);
			have_video = 1;
			encode_video = 1;
		}
		if (fmt->audio_codec != AV_CODEC_ID_NONE && m_audioIS==1) {
			add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
			have_audio = 1;
			encode_audio = 1;
		}

		/* Now that all the parameters are set, we can open the audio and
		* video codecs and allocate the necessary encode buffers. */
		if (have_video)
			open_video(oc, video_codec, &video_st, opt);

		if (have_audio)
			open_audio(oc, audio_codec, &audio_st, opt);

		av_dump_format(oc, 0, filename, 1);

		/* open the output file, if needed */
		if (!(fmt->flags & AVFMT_NOFILE)) {
			ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
			if (ret < 0) {
				fprintf(stderr, "Could not open");
				return 1;
			}
		}

		/* Write the stream header, if any. */
		ret = avformat_write_header(oc, &opt);
		if (ret < 0) {
			fprintf(stderr, "Error occurred when opening output file");
			return 1;
		}


		while ((encode_video || encode_audio)) {

			if (g_capturePauseFlag == 0){

				if (encode_video &&
					(!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
					audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
					encode_video = !write_video_frame(oc, &video_st);
				}
				else {
					encode_audio = !write_audio_frame(oc, &audio_st);
				}
				av_free_packet(packet);
			}
		}

/*		waveInClose(hWaveIn);
		CloseHandle(hThread);
		CloseHandle(hThreadPlay);
		CloseHandle(hEvent_BufferReady);
		CloseHandle(hEvent_FinishedPlaying);
		*/
		/* Write the trailer, if any. The trailer must be written before you
		* close the CodecContexts open when you wrote the header; otherwise
		* av_write_trailer() may try to use memory that was freed on
		* av_codec_close(). */
		av_write_trailer(oc);

		/* Close each codec. */
		if (have_video)
			close_stream(oc, &video_st);
		if (have_audio)
			close_stream(oc, &audio_st);

		if (!(fmt->flags & AVFMT_NOFILE))
			/* Close the output file. */
			avio_closep(&oc->pb);

		/* free the stream */
		avformat_free_context(oc);

//		free(&audio_st);
//		free(&video_st);
		return 999;
	}

	EXTERN int OnStopCapturing(){
		g_captureStopFlag = 1;
		return 888;
	}

	EXTERN int OnPauseResumeCapturing(){
		if (g_capturePauseFlag == 0)
			g_capturePauseFlag = 1;
		else
			g_capturePauseFlag = 0;
		return 888;
	}

	EXTERN void SetAudioInput(int jparam){
		m_audioinput = jparam;
		return;
	}
	EXTERN void SetAudioVolume(int jparam){
		m_audiovolume = jparam;
		return;
	}
	EXTERN void SetAudioFrequency(int jparam){
		m_audiofrequency = jparam;
		return;
	}
	EXTERN void SetAudioBites(int jparam){
		m_audiobites = jparam;
		return;
	}
	EXTERN void SetAudioType(int jparam){
		m_audiotype = jparam;
		return;
	}
	EXTERN void SetAudioFormat(int jparam){
		m_audioformat = jparam;
		return;
	}
	EXTERN void SetAudioIS(int jparam){
		m_audioIS = jparam;
		return;
	}
	EXTERN void SetVideoWidth(int jparam){
		m_videowith = jparam;
		return;
	}
	EXTERN void SetVideoHeight(int jparam){
		m_videoheight = jparam;
		return;
	}
	EXTERN void SetVideoFrameRates(int jparam){
		m_videoframerates = jparam;
		return;
	}
	EXTERN void SetVideoCodec(int jparam){
		m_videoCodec = jparam;
		return;
	}
}

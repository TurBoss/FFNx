#if defined(__cplusplus)
extern "C" {
#endif

#include "ffmpeg_movies.h"

// 10 frames
#define VIDEO_BUFFER_SIZE 10

// 20 seconds
#define AUDIO_BUFFER_SIZE 20

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

#define STACK_MAX_NAME_LENGTH 256

inline double round(double x) { return floor(x + 0.5); }

#define LAG (((now - start_time) - (timer_freq / movie_fps) * movie_frame_counter) / (timer_freq / 1000))

GLint texture_units = 1;

uint yuv_init_done = false;
uint yuv_fast_path = false;

uint audio_must_be_converted = false;

AVFormatContext *format_ctx = 0;
AVCodecContext *codec_ctx = 0;
AVCodec *codec = 0;
AVCodecContext *acodec_ctx = 0;
AVCodec *acodec = 0;
AVFrame *movie_frame = 0;
struct SwsContext *sws_ctx = 0;
SwrContext* swr_ctx = NULL;

int videostream;
int audiostream;

uint use_bgra_texture;

struct video_frame
{
	GLuint bgra_texture;
	GLuint yuv_textures[3];
};

struct video_frame video_buffer[VIDEO_BUFFER_SIZE];
uint vbuffer_read = 0;
uint vbuffer_write = 0;

uint movie_frame_counter = 0;
uint movie_frames;
uint movie_width, movie_height;
double movie_fps;
double movie_duration;

uint skipping_frames;
uint skipped_frames;

IDirectSoundBuffer *sound_buffer = 0;
uint sound_buffer_size;
uint write_pointer = 0;

uint first_audio_packet;

time_t timer_freq;
time_t start_time;

void ffmpeg_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
	char msg[4 * 1024]; // 4K
	static int print_prefix = 1;

	av_log_format_line(ptr, level, fmt, vl, msg, sizeof(msg), &print_prefix);

	switch (level) {
		case AV_LOG_VERBOSE:
		case AV_LOG_DEBUG: trace(msg); break;
		case AV_LOG_INFO:
		case AV_LOG_WARNING: info(msg); break;
		case AV_LOG_ERROR:
		case AV_LOG_FATAL:
		case AV_LOG_PANIC: error(msg); break;
	}

	if (level <= AV_LOG_WARNING) {
		CONTEXT ctx;

		ctx.ContextFlags = CONTEXT_CONTROL;

		if (
			GetThreadContext(
				GetCurrentThread(),
				&ctx
			)
			)
			printStack(&ctx);
	}
}

void ffmpeg_movie_init()
{
	av_log_set_level(AV_LOG_VERBOSE);
	av_log_set_callback(ffmpeg_log_callback);

	info("FFMpeg movie player plugin loaded\n");
	info("FFMpeg version 4.2.1, Copyright (c) 2000-2019 Fabrice Bellard, et al.\n");

	glewInit();

	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &texture_units);

	if(texture_units < 3) info("No multitexturing, codecs with YUV output will be slow. (texture units: %i)\n", texture_units);
	else yuv_fast_path = true;

	QueryPerformanceFrequency((LARGE_INTEGER *)&timer_freq);
}

// clean up anything we have allocated
void ffmpeg_release_movie_objects()
{
	uint i;

	if (movie_frame) av_frame_free(&movie_frame);
	if (codec_ctx) avcodec_close(codec_ctx);
	if (acodec_ctx) avcodec_close(acodec_ctx);
	if (format_ctx) avformat_close_input(&format_ctx);
	if (sound_buffer && *common_externals.directsound) IDirectSoundBuffer_Release(sound_buffer);
	if (swr_ctx) {
		swr_close(swr_ctx);
		swr_free(&swr_ctx);
	}

	codec_ctx = 0;
	acodec_ctx = 0;
	format_ctx = 0;
	sound_buffer = 0;
	
	audio_must_be_converted = false;

	if(skipped_frames > 0) info("release_movie_objects: skipped %i frames\n", skipped_frames);
	skipped_frames = 0;

	for(i = 0; i < VIDEO_BUFFER_SIZE; i++)
	{
		glDeleteTextures(1, &video_buffer[i].bgra_texture);
		video_buffer[i].bgra_texture = 0;
		glDeleteTextures(3, video_buffer[i].yuv_textures);
		memset(video_buffer[i].yuv_textures, 0, sizeof(video_buffer[i].yuv_textures));
	}
}

// prepare a movie for playback
uint ffmpeg_prepare_movie(char *name)
{
	uint i;
	WAVEFORMATEX sound_format;
	DSBUFFERDESC1 sbdesc;
	uint ret;

	if(ret = avformat_open_input(&format_ctx, name, NULL, NULL))
	{
		error("prepare_movie: couldn't open movie file: %s\n", name);
		ffmpeg_release_movie_objects();
		goto exit;
	}

	if(avformat_find_stream_info(format_ctx, NULL) < 0)
	{
		error("prepare_movie: couldn't find stream info\n");
		ffmpeg_release_movie_objects();
		goto exit;
	}

	videostream = -1;
	audiostream = -1;
	for(i = 0; i < format_ctx->nb_streams; i++)
	{
		if(format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && videostream < 0) videostream = i;
		if(format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audiostream < 0) audiostream = i;
	}

	if(videostream == -1)
	{
		error("prepare_movie: no video stream found\n");
		ffmpeg_release_movie_objects();
		goto exit;
	}

	if(audiostream == -1) trace("prepare_movie: no audio stream found\n");

	codec_ctx = format_ctx->streams[videostream]->codec;

	codec = avcodec_find_decoder(codec_ctx->codec_id);
	if(!codec)
	{
		error("prepare_movie: no video codec found\n");
		codec_ctx = 0;
		ffmpeg_release_movie_objects();
		goto exit;
	}

	if(avcodec_open2(codec_ctx, codec, NULL) < 0)
	{
		error("prepare_movie: couldn't open video codec\n");
		ffmpeg_release_movie_objects();
		goto exit;
	}

	if(audiostream != -1)
	{
		acodec_ctx = format_ctx->streams[audiostream]->codec;
		acodec = avcodec_find_decoder(acodec_ctx->codec_id);
		if(!acodec)
		{
			error("prepare_movie: no audio codec found\n");
			ffmpeg_release_movie_objects();
			goto exit;
		}

		if(avcodec_open2(acodec_ctx, acodec, NULL) < 0)
		{
			error("prepare_movie: couldn't open audio codec\n");
			ffmpeg_release_movie_objects();
			goto exit;
		}
	}

	movie_width = codec_ctx->width;
	movie_height = codec_ctx->height;
	movie_fps = 1.0 / (av_q2d(codec_ctx->time_base) * codec_ctx->ticks_per_frame);
	movie_duration = (double)format_ctx->duration / (double)AV_TIME_BASE;
	movie_frames = (uint)round(movie_fps * movie_duration);

	if(movie_fps < 100.0) info("prepare_movie: %s; %s/%s %ix%i, %f FPS, duration: %f, frames: %i\n", name, codec->name, acodec_ctx ? acodec->name : "null", movie_width, movie_height, movie_fps, movie_duration, movie_frames);
	// bogus FPS value, assume the codec provides frame limiting
	else info("prepare_movie: %s; %s/%s %ix%i, duration: %f\n", name, codec->name, acodec_ctx ? acodec->name : "null", movie_width, movie_height, movie_duration);

	if(movie_width > max_texture_size || movie_height > max_texture_size)
	{
		error("prepare_movie: movie dimensions exceed max texture size, skipping\n");
		ffmpeg_release_movie_objects();
		goto exit;
	}

	if(!movie_frame) movie_frame = av_frame_alloc();

	if(sws_ctx) sws_freeContext(sws_ctx);

	if(codec_ctx->pix_fmt == AV_PIX_FMT_YUV420P && yuv_fast_path) use_bgra_texture = false;
	else use_bgra_texture = true;

	vbuffer_read = 0;
	vbuffer_write = 0;

	if(codec_ctx->pix_fmt != AV_PIX_FMT_BGRA && codec_ctx->pix_fmt != AV_PIX_FMT_BGR24 && (codec_ctx->pix_fmt != AV_PIX_FMT_YUV420P || !yuv_fast_path))
	{
		sws_ctx = sws_getContext(movie_width, movie_height, codec_ctx->pix_fmt, movie_width, movie_height, AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR | SWS_ACCURATE_RND, NULL, NULL, NULL);
		info("prepare_movie: slow output format from video codec %s; %i\n", codec->name, codec_ctx->pix_fmt);
	}
	else sws_ctx = 0;

	if(audiostream != -1)
	{
		if (acodec_ctx->sample_fmt != AV_SAMPLE_FMT_U8 && acodec_ctx->sample_fmt != AV_SAMPLE_FMT_S16) {
			audio_must_be_converted = true;
			trace("prepare_movie: Audio must be converted: IN acodec_ctx->sample_fmt: %s\n", av_get_sample_fmt_name(acodec_ctx->sample_fmt));
			trace("prepare_movie: Audio must be converted: IN acodec_ctx->sample_rate: %d\n", acodec_ctx->sample_rate);
			trace("prepare_movie: Audio must be converted: IN acodec_ctx->channel_layout: %u\n", acodec_ctx->channel_layout);

			// Prepare software conversion context
			swr_ctx = swr_alloc_set_opts(
				// Create a new context
				NULL,
				// OUT
				acodec_ctx->channel_layout,
				AV_SAMPLE_FMT_S16,
				acodec_ctx->sample_rate,
				// IN
				acodec_ctx->channel_layout,
				acodec_ctx->sample_fmt,
				acodec_ctx->sample_rate,
				// LOG
				0,
				NULL
			);

			swr_init(swr_ctx);
		}

		sound_format.cbSize = sizeof(sound_format);
		sound_format.wBitsPerSample = (audio_must_be_converted ? 16 : acodec_ctx->bits_per_coded_sample);
		sound_format.nChannels = acodec_ctx->channels;
		sound_format.nSamplesPerSec = acodec_ctx->sample_rate;
		sound_format.nBlockAlign = sound_format.nChannels * sound_format.wBitsPerSample / 8;
		sound_format.nAvgBytesPerSec = sound_format.nSamplesPerSec * sound_format.nBlockAlign;
		sound_format.wFormatTag = WAVE_FORMAT_PCM;

		sound_buffer_size = sound_format.nAvgBytesPerSec * AUDIO_BUFFER_SIZE;

		sbdesc.dwSize = sizeof(sbdesc);
		sbdesc.lpwfxFormat = &sound_format;
		sbdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME;
		sbdesc.dwReserved = 0;
		sbdesc.dwBufferBytes = sound_buffer_size;

		if(ret = IDirectSound_CreateSoundBuffer(*common_externals.directsound, (LPCDSBUFFERDESC)&sbdesc, &sound_buffer, 0))
		{
			error("prepare_movie: couldn't create sound buffer (%i, %i, %i, %i)\n", acodec_ctx->sample_fmt, acodec_ctx->bit_rate, acodec_ctx->sample_rate, acodec_ctx->channels);
			sound_buffer = 0;
		}

		first_audio_packet = true;
		write_pointer = 0;
	}

exit:
	movie_frame_counter = 0;
	skipped_frames = 0;

	return movie_frames;
}

// stop movie playback, no video updates will be requested after this so all we have to do is stop the audio
void ffmpeg_stop_movie()
{
	if(sound_buffer && *common_externals.directsound) IDirectSoundBuffer_Stop(sound_buffer);
}

void buffer_bgra_frame(uint8_t *data, int upload_stride)
{
	uint upload_width = codec_ctx->pix_fmt == AV_PIX_FMT_BGRA ? upload_stride / 4 : upload_stride / 3;

	if(upload_stride < 0) return;

	if(video_buffer[vbuffer_write].bgra_texture) glDeleteTextures(1, &video_buffer[vbuffer_write].bgra_texture);

	glGenTextures(1, &video_buffer[vbuffer_write].bgra_texture);
	glBindTexture(GL_TEXTURE_2D, video_buffer[vbuffer_write].bgra_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, movie_width, movie_height, 0, GL_BGR, GL_UNSIGNED_BYTE, 0);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, upload_width);

	if(codec_ctx->pix_fmt == AV_PIX_FMT_BGRA) glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, movie_width, movie_height, GL_BGRA, GL_UNSIGNED_BYTE, data);
	else glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, movie_width, movie_height, GL_BGR, GL_UNSIGNED_BYTE, data);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	vbuffer_write = (vbuffer_write + 1) % VIDEO_BUFFER_SIZE;
}

void draw_bgra_frame(uint buffer_index)
{
	gl_draw_movie_quad_bgra(video_buffer[buffer_index].bgra_texture, movie_width, movie_height);
}

void upload_yuv_texture(uint8_t **planes, int *strides, uint num, uint buffer_index)
{
	uint upload_width = strides[num];
	uint tex_width = num == 0 ? movie_width : movie_width / 2;
	uint tex_height = num == 0 ? movie_height : movie_height / 2;

	glActiveTexture(GL_TEXTURE0 + num);

	glBindTexture(GL_TEXTURE_2D, video_buffer[buffer_index].yuv_textures[num]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, tex_width, tex_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, upload_width);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_LUMINANCE, GL_UNSIGNED_BYTE, planes[num]);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void buffer_yuv_frame(uint8_t **planes, int *strides)
{
	if(video_buffer[vbuffer_write].yuv_textures[0]) glDeleteTextures(3, video_buffer[vbuffer_write].yuv_textures);

	glGenTextures(3, video_buffer[vbuffer_write].yuv_textures);
	
	upload_yuv_texture(planes, strides, 2, vbuffer_write);
	upload_yuv_texture(planes, strides, 1, vbuffer_write);
	upload_yuv_texture(planes, strides, 0, vbuffer_write);

	vbuffer_write = (vbuffer_write + 1) % VIDEO_BUFFER_SIZE;
}

void draw_yuv_frame(uint buffer_index, uint full_range)
{
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, video_buffer[buffer_index].yuv_textures[2]);
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, video_buffer[buffer_index].yuv_textures[1]);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, video_buffer[buffer_index].yuv_textures[0]);

	gl_draw_movie_quad_yuv(video_buffer[buffer_index].yuv_textures, movie_width, movie_height, full_range);
}

// display the next frame
uint ffmpeg_update_movie_sample()
{
	AVPacket packet;
	int ret;
	time_t now;
	DWORD DSStatus;

	// no playable movie loaded, skip it
	if(!format_ctx) return false;

	// keep track of when we started playing this movie
	if(movie_frame_counter == 0) QueryPerformanceCounter((LARGE_INTEGER *)&start_time);

	// set default values.
	av_init_packet(&packet);

	while((ret = av_read_frame(format_ctx, &packet)) >= 0)
	{
		if(packet.stream_index == videostream)
		{
			ret = avcodec_send_packet(codec_ctx, &packet);

			if (ret < 0) {
				trace("%s: avcodec_send_packet -> %d\n", __func__, ret);
				break;
			}

			ret = avcodec_receive_frame(codec_ctx, movie_frame);

			if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
			{
				trace("%s: avcodec_receive_frame -> %d\n", __func__, ret);
				break;
			}

			if (ret >= 0)
			{
				QueryPerformanceCounter((LARGE_INTEGER *)&now);

				// check if we are falling behind
				if(skip_frames && movie_fps < 100.0 && LAG > 100.0) skipping_frames = true;

				if(skipping_frames && LAG > 0.0)
				{
					skipped_frames++;
					if(((skipped_frames - 1) & skipped_frames) == 0) glitch("update_movie_sample: video playback is lagging behind, skipping frames (frame #: %i, skipped: %i, lag: %f)\n", movie_frame_counter, skipped_frames, LAG);
					av_packet_unref(&packet);
					if(use_bgra_texture) draw_bgra_frame(vbuffer_read);
					else draw_yuv_frame(vbuffer_read, codec_ctx->color_range == AVCOL_RANGE_JPEG);
					break;
				}
				else skipping_frames = false;

				if(movie_sync_debug) info("update_movie_sample(video): DTS %f PTS %f (timebase %f) placed in video buffer at real time %f (play %f)\n", (double)packet.dts, (double)packet.pts, av_q2d(codec_ctx->time_base), (double)(now - start_time) / (double)timer_freq, (double)movie_frame_counter / (double)movie_fps);
				
				if(sws_ctx)
				{
					uint8_t *planes[3] = {0, 0, 0};
					int strides[3] = {0, 0, 0};
					uint8_t *data = (uint8_t*)calloc(movie_width * movie_height, 3);

					planes[0] = data;
					strides[0] = movie_width * 3;

					sws_scale(sws_ctx, movie_frame->extended_data, movie_frame->linesize, 0, movie_height, planes, strides);

					buffer_bgra_frame(data, movie_width * 3);

					free(data);
				}
				else if(use_bgra_texture) buffer_bgra_frame(movie_frame->extended_data[0], movie_frame->linesize[0]);
				else buffer_yuv_frame(movie_frame->extended_data, movie_frame->linesize);

				av_packet_unref(&packet);

				if(vbuffer_write == vbuffer_read)
				{
					if(use_bgra_texture) draw_bgra_frame(vbuffer_read);
					else draw_yuv_frame(vbuffer_read, codec_ctx->color_range == AVCOL_RANGE_JPEG);

					vbuffer_read = (vbuffer_read + 1) % VIDEO_BUFFER_SIZE;

					break;
				}
			}
		}

		if(packet.stream_index == audiostream)
		{
			uint8_t *buffer;
			int buffer_size = 0;
			int used_bytes;
			DWORD playcursor;
			DWORD writecursor;
			uint bytesperpacket = audio_must_be_converted ? av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) : av_get_bytes_per_sample(acodec_ctx->sample_fmt);
			uint bytespersec = bytesperpacket * acodec_ctx->channels * acodec_ctx->sample_rate;

			QueryPerformanceCounter((LARGE_INTEGER *)&now);

			if(movie_sync_debug)
			{
				IDirectSoundBuffer_GetCurrentPosition(sound_buffer, &playcursor, &writecursor);
				info("update_movie_sample(audio): DTS %f PTS %f (timebase %f) placed in sound buffer at real time %f (play %f write %f)\n", (double)packet.dts, (double)packet.pts, av_q2d(acodec_ctx->time_base), (double)(now - start_time) / (double)timer_freq, (double)playcursor / (double)bytespersec, (double)write_pointer / (double)bytespersec);
			}

			ret = avcodec_send_packet(acodec_ctx, &packet);

			if (ret < 0) {
				trace("%s: avcodec_send_packet -> %d\n", __func__, ret);
				break;
			}

			ret = avcodec_receive_frame(acodec_ctx, movie_frame);

			if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
			{
				trace("%s: avcodec_receive_frame -> %d\n", __func__, ret);
				break;
			}

			if (ret >= 0)
			{
				int _size = bytesperpacket * movie_frame->nb_samples * acodec_ctx->channels;

				// Sometimes the captured frame may have no sound samples. Just skip and move forward
				if (_size)
				{
					LPVOID ptr1;
					LPVOID ptr2;
					DWORD bytes1;
					DWORD bytes2;

					av_samples_alloc(&buffer, movie_frame->linesize, acodec_ctx->channels, movie_frame->nb_samples, (audio_must_be_converted ? AV_SAMPLE_FMT_S16 : acodec_ctx->sample_fmt), 0);
					if (audio_must_be_converted) swr_convert(swr_ctx, &buffer, movie_frame->nb_samples, (const uint8_t**)movie_frame->extended_data, movie_frame->nb_samples);
					else av_samples_copy(&buffer, movie_frame->extended_data, 0, 0, movie_frame->nb_samples, acodec_ctx->channels, acodec_ctx->sample_fmt);

					if (sound_buffer) {
						if (IDirectSoundBuffer_GetStatus(sound_buffer, &DSStatus) == DS_OK) {
							if (DSStatus != DSBSTATUS_BUFFERLOST) {
								if (IDirectSoundBuffer_Lock(sound_buffer, write_pointer, _size, &ptr1, &bytes1, &ptr2, &bytes2, 0)) error("update_movie_sample: couldn't lock sound buffer\n");
								memcpy(ptr1, buffer, bytes1);
								memcpy(ptr2, &buffer[bytes1], bytes2);
								if (IDirectSoundBuffer_Unlock(sound_buffer, ptr1, bytes1, ptr2, bytes2)) error("update_movie_sample: couldn't unlock sound buffer\n");
							}
						}

						write_pointer = (write_pointer + bytes1 + bytes2) % sound_buffer_size;
						av_freep(&buffer);
					}
				}
			}
		}

		av_packet_unref(&packet);
	}

	if(sound_buffer && first_audio_packet)
	{
		if(movie_sync_debug) info("audio start\n");

		// reset start time so video syncs up properly
		QueryPerformanceCounter((LARGE_INTEGER *)&start_time);
		if (IDirectSoundBuffer_GetStatus(sound_buffer, &DSStatus) == DS_OK) {
			if (DSStatus != DSBSTATUS_BUFFERLOST) {
				if (IDirectSoundBuffer_Play(sound_buffer, 0, 0, DSBPLAY_LOOPING)) error("update_movie_sample: couldn't play sound buffer\n");
			}
		}
		
		first_audio_packet = false;
	}

	movie_frame_counter++;

	// could not read any more frames, exhaust video buffer then end movie
	if(ret < 0)
	{
		if(vbuffer_write != vbuffer_read)
		{
			if(use_bgra_texture) draw_bgra_frame(vbuffer_read);
			else draw_yuv_frame(vbuffer_read, codec_ctx->color_range == AVCOL_RANGE_JPEG);

			vbuffer_read = (vbuffer_read + 1) % VIDEO_BUFFER_SIZE;
		}
		
		if(vbuffer_write == vbuffer_read) return false;
	}

	// wait for the next frame
	do
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&now);
	} while(LAG < 0.0);

	// keep going
	return true;
}

// draw the current frame, don't update anything
void ffmpeg_draw_current_frame()
{
	if(use_bgra_texture) draw_bgra_frame((vbuffer_read - 1) % VIDEO_BUFFER_SIZE);
	else draw_yuv_frame((vbuffer_read - 1) % VIDEO_BUFFER_SIZE, codec_ctx->color_range == AVCOL_RANGE_JPEG);
}

// loop back to the beginning of the movie
void ffmpeg_loop()
{
	if(format_ctx) avformat_seek_file(format_ctx, -1, 0, 0, 0, 0);
}

// get the current frame number
uint ffmpeg_get_movie_frame()
{
	if(movie_fps != 15.0 && movie_fps < 100.0) return (uint)ceil(movie_frame_counter * 15.0 / movie_fps);
	else return movie_frame_counter;
}

#if defined(__cplusplus)
}
#endif
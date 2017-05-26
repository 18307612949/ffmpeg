/**
 * ע��---------------------�����񲩿ʹ���ѧϰ---------------------
 
 
 * ��򵥵Ļ���FFmpeg����Ƶ������ 2 
 * Simplest FFmpeg Audio Player 2 
 * ������ʵ������Ƶ�Ľ���Ͳ��š�
 * ����򵥵�FFmpeg��Ƶ���뷽��Ľ̡̳�
 * ͨ��ѧϰ�����ӿ����˽�FFmpeg�Ľ������̡�
 *
 * �ð汾ʹ��SDL 2.0�滻�˵�һ���汾�е�SDL 1.0��
 * ע�⣺SDL 2.0����Ƶ�����API���ޱ仯��Ψһ�仯�ĵط�����
 * ��ص��������е�Audio Buffer��û����ȫ��ʼ������Ҫ�ֶ���ʼ����
 * �������м�SDL_memset(stream, 0, len);
 *
 * This software decode and play audio streams.
 * Suitable for beginner of FFmpeg.
 *
 * This version use SDL 2.0 instead of SDL 1.2 in version 1
 * Note:The good news for audio is that, with one exception, 
 * it's entirely backwards compatible with 1.2.
 * That one really important exception: The audio callback 
 * does NOT start with a fully initialized buffer anymore. 
 * You must fully write to the buffer in all cases. In this 
 * example it is SDL_memset(stream, 0, len);
 *
 * Version 2.0
 
////���ṹ���ϵ��blog.csdn.net/leixiaohua1020/article/details/11693997 
////Ĭ��֧�ֺ�264 ��MP3���룬 �������Ӧ���֧��
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "SDL2/SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>
#ifdef __cplusplus
};
#endif
#endif

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

//Output PCM
#define OUTPUT_PCM 1
//Use SDL
#define USE_SDL 1

//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8  *audio_chunk; 
static  Uint32  audio_len; 
static  Uint8  *audio_pos; 

/* The audio function callback takes the following parameters: 
 * stream: A pointer to the audio buffer to be filled 
 * len: The length (in bytes) of the audio buffer 
*/ 
void  fill_audio(void *udata,Uint8 *stream,int len){ 
	//SDL 2.0
	//
	SDL_memset(stream, 0, len);
	//��һ�ν���ȴ��������������룬
	if(audio_len==0)
		return; 

	len=(len>audio_len?audio_len:len);	/*  Mix  as  much  data  as  possible  */ 
    //��pcm�봫��sdl�ռ� ����δ���
	SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
	//��ַƫ��
	audio_pos += len; 
	//�����Լ� 
	audio_len -= len; 
	
} 
//-----------------


int main(int argc, char* argv[])
{   
    // ���װ������AVFormatContext��һ���ᴩʼ�յ����ݽṹ���ܶຯ����Ҫ�õ�����Ϊ������
	//����FFMPEG���װ��flv��mp4��rmvb��avi�����ܵĽṹ��
	AVFormatContext	*pFormatCtx;
	int				i, audioStream; 
	//�����������
	AVCodecContext	*pCodecCtx;
	//�����
	AVCodec			*pCodec; 
	//��-����ǰ������
	AVPacket		*packet;
	//
	uint8_t			*out_buffer; 
	//֡-����������
	AVFrame			*pFrame; 
	//sdl�����ṹ��
	SDL_AudioSpec wanted_spec;
	
    int ret;
	uint32_t len = 0;
	int got_picture;
	int index = 0;
	int64_t in_channel_layout;  
	//�ز����ṹ��
	struct SwrContext *au_convert_ctx;

	FILE *pFile=NULL;
	char url[]="xiaoqingge.mp3";
    //����codecע��
	av_register_all();
	//Э����ʼ��
	avformat_network_init();
	//��װ�������ľ����ʼ��
	pFormatCtx = avformat_alloc_context();
	//���װ��ʼ���Open
	if(avformat_open_input(&pFormatCtx,url,NULL,NULL)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
	// Retrieve stream information 
	//�ú������Զ�ȡһ��������Ƶ���ݲ��һ��һЩ��ص���Ϣ
	if(avformat_find_stream_info(pFormatCtx,NULL)<0){
		printf("Couldn't find stream information.\n");
		return -1;
	}
	// Dump valid information onto standard error ����Ч��Ϣת������׼���� 
	av_dump_format(pFormatCtx, 0, url, false);

	// Find the first audio stream
	audioStream=-1; 
	//��λ��һ����Ƶ֡�����û�б���
	for(i=0; i < pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) 
		{
			audioStream=i;
			break;
		}

	if(audioStream==-1){
		printf("Didn't find a audio stream.\n");
		return -1;
	}

	// Get a pointer to the codec context for the audio stream  
	// �õ�����������ĵĽṹָ��
	pCodecCtx=pFormatCtx->streams[audioStream]->codec;

	// Find the decoder for the audio stream  
    //��ȡ�����������
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL){
		printf("Codec not found.\n");
		return -1;
	}

	// Open codec
    //�򿪱������
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
		printf("Could not open codec.\n");
		return -1;
	}

	
#if OUTPUT_PCM
	pFile=fopen("output.pcm", "wb");
#endif

	packet=(AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(packet);

	//Out Audio Param
	uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
	//nb_samples: AAC-1024 MP3-1152
	//֡��С
	int out_nb_samples=pCodecCtx->frame_size; 
    //�������Ϊ��ƽ���ʽ λ��16λ
	AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
	int out_sample_rate=44100;
	int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
	//Out Buffer Size ��ȡ�������һ֡�����С
	int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);
    //������󻺳�֡
	out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
	//������װ������ṹ
	pFrame=av_frame_alloc();
//SDL------------------
#if USE_SDL
	//Init
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	}
	//SDL_AudioSpec
	wanted_spec.freq = out_sample_rate; 
	wanted_spec.format = AUDIO_S16SYS; 
	wanted_spec.channels = out_channels; 
	wanted_spec.silence = 0; 
	wanted_spec.samples = out_nb_samples; 
	wanted_spec.callback = fill_audio; 
	wanted_spec.userdata = pCodecCtx; 
     //��
	if (SDL_OpenAudio(&wanted_spec, NULL)<0){ 
		printf("can't open audio.\n"); 
		return -1; 
	} 
#endif

	//FIX:Some Codec's Context Information is missing 
	//��ȡ���ͨ����
	in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
	//Swr
    //�ز����ṹ��
	au_convert_ctx = swr_alloc();
	//���ò���װ��
	au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,
		 in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL); 
	//��ʼ��
	swr_init(au_convert_ctx);

	//Play
	SDL_PauseAudio(0);

	while(av_read_frame(pFormatCtx, packet)>=0) 
	//��ȡһ����
	{   //��ǰ��Ϊ��Ƶ����
		if(packet->stream_index==audioStream)
		{   
	        //����
			ret = avcodec_decode_audio4( pCodecCtx, pFrame,&got_picture, packet);
			if ( ret < 0 ) {
                printf("Error in decoding audio frame.\n");
                return -1;
            }
			//ת��������-��ʵ����ƽ��ת��ƽ�棬sdl�ò���
			if ( got_picture > 0 ){
				swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);
#if 1
				printf("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);
#endif


#if OUTPUT_PCM
				//Write PCM
				fwrite(out_buffer, 1, out_buffer_size, pFile);
#endif
				index++;
			}

#if USE_SDL
            //�ȴ�һ֡����
			while(audio_len>0)//Wait until finish
				SDL_Delay(1); 

			//Set audio buffer (PCM data)
			audio_chunk = (Uint8 *) out_buffer; 
			//Audio buffer length
			audio_len =out_buffer_size;
			audio_pos = audio_chunk;

#endif
		}
		av_free_packet(packet);
	}

	swr_free(&au_convert_ctx);

#if USE_SDL
	SDL_CloseAudio();//Close SDL
	SDL_Quit();
#endif
	
#if OUTPUT_PCM
	fclose(pFile);
#endif
	av_free(out_buffer);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}



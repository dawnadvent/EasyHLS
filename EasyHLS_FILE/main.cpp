/*
	Copyright (c) 2013-2014 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.EasyDarwin.org
*/
#include <stdio.h>
#include <stdlib.h>
#include "EasyHLSAPI.h"
#include <Windows.h>
#include <process.h>

//Add by SwordTwelve
//Now we support MP4 file to hls
#include "./MP4Demux/Head.h"

#ifdef _WIN32
#include "getopt.h"
#define KEY "333565546A4969576B5A754144474A636F35337A4A65394659584E355345785458305A4A544555755A58686C6B46634D5671442F70654E4659584E355247467964326C755647566862556C7A5647686C516D567A644541794D4445345A57467A65513D3D"
#else
#include "unistd.h"
#define KEY "333565546A4A4F576B596F41753242636F35394570664E6C59584E356147787A58325A706247564A567778576F502B6C3430566863336C4559584A33615735555A57467453584E55614756435A584E30514449774D54686C59584E35"
#endif

char*	ProgName;	
int		ConfigPlayListCapacity	= 4;
int		ConfigAllowCache		= 0;
int		ConfigM3U8Version		= 3;
int		ConfigTargetDuration	= 4;
char*	ConfigHLSRootDir		= "./";
char*	ConfigHLSessionName		= "easyhls_file";
char*	ConfigHttpRootUrl		= "http://127.0.0.1/";

Easy_HLS_Handle		fHLSHandle	= 0;

void PrintUsage()
{
	printf("Usage:\n");
	printf("------------------------------------------------------\n");
	printf("%s [-c <PlayListCapacity> -C <AllowCache> -v <M3U8Version> -t <TargetDuration> -d <HLSRootDir> -n <HLSessionName> -U <HttpRootUrl>]\n", ProgName);
	printf("Help Mode:   %s -h \n", ProgName );
	printf("For example: %s -c 4 -C 0 -v 3 -t 4 -d ./ -n easyhls_rtsp -u rtsp://admin:admin@anfengde.f3322.org/22 -U http://www.easydarwin.org/easyhls/\n", ProgName); 
	printf("------------------------------------------------------\n");
}

#ifndef HANDLE
#define HANDLE void*
#endif//HANDLE

#define MAX_TRACK_NUM 32

//Globle Func for thread callback
unsigned int _stdcall  VideoThread(void* lParam);
unsigned int _stdcall  AudioThread(void* lParam);

//
// Globle var for all of us to use
// 
// Handler use for thrack demux thread
HANDLE g_mp4TrackThread[MAX_TRACK_NUM];
bool  g_bThreadLiving[MAX_TRACK_NUM];
//获取MP4头box信息
CMp4_root_box g_root;
FILE * g_fin = NULL; 
FILE* g_finA = NULL;
Easy_Pusher_Handle g_fPusherHandle = 0;
CRITICAL_SECTION m_cs;
bool g_bVideoStart = false;
unsigned char	 g_sps_pps_header[200];
int g_nSpsppsHeaderLength = 0;
CMp4_avcC_box videoInfo;
CMp4_mp4a_box audioInfo;

#define VEDIO_PUSH 0
#define AUDIO_PUSH 1

typedef struct _SYN_CLOCK_CTRL_
{
	unsigned long ClockBase;
	unsigned long ClockCurr;
	unsigned long VedioBase;
	unsigned long AudioBase;

}Sync_clock_Ctl;

Sync_clock_Ctl g_clock;

// Add by Ricky
//Audio and video Sync lock
unsigned long Sync_clock(unsigned long TimeScale, unsigned long duration, int type, unsigned long* out)
{
	unsigned long timebase;
	unsigned long DiffClock;
	double TimeCalbase;
	double Timenext;
	unsigned long CurrentTime;
	unsigned long NextTime;
	unsigned long delay;
#ifdef _WIN32
	if(g_clock.ClockBase == 0)
	{
		g_clock.ClockBase = ::GetTickCount()*1000;

	}
	g_clock.ClockCurr = ::GetTickCount()*1000;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	g_clock.ClockCurr = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
	if(g_clock.ClockBase == 0)		{
		g_clock.ClockBase = g_clock.ClockCurr;
	}

#endif
	if(type == VEDIO_PUSH)
	{
		timebase = g_clock.VedioBase;
	}else
	{
		timebase = g_clock.AudioBase;	
	}

	DiffClock = g_clock.ClockCurr - g_clock.ClockBase;//时钟的耗时间Tick数//微妙级别忽略不计	
	TimeCalbase = (double)timebase/TimeScale;
	Timenext = (double)(timebase+duration)/TimeScale;
	//开始计算当前和小一个Sample的时间估计决定延迟//
	NextTime = (unsigned long)(Timenext*1000000);	
	CurrentTime = (unsigned long)(TimeCalbase*1000000);
	*out = CurrentTime;
	if(DiffClock > NextTime) //已经落后，快进
	{
		delay =  0;
	}else
	{
		delay = (NextTime- DiffClock);//重新计算时间
	}
	if(type == VEDIO_PUSH)
	{
		g_clock.VedioBase += duration;
	}else
	{
		g_clock.AudioBase  += duration;	
	}
	return delay;
}



int main(int argc, char * argv[])
{
	//////////////////////////////////////////////////////////////////////////
	/// Demux mp4 file , theoretically we  support almost all type of mp4 packaged file
	//////////////////////////////////////////////////////////////////////////

	std::string sTestFilm  = "./test.mp4";//[阳光电影www.ygdy8.com].港囧.HD.720p.国语中字.mp4";//6004501011.MP4";
	//std::string sTestFilm  = "D:\\360Downloads\\[阳光电影www.ygdy8.com].港囧.HD.720p.国语中字.mp4";//6004501011.MP4";
	//std::string sTestFilm  = "D:\\360Downloads\\Test.mp4";//6004501011.MP4";

	//Open mp4 file
	g_fin = _fsopen(sTestFilm.c_str(), "rb",  _SH_DENYNO );	
	if(g_fin == (FILE*)0)
	{
		printf("failed to open pmp4 file: %s\n", sTestFilm.c_str());
		printf("Press Enter exit...\n");
		getchar();

		return 0;
	}
	g_finA = _fsopen(sTestFilm.c_str(), "rb",  _SH_DENYNO );	
	if(g_finA == (FILE*)0)
	{
		printf("failed to open pmp4 file: %s\n", sTestFilm.c_str());
		printf("Press Enter exit...\n");
		getchar();

		return 0;
	}

	unsigned int cur_pos= _ftelli64(g_fin);
	for(;!feof(g_fin); )
	{
		_fseeki64(g_fin, cur_pos, SEEK_SET);
		printf("----------------------------------------level 0\n");
		cur_pos += g_root.mp4_read_root_box(g_fin);
	}

	printf("---------------------------------------- 0ye\n");
	printf("  %s   MP4Demux is Completed!\n", sTestFilm.c_str());
	printf("---------------------------------------- 0ye\n");

	//////////////////////////////////////////////////////////////////////////
	// Init EasyHLS
	//////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
	extern char* optarg;
#endif
	int ch;
	ProgName = argv[0];
	PrintUsage();
	while ((ch = getopt(argc,argv, "hc:C:v:t:d:n:u:U:")) != EOF) 
	{
		switch(ch)
		{
		case 'h':
			PrintUsage();
			return 0;
			break;
		case 'c':
			ConfigPlayListCapacity =atoi(optarg);
			break;
		case 'C':
			ConfigAllowCache =atoi(optarg);
			break;
		case 'v':
			ConfigM3U8Version =atoi(optarg);
			break;
		case 't':
			ConfigTargetDuration =atoi(optarg);
			break;
		case 'd':
			ConfigHLSRootDir =optarg;
			break;
		case 'n':
			ConfigHLSessionName =optarg;
			break;
		case 'U':
			ConfigHttpRootUrl =optarg;
			break;
		case '?':
			return 0;
			break;
		default:
			break;
		}
	}

	//从MP4文件获取音视频编码信息，填入pusher媒体信息结构中
	memset(&videoInfo, 0x00, sizeof(CMp4_avcC_box));
	memset(&audioInfo, 0x00, sizeof(CMp4_mp4a_box));
	int nVideoTrackId = get_video_info_in_moov(g_root,  videoInfo );
	int nAudioTrackId = get_audio_info_in_moov(g_root,  audioInfo );

	for(int nI=0; nI<MAX_TRACK_NUM; nI++)
	{
		g_mp4TrackThread[nI] = 0;
		g_bThreadLiving[nI] = false;
	}

	InitializeCriticalSection(&m_cs);

	//EasyHLS SDK需要首先经过激活才能继续调用
	if(0 != EasyHLS_Activate(KEY))
		return -1;

	//创建EasyHLS Session
	fHLSHandle = EasyHLS_Session_Create(ConfigPlayListCapacity, ConfigAllowCache, ConfigM3U8Version);

	char subDir[64] = { 0 };
	sprintf(subDir,"%s/",ConfigHLSessionName);
	EasyHLS_ResetStreamCache(fHLSHandle, ConfigHLSRootDir, subDir, ConfigHLSessionName, ConfigTargetDuration);

	printf("HLS URL:%s%s/%s.m3u8\n", ConfigHLSRootDir, ConfigHLSessionName, ConfigHLSessionName);

	//视频轨存在
	if (nVideoTrackId>-1)
	{

		if (videoInfo.sps->sequenceParameterSetNALUnit && videoInfo.sps->sequenceParameterSetLength>0 )
		{
			g_sps_pps_header[0]= 0x00;
			g_sps_pps_header[1]= 0x00;
			g_sps_pps_header[2]= 0x00;
			g_sps_pps_header[3]= 0x01;
			memcpy(g_sps_pps_header+4, videoInfo.sps->sequenceParameterSetNALUnit, videoInfo.sps->sequenceParameterSetLength);
			g_nSpsppsHeaderLength += videoInfo.sps->sequenceParameterSetLength+4;
		}
		if (videoInfo.pps->pictureParameterSetNALUnit && videoInfo.pps->pictureParameterSetLength>0 )
		{
			g_sps_pps_header[g_nSpsppsHeaderLength]= 0x00;
			g_sps_pps_header[g_nSpsppsHeaderLength+1]= 0x00;
			g_sps_pps_header[g_nSpsppsHeaderLength+2]= 0x00;
			g_sps_pps_header[g_nSpsppsHeaderLength+3]= 0x01;
			memcpy(g_sps_pps_header+g_nSpsppsHeaderLength+4, videoInfo.pps->pictureParameterSetNALUnit, videoInfo.pps->pictureParameterSetLength );
			g_nSpsppsHeaderLength += videoInfo.pps->pictureParameterSetLength+4;
		}

		//Create thread to push mp4 demux data( h264)
		g_mp4TrackThread[nVideoTrackId] = (HANDLE)_beginthreadex(NULL, 0, VideoThread, (void*)nVideoTrackId,0,0);
		g_bThreadLiving[nVideoTrackId] = true;
	}

	//音频轨存在
	if (nAudioTrackId>-1)
	{
		//Create thread to push mp4 demux data( aac)
		g_mp4TrackThread[nAudioTrackId] = (HANDLE)_beginthreadex(NULL, 0, AudioThread,  (void*)nAudioTrackId,0,0);
		g_bThreadLiving[nAudioTrackId] = true;
	}

	printf("Press Enter exit...\n");
	getchar();

	EasyHLS_Session_Release(fHLSHandle);
	fHLSHandle = 0;

	return 0;
}


//MP4 file pusher  calllback
unsigned int _stdcall  VideoThread(void* lParam)
{
	int nTrackId = (int)lParam;
	while (g_bThreadLiving[nTrackId])
	{
		g_bVideoStart = true;
		int chunk_offset_amount    = g_root.co[nTrackId].chunk_offset_amount;
		unsigned long lTimeStamp = 0;
		int nSampleId = 0;
		for(int chunk_index = 0 ; chunk_index < chunk_offset_amount; ++chunk_index)
		{
			if (!g_bThreadLiving[nTrackId])
			{
				return 0;
			}

			//copy_sample_data(g_fin, chunk_index, name,nID,root,nSampleId);
			_fseeki64(g_fin, g_root.co[nTrackId].chunk_offset_from_file_begin[chunk_index], SEEK_SET);

			//获取当前chunk中有多少个sample
			uint32_t sample_num_in_cur_chunk_ = get_sample_num_in_cur_chunk(g_root.sc[nTrackId], chunk_index+1);  //@a mark获取chunk中sample的数目
			uint32_t sample_index_ =  get_sample_index(g_root.sc[nTrackId], chunk_index+1);//chunk中第一个sample的序号
			unsigned int cur=_ftelli64(g_fin);
			for(int i = 0; i < sample_num_in_cur_chunk_; i++)
			{
				if (!g_bThreadLiving[nTrackId])
				{
					return 0;
				}
				// #ifdef _WIN32
				// 				DWORD dwStart = ::GetTickCount();
				// #endif
				uint32_t sample_size = get_sample_size(g_root.sz[nTrackId], sample_index_+i);//获取当前sample的大小
				uint32_t sample_time = get_sample_time(g_root.ts[nTrackId], nSampleId );
				//double dbSampleTime = (double)sample_time/g_root.trk[nTrackId].mdia.mdhd.timescale ;
				//uint32_t uSampleTime = dbSampleTime*1000000;

				EnterCriticalSection(&m_cs);
				uint32_t uSampleTime = Sync_clock(g_root.trk[nTrackId].mdia.mdhd.timescale, sample_time,VEDIO_PUSH, &lTimeStamp);
				LeaveCriticalSection(&m_cs);

				_fseeki64(g_fin,cur,SEEK_SET);
				unsigned char *ptr=new unsigned char [sample_size+200];
				fread(ptr, sample_size, 1, g_fin);

				//写一帧数据 --- 可以直接进行网络推送
				//fwrite(ptr, sample_size, 1, fout);

				ptr[0] = 0x00;
				ptr[1] = 0x00;
				ptr[2] = 0x00;
				ptr[3] = 0x01;
				unsigned char naltype = ( (unsigned char)ptr[4] & 0x1F);
				uint32_t nSampleLength = sample_size;
				unsigned int uiFrameType = 0;
				if (naltype == 0x05)//I frame
				{
					uiFrameType = TS_TYPE_PES_VIDEO_I_FRAME;
					//Add sps and pps header
					memmove(ptr+g_nSpsppsHeaderLength, ptr, sample_size);			
					memcpy(ptr, g_sps_pps_header, g_nSpsppsHeaderLength);
					nSampleLength = sample_size+g_nSpsppsHeaderLength;
				}
				else//P Frame
				{
					uiFrameType = TS_TYPE_PES_VIDEO_P_FRAME;
				}

				unsigned long long timestamp = lTimeStamp/1000*90;
				EasyHLS_VideoMux(fHLSHandle, uiFrameType, (unsigned char*)ptr, nSampleLength, timestamp, timestamp, timestamp);

				//lTimeStamp += uSampleTime;

				// #ifdef _WIN32
				// 
				// 				DWORD dwStop = ::GetTickCount();
				// #endif
				//printf("Sleep=%d\r\n", uSampleTime/1000-(dwStop-dwStart));
				if(uSampleTime!=0)
				{
#ifndef _WIN32
					usleep(uSampleTime);
#else
					SleepEx(uSampleTime/1000, FALSE);
#endif
				}
				delete [] ptr;
				cur+=nSampleLength;
				nSampleId++;
			}
		}
	}
	return 0;
}


unsigned int _stdcall  AudioThread(void* lParam)
{
	int nTrackId = (int)lParam;
	while (g_bThreadLiving[nTrackId])
	{
		if (!g_bVideoStart)
		{
			Sleep(1);
			printf("Audio Thread waiting.........\r\n");
			continue;
		}
		int chunk_offset_amount    = g_root.co[nTrackId].chunk_offset_amount;
		unsigned long lTimeStamp = 0;
		int nSampleId = 0;
		for(int chunk_index = 0 ; chunk_index < chunk_offset_amount; ++chunk_index)
		{
			if (!g_bThreadLiving[nTrackId])
			{
				return 0;
			}
			//copy_sample_data(g_fin, chunk_index, name,nID,root,nSampleId);
			_fseeki64(g_finA, g_root.co[nTrackId].chunk_offset_from_file_begin[chunk_index], SEEK_SET);

			//获取当前chunk中有多少个sample
			uint32_t sample_num_in_cur_chunk_ = get_sample_num_in_cur_chunk(g_root.sc[nTrackId], chunk_index+1);  //@a mark获取chunk中sample的数目
			uint32_t sample_index_ =  get_sample_index(g_root.sc[nTrackId], chunk_index+1);//chunk中第一个sample的序号
			unsigned int cur=_ftelli64(g_finA);
			for(int i = 0; i < sample_num_in_cur_chunk_; i++)
			{
				if (!g_bThreadLiving[nTrackId])
				{
					return 0;
				}

				// #ifdef _WIN32
				// 			DWORD dwStart = ::GetTickCount();
				// #endif
				uint32_t sample_size = get_sample_size(g_root.sz[nTrackId], sample_index_+i);//获取当前sample的大小
				uint32_t sample_time = get_sample_time(g_root.ts[nTrackId], nSampleId );
				//double dbSampleTime = (double)sample_time/g_root.trk[nTrackId].mdia.mdhd.timescale ;
				//uint32_t uSampleTime = dbSampleTime*1000000;

				EnterCriticalSection(&m_cs);
				uint32_t uSampleTime = Sync_clock(g_root.trk[nTrackId].mdia.mdhd.timescale, sample_time,AUDIO_PUSH, &lTimeStamp);
				LeaveCriticalSection(&m_cs);

				_fseeki64(g_finA,cur,SEEK_SET);
				unsigned char *ptr=new unsigned char [sample_size+7];
				fread(ptr, sample_size, 1, g_finA);

				//写一帧数据 --- 可以直接进行网络推送
				//fwrite(ptr, sample_size, 1, fout);

				//7字节
				//0xFFF 12bit同步
				//is_mp2 1bit(13) MPEG2(1) MPEG4(0)
				//layer  2bit 一般0x00
				//no_scr 1bit 一般0x01(若为0头部就不为7字节，有扩充字节)
				//profile 2bit(17-18) ObjectType,4种(0~3对应实际MAIN(1) LOW(2) SSR(3) LTP(4))
				//sr_idx  4bit(19-22) 采样索引
				//privatestream 1bit 一般0x00
				//nb_ch      3bit(24-26)  声道数
				//????       4bit  一般0x00
				//frame_size 13bit(31-43) 数据部分长度含头部7个字节
				//////////////////////////////////////////////////////////////////////////
				//ADTS 头中相对有用的信息 采样率、声道数、帧长度
				//adts头
				//typedef struct
				//{
				//	unsigned int syncword;  //12 bslbf 同步字The bit string ‘1111 1111 1111’，说明一个ADTS帧的开始
				//	unsigned int id;        //1 bslbf   MPEG 标示符, 设置为1
				//	unsigned int layer;     //2 uimsbf Indicates which layer is used. Set to ‘00’
				//	unsigned int protection_absent;  //1 bslbf  表示是否误码校验


				//	unsigned int profile;            //2 uimsbf  表示使用哪个级别的AAC，如01 Low Complexity(LC)--- AACLC
				//	unsigned int sf_index;           //4 uimsbf  表示使用的采样率下标
				//	unsigned int private_bit;        //1 bslbf 
				//	unsigned int channel_configuration;  //3 uimsbf  表示声道数
				//	unsigned int original;               //1 bslbf 
				//	unsigned int home;                   //1 bslbf 
				//	/*下面的为改变的参数即每一帧都不同*/
				//	unsigned int copyright_identification_bit;   //1 bslbf 
				//	unsigned int copyright_identification_start; //1 bslbf

				//	unsigned int aac_frame_length;               // 13 bslbf  一个ADTS帧的长度包括ADTS头和raw data block
				//	unsigned int adts_buffer_fullness;           //11 bslbf     0x7FF 说明是码率可变的码流
				//
				//	/*no_raw_data_blocks_in_frame 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧.
				//	所以说number_of_raw_data_blocks_in_frame == 0 
				//	表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
				//    */
				//	unsigned int no_raw_data_blocks_in_frame;    //2 uimsfb
				//} ADTS_HEADER;

				//•0: 96000 Hz
				//•1: 88200 Hz
				//•2: 64000 Hz
				//•3: 48000 Hz
				//•4: 44100 Hz
				//•5: 32000 Hz
				//•6: 24000 Hz
				//•7: 22050 Hz
				//•8: 16000 Hz
				//•9: 12000 Hz
				//•10: 11025 Hz
				//•11: 8000 Hz
				//•12: 7350 Hz
				//•13: Reserved
				//•14: Reserved
				//•15: frequency is written explictly

				//•0: Defined in AOT Specifc Config
				//•1: 1 channel: front-center
				//•2: 2 channels: front-left, front-right
				//•3: 3 channels: front-center, front-left, front-right
				//•4: 4 channels: front-center, front-left, front-right, back-center
				//•5: 5 channels: front-center, front-left, front-right, back-left, back-right
				//•6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
				//•7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
				//•8-15: Reserved
				//////////////////////////////////////////////////////////////////////////

				uint32_t uSampleSize = sample_size;
				
				if (ptr[0] == 0xFF && ptr[1]&0xF0 == 0xF0)
				{
				}
				else//if there is not adt , I will try to  add adts header
				{
					static unsigned long tnsSupportedSamplingRates[16] = //音频采样率标准，下表为写入标志
					{ 96000, 88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0 };
					unsigned char  adts_header[7] ;
					memset(adts_header, 0x00, 7*sizeof(unsigned char));
					adts_header[0] = 0xFF;
					adts_header[1] = 0xF1;//MPEG4(0) ==0xF1   MPEG2(1) == 0xF9
					unsigned char samplerate_t = 0;
					unsigned char channelcount_t = 0;
					unsigned int  aac_frame_length = sample_size + 7;
					unsigned int num_data_block = sample_size/1024;
					unsigned int sample_rate_index = 0xc;//Reserved
					unsigned int channels = audioInfo.channelcount;

					// 注意：因为我还没法获取到AAC编码的等级，所以这里默认为AAC-LC，也许你应该知道这个值是多少，从而填在这里--! [4/10/2016 SwordTwelve]
					//obj_type,   4种(0~3对应实际MAIN(1) LOW(2) SSR(3) LTP(4))
					 unsigned int obj_type = 1;
					int nI= 0;
					for ( nI = 0; nI<13; nI++)
					{
						if (tnsSupportedSamplingRates[nI] == audioInfo.samplerate )
						{
							sample_rate_index =nI; 
							break;
						}
					}
// 					/* Object type over first 2 bits */
// 					adts_header[2] = obj_type << 6;//
// 					/* rate index over next 4 bits */
// 					adts_header[2] |= (sample_rate_index << 2);
// 					/* channels over last 2 bits */
// 					adts_header[2] |= (channels & 0x4) >> 2;
// 					/* channels continued over next 2 bits + 4 bits at zero */
// 					adts_header[3] = (channels & 0x3) << 6;
// 					/* frame size over last 2 bits */
// 					adts_header[3] |= (sample_size & 0x1800) >> 11;
// 					adts_header[4] = (sample_size & 0x1FF8) >> 3;
// 					/* frame size continued first 3 bits */
// 					adts_header[5] = (sample_size & 0x7) << 5;
// 					/* buffer fullness (0x7FF for VBR) over 5 last bits*/
// 					adts_header[5] |= 0x1F;
// 					/* buffer fullness (0x7FF for VBR) continued over 6 first bits + 2 zeros
// 					* number of raw data blocks */
// 					adts_header[6] = 0xFC;// one raw data blocks .
// 					adts_header[6] |= num_data_block & 0x03; //Set raw Data blocks.

					/* Object type over first 2 bits */
					adts_header[2] = obj_type << 6;//
					/* rate index over next 4 bits */
					adts_header[2] |= (sample_rate_index << 2);
					/* channels over last 2 bits */
					adts_header[2] |= (channels & 0x4) >> 2;
					adts_header[3] = (channels & 0x3) << 6;
					adts_header[3] |= (aac_frame_length & 0x1800) >> 11;
					adts_header[4] = (aac_frame_length & 0x7F8) >> 3;
					adts_header[5] = (aac_frame_length & 0x7) << 5  |  0x1F;
					adts_header[6] = 0xFC  | num_data_block & 0x03; //Set raw Data blocks.;

					uSampleSize = sample_size+7;
					memmove(ptr+7, ptr, sample_size);
					memcpy(ptr, adts_header, 7 );
				}
				unsigned long long timestamp = lTimeStamp/1000*90;
				EasyHLS_AudioMux(fHLSHandle, (unsigned char*)ptr, uSampleSize, timestamp, timestamp);

				//lTimeStamp += uSampleTime;
				// #ifdef _WIN32
				// 				DWORD dwStop = ::GetTickCount();
				// #endif
				if(uSampleTime!=0)
				{
#ifndef _WIN32
					usleep(uSampleTime);
#else
					SleepEx(uSampleTime/1000, FALSE);
#endif
				}
				delete [] ptr;
				cur+=sample_size;
				nSampleId++;
			}
		}
	}

	return 0;
}



// //the old main func 
// int main(int argc, char * argv[])
// {
// #ifdef _WIN32
// 	extern char* optarg;
// #endif
// 	int ch;
// 	ProgName = argv[0];
// 	PrintUsage();
// 	while ((ch = getopt(argc,argv, "hc:C:v:t:d:n:u:U:")) != EOF) 
// 	{
// 		switch(ch)
// 		{
// 		case 'h':
// 			PrintUsage();
// 			return 0;
// 			break;
// 		case 'c':
// 			ConfigPlayListCapacity =atoi(optarg);
// 			break;
// 		case 'C':
// 			ConfigAllowCache =atoi(optarg);
// 			break;
// 		case 'v':
// 			ConfigM3U8Version =atoi(optarg);
// 			break;
// 		case 't':
// 			ConfigTargetDuration =atoi(optarg);
// 			break;
// 		case 'd':
// 			ConfigHLSRootDir =optarg;
// 			break;
// 		case 'n':
// 			ConfigHLSessionName =optarg;
// 			break;
// 		case 'U':
// 			ConfigHttpRootUrl =optarg;
// 			break;
// 		case '?':
// 			return 0;
// 			break;
// 		default:
// 			break;
// 		}
// 	}
// 
//     int buf_size = 1024*512;
//     char *pbuf = (char *) malloc(buf_size);
//     FILE *fES = NULL;
// 	int position = 0;
// 	int iFrameNo = 0;
// 	int timestamp = 0;
// 
//     fES = fopen("./EasyPusher.264", "rb");
//     if (NULL == fES)        return 0;
// 
// 	//EasyHLS SDK需要首先经过激活才能继续调用
// 	if(0 != EasyHLS_Activate(KEY))
// 		return -1;
// 
// 	//创建EasyHLS Session
// 	fHLSHandle = EasyHLS_Session_Create(ConfigPlayListCapacity, ConfigAllowCache, ConfigM3U8Version);
// 
// 	char subDir[64] = { 0 };
// 	sprintf(subDir,"%s/",ConfigHLSessionName);
// 	EasyHLS_ResetStreamCache(fHLSHandle, ConfigHLSRootDir, subDir, ConfigHLSessionName, ConfigTargetDuration);
// 
// 	printf("HLS URL:%s%s/%s.m3u8\n", ConfigHLSRootDir, ConfigHLSessionName, ConfigHLSessionName);
// 
// 	while (1)
// 	{
// 		int nReadBytes = fread(pbuf+position, 1, 1, fES);
// 		if (nReadBytes < 1)
// 		{
// 			if (feof(fES))
// 			{
// 				position = 0;
// 				fseek(fES, 0, SEEK_SET);
// 				continue;
// 			}
// 			break;
// 		}
// 
// 		position ++;
// 
// 		if (position > 5)
// 		{
// 			unsigned char naltype = ( (unsigned char)pbuf[position-1] & 0x1F);
// 
// 			if (	(unsigned char)pbuf[position-5]== 0x00 && 
// 					(unsigned char)pbuf[position-4]== 0x00 && 
// 					(unsigned char)pbuf[position-3] == 0x00 &&
// 					(unsigned char)pbuf[position-2] == 0x01 &&
// 					(naltype == 0x07 || naltype == 0x01 ) )
// 			{
// 				int framesize = position - 5;
// 				naltype = (unsigned char)pbuf[4] & 0x1F; 
// 				timestamp += 1000/25;
// 
// 				unsigned int uiFrameType = 0;
// 				if (naltype == 0x07)
// 				{
// 					uiFrameType = TS_TYPE_PES_VIDEO_I_FRAME;
// 				}
// 				else
// 				{
// 					uiFrameType = TS_TYPE_PES_VIDEO_P_FRAME;
// 				}
// 
// 				EasyHLS_VideoMux(fHLSHandle, uiFrameType, (unsigned char*)pbuf, framesize, timestamp*90, timestamp*90, timestamp*90);
// 
// 		#ifndef _WIN32
// 						usleep(30*1000);
// 		#else
// 						Sleep(30);
// 		#endif
// 
// 				memmove(pbuf, pbuf+position-5, 5);
// 				position = 5;
// 
// 				iFrameNo ++;
// 			}
// 		}
// 	}
// 
//     printf("Press Enter exit...\n");
//     getchar();
// 
//     EasyHLS_Session_Release(fHLSHandle);
//     fHLSHandle = 0;
// 	free(pbuf);
//   
// 	return 0;
// }
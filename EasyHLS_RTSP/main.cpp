/*
	Copyright (c) 2013-2014 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.EasyDarwin.org
*/
#include <stdio.h>
#include <stdlib.h>
#include "EasyHLSAPI.h"
#include "EasyRTSPClientAPI.h"

#ifdef _WIN32
#include "getopt.h"
#define KEY "333565546A4969576B5A734132786459702B676A6B75394659584E355345785458314A55553141755A58686C4B56634D5671442B6B75424859585A7062695A4359574A76633246414D6A41784E6B566863336C4559584A33615735555A5746745A57467A65513D3D"
#define EasyRTSPClient_KEY "6A59754D6A3469576B5A734132786459702B676A6B75394659584E355345785458314A55553141755A58686C4931634D5671442B6B75424859585A7062695A4359574A76633246414D6A41784E6B566863336C4559584A33615735555A5746745A57467A65513D3D"
#else
#include "unistd.h"
#define KEY "333565546A4A4F576B596F4132786459702B676A6B764E6C59584E356147787A58334A306333432B567778576F50365334456468646D6C754A6B4A68596D397A595541794D4445325257467A65555268636E6470626C526C5957316C59584E35"
#define EasyRTSPClient_KEY "6A59754D6A354F576B596F4132786459702B676A6B764E6C59584E356147787A58334A3063334345567778576F50365334456468646D6C754A6B4A68596D397A595541794D4445325257467A65555268636E6470626C526C5957316C59584E35"
#endif

char*	ProgName;	
char*	ConfigRTSPURL			= "rtsp://admin:12345@192.168.70.210";
int		ConfigPlayListCapacity	= 4;
int		ConfigAllowCache		= 0;
int		ConfigM3U8Version		= 3;
int		ConfigTargetDuration	= 10;
int		ConfigFirtTSIFrameCount = 1;
char*	ConfigHLSRootDir		= "./";
char*	ConfigHLSessionName		= "easyhls_rtsp";
char*	ConfigHttpRootUrl		= "http://127.0.0.1/";

Easy_HLS_Handle		fHLSHandle	= 0;
Easy_RTSP_Handle	fRTSPHandle = 0;

/* RTSPClient获取数据后回调给上层 */
int Easy_APICALL __RTSPClientCallBack( int _chid, void *_chPtr, int _frameType, char *_pBuf, RTSP_FRAME_INFO *_frameInfo)
{
	if(NULL == fHLSHandle) return -1;

	if (_frameType == EASY_SDK_VIDEO_FRAME_FLAG)
	{
		unsigned long long llPTS = (_frameInfo->timestamp_sec%1000000)*1000 + _frameInfo->timestamp_usec/1000;	

		printf("Get %s Video \tLen:%d \ttm:%u.%u \t%u\n",_frameInfo->type==EASY_SDK_VIDEO_FRAME_I?"I":"P", _frameInfo->length, _frameInfo->timestamp_sec, _frameInfo->timestamp_usec, llPTS);

		unsigned int uiFrameType = 0;
		if (_frameInfo->type == EASY_SDK_VIDEO_FRAME_I)
		{
			uiFrameType = TS_TYPE_PES_VIDEO_I_FRAME;
		}
		else if (_frameInfo->type == EASY_SDK_VIDEO_FRAME_P)
		{
			uiFrameType = TS_TYPE_PES_VIDEO_P_FRAME;
		}

		EasyHLS_VideoMux(fHLSHandle, uiFrameType, (unsigned char*)_pBuf, _frameInfo->length, llPTS*90, llPTS*90, llPTS*90);
	}
	else if (_frameType == EASY_SDK_AUDIO_FRAME_FLAG)
	{

		unsigned long long llPTS = (_frameInfo->timestamp_sec%1000000)*1000 + _frameInfo->timestamp_usec/1000;	

		printf("Get Audio \tLen:%d \ttm:%u.%u \t%u\n", _frameInfo->length, _frameInfo->timestamp_sec, _frameInfo->timestamp_usec, llPTS);

		if (_frameInfo->codec == EASY_SDK_AUDIO_CODEC_AAC)
		{
			EasyHLS_AudioMux(fHLSHandle, (unsigned char*)_pBuf, _frameInfo->length, llPTS*90, llPTS*90);
		}
	}
	else if (_frameType == EASY_SDK_EVENT_FRAME_FLAG)
	{
		if (NULL == _pBuf && NULL == _frameInfo)
		{
			printf("Connecting:%s ...\n", ConfigRTSPURL);
		}
		else if (NULL!=_frameInfo && _frameInfo->type==0xF1)
		{
			printf("Lose Packet:%s ...\n", ConfigRTSPURL);
		}
	}

	return 0;
}
void PrintUsage()
{
	printf("Usage:\n");
	printf("------------------------------------------------------\n");
	printf("%s [-c <PlayListCapacity> -C <AllowCache> -v <M3U8Version> -t <TargetDuration> -i <FirstTSIFrameCount> -d <HLSRootDir> -n <HLSessionName> -u <RTSPURL> -U <HttpRootUrl>]\n", ProgName);
	printf("Help Mode:   %s -h \n", ProgName );
	printf("For example: %s -c 4 -C 0 -v 3 -t 4 -i 3 -d ./ -n easyhls_rtsp -u rtsp://admin:admin@anfengde.f3322.org/22 -U http://www.easydarwin.org/easyhls/\n", ProgName); 
	printf("------------------------------------------------------\n");
}
int main(int argc, char * argv[])
{
#ifdef _WIN32
	extern char* optarg;
#endif
	int ch;
	ProgName = argv[0];
	PrintUsage();
	while ((ch = getopt(argc,argv, "hc:C:v:t:i:d:n:u:U:")) != EOF) 
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
		case 'i':
			ConfigFirtTSIFrameCount = atoi(optarg);
			break;
		case 'd':
			ConfigHLSRootDir =optarg;
			break;
		case 'n':
			ConfigHLSessionName =optarg;
			break;
		case 'u':
			ConfigRTSPURL =optarg;
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

	int isEasyHLSActivated = EasyHLS_Activate(KEY);
	switch(isEasyHLSActivated)
	{
	case EASY_ACTIVATE_INVALID_KEY:
		printf("EasyHLS_KEY is EASY_ACTIVATE_INVALID_KEY!\n");
		break;
	case EASY_ACTIVATE_TIME_ERR:
		printf("EasyHLS_KEY is EASY_ACTIVATE_TIME_ERR!\n");
		break;
	case EASY_ACTIVATE_PROCESS_NAME_LEN_ERR:
		printf("EasyHLS_KEY is EASY_ACTIVATE_PROCESS_NAME_LEN_ERR!\n");
		break;
	case EASY_ACTIVATE_PROCESS_NAME_ERR:
		printf("EasyHLS_KEY is EASY_ACTIVATE_PROCESS_NAME_ERR!\n");
		break;
	case EASY_ACTIVATE_VALIDITY_PERIOD_ERR:
		printf("EasyHLS_KEY is EASY_ACTIVATE_VALIDITY_PERIOD_ERR!\n");
		break;
	case EASY_ACTIVATE_SUCCESS:
		printf("EasyHLS_KEY is EASY_ACTIVATE_SUCCESS!\n");
		break;
	}

	if(EASY_ACTIVATE_SUCCESS != isEasyHLSActivated)
		return -1;

	if( EASY_ACTIVATE_SUCCESS != EasyRTSP_Activate(EasyRTSPClient_KEY))
		return -1;

	//创建EasyRTSPClient
	EasyRTSP_Init(&fRTSPHandle);
	if (NULL == fRTSPHandle) return 0;

	unsigned int mediaType = EASY_SDK_VIDEO_FRAME_FLAG | EASY_SDK_AUDIO_FRAME_FLAG;
	
	//设置数据回调处理
	EasyRTSP_SetCallback(fRTSPHandle, __RTSPClientCallBack);
	//打开RTSP流
	EasyRTSP_OpenStream(fRTSPHandle, 0, ConfigRTSPURL, EASY_RTP_OVER_TCP, mediaType, 0, 0, NULL, 1000, 0, 0x01, 1);

	//创建EasyHLS Session
	fHLSHandle = EasyHLS_Session_Create(ConfigPlayListCapacity, ConfigAllowCache, ConfigM3U8Version);

	char subDir[64] = { 0 };
	sprintf(subDir,"%s/",ConfigHLSessionName);
	EasyHLS_ResetStreamCache(fHLSHandle, ConfigHLSRootDir, subDir, ConfigHLSessionName, ConfigTargetDuration, ConfigFirtTSIFrameCount);

	printf("HLS URL:%s%s/%s.m3u8\n", ConfigHLSRootDir, ConfigHLSessionName, ConfigHLSessionName);

    printf("Press Enter exit...\n");
    getchar();

    EasyHLS_Session_Release(fHLSHandle);
    fHLSHandle = 0;
   
	EasyRTSP_CloseStream(fRTSPHandle);
	EasyRTSP_Deinit(&fRTSPHandle);
	fRTSPHandle = NULL;

    return 0;
}

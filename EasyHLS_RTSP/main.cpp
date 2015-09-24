/*
	Copyright (c) 2013-2014 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.EasyDarwin.org
*/
#include <stdio.h>
#include "EasyHLSAPI.h"
#include "EasyRTSPClientAPI.h"

#define RTSPURL "rtsp://admin:admin@anfengde.f3322.org/"

#define PLAYLIST_CAPACITY	4
#define	ALLOW_CACHE			false
#define	M3U8_VERSION		3
#define TARGET_DURATION		4
#define HLS_ROOT_DIR		"./"
#define HLS_SESSION_NAME	"easyhls_rtsp"
#define HTTP_ROOT_URL		"http://www.easydarwin.org/easyhls/"

Easy_HLS_Handle fHLSHandle = 0;
Easy_RTSP_Handle fRTSPHandle = 0;

/* RTSPClient获取数据后回调给上层 */
int Easy_APICALL __RTSPClientCallBack( int _chid, int *_chPtr, int _frameType, char *_pBuf, RTSP_FRAME_INFO *_frameInfo)
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
			printf("Connecting:%s ...\n", RTSPURL);
		}
		else if (NULL!=_frameInfo && _frameInfo->type==0xF1)
		{
			printf("Lose Packet:%s ...\n", RTSPURL);
		}
	}

	return 0;
}

int main()
{
	//创建NVSource
	EasyRTSP_Init(&fRTSPHandle);
	if (NULL == fRTSPHandle) return 0;

	unsigned int mediaType = EASY_SDK_VIDEO_FRAME_FLAG | EASY_SDK_AUDIO_FRAME_FLAG;
	
	//设置数据回调处理
	EasyRTSP_SetCallback(fRTSPHandle, __RTSPClientCallBack);
	//打开RTSP流
	EasyRTSP_OpenStream(fRTSPHandle, 0, RTSPURL, RTP_OVER_TCP, mediaType, 0, 0, NULL, 1000, 0);

	//创建EasyHLS Session
	fHLSHandle = EasyHLS_Session_Create(PLAYLIST_CAPACITY, ALLOW_CACHE, M3U8_VERSION);

	char subDir[64] = { 0 };
	sprintf(subDir,"%s/",HLS_SESSION_NAME);
	EasyHLS_ResetStreamCache(fHLSHandle, HLS_ROOT_DIR, subDir, HLS_SESSION_NAME, TARGET_DURATION);

	printf("HLS URL:%s%s/%s.m3u8\n", HTTP_ROOT_URL, HLS_SESSION_NAME, HLS_SESSION_NAME);

    printf("Press Enter exit...\n");
    getchar();

    EasyHLS_Session_Release(fHLSHandle);
    fHLSHandle = 0;
   
	EasyRTSP_CloseStream(fRTSPHandle);
	EasyRTSP_Deinit(&fRTSPHandle);
	fRTSPHandle = NULL;

    return 0;
}
/*
	Copyright (c) 2013-2014 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.EasyDarwin.org
*/
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include "getopt.h"
#define KEY "333565546A4969576B5A75415A6574627042535A792F424659584E355345785458314E455379356C65475658444661672F30766A5257467A65555268636E6470626C526C5957314A6331526F5A554A6C633352414D6A41784F47566863336B3D"
#else
#include "unistd.h"
#define KEY "333565546A4A4F576B59714144474A636F35337A4A66526C59584E356147787A58334E6B6131634D5671442F70654E4659584E355247467964326C755647566862556C7A5647686C516D567A644541794D4445345A57467A65513D3D"
#endif
#include "EasyHLSAPI.h"

#include "hi_type.h"
#include "hi_net_dev_sdk.h"
#include "hi_net_dev_errors.h"

HI_U32 u32Handle = 0;
char* ProgName;	
int ConfigPlayListCapacity=4;
int ConfigAllowCache=0;
int ConfigM3U8Version=3;
int ConfigTargetDuration=4;
char* ConfigHLSRootDir="./";
char* ConfigHLSessionName="camera";
char* ConfigHttpRootUrl="http://www.easydarwin.org/easyhls/";
char* ConfigUName	= "admin";			//SDK UserName
char* ConfigPWD		= "admin";			//SDK Password
char* ConfigDHost	= "192.168.66.189";	//SDK Host
char* ConfigDPort	= "80";				//SDK Port

Easy_HLS_Handle fHlsHandle = 0;


HI_S32 OnEventCallback(HI_U32 u32Handle, /* ��� */
                                HI_U32 u32Event,      /* �¼� */
                                HI_VOID* pUserData  /* �û�����*/
                                )
{
	return HI_SUCCESS;
}


HI_S32 NETSDK_APICALL OnStreamCallback(HI_U32 u32Handle, /* ��� */
                                HI_U32 u32DataType,     /* �������ͣ���Ƶ����Ƶ���ݻ�����Ƶ�������� */
                                HI_U8*  pu8Buffer,      /* ���ݰ���֡ͷ */
                                HI_U32 u32Length,      /* ���ݳ��� */
                                HI_VOID* pUserData    /* �û�����*/
                                )
{

	HI_S_AVFrame* pstruAV = HI_NULL;
	HI_S_SysHeader* pstruSys = HI_NULL;
	

	if (u32DataType == HI_NET_DEV_AV_DATA)
	{
		pstruAV = (HI_S_AVFrame*)pu8Buffer;

		if (pstruAV->u32AVFrameFlag == HI_NET_DEV_VIDEO_FRAME_FLAG)
		{
			if(fHlsHandle == 0 ) return 0;
					
			printf("Get Video Len:%d Timestamp:%d \n", pstruAV->u32AVFrameLen, pstruAV->u32AVFramePTS);
	
			unsigned int uiFrameType = 0;
			if (pstruAV->u32VFrameType == HI_NET_DEV_VIDEO_FRAME_I)
			{
				uiFrameType = TS_TYPE_PES_VIDEO_I_FRAME;
			}
			else
			{
				uiFrameType = TS_TYPE_PES_VIDEO_P_FRAME;
			}

			EasyHLS_VideoMux(fHlsHandle, uiFrameType, (unsigned char*)pu8Buffer+sizeof(HI_S_AVFrame), pstruAV->u32AVFrameLen, pstruAV->u32AVFramePTS*90, pstruAV->u32AVFramePTS*90, pstruAV->u32AVFramePTS*90);

		}
		else
		if (pstruAV->u32AVFrameFlag == HI_NET_DEV_AUDIO_FRAME_FLAG)
		{
			//printf("Audio %u PTS: %u \n", pstruAV->u32AVFrameLen, pstruAV->u32AVFramePTS);	
		}
	}
	else if (u32DataType == HI_NET_DEV_SYS_DATA)
	{
		pstruSys = (HI_S_SysHeader*)pu8Buffer;
		printf("Video W:%u H:%u Audio: %u \n", pstruSys->struVHeader.u32Width, pstruSys->struVHeader.u32Height, pstruSys->struAHeader.u32Format);
	} 

	return HI_SUCCESS;
}

HI_S32 OnDataCallback(HI_U32 u32Handle, /* ��� */
                                HI_U32 u32DataType,       /* ��������*/
                                HI_U8*  pu8Buffer,      /* ���� */
                                HI_U32 u32Length,      /* ���ݳ��� */
                                HI_VOID* pUserData    /* �û�����*/
                                )
{
	return HI_SUCCESS;
}
void PrintUsage()
{
	printf("Usage:\n");
	printf("------------------------------------------------------\n");
	printf("%s [-c <PlayListCapacity> -C <AllowCache> -v <M3U8Version> -t <TargetDuration> -d <HLSRootDir> -n <HLSessionName>  -U <HttpRootUrl> -N <Device user> -P <Device password> -H <Device host> -T <Device Port>]\n", ProgName);
	printf("Help Mode:   %s -h \n", ProgName );
	printf("For example: %s -c 4 -C 0 -v 3 -t 4 -d ./ -n easyhls_rtsp  -U http://www.easydarwin.org/easyhls/ -N admin -P admin -H 192.168.66.189 -T 80\n", ProgName); 
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
	while ((ch = getopt(argc,argv, "hc:C:v:t:d:n:U:N:P:H:T:")) != EOF) 
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
		case 'N':
			ConfigUName =optarg;
			break;
		case 'P':
			ConfigPWD =optarg;
			break;
		case 'H':
			ConfigDHost =optarg;
			break;
		case 'T':
			ConfigDPort =optarg;
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

	if( 0 != EasyHLS_Activate(KEY))
		return -1;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_S_STREAM_INFO struStreamInfo;
    HI_U32 a;
    
    HI_NET_DEV_Init();
    
    s32Ret = HI_NET_DEV_Login(&u32Handle, ConfigUName, ConfigPWD, ConfigDHost, atoi(ConfigDPort));
    if (s32Ret != HI_SUCCESS)
    {
        HI_NET_DEV_DeInit();
		return -1;
    }
    
	//HI_NET_DEV_SetEventCallBack(u32Handle, OnEventCallback, &a);
	HI_NET_DEV_SetStreamCallBack(u32Handle, (HI_ON_STREAM_CALLBACK)OnStreamCallback, &a);
	//HI_NET_DEV_SetDataCallBack(u32Handle, OnDataCallback, &a);

	struStreamInfo.u32Channel = HI_NET_DEV_CHANNEL_1;
	struStreamInfo.blFlag = HI_FALSE;//HI_FALSE;
	struStreamInfo.u32Mode = HI_NET_DEV_STREAM_MODE_TCP;
	struStreamInfo.u8Type = HI_NET_DEV_STREAM_ALL;
	s32Ret = HI_NET_DEV_StartStream(u32Handle, &struStreamInfo);
	if (s32Ret != HI_SUCCESS)
	{
		HI_NET_DEV_Logout(u32Handle);
		u32Handle = 0;
		return -1;
	}    
	
	//����EasyHLS Session
	fHlsHandle = EasyHLS_Session_Create(ConfigPlayListCapacity, ConfigAllowCache, ConfigM3U8Version);

	char subDir[64] = { 0 };
	sprintf(subDir,"%s/",ConfigHLSessionName);
	EasyHLS_ResetStreamCache(fHlsHandle, ConfigHLSRootDir, subDir, ConfigHLSessionName, ConfigTargetDuration);

	printf("HLS URL:%s%s/%s.m3u8", ConfigHLSRootDir, ConfigHLSessionName, ConfigHLSessionName);

    printf("Press Enter exit...\n");
    getchar();

    EasyHLS_Session_Release(fHlsHandle);
    fHlsHandle = 0;
   
    HI_NET_DEV_StopStream(u32Handle);
    HI_NET_DEV_Logout(u32Handle);
    
    HI_NET_DEV_DeInit();

    return 0;
}
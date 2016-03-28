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

#ifdef _WIN32
#include "getopt.h"
#define KEY "333565546A4969576B5A734164506857715165453375394659584E355345785458305A4A544555755A58686C4B56627671674434336D566863336B3D"
#else
#include "unistd.h"
#define KEY "333565546A4A4F576B596F41645068577151654533764E6C59584E356147787A58325A706247556A56752B7141506A655A57467A65513D3D"
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
int main(int argc, char * argv[])
{
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

    int buf_size = 1024*512;
    char *pbuf = (char *) malloc(buf_size);
    FILE *fES = NULL;
	int position = 0;
	int iFrameNo = 0;
	int timestamp = 0;

    fES = fopen("./EasyPusher.264", "rb");
    if (NULL == fES)        return 0;

	//EasyHLS SDK需要首先经过激活才能继续调用
	if(0 != EasyHLS_Activate(KEY))
		return -1;

	//创建EasyHLS Session
	fHLSHandle = EasyHLS_Session_Create(ConfigPlayListCapacity, ConfigAllowCache, ConfigM3U8Version);

	char subDir[64] = { 0 };
	sprintf(subDir,"%s/",ConfigHLSessionName);
	EasyHLS_ResetStreamCache(fHLSHandle, ConfigHLSRootDir, subDir, ConfigHLSessionName, ConfigTargetDuration);

	printf("HLS URL:%s%s/%s.m3u8\n", ConfigHLSRootDir, ConfigHLSessionName, ConfigHLSessionName);

	while (1)
	{
		int nReadBytes = fread(pbuf+position, 1, 1, fES);
		if (nReadBytes < 1)
		{
			if (feof(fES))
			{
				position = 0;
				fseek(fES, 0, SEEK_SET);
				continue;
			}
			break;
		}

		position ++;

		if (position > 5)
		{
			unsigned char naltype = ( (unsigned char)pbuf[position-1] & 0x1F);

			if (	(unsigned char)pbuf[position-5]== 0x00 && 
					(unsigned char)pbuf[position-4]== 0x00 && 
					(unsigned char)pbuf[position-3] == 0x00 &&
					(unsigned char)pbuf[position-2] == 0x01 &&
					(naltype == 0x07 || naltype == 0x01 ) )
			{
				int framesize = position - 5;
				naltype = (unsigned char)pbuf[4] & 0x1F; 
				timestamp += 1000/25;

				unsigned int uiFrameType = 0;
				if (naltype == 0x07)
				{
					uiFrameType = TS_TYPE_PES_VIDEO_I_FRAME;
				}
				else
				{
					uiFrameType = TS_TYPE_PES_VIDEO_P_FRAME;
				}

				EasyHLS_VideoMux(fHLSHandle, uiFrameType, (unsigned char*)pbuf, framesize, timestamp*90, timestamp*90, timestamp*90);

		#ifndef _WIN32
						usleep(30*1000);
		#else
						Sleep(30);
		#endif

				memmove(pbuf, pbuf+position-5, 5);
				position = 5;

				iFrameNo ++;
			}
		}
	}

    printf("Press Enter exit...\n");
    getchar();

    EasyHLS_Session_Release(fHLSHandle);
    fHLSHandle = 0;
	free(pbuf);
  
	return 0;
}
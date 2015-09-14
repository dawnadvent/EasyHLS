# EasyHLS #

EasyHLS是EasyDarwin开源流媒体团队开发的一款HLS打包库，接口非常简单，只需要传入打包的文件名、切片存放的目录、单个切片时长以及切片数等参数，EasyHLS库就能轻松将H264+AAC的流媒体切片成m3u8+ts，提供给WEB服务器进行HLS流媒体发布；

## 调用示例 ##

- **EasyHLS_RTSP**：通过RTSPClient将RTSP URL的流媒体音视频数据获取、并进行ts打包，提供HLS直播；

- **EasyHLS_SDK**： 通过调用私有SDK回调的音视频数据，进行ts打包，提供HLS直播，示例中的SDK是我们EasyDarwin开源摄像机的配套库，EasyDarwin开源摄像机硬件可以在：[https://easydarwin.taobao.com/](https://easydarwin.taobao.com/ "EasyCamera")购买，EasyCamera SDK及配套源码可在 [http://www.easydarwin.org](http://www.easydarwin.org "EasyDarwin") 或者 [https://github.com/EasyDarwin/EasyCamera](https://github.com/EasyDarwin/EasyCamera "EasyCamera") 获取到，您也可以用自己项目中用到的SDK获取音视频数据进行打包；


	Windows编译方法，

    	Visual Studio 2010 编译：./EasyHLS-master/win/EasyHLS_Demo.sln

	Linux编译方法，
		
		chmod +x ./Buildit
		./Buildit

- **EasyDarwin**：您也可以参考EasyDarwin中EasyHLSSession对EasyHLS库的调用方法，详细请看：[https://github.com/EasyDarwin/EasyDarwin](https://github.com/EasyDarwin/EasyDarwin "EasyDarwin")；

- **我们同时提供Windows、Linux、ARM版本的libEasyHLS库**：arm版本请将交叉编译工具链发送至[support@easydarwin.org](mailto:support@easydarwin.org "EasyDarwin mail")，我们会帮您具体编译


## 调用过程 ##
![](http://www.easydarwin.org/skin/easydarwin/images/easyhls20150811.png)


## 特殊说明 ##
<pre>
/* 打包H264视频 */
EasyHLS_API int Easy_APICALL EasyHLS_VideoMux(Easy_HLS_Handle handle, unsigned int uiFrameType, unsigned char *data, int dataLength, unsigned long long pcr, unsigned long long pts, unsigned long long dts);
	
</pre>
其中pcr、dts、pts等参数都是unsigned long long型，通常情况下为时间戳（ms毫秒）\*90以后的结果，所以我们通常需要定义：
unsigned long long pcr,dts,pts = timestamp\*90; 千万不要定义unsigned int型接收数据输入！具体过程参考上面的几个示例；


## 获取更多信息 ##

邮件：[support@easydarwin.org](mailto:support@easydarwin.org) 

WEB：[www.EasyDarwin.org](http://www.easydarwin.org)

Copyright &copy; EasyDarwin.org 2012-2015

![EasyDarwin](http://www.easydarwin.org/skin/easydarwin/images/wx_qrcode.jpg)

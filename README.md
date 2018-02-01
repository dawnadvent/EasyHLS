# EasyHLS #

EasyHLS是[紫鲸团队](http://www.pvale.com "紫鲸云")开发的一款HLS/TS/m3u8切片打包库，接口非常简单，只需要传入打包的文件名、切片存放的目录、单个切片时长以及切片数等参数，EasyHLS库就能轻松将H264+AAC的流媒体切片成m3u8+ts直播/点播数据，提供给WEB服务器进行HLS流媒体分发；

## 调用示例 ##

- **EasyHLS_RTSP**：通过RTSPClient将RTSP摄像机IPCamera的流媒体音视频数据流获取到本地，再进行ts的音视频封装打包，并不断更新m3u8列表，以提供HLS直播功能；


	Windows编译方法，

    	Visual Studio 2010 编译：./EasyHLS-master/win/EasyHLS_Demo.sln

	Linux编译方法，
		
		chmod +x ./Buildit
		./Buildit

	> 调用提示：目前的调用示例程序，可以接收参数，具体参数的使用，请在调用时增加 **-h** 命令查阅。


## 调用过程 ##
![](http://www.easydarwin.org/skin/easydarwin/images/easyhls20160328.png)


## 特殊说明 ##
<pre>

/* 打包H264视频 */
EasyHLS_API int Easy_APICALL EasyHLS_VideoMux(Easy_HLS_Handle handle, unsigned int uiFrameType, unsigned char *data, int dataLength, unsigned long long pcr, unsigned long long pts, unsigned long long dts);
	
</pre>

其中pcr、dts、pts等参数都是unsigned long long型，通常情况下为时间戳（ms毫秒）\*90以后的结果，所以我们通常需要定义：
unsigned long long pcr,dts,pts = timestamp\*90; 千万不要定义unsigned int型接收数据输入！具体过程参考上面的几个示例；


## 技术支持 ##

- 邮件：[support@easydarwin.org](mailto:support@easydarwin.org) 

- Tel：13718530929

- QQ交流群：<a href="https://jq.qq.com/?_wv=1027&k=5fm9nKk" title="EasyHLS" target="_blank">**532837588**</a>

> **我们同时提供Windows、Linux、Android、iOS、ARM版本的EasyRTMPClient库**：EasyRTMPClient SDK商业使用需要经过授权才能永久使用，商业授权方案可以通过以上渠道进行更深入的技术与合作咨询；


## 获取更多信息 ##

**EasyDarwin**开源流媒体服务器：[www.EasyDarwin.org](http://www.easydarwin.org)

**EasyDSS**商用流媒体解决方案：[www.EasyDSS.com](http://www.easydss.com)

**EasyNVR**无插件直播方案：[www.EasyNVR.com](http://www.easynvr.com)

Copyright &copy; EasyDarwin Team 2012-2018

![EasyDarwin](http://www.easydarwin.org/skin/easydarwin/images/wx_qrcode.jpg)
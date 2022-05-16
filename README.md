# SpoutVideoPlayer
An Openframeworks video player for Spout and NDI output

SpoutVideoPlayer is a simple video player for Spout and NDI output. The output can be received into any Spout enabled application and by applications supporting the Newtek NDI protocol for sharing data over a network.

### The project depends on :

ofxWinMenu - https://github.com/leadedge/ofxWinMenu \
ofxNDI - https://github.com/leadedge/ofxNDI

The project files are for Visual Studio 2022

Spout - https://github.com/leadedge/Spout2 \
NDI - http://NDI.NewTek.com 

Refer to the ofxNDI addon for further information concerning NDI.

### How to use :

Mouse control :\
&nbsp;&nbsp;&nbsp;&nbsp;RH click central part of window - show / hide controls\
&nbsp;&nbsp;&nbsp;&nbsp;RH click volume - mute\
&nbsp;&nbsp;&nbsp;&nbsp;LH click volume - adjust\
&nbsp;&nbsp;&nbsp;&nbsp;"<"&nbsp;&nbsp; - back one frame if paused\
&nbsp;&nbsp;&nbsp;&nbsp;">"&nbsp;&nbsp; - forward one frame if paused\
&nbsp;&nbsp;&nbsp;&nbsp;"<<" - back 8 frames if paused\
&nbsp;&nbsp;&nbsp;&nbsp;">>" - forward 8 frames if paused\
\
Keyboard :\
&nbsp;&nbsp;&nbsp;&nbsp;SPACE	show / hide controls\
&nbsp;&nbsp;&nbsp;&nbsp;'i'	- show / hide information\
&nbsp;&nbsp;&nbsp;&nbsp;'p'	- play / pause\
&nbsp;&nbsp;&nbsp;&nbsp;'m'	- toggle mute\
&nbsp;&nbsp;&nbsp;&nbsp;LEFT/RIGHT - back/forward one frame\
&nbsp;&nbsp;&nbsp;&nbsp;PGUP/PGDN -	back/forward 8 frames\
&nbsp;&nbsp;&nbsp;&nbsp;HOME/END - start/end of video\
&nbsp;&nbsp;&nbsp;&nbsp;'f' - toggle full screen\
&nbsp;&nbsp;&nbsp;&nbsp;'ESC' - exit full screen

----------------------
Credit for the icons and progress bar - https://github.com/ACMILabs/mini-vod
The MIT License (MIT)
Copyright (c) 2016 the Australian Centre for the Moving Image (ACMI)

----------------------
NDI SDK - Copyright NewTek Inc. [https://www.ndi.tv/](https://www.ndi.tv/).

A license agreement is included with the Newtek SDK when you receive it after registration with NewTek.
The SDK is used by you in accordance with the license you accepted by clicking "accept" during installation. This license is available for review from the root of the SDK folder.
Read the conditions carefully. You can include the NDI dlls as part of your own application, but the Newtek SDK and specfic SDK's which may be contained within it may not be re-distributed.
Your own EULA must cover the specific requirements of the NDI SDK EULA.





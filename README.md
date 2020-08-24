# SpoutVideoPlayer
An Openframeworks video player for Spout and NDI output

SpoutVideoPlayer is a simple video player for Spout and NDI output. The output can be received into any Spout enabled application and by applications supporting the Newtek NDI protocol for sharing data over a network.

##The project depends on :

ofxWinMenu - https://github.com/leadedge/ofxWinMenu
ofxNDI - https://github.com/leadedge/ofxNDI

The project files are for Visual Studio 2017

Spout - https://github.com/leadedge/Spout2
NDI - http://NDI.NewTek.com/

Refer to the ofxNDI addon for further information concerning NDI.

##How to use :
SPACE or 
Mouse control :  
  RH click the central part of the window to show / hide controls   
  RH click volume - mute  
  LH click volume - adjust  
   <	back one frame if paused  
   >	forward one frame if paused  
  <<	back 8 frames if paused  
  >>	forward 8 frames if paused  
  
Keyboard :  
  SPACE	show / hide controls  
  'h'	show / hide information  
  'p'	play / pause  
  'm'	toggle mute  
  LEFT/RIGHT	back/forward one frame  
  PGUP/PGDN	back/forward 8 frames  
  HOME/END	start/end of video  
  'f'	toggle full screen  
  'ESC'	exit full screen  


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





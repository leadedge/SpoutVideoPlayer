# SpoutVideoAudio

A version  of SpoutVideoPlayer using FFmpeg instead of the Openframeworks ofVideoPlayer class.

This project started as an example for the Spout SDK, but has evolved to create a \
practical video player with transparency for both Spout and NDI output.

Due to the additional complexity over a typical example, the project is hosted as\
a branch of "Spout Video Player" rather than example code within the Spout SDK.

Two pipes are created, one for video and the other for audio. \
This is a simple method compared to using FFmpeg libraries \
and supports alpha channel transparency if the video file encoder\
supports it, such as VP9, HapAlpha and ProRes4444.

ofSoundStream and audioOut enable sound output and Draw is kept in sync with audio by timing\
and and frame count matching. Seeking is achieved by specifying the start time for pipe read.\
Performance varies depending on the encoder used for the video.
	
The [ofxWinMenu](https://github.com/leadedge/ofxWinMenu) addon is used to create a menu and manage caption mouse press.\
The [ofxWinDialog](https://github.com/leadedge/ofxWinDialog) addon is used to create an image adjust dialog.\
A static library is used for Spout functions and is included in the project.

### FFmpeg.exe and FFprobe.exe are required

Refer to data/ffmpeg/readme.md\. Further information\
on program functions is available in Help > About > Options.

### The code can be used for reference :

- ofxWinMenu to create a window menu
- ofxWinDialog to create a dialog
- Compute shaders for image adjust
- Setting an icon from a Windows dll
- Detecting non-client area mouse press
- Preview and full screen by changing window style and size
- FFprobe to read video file details
- FFmpeg with two pipes to decode video and audio frames
- Fps control using HoldFps
- Sync video with audio using audio timing and frame matching
- Openframeworks dragEvent for drag and drop
- Openframeworks soundstream and audioOut
- Video duration, frame counter and progress bar
- Draw and position ofTrueTypeFont text
- Using a Spout static library generated using Cmake
- SetSenderName, SendImage, LoadTexturePixels and ReleaseSender
- Utility OpenSpoutConsole and SpoutMessageBox functions
 
### Compiling

The project is for Openframeworks and the folder structure must be :

Openframeworks\
&emsp;Addons\
&emsp;&emsp;ofxNDI\
&emsp;&emsp;ofxWinDialog\
&emsp;&emsp;ofxWinMenu\
&emsp;Apps\
&emsp;&emsp;MyApps\
&emsp;&emsp;&emsp;SpoutVideoAudio\
&emsp;&emsp;&emsp;&emsp;src\
&emsp;&emsp;&emsp;&emsp;libs\
&emsp;&emsp;&emsp;&emsp;&emsp;include\
&emsp;&emsp;&emsp;&emsp;bin\
&emsp;&emsp;&emsp;&emsp;&emsp;data\
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;ffmpeg\
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;fonts\
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;icons\
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;images\
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;videos\
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;rgba2yuv




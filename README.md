# SpoutVideoAudio

A version  of SpoutVideoPlayer using FFmpeg instead of the Openframeworks ofVideoPlayer class.

This project started as an example for the Spout SDK, but has been extended to create a \
practical video player. Due to the additional complexity, the project is hosted as a branch \
of "Spout Video Player" rather than example code within the Spout SDK.

Two pipes are created, one for video and the other for audio. This is a simple method compared \
to using FFmpeg libraries and supports alpha channel transparency if the video file encoder\
supports it, such as VP9, HapAlpha and ProRes4444.

ofSoundStream and audioOut enable sound output and Draw is kept in sync with audio by timing\
and and frame count matching. Seeking is achieved by specifying the start time for pipe read.\
Performance varies depending on the encoder used for the video.
	
Uses the ofxWinMenu addon https://github.com/leadedge/ofxWinMenu to create a menu and manage\
caption mouse press and the ofxWinDialog addon https://github.com/leadedge/ofxWinDialog \
to create an image adjust dialog. The source is included within this project for convenience.\
The project also uses a static library for Spout functions.

The code can be used for reference :

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

FFmpeg.exe and FFprobe.exe are required. Refer to data/ffmpeg/readme.md

Information on program functions is available in Help > About > Options.






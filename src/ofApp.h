/*

	Spout OpenFrameworks Video/Audio Sender example

	Copyright (C) 2026 Lynn Jarvis.

	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#pragma once

#include "ofMain.h"
#include "addons/ofxWinMenu/src/ofxWinMenu.h" // Windows menu
#include "addons/ofxWinDialog/src/ofxWinDialog.h" // Adjust dialog
#include "SpoutShaders.h" // Compute shaders
#include "../libs/include/Spout.h" // For Spout library
#include <format> // for time display
#include <commdlg.h> // for OPENFILENAME

// Spout library (/MD build)
#pragma comment (lib, "libs/Spout_static.lib")

class ofApp : public ofBaseApp{
	public:
		void setup();
		void update();
		void draw();
		void exit();
		void keyPressed(int key);
		void mousePressed(int x, int y, int button);
		void mouseMoved(int x, int y);
		void dragEvent(ofDragInfo dragInfo);
		void audioOut(ofSoundBuffer &buffer);

		ofTexture readTexture; // Read texture
		ofTexture myTexture; // Draw texture
		ofSoundStream soundStream; // To get sound to the speakers
		ofTrueTypeFont myFont;

		// Menu
		HINSTANCE m_hInstance = nullptr;
		HWND m_hWnd = nullptr;
		HICON m_hIcon = nullptr;
		ofxWinMenu* menu = nullptr; // Menu object
		void appMenuFunction(std::string title, bool bChecked); // Menu callback function
		bool bMute = false;
		bool bScale = true;
		bool bPaused = false;
		bool bRestart = false;
		bool bStop = false;
		bool bTopmost = false;
		bool bFullScreen = false;
		bool bPreview = false;
		bool bShowInfo = true;

		// Sender
		Spout sender;  // Sender object
		char m_SenderName[256]{}; // Sender name
		unsigned int m_SenderWidth = 1280; // Sender width (video width)
		unsigned int m_SenderHeight = 720; // Sender height (video height)

		// FFmpeg
		std::string m_exePath;           // Executable location
		std::string m_ffmpegPath;        // FFmpeg location

		// Video
		std::string m_videopath;         // The full video path
		double m_FrameRate = 30.0;       // Video frame rate
		double m_Duration = 0.0;         // Video duration
		long m_Frames = 0;               // Total number of frames
		long m_FramesRead = 0;           // Number of frames read after pause
		double m_progress = 0;           // Progress bar position

		FILE *m_pipein = nullptr;        // Pipe for FFmpeg video
		std::string m_codecName;         // Codec name
		std::string m_input;             // Input string to FFmpeg video
		unsigned char *m_pixelBuffer = nullptr; // RGBA pixel buffer

		// Audio
		std::string m_audioInput;         // Input string to FFmpeg audio
		FILE* m_audioPipe = nullptr;      // Audio pipe
		float* m_audiodata = nullptr;     // Audio data
		std::vector<int> m_audioSequence; // Sequence of sample numbers per frame
		std::vector<char> m_audioBuffer;  // The audio buffer used in audioOut TODO
		std::vector<int16_t> m_pcmBuffer; // PCM data buffer used in audioOut
		int m_nChannels = 0;              // Number of channels (2 for stereo) 
		int m_sampleRate = 0;             // Audio sample rate
		int m_sampleIndex = 0;            // Sample index for this video frame
		int m_nSamples = 0;               // Number of audio samples read
		double m_frameSec = 0;            // Starting seconds for pipe
		bool bReadVideo = true;           // Read the next video frame

		// For audio pause with menu selection or title bar click
		bool bNCmousePressed = false;
		std::atomic<uint64_t> m_audioFramesPlayed = 0; // Audio frame counter
		bool bVideoSync = false; // Syncing video with audio

		bool OpenVideo(std::string filePath, double start = 0.0);  // Open a video with FFmpeg
		bool OpenFFmpeg(std::string filePath, double start = 0.0); // Open FFmpeg video and audio pipes
		void CloseFFmpeg();                         // Release FFmpeg resources
		void RestartVideo(double startseconds = 0); // Close and restart
		bool OpenSender();                          // Open sender
		bool ffprobe(std::string filePath);         // Get video file information
		void ResetWindow(int width, int height);    // Reset window size and position
		void doTopmost(bool bTop);                  // Show topmost or not
		void ShowInfo();                            // On-screen controls and progress bar
		std::string ffdownloadstr(); // FFmpeg download string for messagebox
		std::string EnterFileName(); // File dialog with more options that Openframeworks
		void SaveImageFile(std::string name); // For Capture or Save as

		// Full screen
		void doFullScreen(bool bEnable, bool bPreview = false);
		HWND m_hWndForeground = nullptr;
		HWND m_hwndTop = nullptr;
		RECT m_windowRect{};
		RECT m_clientRect{};
		DWORD m_dwStyle = 0;
		int m_AddX, m_AddY, m_nonFullScreenX, m_nonFullScreenY = 0;

		// icons
		std::vector<ofImage> m_icons;
		std::vector<ofColor> m_iconColor;

		ofImage icon_reverse;     // 0
		ofImage	icon_pause;       // 1
		ofImage	icon_play;        // 2
		ofImage	icon_stop;        // 3
		ofImage	icon_fastforward; // 4
		ofImage	icon_full_screen; // 5
		ofImage	icon_sound;       // 6
		ofImage	icon_mute;        // 7 

		// To display the icons
		ofRectangle	icon_background;
		float icon_size = 0.0f;
		float icon_x = 0.0f;
		float icon_y = 0.0f;
		ofColor	icon_background_color;
		ofColor	icon_highlight_color;
		ofColor	icon_color;

		// Adjust dialog
		ofxWinDialog* adjust;
		HWND hwndAdjust = NULL;
		void CreateAdjustDialog();
		void AdjustCallback(string title, std::string text, int value);
		spoutShaders shaders;
		void ApplyShaders();
		float Brightness  = 0.0; // -1 - 1
		float Contrast    = 1.0; //  0 - 1
		float Saturation  = 1.0; //  0 - 4
		float Gamma       = 1.0; //  0 - 2
		float Temp        = 6500.0; // daylight
		float Sharpness   = 0.0;
		float Sharpwidth  = 3.0; // 3x3, 5x5, 7x7 - 3, 5, 7
		bool b3x3 = true; // Radio buttons
		bool b5x5 = false;
		bool b7x7 = false;
		bool bAdaptive = false; // CAS adaptive sharpen


};

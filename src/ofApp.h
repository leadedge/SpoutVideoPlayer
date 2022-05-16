/*

	ofApp.h

	Spout Video Player

	A simple video player with Spout and NDI output

	Copyright (C) 2017-2022 Lynn Jarvis.

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
#include "SpoutLibrary.h" // For Spout functions
#include "ofxWinMenu.h" // Addon for a windows style menu
#include "ofxNDI.h" // Addon for NDI streaming
#include "resource.h"
#include <shlwapi.h>  // for path functions
#include <Shellapi.h> // for shellexecute

#pragma comment(lib, "shlwapi.lib")  // for path functions
#pragma comment(lib, "Version.lib") // for GetFileVersionInfo


class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();

	void windowResized(int w, int h);
	void keyPressed(int key);
	void mousePressed(int x, int y, int button);
	void mouseMoved(int x, int y);
	void dragEvent(ofDragInfo dragInfo);

	// Spout
	SPOUTLIBRARY* spoutsender = nullptr; // Sender object
	char sendername[256]{}; // Sender name
	bool bSpoutOut = true;
	bool bInitialized = false; // Initialization result

	// NDI
	ofxNDIsender NDIsender;
	char NDIsendername[256]{};
	bool bNDIout = false;
	bool bNDIasync = false;
	bool bNDIinitialized = false;

	ofFbo myFbo;

	// Window dimensions
	float windowWidth = 0;
	float windowHeight = 0;

	// Movie
	ofVideoPlayer myMovie; // Movie to send
	ofImage splashImage; // Startup splash image
	string movieFile;
	float movieWidth = 0;
	float movieHeight = 0;

	// icons and controlbar
	ofImage icon_reverse;
	ofImage	icon_fastforward;
	ofImage	icon_play;
	ofImage	icon_pause;
	ofImage	icon_forward;
	ofImage	icon_back;
	ofImage	icon_full_screen;
	ofImage	icon_sound;
	ofImage	icon_mute;

	// Things to display the icons
	ofRectangle	icon_background;
	float		icon_size = 0.0f;
	float		icon_playpause_pos_x = 0.0f;
	float		icon_playpause_pos_y = 0.0f;
	bool		icon_playpause_hover =false;
	float		icon_sound_pos_x = 0.0f;
	float		icon_sound_pos_y = 0.0f;
	bool		icon_sound_hover = false;
	float		icon_fullscreen_pos_x = 0.0f;
	float		icon_fullscreen_pos_y = 0.0f;
	bool		icon_fullscreen_hover = false;
	float		icon_forward_pos_x = 0.0f;
	float		icon_forward_pos_y = 0.0f;
	bool		icon_forward_hover = 0.0f;
	float		icon_back_pos_x = 0.0f;
	float		icon_back_pos_y = 0.0f;
	bool		icon_back_hover = false;

	float		icon_reverse_pos_x = 0.0f;;
	float		icon_reverse_pos_y = 0.0f;;
	bool		icon_reverse_hover = false;

	float		icon_fastforward_pos_x = 0.0f;;
	float		icon_fastforward_pos_y = 0.0f;;
	bool		icon_fastforward_hover = false;

	ofColor		icon_highlight_color;
	ofColor		icon_background_color;

	ofFbo iconFbo;

	// progress bar
	ofRectangle	progress_bar;
	ofRectangle	progress_bar_played;
	float		controlbar_width = 0.0f;;
	float		controlbar_height = 0.0f;;
	float		controlbar_pos_y = 0.0f;;
	float		controlbar_start_time = 0.0f;;
	float		controlbar_timer_end = 0.0f;;
	float		video_duration = 0.0f;;
	float		video_percent_played = 0.0f;;
	
	// Movie control
	bool OpenMovieFile(string filePath);
	bool bLoaded = false;
	int nOldFrames = 0;
	int nNewFrames = 0;
	float movieVolume = 0.0f;
	void setVideoPlaypause();
	void HandleControlButtons(float x, float y, int button = 0);
	void drawPlayBar();
	void CloseVolume();

	// Menu
	ofxWinMenu* menu = nullptr; // Menu object
	void appMenuFunction(string title, bool bChecked); // Menu callback function

	// Flags
	bool bSplash = true;
	bool bShowControls = false;
	bool bShowInfo = false;
	bool bTopmost = false;
	bool bMute = false;
	bool bFullscreen = false;
	bool bResizeWindow = false;
	bool bPaused = false;
	bool bLoop = false;
	bool bStandard = false;
	bool bMenuExit = false;
	bool bMessageBox = false;
	bool bMouseClicked = false;
	bool bMouseExited = false;

	// Utility
	void doFullScreen(bool bFull);
	void doTopmost(bool bTop);
	void ResetWindow(bool bCentre = false);
	void WriteInitFile(const char* initfile);
	void ReadInitFile();

	// Window
	HWND     hWnd = NULL;            // Application window
	HWND     hWndForeground = NULL;  // current foreground window
	HWND     g_hwnd = NULL;          // global app winodw handlehandle to the OpenGL render window
	RECT     windowRect;      // Render window rectangle
	RECT     clientRect;      // Render window client rectangle
	LONG_PTR dwStyle = NULL;         // original window style
	int      nonFullScreenX = 0;  // original window position
	int      nonFullScreenY = 0;
	unsigned int AddX, AddY = 0;      // adjustment to client rect for reset of window size
	
	int doMessageBox(HWND hwnd, LPCSTR message, LPCSTR caption, UINT uType);

	ofTrueTypeFont myFont;
	char info[1024]{}; // for info box

	// For received frame fps calculations
	double startTime, lastTime, frameTime, frameRate, fps = 0.0;

};

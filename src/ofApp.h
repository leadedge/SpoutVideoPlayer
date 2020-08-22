/*

	Spout Video Player

	Copyright (C) 2017 Lynn Jarvis.

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


class ofApp : public ofBaseApp{
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
		SPOUTLIBRARY * spoutsender;	// A sender object
		char sendername[256];      // Sender name
		bool bInitialized;         // Initialization result
		bool bSpoutOut;

		// NDI
		ofxNDIsender NDIsender;
		char NDIsendername[256];
		bool bNDIout;
		bool bNDIinitialized;
		int idx;
		ofPixels ndiBuffer[2];

		// Viewport and size
		ofFbo myFbo;
		ofTexture myTexture;
		unsigned int ResolutionWidth; // global width and height
		unsigned int ResolutionHeight;
		float movieWidth;
		float movieHeight;
		float windowWidth;
		float windowHeight;

		ofVideoPlayer myMovie; // Movie to send
		ofImage splashImage; // Startup splash image
		string movieFile;
		string imagePath;
		string imagePrefix;
		string imageType;

		// icons and controlbar
		ofImage			icon_reverse;
		ofImage			icon_fastforward;
		ofImage			icon_play;
		ofImage			icon_pause;
		ofImage			icon_forward;
		ofImage			icon_back;
		ofImage			icon_full_screen;
		ofImage			icon_sound;
		ofImage			icon_mute;

		
		// Things to display the icons
		ofRectangle		icon_background;
		float			icon_size;
		float			icon_playpause_pos_x;
		float			icon_playpause_pos_y;
		bool			icon_playpause_hover;
		float			icon_sound_pos_x;
		float			icon_sound_pos_y;
		bool			icon_sound_hover;
		float			icon_fullscreen_pos_x;
		float			icon_fullscreen_pos_y;
		bool			icon_fullscreen_hover;
		float			icon_forward_pos_x;
		float			icon_forward_pos_y;
		bool			icon_forward_hover;
		float			icon_back_pos_x;
		float			icon_back_pos_y;
		bool			icon_back_hover;

		float			icon_reverse_pos_x;
		float			icon_reverse_pos_y;
		bool			icon_reverse_hover;

		float			icon_fastforward_pos_x;
		float			icon_fastforward_pos_y;
		bool			icon_fastforward_hover;

		ofColor			icon_highlight_color;
		ofColor			icon_background_color;

		ofFbo iconFbo;

		// progress bar
		ofRectangle		progress_bar;
		ofRectangle		progress_bar_played;
		float			controlbar_width;
		float			controlbar_height;
		float			controlbar_pos_y;
		float			controlbar_start_time;
		float			controlbar_timer_end;
		float			video_duration;
		float			video_percent_played;

		// Movie control
		bool OpenMovieFile(string filePath);
		bool bLoaded;
		int nOldFrames;
		int nNewFrames;
		void setVideoPlaypause();
		void HandleControlButtons(float x, float y);
		void drawPlayBar();

		// Menu
		ofxWinMenu * menu; // Menu object
		void appMenuFunction(string title, bool bChecked); // Menu callback function

		bool bSplash;
		bool bShowControls;
		bool bShowInfo;
		bool bTopmost;
		bool bMute;
		bool bFullscreen;
		bool bResizeWindow;
		bool bPaused;
		bool bLoop;
		bool bStandard;
		bool bMenuExit;
		bool bMessageBox;
		bool bMouseClicked;
		bool bMouseExited;
		
		void doFullScreen(bool bFull);
		void doTopmost(bool bTop);
		void ResetWindow();
		void WriteInitFile(const char *initfile);
		void ReadInitFile();

		HWND         hWnd;            // Application window
		HWND         hWndForeground;  // current foreground window
		HWND         g_hwnd;          // global handle to the OpenGL render window
		RECT         windowRect;      // Render window rectangle
		RECT         clientRect;      // Render window client rectangle
		LONG_PTR     dwStyle;         // original window style
		int          nonFullScreenX;  // original window position
		int          nonFullScreenY;
		unsigned int AddX, AddY;      // adjustment to client rect for reset of window size

		bool EnterResolution();
		bool bUseMovieResolution;
		int doMessageBox(HWND hwnd, LPCSTR message, LPCSTR caption, UINT uType);

		ofTrueTypeFont myFont;
		char info[1024]; // for info box
		// For received frame fps calculations
		double startTime, lastTime, frameTime, frameRate, fps;


};

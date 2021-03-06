/*

	Spout Video Player

	A simple video player

	Copyright (C) 2017-2021 Lynn Jarvis.

	=========================================================================
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
	=========================================================================


	16.12.17	- Disable NDI
				- Disable HAP
	23.12.17	- Remove Hap
				- include "Open movie folder"
				- Changed from Spout SDK to SpoutLibrary
				- Version 1.000 release (Spout supporter bonus)
	20.08.20	- Updated to OpenFrameworks 11.0
				  Latest ofxNDI using dynamic loading (NDI 4.5)
				  Add missing play icon.
				  Cleanup throughout.
				  Tested with the most recent Klite Codec pack
				  (Basic pack without player)
	22.08.20	- Add audio mute
				  Change progress bar position if no info
	24.08.20	- Add volume slider control
				- Fix window sizing
				- Allow for multiple monitors full screen
				- Update to 2.007 SpoutLibrary - no code changes
				  Version 1.002
				- Update to revised 2.007 SpoutLibrary
				- Change default size from 640x360 to 800x450
				  Version 1.003
	13.01.21	- Update SpoutLibrary
				  Version 1.004


*/
#include "ofApp.h"

// DIALOGS
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static string NDInumber; // NDI library version number
static HINSTANCE g_hInstance;

LRESULT CALLBACK UserResolution(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static PSTR szText;
static PSTR szXtext;
static PSTR szYtext;
static PSTR szResolutionX;
static PSTR szResolutionY;
static bool busemovie;

// volume control modal dialog
LRESULT CALLBACK UserVolume(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static HWND hwndVolume = NULL;
// ofApp class pointer for the dialog to access class variables
static ofApp * pThis = NULL;
// Hook for volume dialog keyboard detection
HHOOK hHook = NULL;
LRESULT CALLBACK KeyProc(int nCode, WPARAM wParam, LPARAM lParam);

//--------------------------------------------------------------
void ofApp::setup(){

	ofBackground(255, 255, 255);

	// Get instance
	g_hInstance = GetModuleHandle(NULL);

	// Get product version number
	char title[256];
	DWORD dummy, dwSize;
	char temp[MAX_PATH];


	// Debug console window so printf works
	// Note use of WinMain in main.cpp and 
	// Linker > System > Subsystem setting
	/*
	FILE* pCout;
	AllocConsole();
	freopen_s(&pCout, "CONOUT$", "w", stdout);
	printf("Spout Video Player\n");
	*/

	strcpy_s(title, 256, "Spout Video Player");
	if (GetModuleFileNameA(g_hInstance, temp, MAX_PATH)) {
		dwSize = GetFileVersionInfoSizeA(temp, &dummy);
		if (dwSize > 0) {
			vector<BYTE> data(dwSize);
			if (GetFileVersionInfoA(temp, NULL, dwSize, &data[0])) {
				LPVOID pvProductVersion = NULL;
				unsigned int iProductVersionLen = 0;
				if (VerQueryValueA(&data[0], ("\\StringFileInfo\\080904E4\\ProductVersion"), &pvProductVersion, &iProductVersionLen)) {
					strcat_s(title, 256, " - v");
					strcat_s(title, 256, (char *)pvProductVersion);
				}
			}
		}
	}
	ofSetWindowTitle(title); // show it on the title bar

	// Load a font rather than the default
	myFont.load("fonts/verdana.ttf", 12, true, true);

	// Main window handle
	hWnd = WindowFromDC(wglGetCurrentDC());

	// ofApp class pointer for dialogs to use
	pThis = this;

	// Set a custom window icon
	SetClassLong(hWnd, GCL_HICON, (LONG)LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(IDI_SPOUTICON)));

	// Disable escape key exit so we can exit fullscreen with Escape (see keyPressed)
	ofSetEscapeQuitsApp(false);

	//
	// Create a menu using ofxWinMenu
	//

	// A new menu object with a pointer to this class
	menu = new ofxWinMenu(this, hWnd);

	// Register an ofApp function that is called when a menu item is selected.
	// The function can be called anything but must exist. 
	// See the example "appMenuFunction".
	menu->CreateMenuFunction(&ofApp::appMenuFunction);

	// Create a window menu
	HMENU hMenu = menu->CreateWindowMenu();

	//
	// Create a "File" popup menu
	//
	HMENU hPopup = menu->AddPopupMenu(hMenu, "File");
	// Add popup items to the File menu
	// Open a movie of image file
	menu->AddPopupItem(hPopup, "Open movie", false, false); // Not checked and not auto-checked
	// Explore the folder of the current movie
	menu->AddPopupItem(hPopup, "Open movie folder", false, false);
	// Final File popup menu item is "Exit" - add a separator before it
	menu->AddPopupSeparator(hPopup);
	menu->AddPopupItem(hPopup, "Exit", false, false);

	//
	// View popup menu
	//
	hPopup = menu->AddPopupMenu(hMenu, "View");
	bShowControls = false;  // don't show controls yet
	menu->AddPopupItem(hPopup, "Controls");
	bLoop = false;  // movie loop
	menu->AddPopupItem(hPopup, "Loop");
	bMute = false; // Audio mute
	menu->AddPopupItem(hPopup, "Mute");
	bResizeWindow = false; // not resizing
	menu->AddPopupItem(hPopup, "Resize to movie", false, false); // Not checked and not auto-check
	bShowInfo = true;
	menu->AddPopupItem(hPopup, "Info", true); // Checked and auto-check
	bFullscreen = false; // not fullscreen yet
	menu->AddPopupItem(hPopup, "Full screen", false, false); // Not checked and not auto-check
	bTopmost = false; // app is not topmost yet
	menu->AddPopupItem(hPopup, "Show on top"); // Not checked (default)
	
	//
	// Output popup menu
	//
	hPopup = menu->AddPopupMenu(hMenu, "Output");
	bSpoutOut = true;
	menu->AddPopupItem(hPopup, "Spout", true); // Checked
	bNDIout = false;
	menu->AddPopupItem(hPopup, "NDI", false);  // Not checked
	menu->AddPopupItem(hPopup, "Resolution", false, false);
	
	//
	// Help popup menu
	//
	hPopup = menu->AddPopupMenu(hMenu, "Help");
	menu->AddPopupItem(hPopup, "Information", false, false); // No auto check
	menu->AddPopupItem(hPopup, "About", false, false); // No auto check

	// Adjust window for the starting client size (in main.cpp)
	// allowing for a menu and centre on the screen
	ResetWindow(true);

	// Set the menu to the window after adjusting the size
	menu->SetWindowMenu();

	bMenuExit = false; // to handle mouse position
	bMessageBox = false; // To handle messagebox and mouse events
	bMouseClicked = false;
	bMouseExited = false;

	// Text for Help > Information
	strcat_s(info, 1024, "\nMouse\n");
	strcat_s(info, 1024, "  RH click window	show / hide controls\n");
	strcat_s(info, 1024, "  RH click volume	mute\n");
	strcat_s(info, 1024, "  LH click volume	adjust\n");
	strcat_s(info, 1024, "   <	back one frame if paused\n");
	strcat_s(info, 1024, "   >	forward one frame if paused\n");
	strcat_s(info, 1024, "  <<	back 8 frames if paused\n");
	strcat_s(info, 1024, "  >>	forward 8 frames if paused\n");
	strcat_s(info, 1024, "\nKeyboard\n");
	strcat_s(info, 1024, "  SPACE	show / hide controls\n");
	strcat_s(info, 1024, "  'h'	show / hide information\n");
	strcat_s(info, 1024, "  'p'	play / pause\n");
	strcat_s(info, 1024, "  'm'	toggle mute\n");
	strcat_s(info, 1024, "  LEFT/RIGHT	back/forward one frame\n");
	strcat_s(info, 1024, "  PGUP/PGDN	back/forward 8 frames\n");
	strcat_s(info, 1024, "  HOME/END	start/end of video\n");
	strcat_s(info, 1024, "  'f'	toggle full screen\n");
	strcat_s(info, 1024, "  'ESC'	exit full screen\n");

	bUseMovieResolution = true; //  false; // Use movie size for sender resolution
	bSplash = true; // set false for movie load testing

	// starting resolution
	ResolutionWidth  = (unsigned int)ofGetWidth();
	ResolutionHeight = (unsigned int)ofGetHeight();

	// Load splash screen
	if(bSplash) {
		splashImage.load("images/SpoutVideoPlayer.png");
		splashImage.setImageType(OF_IMAGE_COLOR_ALPHA);
	}

	// Read ini file to get bLoop, bNDIout and bTopmost flags and resolution
	ReadInitFile();

	// Set fbo to current Resolution for splash and in case movie did not load
	myFbo.allocate(ResolutionWidth, ResolutionHeight, GL_RGB);

	//
	// Play bar
	//

	// icons
	icon_size = 27; // 32; //  64;
	icon_reverse.load("icons/reverse.png");
	icon_fastforward.load("icons/fastforward.png");

	icon_back.load("icons/back.png");
	icon_play.load("icons/play.png");
	icon_pause.load("icons/pause.png");
	icon_forward.load("icons/forward.png");
	icon_full_screen.load("icons/full_screen.png");
	icon_sound.load("icons/speaker.png");
	icon_mute.load("icons/speaker_mute.png");

	icon_reverse.resize(icon_size, icon_size);
	icon_fastforward.resize(icon_size, icon_size);
	icon_play.resize(icon_size, icon_size);
	icon_pause.resize(icon_size, icon_size);
	icon_back.resize(icon_size, icon_size);
	icon_forward.resize(icon_size, icon_size);
	icon_full_screen.resize(icon_size, icon_size);
	icon_sound.resize(icon_size, icon_size);
	icon_mute.resize(icon_size, icon_size);

	icon_playpause_hover = false;
	icon_reverse_hover = false;
	icon_fastforward_hover = false;
	icon_back_hover = false;
	icon_forward_hover = false;
	icon_fullscreen_hover = false;
	icon_sound_hover = false;

	icon_highlight_color = ofColor(ofColor(74, 144, 226, 255));
	icon_background_color = ofColor(128, 128, 128, 255);

	progress_bar.width = ofGetWidth();
	progress_bar.height = 12; //  8;

	controlbar_width = ofGetWidth();
	controlbar_height = icon_size + progress_bar.height * 3;
	controlbar_pos_y = ofGetHeight();

	controlbar_timer_end = false;
	controlbar_start_time = ofGetElapsedTimeMillis();
	bShowControls = false;

	// SPOUT

	// Initialize SpoutLibrary
	spoutsender = GetSpout(); // Create an instance of the Spout library

	bInitialized = false; // Spout sender initialization
	bNDIinitialized = false; // NDI sende intiialization
	strcpy_s(sendername, 256, "Spout Video Player"); // Set the sender name
	
	// NDI
	// Initialize ofPixel buffers
	ndiBuffer[0].allocate(ResolutionWidth, ResolutionHeight, 4);
	ndiBuffer[1].allocate(ResolutionWidth, ResolutionHeight, 4);
	idx = 0;
	
	// Optionally set NDI asynchronous sending instead of clocked at 60fps
	NDIsender.SetAsync(false); // change to true for async
	// Get NewTek library version number (dll) for the about box
	// Version number is the last 7 chars - e.g 2.1.0.3
	string NDIversion = NDIsender.GetNDIversion();
	NDInumber = NDIversion.substr(NDIversion.length() - 7, 7);

	// For movie frame fps calculations
	// independent of the rendering rate
	startTime = lastTime = frameTime = 0.0;
	fps = frameRate = 30.0; // starting value

	// Keyboard hook for volume dialog
	hHook = SetWindowsHookExA(WH_KEYBOARD, KeyProc, NULL, GetCurrentThreadId());

}


//--------------------------------------------------------------
void ofApp::update() {

	if (bLoaded) {
		myMovie.update();
		// Handle pause at the end of a movie if not looping
		// This also prevents the old frame count from incrementing at the end of the movie
		if (!bPaused && !bLoop) {
			if (myMovie.getCurrentFrame() >= myMovie.getTotalNumFrames() - 2) {
				myMovie.setPosition(0.0);
				myMovie.setPaused(true);
				bPaused = true; // Movie has ended and not looping
			}
		}

		// Check the old frame count
		// if excessive, the movie is not playing
		if (myMovie.isFrameNew()) {

			nOldFrames = 0;
			nNewFrames++;
			myFbo.begin();
			myMovie.draw(0, 0, myFbo.getWidth(), myFbo.getHeight());
			myFbo.end();
	
			// Calculate movie fps
			lastTime = startTime;
			startTime = ofGetElapsedTimeMicros();
			frameTime = (startTime - lastTime) / 1000000; // seconds
			if (frameTime > 0.01) {
				frameRate = floor(1.0 / frameTime + 0.5);
				// damping from a starting fps value
				fps *= 0.99;
				fps += 0.01*frameRate;
			}

		}
		else {
			if(!bPaused) {
				nOldFrames++;
				if (nOldFrames > 15 && nNewFrames < 16) {// 1/2 second
					// cannot call stop for failure
					myMovie.close();
					bLoaded = false;
					bSplash = true;
					if (splashImage.isAllocated()) {
						// draw splash image again
						myFbo.begin();
						splashImage.draw(0, 0, myFbo.getWidth(), myFbo.getHeight());
						myFbo.end();
					}
					// Reset the resolution and textures
					if (bInitialized) {
						spoutsender->ReleaseSender(); // Release the sender
						bInitialized = false;
					}
					if (bNDIinitialized) {
						NDIsender.ReleaseSender();
						bNDIinitialized = false;
					}
					doMessageBox(NULL, "Could not play the movie - unknown error\nOF recommends the free K - Lite Codec pack.\nHowever, some formats still do not play.\nProblems noted with WMV3 and MP42 codecs\nTry converting the file to another format.", "SpoutVideoPlayer", MB_ICONERROR);
					return;
				}
			}
		}
	}
	else if (splashImage.isAllocated()) {
		myFbo.begin();
		splashImage.draw(0, 0, myFbo.getWidth(), myFbo.getHeight());
		myFbo.end();
	}

}


//--------------------------------------------------------------
void ofApp::draw() {

	char str[256];
	ofSetColor(255);

	if(myFbo.isAllocated())
		myFbo.draw(0, 0, ofGetWidth(), ofGetHeight());

	if (bSplash || !bLoaded)
		return;

	// SPOUT
	if (bSpoutOut) {
		if (!bInitialized) {
			// Create a Spout sender the same size as the fbo resolution
			// sendername initialized by movie load
			bInitialized = spoutsender->CreateSender(sendername, (unsigned int)myFbo.getWidth(), (unsigned int)myFbo.getHeight());
		}

		if (bInitialized && myMovie.isFrameNew()) {
			// Send the video texture out.
			// Receivers will detect the movie frame rate
			spoutsender->SendTexture(myFbo.getTexture().getTextureData().textureID,
				myFbo.getTexture().getTextureData().textureTarget,
				myFbo.getWidth(), myFbo.getHeight(), false);
		}
	}

	// NDI
	if (bNDIout) {
		if (!bNDIinitialized) {
			if (NDIsender.CreateSender(sendername, ResolutionWidth, ResolutionHeight)) {
				bNDIinitialized = true;
				// Reset buffers
				if (ndiBuffer[0].isAllocated()) ndiBuffer[0].clear();
				if (ndiBuffer[1].isAllocated()) ndiBuffer[1].clear();
				ndiBuffer[0].allocate(ResolutionWidth, ResolutionHeight, 4);
				ndiBuffer[1].allocate(ResolutionWidth, ResolutionHeight, 4);
				idx = 0;
			}
		}
		if (bNDIinitialized && myMovie.isFrameNew()) {
			myFbo.bind();
			glReadPixels(0, 0, ResolutionWidth, ResolutionHeight, GL_RGBA, GL_UNSIGNED_BYTE, ndiBuffer[0].getData());
			myFbo.unbind();
			NDIsender.SendImage(ndiBuffer[0].getData(), ResolutionWidth, ResolutionHeight);
		}
	}

	if (!bFullscreen) {

		drawPlayBar();

		if (bShowInfo) {
			ofSetColor(255);
			if (bLoaded) {
				sprintf_s(str, 256, "Sending as : [%s] (%dx%d)", sendername, ResolutionWidth, ResolutionHeight);
				myFont.drawString(str, 20, 30);
			}
			sprintf_s(str, 256, "fps: %3.3d", (int)fps);
			myFont.drawString(str, ofGetWidth() - 90, 30);
		}
	}
	

}

//--------------------------------------------------------------
void ofApp::drawPlayBar()
{
	char str[128];

	//
	// Play bar
	//

	// Position of the first button
	float icon_pos_x = icon_size / 2;
	float icon_pos_y = progress_bar.getTop() + progress_bar.height *1.5;

	// Draw controls unless the startup image is showing
	if (!bSplash) {

		// Show the control buttons and progress bar
		if (bShowControls) {

			controlbar_pos_y = (float)ofGetHeight() - controlbar_height;
			controlbar_width = (float)ofGetWidth();

			progress_bar.x = 0;
			progress_bar.width = (float)ofGetWidth();
			progress_bar.y = controlbar_pos_y;
			if (!bShowInfo)
				progress_bar.y += 14;

			ofEnableAlphaBlending();
			ofSetColor(26, 26, 26, 96);
			ofDrawRectangle(0, progress_bar.y, controlbar_width, controlbar_height);

			// If the movie is loaded, show the control bar
			// bLoaded is set in OpenMovieFile
			if (bLoaded) {

				float position = myMovie.getPosition() / myMovie.getDuration();

				// Draw progress bar
				ofSetColor(0, 0, 0, 96);
				ofDrawRectangle(progress_bar);

				// Draw the movie progress in blue
				ofSetColor(ofColor(74, 144, 226, 255));
				progress_bar_played.x = progress_bar.x;
				progress_bar_played.y = progress_bar.y;

				progress_bar_played.width = progress_bar.width*myMovie.getPosition(); // pct
				progress_bar_played.height = progress_bar.height;
				ofDrawRectangle(progress_bar_played);

				// Reverse
				icon_reverse_pos_x = icon_pos_x;
				icon_reverse_pos_y = icon_pos_y;
				if (bPaused && icon_reverse_hover)
					ofSetColor(icon_highlight_color);
				else
					ofSetColor(icon_background_color);
				icon_background.x = icon_reverse_pos_x;
				icon_background.y = icon_reverse_pos_y;
				icon_background.width = icon_size;
				icon_background.height = icon_size;
				ofDrawRectRounded(icon_background, 2);
				ofSetColor(255);
				icon_reverse.draw(icon_reverse_pos_x, icon_reverse_pos_y);

				// Back
				icon_back_pos_x = icon_pos_x + 1.0*(icon_size * 3 / 2);
				icon_back_pos_y = icon_pos_y;
				if (bPaused && icon_back_hover)
					ofSetColor(icon_highlight_color);
				else
					ofSetColor(icon_background_color);
				icon_background.x = icon_back_pos_x;
				icon_background.y = icon_back_pos_y;
				icon_background.width = icon_size;
				icon_background.height = icon_size;
				ofDrawRectRounded(icon_background, 2);
				ofSetColor(255);
				icon_back.draw(icon_back_pos_x, icon_back_pos_y);

				// Play/Pause
				icon_playpause_pos_x = icon_pos_x + 2.0*(icon_size * 3 / 2);
				icon_playpause_pos_y = icon_pos_y;
				if (icon_playpause_hover)
					ofSetColor(icon_highlight_color);
				else
					ofSetColor(icon_background_color);
				icon_background.x = icon_playpause_pos_x;
				icon_background.y = icon_playpause_pos_y;
				icon_background.width = icon_size;
				icon_background.height = icon_size;
				ofDrawRectRounded(icon_background, 2);

				// Draw play or pause button
				ofSetColor(255);
				if (bPaused)
					icon_play.draw(icon_playpause_pos_x, icon_playpause_pos_y);
				else
					icon_pause.draw(icon_playpause_pos_x, icon_playpause_pos_y);

			} // endif movie loaded

			// Forward
			icon_forward_pos_x = icon_pos_x + 3.0*(icon_size * 3 / 2);
			icon_forward_pos_y = icon_pos_y;
			if (bPaused && icon_forward_hover)
				ofSetColor(icon_highlight_color);
			else
				ofSetColor(icon_background_color);
			icon_background.x = icon_forward_pos_x;
			icon_background.y = icon_forward_pos_y;
			icon_background.width = icon_size;
			icon_background.height = icon_size;
			ofDrawRectRounded(icon_background, 2);
			ofSetColor(255);
			icon_forward.draw(icon_forward_pos_x, icon_forward_pos_y);

			// Fast forward
			icon_fastforward_pos_x = icon_pos_x + 4.0*(icon_size * 3 / 2);
			icon_fastforward_pos_y = icon_pos_y;
			if (bPaused && icon_fastforward_hover)
				ofSetColor(icon_highlight_color);
			else
				ofSetColor(icon_background_color);
			icon_background.x = icon_fastforward_pos_x;
			icon_background.y = icon_fastforward_pos_y;
			icon_background.width = icon_size;
			icon_background.height = icon_size;
			ofDrawRectRounded(icon_background, 2);
			ofSetColor(255);
			icon_fastforward.draw(icon_fastforward_pos_x, icon_fastforward_pos_y);

			// Full screen
			icon_fullscreen_pos_x = ofGetWidth() - (icon_size*1.5);
			icon_fullscreen_pos_y = icon_pos_y;
			if (icon_fullscreen_hover)
				ofSetColor(icon_highlight_color);
			else
				ofSetColor(icon_background_color);
			icon_background.x = icon_fullscreen_pos_x;
			icon_background.y = icon_fullscreen_pos_y;
			icon_background.width = icon_size;
			icon_background.height = icon_size;
			ofDrawRectRounded(icon_background, 2);
			ofSetColor(255);
			icon_full_screen.draw(icon_fullscreen_pos_x, icon_fullscreen_pos_y);

			// Sound play / mute
			icon_sound_pos_x = ofGetWidth() - (icon_size*3.0);
			icon_sound_pos_y = icon_pos_y;
			if (icon_sound_hover)
				ofSetColor(icon_highlight_color);
			else
				ofSetColor(icon_background_color);
			icon_background.x = icon_sound_pos_x;
			icon_background.y = icon_sound_pos_y;
			icon_background.width = icon_size;
			icon_background.height = icon_size;
			ofDrawRectRounded(icon_background, 2);

			// Draw sound button
			ofSetColor(255);
			if (bMute)
				icon_mute.draw(icon_sound_pos_x, icon_sound_pos_y);
			else
				icon_sound.draw(icon_sound_pos_x, icon_sound_pos_y);

			ofDisableAlphaBlending();
		} // endif show controls
	} // endif no splash image

	// Show keyboard shortcuts if not full screen
	if (!bFullscreen && bShowInfo) {
		sprintf_s(str, 256, "'  ' show controls : 'h' hide info : 'p' pause : 'm' mute : 'f' fullscreen");
		myFont.drawString(str, (ofGetWidth() - myFont.stringWidth(str)) / 2, (ofGetHeight() - 3));
		ofSetColor(255);
	}
	// ============ end playbar controls ==============
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

	if (!bResizeWindow) {
		RECT rect;
		GetWindowRect(hWnd, &rect);
		windowWidth = (float)(rect.right - rect.left);
		windowHeight = (float)(rect.bottom - rect.top);
	}
}


//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

	if (!OpenMovieFile(dragInfo.files[0])) {
		// doMessageBox(NULL, "File load failed", "Error", MB_ICONERROR);
		return;
	}
	else {
		bLoaded = true;
		movieFile = dragInfo.files[0];
		bPaused = false;
	}

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

	// Close volume dialog
	CloseVolume(); 

	// Escape key exit has been disabled but it can still be checked here
	if (key == VK_ESCAPE) {
		// Disable fullscreen set, otherwise quit the application as usual
		if (bFullscreen) {
			bFullscreen = false;
			doFullScreen(false);
		}
		else {
			if (doMessageBox(NULL, "Exit - are your sure?", "Warning", MB_YESNO) == IDYES) {
				// SPOUT
				if (bInitialized)
					spoutsender->ReleaseSender(); // Release the sender
				// NDI
				if (bNDIinitialized) NDIsender.ReleaseSender();
				ofExit();
			}
		}
	}

	if (key == 'f' || key == 'F') {
		bFullscreen = !bFullscreen;
		doFullScreen(bFullscreen);
		// Do not check this menu item
		// If there is no menu when you call the SetPopupItem function it will crash
	}

	if (key == 'l' || key == 'L') {
		if (bLoaded) {
			bLoop = !bLoop;
			menu->SetPopupItem("Loop", bLoop);
		}
	}

	if (key == 'm' || key == 'm') {
		if (bLoaded) {
			bMute = !bMute;
			if(bMute)
				myMovie.setVolume(0.0f);
			else
				myMovie.setVolume(movieVolume);
			menu->SetPopupItem("Mute", bMute);
		}
	}

	if (key == ' ') {
		if (bLoaded) {
			bShowControls = !bShowControls;
			menu->SetPopupItem("Controls", bShowControls);
		}
	}

	if (key == 'h') {
		bShowInfo = !bShowInfo;
		menu->SetPopupItem("Info", bShowInfo);
	}

	if (key == 'p') {
		bPaused = !bPaused;
		if(bLoaded) 
			myMovie.setPaused(bPaused);
	}

	// Go to the start of the movie
	if (key == OF_KEY_HOME) {
		if (bLoaded) {
			myMovie.setPosition(0.0f);
			if (!bPaused)
				myMovie.play();
		}
	}

	// Go to the end of the movie
	if (key == OF_KEY_END) {
		if (bLoaded) {
			myMovie.setPosition(myMovie.getDuration());
			if (!bPaused) {
				bPaused = true;
				myMovie.setPaused(bPaused);
			}
		}
	}

	// Hits an icon whether show info or not
	float y = (float)ofGetHeight()-icon_size;

	// Back one frame if paused
	if (key == OF_KEY_LEFT)
		HandleControlButtons(66.0f, y);

	// Forward one frame if paused
	if (key == OF_KEY_RIGHT)
		HandleControlButtons(148.0f, y);

	// Back 8 frames
	if (key == OF_KEY_PAGE_UP)
		HandleControlButtons(28.0f, y);

	// Forward 8 frames
	if (key == OF_KEY_PAGE_DOWN)
		HandleControlButtons(188.0f, y);

}


//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

	// RH click in window area to show/hide controls
	if (button == 2 && bLoaded && y < (ofGetHeight() - (int)(icon_size*2.0))) {
		bShowControls = !bShowControls;
		CloseVolume();
		drawPlayBar();
	}

	HandleControlButtons((float)x, (float)y, button);

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

	// Return if a messagebox is open
	if (bMessageBox) return;

	icon_reverse_hover = false;
	icon_back_hover = false;
	icon_playpause_hover = false;
	icon_forward_hover = false;
	icon_fastforward_hover = false;
	icon_fullscreen_hover = false;
	icon_sound_hover = false;

	if (bShowControls && !bFullscreen) {

		if (x >= (icon_playpause_pos_x) &&
			x <= (icon_playpause_pos_x + icon_size) &&
			y >= (icon_playpause_pos_y) &&
			y <= (icon_playpause_pos_y + icon_size)) {
			icon_playpause_hover = true;
		}

		if (x >= (icon_fullscreen_pos_x) &&
			x <= (icon_fullscreen_pos_x + icon_size) &&
			y >= (icon_fullscreen_pos_y) &&
			y <= (icon_fullscreen_pos_y + icon_size)) {
			icon_fullscreen_hover = true;
		}

		if (x >= (icon_reverse_pos_x) &&
			x <= (icon_reverse_pos_x + icon_size) &&
			y >= (icon_reverse_pos_y) &&
			y <= (icon_reverse_pos_y + icon_size)) {
			icon_reverse_hover = true;
		}

		if (x >= (icon_back_pos_x) &&
			x <= (icon_back_pos_x + icon_size) &&
			y >= (icon_back_pos_y) &&
			y <= (icon_back_pos_y + icon_size)) {
			icon_back_hover = true;
		}

		if (x >= (icon_forward_pos_x) &&
			x <= (icon_forward_pos_x + icon_size) &&
			y >= (icon_forward_pos_y) &&
			y <= (icon_forward_pos_y + icon_size)) {
			icon_forward_hover = true;
		}

		if (x >= (icon_fastforward_pos_x) &&
			x <= (icon_fastforward_pos_x + icon_size) &&
			y >= (icon_fastforward_pos_y) &&
			y <= (icon_fastforward_pos_y + icon_size)) {
			icon_fastforward_hover = true;
		}

		if (x >= (icon_sound_pos_x) &&
			x <= (icon_sound_pos_x + icon_size) &&
			y >= (icon_sound_pos_y) &&
			y <= (icon_sound_pos_y + icon_size)) {
			icon_sound_hover = true;
		}

	} // end controls hover

}


//--------------------------------------------------------------
void ofApp::exit() {

	if (bInitialized) {
		spoutsender->ReleaseSender();
		spoutsender->Release(); // Release the Spout SDK library instance
	}

	if (bNDIinitialized) 
		NDIsender.ReleaseSender();

	char initfile[MAX_PATH];
	GetModuleFileNameA(NULL, initfile, MAX_PATH);
	PathRemoveFileSpecA(initfile);

	// Save ini file to load again on start
	strcat_s(initfile, MAX_PATH, "\\SpoutVideoPlayer.ini");
	WriteInitFile(initfile);

	// Remove dialog keyboard hook
	if(hHook)
		UnhookWindowsHookEx(hHook);

}

//--------------------------------------------------------------
void ofApp::HandleControlButtons(float x, float y, int button) {

	// handle clicking on progress bar (trackbar)
	bool bPaused = false;
	int frame = 0;
	float position = 0.0f;
	float frametime = 0.0333333333333333; // 30 fps

	if(bLoaded)
		bPaused = myMovie.isPaused();

	if (bLoaded &&
		x >= progress_bar.x &&
		x <= (progress_bar.x + progress_bar.getWidth()) &&
		y >= progress_bar.y &&
		y <= (progress_bar.y + progress_bar.getHeight())) {

		// Click on progress bar
		float pos = (x - progress_bar.x) / progress_bar.width;
		myMovie.setPosition(pos);
		if (bPaused)
			myMovie.setPaused(true);

		controlbar_timer_end = false;
		controlbar_start_time = ofGetElapsedTimeMillis();

		return;

	} // endif trackbar

	//
	// handle clicking on buttons
	//

	// Reverse
	if (bLoaded && bPaused &&
		x >= (icon_reverse_pos_x) &&
		x <= (icon_reverse_pos_x + icon_size) &&
		y >= (icon_reverse_pos_y) &&
		y <= (icon_reverse_pos_y + icon_size)) {
		// Skip back 8 frames if paused
		frame = myMovie.getCurrentFrame();
		if (frame > 8)
			myMovie.setFrame(frame - 8);
		else
			myMovie.setFrame(1);
	}

	// Back
	else if (bLoaded &&	bPaused &&
		x >= (icon_back_pos_x) &&
		x <= (icon_back_pos_x + icon_size) &&
		y >= (icon_back_pos_y) &&
		y <= (icon_back_pos_y + icon_size)) {
			myMovie.previousFrame();
	}

	// Play / pause
	else if (bLoaded &&
		x >= (icon_playpause_pos_x) &&
		x <= (icon_playpause_pos_x + icon_size) &&
		y >= (icon_playpause_pos_y) &&
		y <= (icon_playpause_pos_y + icon_size)) {
		setVideoPlaypause();
	}

	// Forward
	else if (bLoaded &&	bPaused &&
		x >= (icon_forward_pos_x) &&
		x <= (icon_forward_pos_x + icon_size) &&
		y >= (icon_forward_pos_y) &&
		y <= (icon_forward_pos_y + icon_size)) {
			myMovie.nextFrame();
	}

	// Fast forward
	else if (bLoaded && bPaused &&
		x >= (icon_fastforward_pos_x) &&
		x <= (icon_fastforward_pos_x + icon_size) &&
		y >= (icon_fastforward_pos_y) &&
		y <= (icon_fastforward_pos_y + icon_size)) {
		// Skip forward if paused
		int frame = myMovie.getCurrentFrame();
		if (frame < myMovie.getTotalNumFrames() - 8) {
			// Not sure why, but nextFrame() is needed
			// after setFrame() or next frame is one to few
			myMovie.setFrame(frame + 7);
			myMovie.nextFrame();
			frame = myMovie.getCurrentFrame();
		}
		else {
			myMovie.setFrame(myMovie.getTotalNumFrames());
		}
	}

	// Full screen
	else if (myMovie.isLoaded() &&
		x >= (icon_fullscreen_pos_x) &&
		x <= (icon_fullscreen_pos_x + icon_size) &&
		y >= (icon_fullscreen_pos_y) &&
		y <= (icon_fullscreen_pos_y + icon_size)) {
		bFullscreen = !bFullscreen;
			doFullScreen(bFullscreen);
	}

	// Sound
	else if (myMovie.isLoaded() &&
		x >= (icon_sound_pos_x) &&
		x <= (icon_sound_pos_x + icon_size) &&
		y >= (icon_sound_pos_y) &&
		y <= (icon_sound_pos_y + icon_size)) {
		// LH click to open/close for volume slider
		if (button == 0) {
			if (hwndVolume)
				CloseVolume();
			else
				hwndVolume = CreateDialogA(g_hInstance, MAKEINTRESOURCEA(IDD_OPTIONSBOX), hWnd, (DLGPROC)UserVolume);
		}
		else if (button == 2) {
			// RH click to mute
			bMute = !bMute;
			if (bMute)
				myMovie.setVolume(0.0f);
			else
				myMovie.setVolume(movieVolume);
		}
	}

}


//--------------------------------------------------------------
void ofApp::setVideoPlaypause() {

	controlbar_timer_end = false;
	controlbar_start_time = ofGetElapsedTimeMillis();
	bPaused = !bPaused;

	if (bPaused)
		bShowControls = true;

	if(bLoaded)
		myMovie.setPaused(bPaused);

}

//--------------------------------------------------------------
bool ofApp::OpenMovieFile(string filePath) {

	// Close volume dialog
	CloseVolume();

	// Stop if playing
	if (bLoaded) {
		myMovie.stop();
		myMovie.close();
		bLoaded = false;
	}

	bLoaded = myMovie.load(filePath);
	if (bLoaded) {
			
		// For test play 15 frames in case of incompatible codec to avoid a freeze
		nOldFrames = 0;
		nNewFrames = 0;

		// fps is calculated when playing
		fps = frameRate = 30.0;

		bPaused = false;
		myMovie.setPosition(0.0f);

		if (bLoop)
			myMovie.setLoopState(OF_LOOP_NORMAL);
		else
			myMovie.setLoopState(OF_LOOP_NONE);

		myMovie.setVolume(movieVolume);

		myMovie.play();

	}
	else {
		doMessageBox(NULL, "Could not load the file\nMake sure you have codecs installed on your system.\nOF recommends the free K - Lite Codec pack.", "SpoutVideoPlayer", MB_ICONERROR);
		bLoaded = false;
		bSplash = true;
		return false;
	}

	if(bLoaded) {

		movieFile = filePath;
		bSplash = false;

		movieWidth  = myMovie.getWidth();
		movieHeight = myMovie.getHeight();

		if (bUseMovieResolution) {
			ResolutionWidth  = movieWidth;
			ResolutionHeight = movieHeight;
		}

		if(bResizeWindow)
			ResetWindow(true);

		// Re-allocate buffers
		if(myFbo.isAllocated()) myFbo.clear();
		myFbo.allocate(ResolutionWidth, ResolutionHeight);

		if (ndiBuffer[0].isAllocated()) ndiBuffer[0].clear();
		if (ndiBuffer[1].isAllocated()) ndiBuffer[1].clear();
		ndiBuffer[0].allocate(ResolutionWidth, ResolutionHeight, 4); // TODO : check
		ndiBuffer[1].allocate(ResolutionWidth, ResolutionHeight, 4);

		// Release senders to recreate
		if (bInitialized) {
			spoutsender->ReleaseSender();
			bInitialized = false;
		}

		if (bNDIinitialized) {
			NDIsender.ReleaseSender();
			bNDIinitialized = false;
		}

		// Set the sender name to the movie file name
		strcpy_s(sendername, 256, filePath.c_str());
		PathStripPathA(sendername);
		PathRemoveExtensionA(sendername);

	}

	// It got this far OK
	return true;

}


//--------------------------------------------------------------
void ofApp::ResetWindow(bool bCentre)
{
	if (bSplash)
		return;

	// Close volume dialog
	CloseVolume();

	// Default desired client size
	RECT rect;
	GetClientRect(hWnd, &rect);
	windowWidth = (float)(rect.right - rect.left);
	windowHeight = (float)(rect.bottom - rect.top);

	if (bResizeWindow) {

		if (movieWidth < 1280) {
			// Less than 1280 wide so use the movie dimensions
			windowWidth  = movieWidth;
			windowHeight = movieHeight;
		}
		else {
			// Larger than 1280 so set standard 1280 wide and adjust window height
			// to match movie aspect ratio
			windowWidth = 1280;
			windowHeight = windowWidth*movieHeight / movieWidth;
		}
	}

	// Restore topmost state
	HWND hWndMode = HWND_TOP;
	if (bTopmost)
		hWndMode = HWND_TOPMOST;

	// Adjust window to desired client size allowing for the menu
	rect.left = 0;
	rect.top = 0;
	rect.right = windowWidth;
	rect.bottom = windowHeight;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW | WS_BORDER, true);

	// Full window size
	windowWidth  = (float)(rect.right - rect.left);
	windowHeight = (float)(rect.bottom - rect.top);

	// Get current position
	GetWindowRect(hWnd, &rect);
	
	// Set size and optionally centre on the screen
	if (bCentre) {
		SetWindowPos(hWnd, hWndMode,
			(ofGetScreenWidth() - windowWidth) / 2,
			(ofGetScreenHeight() - windowHeight) / 2,
			windowWidth, windowHeight, SWP_SHOWWINDOW);
	}
	else {
		SetWindowPos(hWnd, hWndMode,
			rect.left,
			rect.top,
			windowWidth, windowHeight, SWP_SHOWWINDOW);
	}

}

//--------------------------------------------------------------
//
// Menu function callback
//
// This function is called by ofxWinMenu when an item is selected.
// The the title and state can be checked for required action.
// 
void ofApp::appMenuFunction(string title, bool bChecked) {

	ofFileDialogResult result;
	string filePath;


	// Keep the movie in sync while the menu stops drawing
	if (title == "WM_ENTERMENULOOP") {
		myMovie.setPaused(true);
	}

	if (title == "WM_EXITMENULOOP") {
		if (bLoaded) { // TODO
			if (!bPaused) {
				myMovie.play();
			}
		}
		bMenuExit = true;
	}

	//
	// File menu
	//
	if (title == "Open movie") {
		result = ofSystemLoadDialog("Select a video file", false);
		if (result.bSuccess) {
			if (OpenMovieFile(result.getPath())) {
				if (bLoaded) {
					if (bLoop)
						myMovie.setLoopState(OF_LOOP_NORMAL);
					else
						myMovie.setLoopState(OF_LOOP_NONE);
					myMovie.play();
					bPaused = false;
				}
			}
		}
	}

	if (title == "Open movie folder") {
		if (bLoaded) {
			char tmp[MAX_PATH];
			strcpy_s(tmp, MAX_PATH, movieFile.c_str());
			PathRemoveFileSpecA(tmp);
			ShellExecuteA(g_hwnd, "open", tmp, NULL, NULL, SW_SHOWNORMAL);
		}
	}

	if (title == "Exit") {
		ofExit(); // Quit the application
	}

	//
	// View menu
	//

	if (title == "Loop") {
		bLoop = bChecked;
		if (bLoop)
			myMovie.setLoopState(OF_LOOP_NORMAL);
		else
			myMovie.setLoopState(OF_LOOP_NONE);
	}

	if (title == "Mute") {
		bMute = bChecked;
		if (bLoaded) {
			if (bMute)
				myMovie.setVolume(0.0f);
			else
				myMovie.setVolume(movieVolume);
		}
	}

	if (title == "Controls") {

		if (bSplash) {
			doMessageBox(NULL, "No video loaded", "SpoutVideoPlayer", MB_ICONERROR);
			return;
		}
		CloseVolume();
		bShowControls = !bShowControls;  // Flag is used elsewhere in Draw
		menu->SetPopupItem("Controls", bShowControls);
	}

	if (title == "Resolution") {
		// Resolution dialog
		if (EnterResolution()) {
			// Reset the resolution and textures
			if (bInitialized) {
				spoutsender->ReleaseSender(); // Release the sender
				bInitialized = false;
			}
			if (bNDIinitialized) {
				NDIsender.ReleaseSender();
				bNDIinitialized = false;
			}
			myFbo.allocate(ResolutionWidth, ResolutionHeight, GL_RGB);
		}
	}

	if (title == "Show on top") {
		bTopmost = bChecked;
		doTopmost(bTopmost);
	}

	
	if (title == "Info") {
		bShowInfo = bChecked;
	}

	if (title == "Full screen") {
		bFullscreen = !bFullscreen; // Not auto-checked and also used in the keyPressed function
		doFullScreen(bFullscreen); // But take action immediately
	}

	if (title == "Resize to movie") {
		bResizeWindow = !bResizeWindow;
		// Adjust window and centre on the screen
		if(bResizeWindow)
			ResetWindow(true);
		menu->SetPopupItem("Resize to movie", bResizeWindow);
	}

	if (title == "Spout") {
		// Auto-check
		bSpoutOut = bChecked;
		if (!bSpoutOut) {
			if (bInitialized) {
				spoutsender->ReleaseSender(); // Release the sender
				bInitialized = false;
			}
		}
	}

	if (title == "NDI") {
		// Auto-check
		bNDIout = bChecked;
		if (!bNDIout) {
			if (bNDIinitialized) {
				NDIsender.ReleaseSender(); // Release the sender
				bNDIinitialized = false;
			}
		}
	}


	//
	// Help menu
	//

	if (title == "About") {
		// Keep the movie in sync while the menu stops drawing
		if (bLoaded) myMovie.setPaused(true);
		DialogBoxA(g_hInstance, MAKEINTRESOURCEA(IDD_ABOUTBOX), hWnd, About);
		if (bLoaded && !bPaused) myMovie.setPaused(false);
	}

	if (title == "Information") {
		doMessageBox(NULL, info, "Information", MB_OK);
	}

} // end appMenuFunction


//--------------------------------------------------------------
void ofApp::doFullScreen(bool bFullscreen)
{
	RECT rectTaskBar;
	HWND hWndTaskBar;
	HWND hWndMode;

	// Close volume dialog
	CloseVolume();

	if (bFullscreen) {

		// set to full screen

		// Remove the controls if shown
		bShowControls = false;

		// get the current top window
		hWndForeground = GetForegroundWindow();

		// Get the client/window adjustment values
		GetWindowRect(hWnd, &windowRect);
		GetClientRect(hWnd, &clientRect);
		AddX = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
		AddY = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);

		// Get current client window size for return
		nonFullScreenX = ofGetWidth();
		nonFullScreenY = ofGetHeight();

		// find the OpenGL window
		g_hwnd = WindowFromDC(wglGetCurrentDC());
		GetWindowRect(g_hwnd, &windowRect); // preserve current size values
		GetClientRect(g_hwnd, &clientRect);
		dwStyle = GetWindowLongPtrA(g_hwnd, GWL_STYLE);
		SetWindowLongPtrA(g_hwnd, GWL_STYLE, WS_VISIBLE); // no other styles but visible

		// Remove the menu but don't destroy it
		menu->RemoveWindowMenu();

		hWndTaskBar = FindWindowA("Shell_TrayWnd", "");
		GetWindowRect(hWnd, &rectTaskBar);

		// Hide the System Task Bar
		SetWindowPos(hWndTaskBar, HWND_NOTOPMOST, 0, 0, (rectTaskBar.right - rectTaskBar.left), (rectTaskBar.bottom - rectTaskBar.top), SWP_NOMOVE | SWP_NOSIZE);
		
		// Allow for multiple monitors
		HMONITOR monitor = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		GetMonitorInfoA(monitor, &mi);
		int x = (int)mi.rcMonitor.left;
		int y = (int)mi.rcMonitor.top;
		int w = (int)(mi.rcMonitor.right - mi.rcMonitor.left); // rcMonitor dimensions are LONG
		int h = (int)(mi.rcMonitor.bottom - mi.rcMonitor.top);
		// Setting HWND_TOPMOST causes a grey screen for Windows 10
		// if scaling is set larger than 100%. This seems to fix it.
		// Topmost is restored when returning from full screen.
		SetWindowPos(g_hwnd, HWND_NOTOPMOST, x, y, w, h, SWP_HIDEWINDOW);
		SetWindowPos(g_hwnd, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW);
		ShowCursor(FALSE);

		SetFocus(g_hwnd);
		
	} // endif bFullscreen
	else {

		// Exit full screen

		// restore original style
		SetWindowLongPtrA(hWnd, GWL_STYLE, dwStyle);

		// Restore the menu
		menu->SetWindowMenu();

		// Restore topmost state
		if (bTopmost)
			hWndMode = HWND_TOPMOST;
		else
			hWndMode = HWND_TOP;

		// Restore our window
		SetWindowPos(g_hwnd, hWndMode, windowRect.left, windowRect.top, nonFullScreenX + AddX, nonFullScreenY + AddY, SWP_SHOWWINDOW);

		// Reset the window that was top before - could be ours
		if (GetWindowLong(hWndForeground, GWL_EXSTYLE) & WS_EX_TOPMOST)
			SetWindowPos(hWndForeground, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		else
			SetWindowPos(hWndForeground, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		ShowCursor(TRUE);

		DrawMenuBar(hWnd);

		// Show cursor for all modes
		ofShowCursor();

	} // endif not bFullscreen

}


//--------------------------------------------------------------
void ofApp::doTopmost(bool bTop)
{
	if (bTop) {
		// get the current top window for return
		hWndForeground = GetForegroundWindow();
		// Set this window topmost
		hWnd = WindowFromDC(wglGetCurrentDC());
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		ShowWindow(hWnd, SW_SHOW);
	}
	else {
		hWnd = WindowFromDC(wglGetCurrentDC());
		SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		ShowWindow(hWnd, SW_SHOW);
		// Reset the window that was topmost before
		if (GetWindowLong(hWndForeground, GWL_EXSTYLE) & WS_EX_TOPMOST)
			SetWindowPos(hWndForeground, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		else
			SetWindowPos(hWndForeground, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
} // end doTopmost


  //--------------------------------------------------------------
  // Save a configuration file in the executable folder
void ofApp::WriteInitFile(const char *initfile)
{
	char tmp[MAX_PATH];

	//
	// OPTIONS
	//	
	if (bLoop)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"loop", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"loop", (LPCSTR)"0", (LPCSTR)initfile);

	if (bResizeWindow)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"resize", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"resize", (LPCSTR)"0", (LPCSTR)initfile);

	if (bTopmost)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"topmost", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"topmost", (LPCSTR)"0", (LPCSTR)initfile);

	if (bSpoutOut)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Spout", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"Spout", (LPCSTR)"0", (LPCSTR)initfile);

	if (bNDIout)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"NDI", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"NDI", (LPCSTR)"0", (LPCSTR)initfile);

	//
	// RESOLUTION
	//
	if (bUseMovieResolution)
		WritePrivateProfileStringA((LPCSTR)"Resolution", (LPCSTR)"usemovie", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Resolution", (LPCSTR)"usemovie", (LPCSTR)"0", (LPCSTR)initfile);

	// If fit to movie size
	if (bUseMovieResolution) {
		sprintf_s(tmp, 256, "%-8.0d", ResolutionWidth);
		tmp[8] = 0;
	}
	else {
		sprintf_s(tmp, 256, "1280");
	}
	WritePrivateProfileStringA((LPCSTR)"Resolution", (LPCSTR)"width", (LPCSTR)tmp, (LPCSTR)initfile);

	if (bUseMovieResolution) {
		sprintf_s(tmp, 256, "%-8.0d", ResolutionHeight);
		tmp[8] = 0;
	}
	else {
		sprintf_s(tmp, 256, "720");
	}
	WritePrivateProfileStringA((LPCSTR)"Resolution", (LPCSTR)"height", (LPCSTR)tmp, (LPCSTR)initfile);

	// Volume
	sprintf_s(tmp, 256, "%-8.2f", movieVolume); tmp[8] = 0;
	WritePrivateProfileStringA((LPCSTR)"Audio", (LPCSTR)"volume", (LPCSTR)tmp, (LPCSTR)initfile);


}

//--------------------------------------------------------------
void ofApp::ReadInitFile()
{
	char initfile[MAX_PATH];
	char tmp[MAX_PATH];

	GetModuleFileNameA(NULL, initfile, MAX_PATH);
	PathRemoveFileSpecA(initfile);
	strcat_s(initfile, MAX_PATH, "\\SpoutVideoPlayer.ini");

	//
	// OPTIONS
	//
	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"loop", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bLoop = (atoi(tmp) == 1);

	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"resize", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bResizeWindow = (atoi(tmp) == 1);

	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"topmost", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bTopmost = (atoi(tmp) == 1);

	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"Spout", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bSpoutOut = (atoi(tmp) == 1);

	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"NDI", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bNDIout = (atoi(tmp) == 1);

	//
	// RESOLUTION
	//
	GetPrivateProfileStringA((LPCSTR)"Resolution", (LPSTR)"usemovie", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bUseMovieResolution = (atoi(tmp) == 1);

	ResolutionWidth = (unsigned int)ofGetWidth();
	ResolutionHeight = (unsigned int)ofGetHeight();

	if (GetPrivateProfileStringA((LPCSTR)"Resolution", (LPSTR)"width", (LPSTR)"1280", (LPSTR)tmp, 8, initfile) > 0)
		ResolutionWidth = atoi(tmp);

	if (GetPrivateProfileStringA((LPCSTR)"Resolution", (LPSTR)"height", (LPSTR)"720", (LPSTR)tmp, 8, initfile) > 0)
		ResolutionHeight = atoi(tmp);

	// Volume
	if (GetPrivateProfileStringA((LPCSTR)"Audio", (LPSTR)"volume", (LPSTR)"1.00", (LPSTR)tmp, 8, initfile) > 0)
		movieVolume = atof(tmp);

	// Set up menus etc - assume menu has been set
	menu->SetPopupItem("Loop", bLoop);
	menu->SetPopupItem("Resize to movie", bResizeWindow);
	menu->SetPopupItem("Topmost", bTopmost);
	menu->SetPopupItem("Spout", bSpoutOut);
	menu->SetPopupItem("NDI", bNDIout);

}


//
// DIALOGS
//

int ofApp::doMessageBox(HWND hwnd, LPCSTR message, LPCSTR caption, UINT uType)
{
	int iRet = 0;

	bMessageBox = true; // To skip mouse events

	// Pause the movie or it still plays in the background
	if (bLoaded)
		myMovie.setPaused(true);

	// Keep the messagebox topmost
	iRet = MessageBoxA(hwnd, message, caption, uType | MB_TOPMOST);

	if (bLoaded && !bPaused)
		myMovie.setPaused(false);

	bMessageBox = false;

	return iRet;

}


bool ofApp::EnterResolution()
{
	int retvalue;
	char xtextin[256];
	char ytextin[256];
	char xresin[256];
	char yresin[256];

	sprintf_s(xtextin, 256, "%d", ResolutionWidth);
	szXtext = (PSTR)xtextin; // move to a static string for the dialog
	sprintf_s(ytextin, 256, "%d", ResolutionHeight);
	szYtext = (PSTR)ytextin;

	if (bLoaded) {
		sprintf_s(xresin, 256, "%d", (int)myMovie.getWidth());
		szResolutionX = (PSTR)xresin;
		sprintf_s(yresin, 256, "%d", (int)myMovie.getHeight());
		szResolutionY = (PSTR)yresin;
	}
	else {
		sprintf_s(xresin, 256, "1280");
		szResolutionX = (PSTR)xresin;
		sprintf_s(yresin, 256, "720");
		szResolutionY = (PSTR)yresin;
	}

	// Set the current movie size
	busemovie = bUseMovieResolution;

	// Keep the movie in sync while the menu stops drawing
	if (bLoaded)
		myMovie.setPaused(true);

	// Show the dialog box 
	retvalue = DialogBoxA(g_hInstance, MAKEINTRESOURCEA(IDD_RESOLUTION), hWnd, (DLGPROC)UserResolution);

	if (bLoaded && !bPaused) 
		myMovie.setPaused(false);

	// OK 
	if (retvalue != 0) {

		bUseMovieResolution = busemovie;

		if (bUseMovieResolution) {
			if (bLoaded) {
				ResolutionWidth  = (unsigned int)myMovie.getWidth();
				ResolutionHeight = (unsigned int)myMovie.getHeight();
			}
		}
		else {
			ResolutionWidth = atoi(szXtext);
			// Width a multiple of 4 for video
			ResolutionWidth = (ResolutionWidth / 4) * 4;
			ResolutionHeight = atoi(szYtext);
			ResolutionHeight = (ResolutionHeight / 2) * 2;
		}

		return true;
	}

	// Cancel
	return false;

}

// Message handler for About box
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	char tmp[MAX_PATH];
	char about[1024];
	DWORD dummy, dwSize;
	LPDRAWITEMSTRUCT lpdis;
	HWND hwnd = NULL;
	HCURSOR cursorHand = NULL;
	HINSTANCE hInstance = GetModuleHandle(NULL);

	switch (message) {

	case WM_INITDIALOG:

		sprintf_s(about, 256, "  Spout Video Player - Version ");
		// Get product version number
		if (GetModuleFileNameA(hInstance, tmp, MAX_PATH)) {
			dwSize = GetFileVersionInfoSizeA(tmp, &dummy);
			if (dwSize > 0) {
				vector<BYTE> data(dwSize);
				if (GetFileVersionInfoA(tmp, NULL, dwSize, &data[0])) {
					LPVOID pvProductVersion = NULL;
					unsigned int iProductVersionLen = 0;
					if (VerQueryValueA(&data[0], ("\\StringFileInfo\\080904E4\\ProductVersion"), &pvProductVersion, &iProductVersionLen)) {
						sprintf_s(tmp, MAX_PATH, "%s\n", (char *)pvProductVersion);
						strcat_s(about, 1024, tmp);
					}
				}
			}
		}

		// Newtek credit - see resource,rc
		strcat_s(about, 1024, "\n\n");
		strcat_s(about, 1024, "  NewTek NDI� - Version ");
		// Add NewTek library version number (dll)
		strcat_s(about, 1024, NDInumber.c_str());
		strcat_s(about, 1024, "\n  NDI� is a trademark of NewTek, Inc.");
		SetDlgItemTextA(hDlg, IDC_ABOUT_TEXT, (LPCSTR)about);

		//
		// Hyperlink hand cursor
		//
		
		// Spout
		cursorHand = LoadCursor(NULL, IDC_HAND);
		hwnd = GetDlgItem(hDlg, IDC_SPOUT_URL);
		SetClassLongA(hwnd, GCL_HCURSOR, (long)cursorHand);

		// NDI
		hwnd = GetDlgItem(hDlg, IDC_NEWTEC_URL);
		SetClassLong(hwnd, GCL_HCURSOR, (long)cursorHand);
		break;

	case WM_DRAWITEM:

		// The blue hyperlinks
		lpdis = (LPDRAWITEMSTRUCT)lParam;
		if (lpdis->itemID == -1) break;
		SetTextColor(lpdis->hDC, RGB(6, 69, 173));
		switch (lpdis->CtlID) {
			case IDC_NEWTEC_URL:
				DrawTextA(lpdis->hDC, "https://www.ndi.tv/", -1, &lpdis->rcItem, DT_LEFT);
				break;
			case IDC_SPOUT_URL:
				DrawTextA(lpdis->hDC, "http://spout.zeal.co", -1, &lpdis->rcItem, DT_LEFT);
				break;
			default:
				break;
		}
		break;

	case WM_COMMAND:

		if (LOWORD(wParam) == IDC_NEWTEC_URL) {
			sprintf_s(tmp, 256, "https://ndi.tv/");
			ShellExecuteA(hDlg, "open", tmp, NULL, NULL, SW_SHOWNORMAL);
			EndDialog(hDlg, 0);
			return (INT_PTR)TRUE;
		}

		if (LOWORD(wParam) == IDC_SPOUT_URL) {
			sprintf_s(tmp, 256, "http://spout.zeal.co");
			ShellExecuteA(hDlg, "open", tmp, NULL, NULL, SW_SHOWNORMAL);
			EndDialog(hDlg, 0);
			return (INT_PTR)TRUE;
		}

		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// Message handler for resolution entry
LRESULT CALLBACK UserResolution(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam); // suppress warning

	static HWND hBar, hBox, hFocus;

	switch (message) {

	case WM_INITDIALOG:

		// Fill current values
		if (busemovie) {
			SetDlgItemTextA(hDlg, IDC_RESOLUTION_X, szResolutionX);
			SetDlgItemTextA(hDlg, IDC_RESOLUTION_Y, szResolutionY);
			CheckDlgButton(hDlg, IDC_RESOLUTION_DEFAULT, BST_CHECKED);
			CheckDlgButton(hDlg, IDC_RESOLUTION_CUSTOM, BST_UNCHECKED);
			EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_X), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_Y), FALSE);
		}
		else {
			SetDlgItemTextA(hDlg, IDC_RESOLUTION_X, szXtext);
			SetDlgItemTextA(hDlg, IDC_RESOLUTION_Y, szYtext);
			CheckDlgButton(hDlg, IDC_RESOLUTION_DEFAULT, BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_RESOLUTION_CUSTOM, BST_CHECKED);
			EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_X), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_Y), TRUE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDC_RESOLUTION_DEFAULT :

			if (IsDlgButtonChecked(hDlg, IDC_RESOLUTION_DEFAULT) == BST_CHECKED) {
				busemovie = true;
				CheckDlgButton(hDlg, IDC_RESOLUTION_CUSTOM, BST_UNCHECKED);
				SetDlgItemTextA(hDlg, IDC_RESOLUTION_X, szResolutionX);
				SetDlgItemTextA(hDlg, IDC_RESOLUTION_Y, szResolutionY);
				EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_X), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_Y), FALSE);
			}
			else {
				busemovie = false;
				CheckDlgButton(hDlg,  IDC_RESOLUTION_CUSTOM, BST_CHECKED);
				SetDlgItemTextA(hDlg, IDC_RESOLUTION_X, szXtext);
				SetDlgItemTextA(hDlg, IDC_RESOLUTION_Y, szYtext);
				EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_X), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_Y), TRUE);
			}
			break;

		case IDC_RESOLUTION_CUSTOM:

			if (IsDlgButtonChecked(hDlg, IDC_RESOLUTION_CUSTOM) == BST_CHECKED) {
				busemovie = false;
				EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_X), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_Y), TRUE);
				CheckDlgButton(hDlg, IDC_RESOLUTION_DEFAULT, BST_UNCHECKED);
				SetDlgItemTextA(hDlg, IDC_RESOLUTION_X, szXtext);
				SetDlgItemTextA(hDlg, IDC_RESOLUTION_Y, szYtext);
			}
			else {
				busemovie = true;
				EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_X), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_Y), FALSE);
				CheckDlgButton(hDlg, IDC_RESOLUTION_DEFAULT, BST_CHECKED);
				SetDlgItemTextA(hDlg, IDC_RESOLUTION_X, szResolutionX);
				SetDlgItemTextA(hDlg, IDC_RESOLUTION_Y, szResolutionY);
			}
			break;

		case IDC_RESET:
			sprintf_s(szXtext, 256, "1280");
			SetDlgItemTextA(hDlg, IDC_RESOLUTION_X, szXtext);
			sprintf_s(szYtext, 256, "720");
			SetDlgItemTextA(hDlg, IDC_RESOLUTION_Y, szYtext);
			CheckDlgButton(hDlg, IDC_RESOLUTION_CUSTOM, BST_CHECKED);
			CheckDlgButton(hDlg, IDC_RESOLUTION_DEFAULT, BST_UNCHECKED);
			EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_X), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_Y), TRUE);
			break;

		case IDOK:
			// Get contents of edit fields into static 256 char string
			GetDlgItemTextA(hDlg, IDC_RESOLUTION_X, szXtext, 256);
			GetDlgItemTextA(hDlg, IDC_RESOLUTION_Y, szYtext, 256);
			if (IsDlgButtonChecked(hDlg, IDC_RESOLUTION_DEFAULT) == BST_CHECKED)
				busemovie = true;
			else
				busemovie = false;
			EndDialog(hDlg, 1);
			break;

		case IDCANCEL:
			// User pressed cancel. Just take down dialog box.
			EndDialog(hDlg, 0);
			return TRUE;
		default:
			return FALSE;
		}
		break;
	}

	return FALSE;
}

// Message handler for Volume control dialog
LRESULT CALLBACK UserVolume(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam); // suppress warning

	static HWND hBar, hBox, hFocus;
	static int iPos, dn, TrackBarPos;
	float fValue;

	switch (message) {

	case WM_INITDIALOG:
		// Open the window to the left of the volume icon
		int x, y, w, h;
		RECT rect;
		GetWindowRect(hDlg, &rect);
		w = rect.right - rect.left;
		h = rect.bottom - rect.top;
		GetWindowRect(pThis->hWnd, &rect);
		x = rect.right  - w - (int)pThis->icon_size * 3.5; // (int)(icon_sound_pos_x - icon_size * 1.5);
		y = rect.bottom - h - (int)pThis->icon_size; // (int)icon_sound_pos_y;
		if (!pThis->bShowInfo)
			y += 14;
		SetWindowPos(hDlg, HWND_TOPMOST, x,	y, w, h, SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW);

		// Set the scroll bar limits and text
		hBar = GetDlgItem(hDlg, IDC_TRACKBAR);
		SendMessage(hBar, TBM_SETRANGEMIN, (WPARAM)1, (LPARAM)0);
		SendMessage(hBar, TBM_SETRANGEMAX, (WPARAM)1, (LPARAM)100);
		TrackBarPos = (int)(pThis->movieVolume * 100.0f);
		SendMessage(hBar, TBM_SETPOS, (WPARAM)1, (LPARAM)TrackBarPos);

		return TRUE;

	// https://msdn.microsoft.com/en-us/library/windows/desktop/hh298416(v=vs.85).aspx
	case WM_HSCROLL:
		hBar = GetDlgItem(hDlg, IDC_TRACKBAR);
		iPos = SendMessage(hBar, TBM_GETPOS, 0, 0);
		if (iPos > 100)
			SendMessage(hBar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)100);
		else if (iPos < 0) SendMessage(hBar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)0);
		TrackBarPos = iPos;
		fValue = ((float)iPos) / 100.0f;
		if (fValue < 0.0) fValue = 0.0f;
		if (fValue > 1.0) fValue = 1.0f;
		pThis->movieVolume = fValue;
		pThis->myMovie.setVolume(fValue);
		break;
		
	case WM_DESTROY:
		DestroyWindow(hwndVolume);
		hwndVolume = NULL;
		break;
	}

	return FALSE;
}

LRESULT CALLBACK KeyProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0 && (KF_UP & HIWORD(lParam)) != 0)
	{
		// Remove volume control dialog for any key
		if (hwndVolume) {
			DestroyWindow(hwndVolume);
			hwndVolume = NULL;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void ofApp::CloseVolume()
{
	if (hwndVolume) {
		DestroyWindow(hwndVolume);
		hwndVolume = NULL;
	}
}
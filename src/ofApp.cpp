/*

	Spout Video Player

	A simple video player
	with Spout and NDI output

	Copyright (C) 2017-2024 Lynn Jarvis.

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
	20.04.22	- Update with Openframeworks 11 and Visual Studio 2022
				  Change to GCLP_HICON and GCLP_HCURSOR for SetClassLong
				  SpoutLibrary updated to VS2022
				  Updated ofxNDI addon
				  Include VS2022 runtime dlls in bin folder
	10.05.22	- Changes thoughout to simplify the program
				  Notes :
					Performance limited or load may fail for high resolution (4K) videos
					Debug build very slow
	16.05.22	- Rebuild Release /MD Win32
				  Tested with K-Lite codec pack 16.9.8 (Basic - April 15th 2022)
				  https://codecguide.com/download_k-lite_codec_pack_basic.htm
				  Resolution is limited to 1920x1080 for best performance
				  4K videos may fail to load.
				  Version 2.000
	21.12.22	- Change SetWindowLong to SetWindowLongPtr for custom icon
				  Add mouse click test for outside client area to pause movie
				  Add WM_NCLBUTTONDOWN to ofxWinMenu
				  Rebuild /MD x64 with updated ofxNDI, SpoutLibrary and NDI 5.5.2
				  Version 2.001
	13.10.23	- Replace AboutBox dialog with SpoutMessageBox
				  Replace Information box with SpoutMessageBox
				  Reduce icon height for status bar
	25.10.23	- Add Adjust dialog and compute shaders
				  Add stop button
				  Add command line
	10.11.23	- Add contrast adaptive sharpening
			      Sharpness range changed from 0-400 to 0-100
	04.03.24	- Rebuild VS 2022 /MT x64 for Openframeworks 12.0
				  with updated ofxNDI, ofxWinMenu, SpoutGL, SpoutLibrary and NDI 5.6.0
				  Version 2.002

*/
#include "ofApp.h"

static string NDInumber; // NDI library version number
static HINSTANCE g_hInstance = NULL;

// volume control modal dialog
LRESULT CALLBACK UserVolume(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static HWND hwndVolume = NULL;

//
// Adjustment controls modeless dialog
//
LRESULT CALLBACK UserAdjust(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static HWND hwndAdjust = NULL;

// ofApp class pointer for the dialog to access class variables
static ofApp* pThis = NULL;

// Hook for volume dialog keyboard detection
HHOOK hHook = NULL;
LRESULT CALLBACK KeyProc(int nCode, WPARAM wParam, LPARAM lParam);


//--------------------------------------------------------------
void ofApp::setup(){

	ofBackground(255, 255, 255);

	// Initialize SpoutLibrary
	spoutsender = GetSpout();

	// Get instance and window for dialogs
	g_hInstance = GetModuleHandle(NULL);
	g_hWnd = ofGetWin32Window();


	// Debug console window so printf works
	// Note use of WinMain in main.cpp and 
	// Linker > System > Subsystem setting
	// spoutsender->OpenSpoutConsole();
	// printf("Spout Video Player\n");
	// Option to show Spout logs
	// spoutsender->EnableSpoutLog(); 

	// Window title
	char title[256];
	strcpy_s(title, 256, "Spout Video Player");
	// Get product version number
	DWORD dummy, dwSize;
	char temp[MAX_PATH];
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
	SetClassLongPtrA(hWnd, GCLP_HICON, (LONG_PTR)LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(IDI_SPOUTICON)));

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
	menu->AddPopupItem(hPopup, "Adjust", false, false);
	bShowControls = false;  // don't show controls yet
	menu->AddPopupItem(hPopup, "Controls");
	bLoop = false;  // movie loop
	menu->AddPopupItem(hPopup, "Loop");
	bMute = false; // Audio mute
	menu->AddPopupItem(hPopup, "Mute");
	bResizeWindow = false; // not resizing
	menu->AddPopupItem(hPopup, "Resize", false, false); // Not checked and not auto-check
	bShowInfo = false;
	menu->AddPopupItem(hPopup, "Info", false, true); // Not checked and auto-check
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
	menu->AddPopupSeparator(hPopup);
	bNDIout = false;
	menu->AddPopupItem(hPopup, "NDI", false);  // Not checked
	// Add NDI options
	menu->AddPopupItem(hPopup, "    Async", false);  // Not checked
	menu->EnablePopupItem("    Async", false); // Until "NDI" is checked

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
	strcat_s(info, 1024, "\nControls\n");
	strcat_s(info, 1024, "  RH click volume    mute audio\n");
	strcat_s(info, 1024, "  LH click volume    adjust audio\n");
	strcat_s(info, 1024, "  <<  go to start\n");
	strcat_s(info, 1024, "   <   back one frame if paused\n");
	strcat_s(info, 1024, "   | |   play / pause\n");
	strcat_s(info, 1024, "   >   forward one frame if paused\n");
	strcat_s(info, 1024, "  >>  go to end\n");
	strcat_s(info, 1024, "  [  ]  stop and close movie\n");
	strcat_s(info, 1024, "\nKeys\n");
	strcat_s(info, 1024, "  SPACE show / hide controls\n");
	strcat_s(info, 1024, "  'i'         show / hide information\n");
	strcat_s(info, 1024, "  'p'	        play / pause\n");
	strcat_s(info, 1024, "  'm'	       toggle mute\n");
	strcat_s(info, 1024, "  'r'	        resize window\n");
	strcat_s(info, 1024, "  'f'	         full screen\n");
	strcat_s(info, 1024, "  'ESC'    exit full screen\n");
	strcat_s(info, 1024, "  LEFT/RIGHT   back/forward one frame\n");
	strcat_s(info, 1024, "  PGUP/PGDN  back/forward 8 frames\n");
	strcat_s(info, 1024, "  HOME/END   start/end of video\n");
	strcat_s(info, 1024, "  's'	        stop and close movie\n\n");
	strcat_s(info, 1024, "  RH click window - show / hide Adjust dialog\n");

	// Load splash screen
	bSplash = true;
	splashImage.load("images/SpoutVideoPlayer.png");

	// Read ini file to get bLoop, bSpoutOut, bNDIout, bNDIasync and bTopmost flags
	ReadInitFile();

	//
	// Play bar
	//

	// icons
	icon_size = 20; // 22; // 27; // 32; // 64;
	icon_reverse.load("icons/reverse.png");
	icon_fastforward.load("icons/fastforward.png");
	icon_stop.load("icons/stop.png");
	icon_back.load("icons/back.png");
	icon_play.load("icons/play.png");
	icon_pause.load("icons/pause.png");
	icon_forward.load("icons/forward.png");
	icon_full_screen.load("icons/full_screen.png");
	icon_sound.load("icons/speaker.png");
	icon_mute.load("icons/speaker_mute.png");

	icon_reverse.resize(icon_size, icon_size);
	icon_fastforward.resize(icon_size, icon_size);
	icon_stop.resize(icon_size, icon_size);
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
	icon_stop_hover = false;
	icon_back_hover = false;
	icon_forward_hover = false;
	icon_fullscreen_hover = false;
	icon_sound_hover = false;

	icon_highlight_color = ofColor(ofColor(74, 144, 226, 255));
	icon_background_color = ofColor(128, 128, 128, 255);

	progress_bar.width = ofGetWidth();
	progress_bar.height = 12;

	controlbar_width = ofGetWidth();
	controlbar_height = icon_size + progress_bar.height * 3;
	controlbar_pos_y = ofGetHeight();

	controlbar_timer_end = false;
	controlbar_start_time = ofGetElapsedTimeMillis();
	bShowControls = false;

	//
	// SPOUT
	//

	bInitialized = false; // Spout sender initialization
	bNDIinitialized = false; // NDI sender intiialization
	strcpy_s(sendername, 256, "Spout Video Player"); // Set the sender name

	//
	// NDI
	//

	// Asynchronous sending instead of clocked at the movie fps
	NDIsender.SetAsync(bNDIasync);
	// Get NewTek library version number (dll) for the about box
	// Version number is the last 7 chars - e.g 2.1.0.3
	string NDIversion = NDIsender.GetNDIversion();
	NDInumber = NDIversion.substr(NDIversion.length() - 7, 7);

	// For movie frame fps calculations
	// independent of the rendering rate
	startTime = lastTime = frameTime = 0.0;

	// starting fps value
	fps = frameRate = 30.0;

	// Keyboard hook for volume dialog
	hHook = SetWindowsHookExA(WH_KEYBOARD, KeyProc, NULL, GetCurrentThreadId());

	// Set RGBA pixel format for Spout, NDI and shaders
	myMovie.setPixelFormat(OF_PIXELS_RGBA);

	// Movie pixels alpha may be zero
	// If NDI format set to RGBX and will produce alpha = 255
	// Studio Monitor : Settings > Video > Show alpha should be checked off
	NDIsender.SetFormat(NDIlib_FourCC_video_type_RGBX);

	// Necessary to draw fbo
	ofDisableAlphaBlending();

	// Command line for movie file and image adjustment
	if (lpCmdLine && *lpCmdLine) {
		Brightness = 0.0; // -1 - 1
		Contrast   = 1.0; //  0 - 4
		Saturation = 1.0; //  0 - 4
		Gamma      = 0.0; //  0 - 4
		Sharpness  = 0.0; //  0 - 4
		bAdaptive  = false;
		ParseCommandLine(lpCmdLine);
		
		/*
		printf("[%s]\n", movieFile.c_str());
		printf("Brightness = %f\n", Brightness);
		printf("Brightness = %f\n", Brightness);
		printf("Contrast   = %f\n", Contrast);
		printf("Saturation = %f\n", Saturation);
		printf("Gamma      = %f\n", Gamma);
		printf("Sharpness  = %f\n", Sharpness);
		*/

	}

	// For hide/show controls
	start = std::chrono::steady_clock::now();
	end = std::chrono::steady_clock::now();

	// Command line movie file
	if (!movieFile.empty()) {
		if (OpenMovieFile(movieFile)) {
			myMovie.setPaused(false);
			myMovie.play();
			bLoaded = true;
			bPaused = false;
			if (bFullscreen)
				doFullScreen(true);
		}
	}

}

void ofApp::ParseCommandLine(LPSTR lpCmdLine) {

	// printf("lpCmdLine [%s]\n", lpCmdLine);

	// Remove double quotes
	std::string line = lpCmdLine;
	line.erase(std::remove(line.begin(), line.end(), '\"'), line.end());

	// Movie name
	std::string argstr;
	argstr = FindArgString(line, "-movie");
	if (!argstr.empty()) {
		movieFile = argstr;
		// printf("movie [%s]\n", argstr.c_str());
	}

	argstr = FindArgString(line, "-brightness");
	if (!argstr.empty()) {
		Brightness = atof(argstr.c_str());
	}

	 argstr = FindArgString(line, "-contrast");
	 if (!argstr.empty()) {
		 Contrast = atof(argstr.c_str());
	 }

	argstr = FindArgString(line, "-saturation");
	if (!argstr.empty()) {
		Saturation = atof(argstr.c_str());
	}

	argstr = FindArgString(line, "-gamma");
	if (!argstr.empty()) {
		Gamma = atof(argstr.c_str());
	}

	argstr = FindArgString(line, "-sharpness");
	if (!argstr.empty()) {
		Sharpness = atof(argstr.c_str());
	}
	
	argstr = FindArgString(line, "-adaptive");
	if (!argstr.empty()) {
		bAdaptive = (atoi(argstr.c_str()) == 1);
	}

	argstr = FindArgString(line, "-fullscreen");
	if (!argstr.empty()) {
		bFullscreen = (atoi(argstr.c_str()) == 1);
	}

}

std::string ofApp::FindArgString(std::string line, std::string arg)
{
	std::string argstr;

	// Start of arg string preceded by "-"
	size_t pos = line.find(arg);
	if (pos != std::string::npos) {
		argstr = line.substr(pos+1); // skip the space
		// printf("0 [%s]\n", argstr.c_str());
		pos = argstr.find("movie");
		if (pos != std::string::npos) {  // A movie name with extension "-movie name.ext "
			argstr = argstr.substr(pos+6); // Skip the arg
			// printf("1 [%s]\n", argstr.c_str());
			// Skip to the next stop preceding the extension
			pos = argstr.find(".");
			if (pos != std::string::npos) {
				argstr = argstr.substr(0, pos+4);
				// printf("2 [%s]\n", argstr.c_str());
			}
		}
		else { // brightness, contrast etc. Get the value.
			pos = argstr.find(" ");
			if (pos != std::string::npos) {
				argstr = argstr.substr(pos+1);
				// printf("3 [%s]\n", argstr.c_str());
				// find the next arg "-" if any
				pos = argstr.find("-");
				if (pos != std::string::npos) {
					argstr = argstr.substr(0, pos-1);
					// printf("4 [%s]\n", argstr.c_str());
				}
			}
		}
	}

	return argstr;

}



//--------------------------------------------------------------
void ofApp::update(){

	if (bLoaded) {

		myMovie.update();

		// Attach the movie frame to an fbo with rgba internal format
		// necessary for shaders. Also the movie frame alpha may be zero.
		myFbo.attachTexture(myMovie.getTexture(), GL_RGBA8, 0);

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

			// Activate shaders on the received texture.
			// Shaders have source and destination textures but the source
			// can also be the destination. Compute shader extensions are 
			// loaded when a sender is created in Draw().
			if (bInitialized) {

				GLuint myTextureID  = myFbo.getTexture().getTextureData().textureID;
				unsigned int width  = spoutsender->GetSenderWidth();
				unsigned int height = spoutsender->GetSenderHeight();

				// Brightness    -1 - 1   default 0
				// Contrast       0 - 4   default 1
				// Saturation     0 - 4   default 1
				// Gamma          0 - 4   default 1
				// 0.005 - 0.007 msec
				if (Brightness     != 0.0
					|| Contrast    != 1.0
					|| Saturation  != 1.0
					|| Gamma       != 1.0) {
					shaders.Adjust(myTextureID, myTextureID,
						width, height, Brightness, Contrast, Saturation, Gamma);
				}

				// Blur 0 - 4  (default 0)
				// 0.001 - 0.002 msec
				if (Blur > 0.0) {
					shaders.Blur(myTextureID, myTextureID, width, height, Blur);
				}

				// Sharpness 0 - 1   default 0
				// 0.001 - 0.002 msec
				if (Sharpness > 0.0) {
					if (bAdaptive) {
						// Sharpness width radio buttons
						// 3x3, 5x5, 7x7 : 3.0, 5.0, 7.0
						float caswidth = 1.0f+(Sharpwidth-3.0f)/2.0f; // 1.0, 2.0, 3.0
						// Sharpness; // 0.0 - 1.0
						shaders.AdaptiveSharpen(myTextureID,
							width, height, caswidth, Sharpness);
					}
					else {
						shaders.Sharpen(myTextureID, myTextureID, width, height, Sharpwidth, Sharpness);
					}
				}

				if (bFlip)
					shaders.Flip(myTextureID, width, height);
				if (bMirror)
					shaders.Mirror(myTextureID, width, height);
				if (bSwap)
					shaders.Swap(myTextureID, width, height);

			}
						
			// Calculate movie fps
			lastTime = startTime;
			startTime = ofGetElapsedTimeMicros();
			frameTime = (startTime - lastTime) / 1000000; // seconds
			if (frameTime > 0.01) {
				frameRate = round(1.0 / frameTime + 0.5);
				// damping from a starting fps value
				fps *= 0.98;
				fps += 0.02 * frameRate;
			}

		}
		else {
			if (!bPaused) {
				nOldFrames++;
				if (nOldFrames > 60 && nNewFrames < 61) { // 2 seconds at 30 fps

					// Cannot call close for failure
					bLoaded = false;
					bSplash = true;

					// Release the senders
					spoutsender->ReleaseSender();
					bInitialized = false;
					NDIsender.ReleaseSender();
					bNDIinitialized = false;

					doMessageBox(NULL, "Could not play the movie - unknown error - try loading again.\nResolutions greater than 1920x1080 can cause problems.\n\nMake sure you have installed the required codecs\nOF recommends the free K - Lite Codec pack.\nHowever, some formats do not play.\n\nProblems noted with WMV3 and MP42 codecs\nTry converting the file to another format.", "SpoutVideoPlayer", MB_ICONERROR);
					return;
				}
			}
		}
	}

}

//--------------------------------------------------------------
void ofApp::draw() {

	char str[256]{};
	ofSetColor(255);
	ofBackground(0);

	// Continue play if paused by menu selection
	// or mouse click outside the client area
	if (bNCmousePressed) {
		if (myMovie.isLoaded())
			myMovie.setPaused(false);
		bNCmousePressed = false;
	}

	if (bSplash || !bLoaded) {
		splashImage.draw(0, 0, ofGetWidth(), ofGetHeight());
		return;
	}

	// Draw the movie frame sized to the aspect ratio of the movie
	float drawWidth = ofGetHeight()*movieWidth/movieHeight;
	float leftx = (ofGetWidth()-drawWidth)/2.0f;
	myFbo.draw(leftx, 0, drawWidth, ofGetHeight());

	if (myMovie.isFrameNew()) {

		//
		// Spout
		//
		if (bSpoutOut) {
			// If not initialized, create a Spout sender the same size as the movie
			// (sendername is initialized by movie load)
			if (!bInitialized) {
				bInitialized = spoutsender->CreateSender(sendername,
					(unsigned int)myFbo.getWidth(), (unsigned int)myFbo.getHeight());
			}
			else {
				// Receivers will detect the movie frame rate
				spoutsender->SendTexture(myFbo.getTexture().getTextureData().textureID,
					myFbo.getTexture().getTextureData().textureTarget,
					(unsigned int)myFbo.getWidth(), (unsigned int)myFbo.getHeight(), false);
			}
		}

		//
		// NDI
		//
		if (bNDIout) {
			if (!bNDIinitialized) {
				bNDIinitialized = NDIsender.CreateSender(sendername,
					(unsigned int)myMovie.getWidth(), (unsigned int)myMovie.getHeight());
			}
			else {
				// Send the movie pixels
				// NDI format set to RGBX will produce alpha = 255
				NDIsender.SendImage(myMovie.getPixels().getData(),
					(unsigned int)myMovie.getWidth(), (unsigned int)myMovie.getHeight());
			}
		}

	} // endif new frame

	// 'Space" to show or hide controls
	drawPlayBar();

	ofSetColor(255);
	if (bLoaded && !bFullscreen && bShowInfo) {

		if (spoutsender->IsInitialized()) {
			sprintf_s(str, 256, "Sending as : [%s] (%dx%d)", sendername, (int)myMovie.getWidth(), (int)myMovie.getHeight());
			myFont.drawString(str, 20, 20);
			sprintf_s(str, 256, "fps: %3.3d", (int)fps);
			myFont.drawString(str, ofGetWidth() - 90, 20);
		}

		sprintf_s(str, 256, "Space - show controls : RH click - adjust dialog");
		myFont.drawString(str, 20, 40);
		sprintf_s(str, 256, "'f' fullscreen : 'i' hide info : Help menu for details");
		myFont.drawString(str, 20, 60);

	}

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

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
			if (doMessageBox(NULL, "Escape exit - are you sure?", "Warning", MB_YESNO | MB_ICONWARNING) == IDYES) {
				ofExit();
			}
		}
	}

	if (key == 'f' || key == 'F') {
		bFullscreen = !bFullscreen;
		doFullScreen(bFullscreen);
		// Do not check this menu item because if there is no menu
		// when you call the SetPopupItem function it will crash
	}

	if (key == 'l' || key == 'L') {
		if (bLoaded) {
			bLoop = !bLoop;
			menu->SetPopupItem("Loop", bLoop);
		}
	}

	if (key == 'm' || key == 'M') {
		if (bLoaded) {
			bMute = !bMute;
			if (bMute)
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
			if(!bShowControls && bFullscreen && hwndAdjust) {
				// Close adjust dialog if open full screen
				// It can be opened with mouse click when the controls are visible
				SendMessageA(hwndAdjust, WM_DESTROY, 0, 0L);
				hwndAdjust = NULL;
			}
		}
	}

	if (key == 'i' || key == 'I') {
		bShowInfo = !bShowInfo;
		menu->SetPopupItem("Info", bShowInfo);
	}

	if (key == 'p' || key == 'P') {
		bPaused = !bPaused;
		if (bLoaded)
			myMovie.setPaused(bPaused);
	}

	if (key == 'r' || key == 'R') {
		if (bLoaded) {
			bResizeWindow = !bResizeWindow;
			ResetWindow(true);
			menu->SetPopupItem("Resize", bResizeWindow);
		}
	}

	// Stop and close movie
	if (key == 's' || key == 'S') {
		CloseMovie();
	}

	// Go to the start of the movie
	if (key == OF_KEY_HOME) {
		if (bLoaded) {
			myMovie.setPosition(0.0f);
			bPaused = false;
			myMovie.play();
		}
	}

	// Go to the end of the movie
	if (key == OF_KEY_END) { // 49 (0x31) 57363
		if (bLoaded) {
			myMovie.setPosition(myMovie.getDuration());
			bPaused = false;
			myMovie.play();
		}
	}

	// Hits an icon whether show info or not
	float y = (float)ofGetHeight() - icon_size;

	// Back one frame if paused
	if (key == OF_KEY_LEFT) // 52 (0x34) 57356
		HandleControlButtons(66.0f, y);

	// Forward one frame if paused
	if (key == OF_KEY_RIGHT) // 54 (0x36) 57358
		HandleControlButtons(148.0f, y);

	// Back 8 frames
	if (key == OF_KEY_PAGE_UP)
		HandleControlButtons(28.0f, y);

	// Forward 8 frames
	if (key == OF_KEY_PAGE_DOWN)
		HandleControlButtons(188.0f, y);
}


//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){


	// Return if a messagebox is open
	if (bMessageBox) return;

	end = std::chrono::steady_clock::now();
	double elapsed = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0);

	icon_reverse_hover = false;
	icon_back_hover = false;
	icon_playpause_hover = false;
	icon_forward_hover = false;
	icon_fastforward_hover = false;
	icon_stop_hover = false;
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

		if (x >= (icon_stop_pos_x) &&
			x <= (icon_stop_pos_x + icon_size) &&
			y >= (icon_stop_pos_y) &&
			y <= (icon_stop_pos_y + icon_size)) {
			icon_stop_hover = true;
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
void ofApp::mousePressed(int x, int y, int button){

	// Right click in window area when controls are visible to show/hide adjust dialog
	if (bLoaded) {
		if (button == 2) {
			if (bFullscreen && bShowControls)
				appMenuFunction("Adjust", false);
			else if (!bFullscreen)
				appMenuFunction("Adjust", false);
		}
	}

	HandleControlButtons((float)x, (float)y, button);

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

	if (!bResizeWindow) {
		RECT rect{};
		GetWindowRect(hWnd, &rect);
		windowWidth = (float)(rect.right - rect.left);
		windowHeight = (float)(rect.bottom - rect.top);
	}

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) { 

	if (OpenMovieFile(dragInfo.files[0])) {
		myMovie.setPaused(false);
		myMovie.play();
		movieFile = dragInfo.files[0];
		bLoaded = true;
		bPaused = false;
	}
	else {
		bLoaded = false;
	}

}


//--------------------------------------------------------------
void ofApp::exit() {

	spoutsender->ReleaseSender();
	spoutsender->Release(); // Release the Spout SDK library instance
	NDIsender.ReleaseSender();

	// Get ini file path for read and write
	char initfile[MAX_PATH];
	GetModuleFileNameA(NULL, initfile, MAX_PATH);
	PathRemoveFileSpecA(initfile);
	strcat_s(initfile, MAX_PATH, "\\SpoutVideoPlayer.ini");

	// Save ini file to load again on start
	WriteInitFile(g_InitFile);

	// Remove dialog keyboard hook
	if (hHook)
		UnhookWindowsHookEx(hHook);

}


//--------------------------------------------------------------
void ofApp::drawPlayBar()
{
	char str[256]{};

	//
	// Play bar
	//

	// Position of the first button
	float icon_pos_x = icon_size / 2;
	float icon_pos_y = progress_bar.getTop() + progress_bar.height * 1.5;

	// Draw controls unless the startup image is showing
	if (!bSplash) {

		// Show the control buttons and progress bar
		if (bShowControls) {

			if (bFullscreen) {
				CURSORINFO info{};
				info.cbSize =sizeof(CURSORINFO);
				GetCursorInfo(&info);
				if (!info.hCursor) ShowCursor(TRUE);
			}

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

				progress_bar_played.width = progress_bar.width * myMovie.getPosition(); // pct
				progress_bar_played.height = progress_bar.height;
				ofDrawRectangle(progress_bar_played);

				// Reverse
				icon_reverse_pos_x = icon_pos_x;
				icon_reverse_pos_y = icon_pos_y;
				// if (bPaused && icon_reverse_hover)
				if (icon_reverse_hover)
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
				icon_back_pos_x = icon_pos_x + 1.0 * (icon_size * 3 / 2);
				icon_back_pos_y = icon_pos_y;
				// if (bPaused && icon_back_hover)
				if (icon_back_hover)
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
				icon_playpause_pos_x = icon_pos_x + 2.0 * (icon_size * 3 / 2);
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
			icon_forward_pos_x = icon_pos_x + 3.0 * (icon_size * 3 / 2);
			icon_forward_pos_y = icon_pos_y;
			if (icon_forward_hover)
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
			icon_fastforward_pos_x = icon_pos_x + 4.0 * (icon_size * 3 / 2);
			icon_fastforward_pos_y = icon_pos_y;
			if (icon_fastforward_hover)
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

			// Stop
			icon_stop_pos_x = icon_pos_x + 5.0 * (icon_size * 3 / 2);
			icon_stop_pos_y = icon_pos_y;
			if (icon_stop_hover)
				ofSetColor(icon_highlight_color);
			else
				ofSetColor(icon_background_color);
			icon_background.x = icon_stop_pos_x;
			icon_background.y = icon_stop_pos_y;
			icon_background.width = icon_size;
			icon_background.height = icon_size;
			ofDrawRectRounded(icon_background, 2);
			ofSetColor(255);
			icon_stop.draw(icon_stop_pos_x, icon_stop_pos_y);

			// Full screen
			icon_fullscreen_pos_x = ofGetWidth() - (icon_size * 1.5);
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
			icon_sound_pos_x = ofGetWidth() - (icon_size * 3.0);
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
		else {
			if (bFullscreen) {
				CURSORINFO info{};
				info.cbSize =sizeof(CURSORINFO);
				GetCursorInfo(&info);
				if (info.hCursor) ShowCursor(FALSE);
			}
		}
	} // endif no splash image

	// ============ end playbar controls ==============
}

//--------------------------------------------------------------
void ofApp::HandleControlButtons(float x, float y, int button) {

	// handle clicking on progress bar (trackbar)
	bool bPaused = false;
	int frame = 0;
	float position = 0.0f;
	float frametime = 0.0333333333333333; // 30 fps

	if (bLoaded) {
		bPaused = myMovie.isPaused();
	}

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

	// Reverse (go to start)
	if (bLoaded &&
		x >= (icon_reverse_pos_x) &&
		x <= (icon_reverse_pos_x + icon_size) &&
		y >= (icon_reverse_pos_y) &&
		y <= (icon_reverse_pos_y + icon_size)) {
			myMovie.setPosition(0);
	}

	// Back
	else if (bLoaded && bPaused &&
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
	else if (bLoaded && bPaused &&
		x >= (icon_forward_pos_x) &&
		x <= (icon_forward_pos_x + icon_size) &&
		y >= (icon_forward_pos_y) &&
		y <= (icon_forward_pos_y + icon_size)) {
		myMovie.nextFrame();
	}

	// Fast forward (go to end)
	else if (bLoaded &&
		x >= (icon_fastforward_pos_x) &&
		x <= (icon_fastforward_pos_x + icon_size) &&
		y >= (icon_fastforward_pos_y) &&
		y <= (icon_fastforward_pos_y + icon_size)) {
		// Show the last frame (-2 is minimum)
		myMovie.setFrame(myMovie.getTotalNumFrames()-2);
		myMovie.update();
	}

	// Stop (stop movie)
	else if (bLoaded &&
		x >= (icon_stop_pos_x) &&
		x <= (icon_stop_pos_x + icon_size) &&
		y >= (icon_stop_pos_y) &&
		y <= (icon_stop_pos_y + icon_size)) {
		CloseMovie();
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

	if (bLoaded)
		myMovie.setPaused(bPaused);

}

//--------------------------------------------------------------
bool ofApp::OpenMovieFile(string filePath) {

	// Close volume dialog
	CloseVolume();

	bLoaded = false;
	nOldFrames = 0;
	nNewFrames = 0;

	myMovie.stop();
	myMovie.close();
	
	// Load and check the duration in case the user loads an image
	if (myMovie.load(filePath) && myMovie.getDuration() > 0.0f) {

		// Play 60 frames in case of incompatible codec to avoid a freeze
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

		movieFile = filePath; // For movie folder open
		bSplash = false;

		movieWidth = myMovie.getWidth();
		movieHeight = myMovie.getHeight();

		if (bResizeWindow)
			ResetWindow(true);

		// Allocat an rgba fbno the size of the movie
		myFbo.allocate(movieWidth, movieHeight, GL_RGBA);

		// Release senders to recreate
		spoutsender->ReleaseSender();
		bInitialized = false;

		NDIsender.ReleaseSender();
		bNDIinitialized = false;

		// Set the sender name to the movie file name
		strcpy_s(sendername, 256, filePath.c_str());
		PathStripPathA(sendername);
		PathRemoveExtensionA(sendername);

		return true;

	}
	else {
		doMessageBox(NULL, "Could not load the movie file\nMake sure you have codecs installed on your system.\nOF recommends the free K - Lite Codec pack.", "SpoutVideoPlayer", MB_ICONERROR | MB_OK);
		bLoaded = false;
		bSplash = true;
		return false;
	}

}

void ofApp::CloseMovie() {

	// Close volume dialog
	CloseVolume();
	myMovie.stop();
	myMovie.close();

	nOldFrames = 0;
	nNewFrames = 0;
	movieWidth = 0;
	movieHeight = 0;
	bPaused = false;
	bLoaded = false;
	bSplash = true;

	// Release senders to recreate
	spoutsender->ReleaseSender();
	bInitialized = false;

	NDIsender.ReleaseSender();
	bNDIinitialized = false;

	bFullscreen = false;
	doFullScreen(false);
	bResizeWindow = false;
	ResetWindow(true);
	menu->SetPopupItem("Resize", false);

}

//--------------------------------------------------------------
void ofApp::ResetWindow(bool bCentre)
{
	// Close volume dialog
	CloseVolume();

	// Default desired client size
	RECT rect;
	GetClientRect(hWnd, &rect);
	windowWidth  = (float)(rect.right - rect.left);
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
			windowHeight = windowWidth * movieHeight / movieWidth;
		}
	}
	else {
		windowWidth  = 640;
		windowHeight = 360;
	}

	// Restore topmost state
	HWND hWndMode = HWND_TOP;
	if (bTopmost)
		hWndMode = HWND_TOPMOST;

	// Adjust window to desired client size allowing for the menu
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = windowWidth;
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
			(ofGetScreenWidth()  - windowWidth) / 2,
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

	// Keep the movie in sync while the menu selection or
	// mouse click on the title bar stops drawing.
	// WM_ENTERMENULOOP and WM_EXITMENULOOP are returned by ofxWinMenu
	// but are not required if WM_NCLBUTTONDOWN is tested.
	if (title == "WM_NCLBUTTONDOWN") {
		if (myMovie.isLoaded())
			myMovie.setPaused(true);
		// WM_NCLBUTTONUP is not generated if the
		// mouse is released on the title bar.
		// Reset the flag when Draw() resumes 
		bNCmousePressed = true;
		return;
	}

	//
	// File menu
	//
	if (title == "Open movie") {
		result = ofSystemLoadDialog("Select a video file", false);
		if (result.bSuccess) {
			if (OpenMovieFile(result.getPath())) {
				myMovie.setPaused(false);
				myMovie.play();
				movieFile = result.getPath();
				bLoaded = true;
				bPaused = false;
			}
			else {
				bLoaded = false;
			}
		}
	}

	if (title == "Open movie folder") {
		if (bLoaded) {
			char tmp[MAX_PATH];
			strcpy_s(tmp, MAX_PATH, movieFile.c_str());
			PathRemoveFileSpecA(tmp);
			ShellExecuteA(g_hWnd, "open", tmp, NULL, NULL, SW_SHOWNORMAL);
		}
		else {
			doMessageBox(NULL, "No movie loaded", "Warning", MB_ICONWARNING | MB_OK);
		}
	}

	if (title == "Exit") {
		if (doMessageBox(NULL, "Exit - are you sure?", "Warning", MB_ICONWARNING | MB_YESNO) == IDYES) {
			ofExit();
		}
	}

	//
	// View menu
	//

		// Image adjustment
	if (title == "Adjust") {
		if (bLoaded) {
			if (!hwndAdjust) {
				// Save old values for dialog Restore
				OldBrightness = Brightness;
				OldContrast   = Contrast;
				OldSaturation = Saturation;
				OldGamma      = Gamma;
				OldSharpness  = Sharpness;
				OldSharpwidth = Sharpwidth;
				OldAdaptive   = bAdaptive;
				OldBlur       = Blur;
				OldFlip       = bFlip;
				OldMirror     = bMirror;
				OldSwap       = bSwap;
				hwndAdjust = CreateDialogA(g_hInstance, MAKEINTRESOURCEA(IDD_ADJUSTBOX), g_hWnd, (DLGPROC)UserAdjust);
			}
			else {
				SendMessageA(hwndAdjust, WM_DESTROY, 0, 0L);
				hwndAdjust = NULL;
			}
		}
		else {
			doMessageBox(NULL, "No video loaded", "SpoutVideoPlayer", MB_ICONERROR);
		}
	}

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
		}
		else {
			CloseVolume();
			bShowControls = !bShowControls;  // Flag is used elsewhere in Draw
			menu->SetPopupItem("Controls", bShowControls);
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

	if (title == "Resize") {
		bResizeWindow = !bResizeWindow;
		// Adjust window and centre on the screen
		if (bLoaded) ResetWindow(true);
		menu->SetPopupItem("Resize", bResizeWindow);
	}

	if (title == "Spout") {
		// Auto-check
		bSpoutOut = bChecked;
		// If checked off close the Spout sender
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
			NDIsender.ReleaseSender(); // Release the sender
			bNDIinitialized = false;
			menu->EnablePopupItem("    Async", false);
		}
		else {
			menu->EnablePopupItem("    Async", true);
		}
	}

	if (title == "    Async") {
		// Auto-check
		bNDIasync = bChecked;
		// Enable or disable asynchronous sending
		NDIsender.SetAsync(bNDIasync);
	}

	//
	// Help menu
	//

	if (title == "About") {
		// Keep the movie in sync while the menu stops drawing
		if (bLoaded) myMovie.setPaused(true);
		char about[1024]{};
		DWORD dwSize = 0;
		DWORD dummy = 0;
		char tmp[MAX_PATH]{};
		sprintf_s(about, 256, "  Spout Video Player - Version ");
		// Get product version number
		if (GetModuleFileNameA(g_hInstance, tmp, MAX_PATH)) {
			dwSize = GetFileVersionInfoSizeA(tmp, &dummy);
			if (dwSize > 0) {
				vector<BYTE> data(dwSize);
				if (GetFileVersionInfoA(tmp, NULL, dwSize, &data[0])) {
					LPVOID pvProductVersion = NULL;
					unsigned int iProductVersionLen = 0;
					if (VerQueryValueA(&data[0], ("\\StringFileInfo\\080904E4\\ProductVersion"), &pvProductVersion, &iProductVersionLen)) {
						sprintf_s(tmp, MAX_PATH, "%s\n\n", (char*)pvProductVersion);
						strcat_s(about, 1024, tmp);
					}
				}
			}
		}

		// Spout version
		strcat_s(about, 1024, "                <a href=\"http://spout.zeal.co\">Spout</a>  ");
		sprintf_s(tmp, MAX_PATH, "%s\n", spoutsender->GetSDKversion().c_str());
		strcat_s(about, 1024, tmp);

		// Newtek credit
		strcat_s(about, 1024, "                <a href=\"https://www.ndi.tv/\">NDI</a>     ");
		strcat_s(about, 1024, NDInumber.c_str());

		HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SPOUTICON));
		spoutsender->SpoutMessageBoxIcon(hIcon);
		spoutsender->SpoutMessageBox(NULL, about, "About", MB_USERICON | MB_OK);
		if (bLoaded && !bPaused) myMovie.setPaused(false);
	}

	if (title == "Information") {
		doMessageBox(NULL, info, "Information", MB_OK | MB_ICONINFORMATION);
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

		//
		// Set full screen
		//

		// Close adjust dialog if open
		// It can be opened with middle click when the controls are visible
		if (hwndAdjust) {
			SendMessageA(hwndAdjust, WM_DESTROY, 0, 0L);
			hwndAdjust = NULL;
		}

		// Optional
		// Remove the controls if shown
		// bShowControls = false;

		// Get the current top window
		hWndForeground = GetForegroundWindow();

		// Get the client/window adjustment values
		GetWindowRect(hWnd, &windowRect);
		GetClientRect(hWnd, &clientRect);
		AddX = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
		AddY = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);

		// Get current client window size for return
		nonFullScreenX = ofGetWidth();
		nonFullScreenY = ofGetHeight();

		// Find the OpenGL window
		g_hWnd = WindowFromDC(wglGetCurrentDC());
		GetWindowRect(g_hWnd, &windowRect); // preserve current size values
		GetClientRect(g_hWnd, &clientRect);
		dwStyle = GetWindowLongPtrA(g_hWnd, GWL_STYLE);
		SetWindowLongPtrA(g_hWnd, GWL_STYLE, WS_VISIBLE); // no other styles but visible

		// Remove the menu but don't destroy it
		menu->RemoveWindowMenu();

		hWndTaskBar = FindWindowA("Shell_TrayWnd", "");
		GetWindowRect(hWnd, &rectTaskBar);

		// Hide the System Task Bar
		SetWindowPos(hWndTaskBar, HWND_NOTOPMOST, 0, 0, (rectTaskBar.right - rectTaskBar.left), (rectTaskBar.bottom - rectTaskBar.top), SWP_NOMOVE | SWP_NOSIZE);

		// Allow for multiple monitors
		HMONITOR monitor = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTOPRIMARY);
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
		SetWindowPos(g_hWnd, HWND_NOTOPMOST, x, y, w, h, SWP_HIDEWINDOW);
		SetWindowPos(g_hWnd, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW);

		ShowWindow(g_hWnd, SW_SHOW);
		SetFocus(g_hWnd);

		if (!hwndAdjust)
			ShowCursor(FALSE);

	} // endif bFullscreen
	else {
		
		//
		// Exit full screen
		//

		// Restore original style
		SetWindowLongPtrA(hWnd, GWL_STYLE, dwStyle);

		// Restore the menu
		menu->SetWindowMenu();

		// Restore topmost state
		if (bTopmost)
			hWndMode = HWND_TOPMOST;
		else
			hWndMode = HWND_TOP;

		// Restore our window
		SetWindowPos(g_hWnd, hWndMode, windowRect.left, windowRect.top, nonFullScreenX + AddX, nonFullScreenY + AddY, SWP_SHOWWINDOW);

		// Reset the window that was top before - could be ours
		if (GetWindowLong(hWndForeground, GWL_EXSTYLE) & WS_EX_TOPMOST)
			SetWindowPos(hWndForeground, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		else
			SetWindowPos(hWndForeground, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		ShowCursor(TRUE);

		DrawMenuBar(hWnd);

	} // endif not bFullscreen

}

//--------------------------------------------------------------
void ofApp::doTopmost(bool bTop)
{
	if (bTop) {
		// Get the current top window for return
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
void ofApp::WriteInitFile(const char* initfile)
{
	char tmp[MAX_PATH]{};

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

	if(bNDIasync)
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"async", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Options", (LPCSTR)"async", (LPCSTR)"0", (LPCSTR)initfile);

	// Volume
	sprintf_s(tmp, 256, "%-8.2f", movieVolume); tmp[8] = 0;
	WritePrivateProfileStringA((LPCSTR)"Audio", (LPCSTR)"volume", (LPCSTR)tmp, (LPCSTR)initfile);

	// Image adjustment
	sprintf_s(tmp, MAX_PATH, "%.3f", Brightness);
	WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Brightness", (LPCSTR)tmp, (LPCSTR)initfile);
	sprintf_s(tmp, MAX_PATH, "%.3f", Contrast);
	WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Contrast", (LPCSTR)tmp, (LPCSTR)initfile);
	sprintf_s(tmp, MAX_PATH, "%.3f", Saturation);
	WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Saturation", (LPCSTR)tmp, (LPCSTR)initfile);
	sprintf_s(tmp, MAX_PATH, "%.3f", Gamma);
	WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Gamma", (LPCSTR)tmp, (LPCSTR)initfile);
	sprintf_s(tmp, MAX_PATH, "%.3f", Blur);
	WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Blur", (LPCSTR)tmp, (LPCSTR)initfile);
	sprintf_s(tmp, MAX_PATH, "%.3f", Sharpness);
	WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Sharpness", (LPCSTR)tmp, (LPCSTR)initfile);
	sprintf_s(tmp, MAX_PATH, "%.3f", Sharpwidth);
	WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Sharpwidth", (LPCSTR)tmp, (LPCSTR)initfile);
	if (bAdaptive)
		WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Adaptive", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Adaptive", (LPCSTR)"0", (LPCSTR)initfile);
	if (bFlip)
		WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Flip", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Flip", (LPCSTR)"0", (LPCSTR)initfile);
	if (bMirror)
		WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Mirror", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Mirror", (LPCSTR)"0", (LPCSTR)initfile);
	if (bSwap)
		WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Swap", (LPCSTR)"1", (LPCSTR)initfile);
	else
		WritePrivateProfileStringA((LPCSTR)"Adjust", (LPCSTR)"Swap", (LPCSTR)"0", (LPCSTR)initfile);
}

//--------------------------------------------------------------
void ofApp::ReadInitFile()
{
	char initfile[MAX_PATH]{};
	char tmp[MAX_PATH]{};

	GetModuleFileNameA(NULL, initfile, MAX_PATH);
	PathRemoveFileSpecA(initfile);
	strcat_s(initfile, MAX_PATH, "\\SpoutVideoPlayer.ini");
	strcpy_s(g_InitFile, MAX_PATH, initfile);

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

	GetPrivateProfileStringA((LPCSTR)"Options", (LPSTR)"async", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bNDIasync = (atoi(tmp) == 1);

	// Volume
	if (GetPrivateProfileStringA((LPCSTR)"Audio", (LPSTR)"volume", (LPSTR)"1.00", (LPSTR)tmp, 8, initfile) > 0)
		movieVolume = atof(tmp);

	// Set up menus etc (menu must have been set up)
	menu->SetPopupItem("Loop", bLoop);
	menu->SetPopupItem("Resize", bResizeWindow);
	menu->SetPopupItem("Topmost", bTopmost);
	menu->SetPopupItem("Spout", bSpoutOut);
	menu->SetPopupItem("NDI", bNDIout);
	menu->SetPopupItem("    Async", bNDIasync);
	if (bNDIout)
		menu->EnablePopupItem("    Async", true);
	else
		menu->EnablePopupItem("    Async", false);

	// Image adjustment
	// Brightness    -1 - 1   default 0
	// Contrast       0 - 4   default 1
	// Saturation     0 - 4   default 1
	// Gamma          0 - 4   default 1

	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"Brightness", NULL, (LPSTR)tmp, 8, initfile);
	if (tmp[0]) Brightness = (float)atof(tmp);
	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"Contrast", NULL, (LPSTR)tmp, 8, initfile);
	if (tmp[0]) Contrast = (float)atof(tmp);
	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"Saturation", NULL, (LPSTR)tmp, 8, initfile);
	if (tmp[0]) Saturation = (float)atof(tmp);
	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"Gamma", NULL, (LPSTR)tmp, 8, initfile);
	if (tmp[0]) Gamma = (float)atof(tmp);
	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"Blur", NULL, (LPSTR)tmp, 8, initfile);
	if (tmp[0]) Blur = (float)atof(tmp);
	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"Sharpness", NULL, (LPSTR)tmp, 8, initfile);
	if (tmp[0]) Sharpness = (float)atof(tmp);
	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"Sharpwidth", NULL, (LPSTR)tmp, 8, initfile);
	if (tmp[0]) Sharpwidth = (float)atof(tmp);
	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"Adaptive", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bAdaptive = (atoi(tmp) == 1);
	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"bFlip", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bFlip = (atoi(tmp) == 1);
	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"bMirror", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bMirror = (atoi(tmp) == 1);
	GetPrivateProfileStringA((LPCSTR)"Adjust", (LPSTR)"bSwap", NULL, (LPSTR)tmp, 3, initfile);
	if (tmp[0]) bSwap = (atoi(tmp) == 1);

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
	iRet = spoutsender->SpoutMessageBox(hwnd, message, caption, uType | MB_TOPMOST);

	if (bLoaded && !bPaused)
		myMovie.setPaused(false);

	bMessageBox = false;

	return iRet;

}


// Message handler for About box
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	char tmp[MAX_PATH]{};
	char about[1024]{};
	DWORD dummy = 0;
	DWORD dwSize = 0;
	LPDRAWITEMSTRUCT lpdis{};
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
						sprintf_s(tmp, MAX_PATH, "%s\n", (char*)pvProductVersion);
						strcat_s(about, 1024, tmp);
					}
				}
			}
		}

		// Newtek credit - see resource.rc
		strcat_s(about, 1024, "\n\n");
		strcat_s(about, 1024, "  NewTek NDI™ - Version ");
		// Add NewTek library version number (dll)
		strcat_s(about, 1024, NDInumber.c_str());
		strcat_s(about, 1024, "\n  NDI™ is a trademark of NewTek, Inc.");
		SetDlgItemTextA(hDlg, IDC_ABOUT_TEXT, (LPCSTR)about);

		//
		// Hyperlink hand cursor
		//

		// Spout
		cursorHand = LoadCursor(NULL, IDC_HAND);
		hwnd = GetDlgItem(hDlg, IDC_SPOUT_URL);
		SetClassLongA(hwnd, GCLP_HCURSOR, (LONGLONG)cursorHand);

		// NDI
		hwnd = GetDlgItem(hDlg, IDC_NEWTEC_URL);
		SetClassLongA(hwnd, GCLP_HCURSOR, (LONGLONG)cursorHand);
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


// Message handler for Volume control dialog
LRESULT CALLBACK UserVolume(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam); // suppress warning

	HWND hBar = NULL;
	HWND hBox = NULL;
	HWND hFocus = NULL;
	int iPos = 0;
	int dn = 0;
	int TrackBarPos = 0;
	float fValue = 0.0f;

	switch (message) {

	case WM_INITDIALOG:
		// Open the window to the left of the volume icon
		int x, y, w, h;
		RECT rect;
		GetWindowRect(hDlg, &rect);
		w = rect.right - rect.left;
		h = rect.bottom - rect.top;
		GetWindowRect(pThis->hWnd, &rect);
		x = rect.right - w - (int)pThis->icon_size * 3.5; // (int)(icon_sound_pos_x - icon_size * 1.5);
		y = rect.bottom - h - (int)pThis->icon_size; // (int)icon_sound_pos_y;
		if (!pThis->bShowInfo)
			y += 14;
		SetWindowPos(hDlg, HWND_TOPMOST, x, y, w, h, SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW);

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

//
// Message handler for Adjustment options dialog
//
LRESULT CALLBACK UserAdjust(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char str1[MAX_PATH]={};
	HWND hBar = NULL;
	LRESULT iPos = 0;
	float fValue = 0.0f;

	switch (message) {

	case WM_INITDIALOG:

		// Set the scroll bar limits and text

		// Brightness  -1 - 0 - 1 default 0
		// Range 2 Brightness*400 - 200
		hBar = GetDlgItem(hDlg, IDC_BRIGHTNESS);
		SendMessage(hBar, TBM_SETRANGEMIN, (WPARAM)1, (LPARAM)0);
		SendMessage(hBar, TBM_SETRANGEMAX, (WPARAM)1, (LPARAM)400);
		SendMessage(hBar, TBM_SETPAGESIZE, (WPARAM)1, (LPARAM)20);
		// -1 > +1   - 0 - 200
		// pos + 1 * 100
		iPos = (int)(pThis->Brightness+1*200.0f);
		SendMessage(hBar, TBM_SETPOS, (WPARAM)1, (LPARAM)iPos);
		sprintf_s(str1, 256, "%.3f", pThis->Brightness);
		SetDlgItemTextA(hDlg, IDC_BRIGHTNESS_TEXT, (LPCSTR)str1);

		hBar = GetDlgItem(hDlg, IDC_CONTRAST);
		SendMessage(hBar, TBM_SETRANGEMIN, (WPARAM)1, (LPARAM)0);
		SendMessage(hBar, TBM_SETRANGEMAX, (WPARAM)1, (LPARAM)200);
		SendMessage(hBar, TBM_SETPAGESIZE, (WPARAM)1, (LPARAM)10);
		iPos = (int)(pThis->Contrast*100.0f);
		SendMessage(hBar, TBM_SETPOS, (WPARAM)1, (LPARAM)iPos);
		sprintf_s(str1, 256, "%.3f", pThis->Contrast);
		SetDlgItemTextA(hDlg, IDC_CONTRAST_TEXT, (LPCSTR)str1);

		// 0 > 4  - 0 - 400
		hBar = GetDlgItem(hDlg, IDC_SATURATION);
		SendMessage(hBar, TBM_SETRANGEMIN, (WPARAM)1, (LPARAM)0);
		SendMessage(hBar, TBM_SETRANGEMAX, (WPARAM)1, (LPARAM)400);
		SendMessage(hBar, TBM_SETPAGESIZE, (WPARAM)1, (LPARAM)10);
		iPos = (int)(pThis->Saturation * 100.0f);
		SendMessage(hBar, TBM_SETPOS, (WPARAM)1, (LPARAM)iPos);
		sprintf_s(str1, 256, "%.3f", pThis->Saturation);
		SetDlgItemTextA(hDlg, IDC_SATURATION_TEXT, (LPCSTR)str1);

		hBar = GetDlgItem(hDlg, IDC_GAMMA);
		SendMessage(hBar, TBM_SETRANGEMIN, (WPARAM)1, (LPARAM)0);
		SendMessage(hBar, TBM_SETRANGEMAX, (WPARAM)1, (LPARAM)200);
		SendMessage(hBar, TBM_SETPAGESIZE, (WPARAM)1, (LPARAM)10);
		iPos = (int)(pThis->Gamma * 100.0f);
		SendMessage(hBar, TBM_SETPOS, (WPARAM)1, (LPARAM)iPos);
		sprintf_s(str1, 256, "%.3f", pThis->Gamma);
		SetDlgItemTextA(hDlg, IDC_GAMMA_TEXT, (LPCSTR)str1);

		hBar = GetDlgItem(hDlg, IDC_SHARPNESS);
		SendMessage(hBar, TBM_SETRANGEMIN, (WPARAM)1, (LPARAM)0);
		SendMessage(hBar, TBM_SETRANGEMAX, (WPARAM)1, (LPARAM)100);
		SendMessage(hBar, TBM_SETPAGESIZE, (WPARAM)1, (LPARAM)10);
		iPos = (int)(pThis->Sharpness * 100.0f);
		SendMessage(hBar, TBM_SETPOS, (WPARAM)1, (LPARAM)iPos);
		sprintf_s(str1, 256, "%.3f", pThis->Sharpness);
		SetDlgItemTextA(hDlg, IDC_SHARPNESS_TEXT, (LPCSTR)str1);

		hBar = GetDlgItem(hDlg, IDC_BLUR);
		SendMessage(hBar, TBM_SETRANGEMIN, (WPARAM)1, (LPARAM)0);
		SendMessage(hBar, TBM_SETRANGEMAX, (WPARAM)1, (LPARAM)400);
		SendMessage(hBar, TBM_SETPAGESIZE, (WPARAM)1, (LPARAM)10);
		iPos = (int)(pThis->Blur * 100.0f);
		SendMessage(hBar, TBM_SETPOS, (WPARAM)1, (LPARAM)iPos);
		sprintf_s(str1, 256, "%.3f", pThis->Blur);
		SetDlgItemTextA(hDlg, IDC_BLUR_TEXT, (LPCSTR)str1);

		// Sharpness width radio buttons
		// 3x3, 5x5, 7x7
		iPos = ((int)pThis->Sharpwidth-3)/2; // 0, 1, 2
		CheckRadioButton(hDlg, IDC_SHARPNESS_3x3, IDC_SHARPNESS_7x7, IDC_SHARPNESS_3x3+(int)iPos);

		// Adaptive sharpen checkbox
		if (pThis->bAdaptive)
			CheckDlgButton(hDlg, IDC_ADAPTIVE, BST_CHECKED);
		else
			CheckDlgButton(hDlg, IDC_ADAPTIVE, BST_UNCHECKED);

		// Option checkboxes
		if (pThis->bFlip)
			CheckDlgButton(hDlg, IDC_FLIP, BST_CHECKED);
		else
			CheckDlgButton(hDlg, IDC_FLIP, BST_UNCHECKED);
		if (pThis->bMirror)
			CheckDlgButton(hDlg, IDC_MIRROR, BST_CHECKED);
		else
			CheckDlgButton(hDlg, IDC_MIRROR, BST_UNCHECKED);
		if (pThis->bSwap)
			CheckDlgButton(hDlg, IDC_SWAP, BST_CHECKED);
		else
			CheckDlgButton(hDlg, IDC_SWAP, BST_UNCHECKED);

		return TRUE;

		// https://msdn.microsoft.com/en-us/library/windows/desktop/hh298416(v=vs.85).aspx
	case WM_HSCROLL:
		hBar = (HWND)lParam;
		if (hBar == GetDlgItem(hDlg, IDC_BRIGHTNESS)) {
			// 0 - 200 > -1 - +1
			iPos = SendMessage(hBar, TBM_GETPOS, 0, 0);
			fValue = ((float)iPos/200.0f)-1.0f;
			pThis->Brightness = fValue;
			sprintf_s(str1, 256, "%.3f", fValue);
			SetDlgItemTextA(hDlg, IDC_BRIGHTNESS_TEXT, (LPCSTR)str1);
		}
		else if (hBar == GetDlgItem(hDlg, IDC_CONTRAST)) {
			//  0 - 1 - 4 default 1
			iPos = SendMessage(hBar, TBM_GETPOS, 0, 0);
			fValue = ((float)iPos/100.0f);
			pThis->Contrast = fValue;
			sprintf_s(str1, 256, "%.3f", fValue);
			SetDlgItemTextA(hDlg, IDC_CONTRAST_TEXT, (LPCSTR)str1);
		}
		else if (hBar == GetDlgItem(hDlg, IDC_SATURATION)) {
			iPos = SendMessage(hBar, TBM_GETPOS, 0, 0);
			fValue = ((float)iPos)/100.0f;
			pThis->Saturation = fValue;
			sprintf_s(str1, 256, "%.3f", fValue);
			SetDlgItemTextA(hDlg, IDC_SATURATION_TEXT, (LPCSTR)str1);
		}
		else if (hBar == GetDlgItem(hDlg, IDC_GAMMA)) {
			iPos = SendMessage(hBar, TBM_GETPOS, 0, 0);
			fValue = ((float)iPos)/100.0f;
			pThis->Gamma = fValue;
			sprintf_s(str1, 256, "%.3f", fValue);
			SetDlgItemTextA(hDlg, IDC_GAMMA_TEXT, (LPCSTR)str1);
		}
		else if (hBar == GetDlgItem(hDlg, IDC_SHARPNESS)) {
			iPos = SendMessage(hBar, TBM_GETPOS, 0, 0);
			fValue = ((float)iPos)/100.0f;
			pThis->Sharpness = fValue;
			sprintf_s(str1, 256, "%.3f", fValue);
			SetDlgItemTextA(hDlg, IDC_SHARPNESS_TEXT, (LPCSTR)str1);
		}
		else if (hBar == GetDlgItem(hDlg, IDC_BLUR)) {
			iPos = SendMessage(hBar, TBM_GETPOS, 0, 0);
			fValue = ((float)iPos) / 100.0f;
			pThis->Blur = fValue;
			sprintf_s(str1, 256, "%.3f", fValue);
			SetDlgItemTextA(hDlg, IDC_BLUR_TEXT, (LPCSTR)str1);
		}
		break;

	case WM_DESTROY:
		DestroyWindow(hwndAdjust);
		hwndAdjust = NULL;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDC_SHARPNESS_3x3:
			pThis->Sharpwidth = 3.0;
			break;
		case IDC_SHARPNESS_5x5:
			pThis->Sharpwidth = 5.0;
			break;
		case IDC_SHARPNESS_7x7:
			pThis->Sharpwidth = 7.0;
			break;

		case IDC_ADAPTIVE:
			if (IsDlgButtonChecked(hDlg, IDC_ADAPTIVE) == BST_CHECKED)
				pThis->bAdaptive = true;
			else
				pThis->bAdaptive = false;
			break;


		case IDC_FLIP:
			if (IsDlgButtonChecked(hDlg, IDC_FLIP) == BST_CHECKED)
				pThis->bFlip = true;
			else
				pThis->bFlip = false;
			break;

		case IDC_MIRROR:
			if (IsDlgButtonChecked(hDlg, IDC_MIRROR) == BST_CHECKED)
				pThis->bMirror = true;
			else
				pThis->bMirror = false;
			break;

		case IDC_SWAP:
			if (IsDlgButtonChecked(hDlg, IDC_SWAP) == BST_CHECKED)
				pThis->bSwap = true;
			else
				pThis->bSwap = false;
			break;

		case IDC_RESTORE:
			pThis->Brightness = pThis->OldBrightness;
			pThis->Contrast   = pThis->OldContrast;
			pThis->Saturation = pThis->OldSaturation;
			pThis->Gamma      = pThis->OldGamma;
			pThis->Sharpness  = pThis->OldSharpness;
			pThis->Sharpwidth = pThis->OldSharpwidth;
			pThis->bAdaptive  = pThis->OldAdaptive;
			pThis->Blur       = pThis->OldBlur;
			pThis->bFlip      = pThis->OldFlip;
			pThis->bMirror    = pThis->OldMirror;
			pThis->bSwap      = pThis->OldSwap;
			SendMessage(hDlg, WM_INITDIALOG, 0, 0L);
			break;

		case IDC_RESET:
			pThis->Brightness = 0.0; // -1 - 0 - 1 default 0
			pThis->Contrast   = 1.0; //  0 - 1 - 4 default 1
			pThis->Saturation = 1.0; //  0 - 1 - 4 default 1
			pThis->Gamma      = 1.0; //  0 - 1 - 4 default 1
			pThis->Blur       = 0.0;
			pThis->Sharpness  = 0.0; //  0 - 4 default 0
			pThis->Sharpwidth = 3.0;
			pThis->bAdaptive  = false;
			pThis->bFlip      = false;
			pThis->bMirror    = false;
			pThis->bSwap      = false;
			SendMessage(hDlg, WM_INITDIALOG, 0, 0L);
			break;

		case IDOK:
			DestroyWindow(hwndAdjust);
			hwndAdjust = NULL;
			return TRUE;

		case IDCANCEL:
			pThis->Brightness = pThis->OldBrightness;
			pThis->Contrast   = pThis->OldContrast;
			pThis->Saturation = pThis->OldSaturation;
			pThis->Gamma      = pThis->OldGamma;
			pThis->Blur       = pThis->OldBlur;
			pThis->Sharpness  = pThis->OldSharpness;
			pThis->Sharpwidth = pThis->OldSharpwidth;
			pThis->bAdaptive  = pThis->OldAdaptive;
			pThis->bFlip      = pThis->OldFlip;
			pThis->bMirror    = pThis->OldMirror;
			pThis->bSwap      = pThis->OldSwap;
			DestroyWindow(hwndAdjust);
			hwndAdjust = NULL;
			return TRUE;

		default:
			return FALSE;
		}
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


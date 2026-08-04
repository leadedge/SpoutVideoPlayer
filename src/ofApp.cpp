/*

	Spout OpenFrameworks Video/Audio Sender example

	This project is extended over a typical example to create a
	video player using FFmpeg. Due to the additional complexity,
	the project is hosted as a branch of "Spout Video Player",
	which uses Openframeworks ofVideoPlayer.

	Two pipes are created, one for video and the other for audio.
	This is a simple method compared to using FFmpeg libraries
	and supports alpha channel transparency if the video file
	encoder supports it, such as VP9, HapAlpha and ProRes4444.

	ofSoundStream and audioOut enable sound output and Draw is
	kept in sync with audio by timing and and frame count matching.
	Seeking is achieved by specifying the start time for pipe read.
	Performance varies depending on the encoder used for the video.
		
	Uses the ofxWinMenu addon to create a menu and manage
	caption mouse press and the ofxWinDialog addon to create
	an image adjust dialog. These can be used as is usual for
	an Openframeworks addon, but source is included here for
	convenience. Uses a static library for Spout functions.

	The code can be used for reference :

	o ofxWinMenu to create a window menu
	o ofxWinDialog to create a dialog
	o Compute shaders for image adjust
	o Setting an icon from a Windows dll
	o Detecting non-client area mouse press
	o Preview and full screen by changing window style and size
    o FFprobe to read video file details
    o FFmpeg with two pipes to decode video and audio frames
	o Fps control using HoldFps
	o Sync video with audio using audio timing and frame matching
    o Openframeworks dragEvent for drag and drop
	o Openframeworks soundstream and audioOut
	o Video duration, frame counter and progress bar
	o Draw and position ofTrueTypeFont text
 	o Using a Spout static library generated using Cmake
	o SetSenderName, SendImage, LoadTexturePixels and ReleaseSender
	o Utility OpenSpoutConsole and SpoutMessageBox functions
    
	FFmpeg.exe and FFprobe.exe are required.
	Refer to data/ffmpeg/readme.md

	Copyright (C) 2026 Lynn Jarvis.

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
*/
#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	// OpenSpoutConsole(); // for debugging

	// Set the sender name
	strcpy_s(m_SenderName, 256, "Video Audio Sender");
	sender.SetSenderName(m_SenderName);

	 // show it on the title bar
	ofSetWindowTitle(m_SenderName);

	// Load a font rather than the default
	myFont.load("fonts/verdana.ttf", 12, true, true);

	// Executable location
	char exePath[MAX_PATH]{};
	GetModuleFileNameA(NULL, exePath, MAX_PATH); // Path of the executable
	PathRemoveFileSpecA(exePath);
	m_exePath = exePath;

	// FFmpeg location - /data/ffmpeg/
	m_ffmpegPath = m_exePath;
	m_ffmpegPath += "/data/ffmpeg/ffmpeg.exe";
	if (_access(m_ffmpegPath.c_str(), 0) == -1) {
		// FFmpeg download instructions
		// Keep the dialog open with "?noclose" in the url
		// and topmost so that the instructions remain visible
		// (the ffdownloadstr() content string is re-used)
		std::string str = "FFmpeg not found\n\n" + ffdownloadstr();
		SpoutMessageBoxIconSmall();
		SpoutMessageBox(NULL, str.c_str(), "FFmpeg", MB_ICONWARNING | MB_TOPMOST | MB_OK);
	}
	else {
		// FFmpeg found
		// Look for FFprobe.exe
		std::string ffpath = m_exePath;
		ffpath += "/data/ffmpeg/ffprobe.exe";
		if (_access(ffpath.c_str(), 0) == -1) {
			std::string str = "FFprobe not found\n\n" + ffdownloadstr();
			SpoutMessageBoxIconSmall();
			SpoutMessageBox(NULL, str.c_str(), "FFprobe", MB_ICONWARNING | MB_TOPMOST | MB_OK);
		}
	}

	// Instance for adjust dialog
	m_hInstance = GetModuleHandleA(NULL);

	// Main window handle
	m_hWnd = ofGetWin32Window();

	// Set a custom icon from C:\Windows\System32\imageres.dll
	m_hIcon = ExtractWindowsIcon(5201, "imageres.dll");
	SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
	SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);

	// Disable Openframeworks escape key exit
	// for fullscreen (see keyPressed)
	ofSetEscapeQuitsApp(false);

	//
	// Create a menu using ofxWinMenu
	//

	// A new menu object with a pointer to this class
	menu = new ofxWinMenu(this, m_hWnd);
	// Register an ofApp function that is called when a menu item is selected.
	menu->CreateMenuFunction(&ofApp::appMenuFunction);
	// Create a window menu
	HMENU hMenu = menu->CreateWindowMenu();
	// File popup
	HMENU hPopup = menu->AddPopupMenu(hMenu, "File");
	// Open a movie of image file
	menu->AddPopupItem(hPopup, "Open video", false, false); // Not checked and not auto-checked
	// The folder of the current movie
	menu->AddPopupItem(hPopup, "Video folder", false, false);
	// Image capture folder
	menu->AddPopupItem(hPopup, "Image folder", false, false);

	// Separator before the Exit item
	menu->AddPopupSeparator(hPopup);
	// Exit
	menu->AddPopupItem(hPopup, "Exit", false, false);

	// Output popup
	hPopup = menu->AddPopupMenu(hMenu, "Output");
	menu->AddPopupItem(hPopup, "Adjust 'a'", false, false);
	menu->AddPopupItem(hPopup, "Go to 'g'", false, false);
	menu->AddPopupItem(hPopup, "Copy 'c'", false, false);
	menu->AddPopupItem(hPopup, "Capture", false, false);
	menu->AddPopupItem(hPopup, "Save as", false, false);
	menu->AddPopupItem(hPopup, "Mute 'm'", bMute);
	menu->AddPopupItem(hPopup, "Resize", bScale);

	// View popup
	hPopup = menu->AddPopupMenu(hMenu, "View");
	menu->AddPopupItem(hPopup, "Show on top", bTopmost);
	menu->AddPopupItem(hPopup, "Show controls - Space", bShowInfo);
	menu->AddPopupItem(hPopup, "Preview 'v'", false, false); // Unchecked, no auto-check
	menu->AddPopupItem(hPopup, "Full screen 'f'", false, false); // No autocheck

	// Help popup
	hPopup = menu->AddPopupMenu(hMenu, "Help");
	menu->AddPopupItem(hPopup, "About", false, false); // No auto check

	// Load previous menu settings
	// after the menu items are established
	menu->Load("sender-video-audio");
	
	// Adjust window for the starting client size (in main.cpp)
	// allowing for a menu and centre on the screen
	ResetWindow(ofGetWidth(), ofGetHeight());

	// Set the menu to the window after adjusting the size
	menu->SetWindowMenu();

	//
	// Image adjust dialog
	//
	adjust = new ofxWinDialog(this, m_hInstance, m_hWnd, "Adjust 'a'");
	adjust->SetIcon(m_hIcon);
	adjust->SetFont("Segoe UI", 9);
	adjust->AppDialogFunction(&ofApp::AdjustCallback);
	// Create adjust controls
	CreateAdjustDialog();
	// Load saved settings after controls have been created
	adjust->Load("Adjust");
	// Get the loaded control values
	adjust->GetControls();

	//
	// icons
	//
	icon_reverse.load("icons/reverse.png");
	icon_pause.load("icons/pause.png");
	icon_play.load("icons/play.png");
	icon_stop.load("icons/stop.png");
	icon_fastforward.load("icons/fastforward.png");
	icon_full_screen.load("icons/full_screen.png");
	icon_sound.load("icons/speaker.png");
	icon_mute.load("icons/speaker_mute.png");

	icon_size = 20;
	icon_reverse.resize(icon_size, icon_size);
	icon_pause.resize(icon_size, icon_size);
	icon_play.resize(icon_size, icon_size);
	icon_fastforward.resize(icon_size, icon_size);
	icon_stop.resize(icon_size, icon_size);
	icon_full_screen.resize(icon_size, icon_size);
	icon_sound.resize(icon_size, icon_size);
	icon_mute.resize(icon_size, icon_size);

	m_icons.push_back(icon_reverse);     // 0
	m_icons.push_back(icon_pause);       // 1
	m_icons.push_back(icon_play);        // 2
	m_icons.push_back(icon_stop);        // 3
	m_icons.push_back(icon_fastforward); // 4
	m_icons.push_back(icon_full_screen); // 5
	m_icons.push_back(icon_sound);       // 6
	m_icons.push_back(icon_mute);        // 7 

	// Icon foreground/background used in mouseMoved
	icon_highlight_color  = ofColor(40, 125, 204); // VLC blue
	icon_background_color = ofColor(204); // Light grey
	for (int i=0; i<(int)m_icons.size(); i++) {
		m_iconColor.push_back(icon_background_color);
	}

}


//--------------------------------------------------------------
void ofApp::update() {

}


//--------------------------------------------------------------
void ofApp::draw()
{

	ofBackground(0);
	ofSetColor(255);

	// If not initialized
	if (!m_pipein) {
		ofBackground(0, 20, 70); // Dark steel blue
		std::string str = "DRAG AND DROP VIDEOS HERE";
		// Get the width of the string
		int strwidth = myFont.stringWidth(str);
		// Center the string in the client area
		RECT dr ={ 0 };
		GetClientRect(m_hWnd, &dr);
		int xpos = (dr.right - dr.left)/2 - strwidth/2;
		int ypos = (dr.bottom - dr.top)/2;
		myFont.drawString(str, xpos, ypos);
		return;
	}

	// Continue to play audio if paused by
	// menu selection or click on the caption
	bNCmousePressed = false;

	// Read a video frame from the FFmpeg input pipe
	// if not paused or positioning
	if(!bPaused || bPosition) {
		if (m_pipein && m_pixelBuffer && m_SenderWidth > 0 && m_SenderHeight > 0) {
			// audioOut signals to read the next frame based on the video frame rate
			if (bReadVideo) {
				if (fread(m_pixelBuffer, 1, m_SenderWidth*m_SenderHeight*4, m_pipein) == 0) {
					// fread = 0 means the end of the file
					// Use the same file and start again
					RestartVideo();
					return;
				}
				m_FramesRead++;  // Number of frames read after pause
				m_progress += 1.0/(double)m_Frames; // Progress bar position

				// Load the read texture with pixels
				sender.LoadTexturePixels(readTexture.getTextureData().textureID,
					readTexture.getTextureData().textureTarget,
					m_SenderWidth, m_SenderHeight, m_pixelBuffer, GL_BGRA);

				// Activate shaders from the read texture to the draw texture
				ApplyShaders();

				// Send at the video frame rate
				sender.SendTexture(myTexture.getTextureData().textureID,
					myTexture.getTextureData().textureTarget,
					m_SenderWidth, m_SenderHeight, false);
		
				if(m_audioPipe)
					bReadVideo = false; // Wait for audio
			}
			// Reset sync flag to play audio (set in RestartVideo)
			if(m_audioPipe)
				bVideoSync = false;
		}
		// Clear positioning flag for progress bar or goto
		bPosition = false;
	}
	else if (myTexture.isAllocated()) {
		// If paused, activate shaders on the same read texture
		ApplyShaders();
	}

	// Do not draw if iconic
	if (!IsIconic(ofGetWin32Window()) && myTexture.isAllocated()) {
		// Draw the result fitted to the display window
		// Adjust height from video aspect ratio
		int width = ofGetWidth();
		int height = width*m_SenderHeight/m_SenderWidth;
		int ypos = (ofGetHeight()-height)/2;
		myTexture.draw(0, ypos, width, height);
		// Key shortcuts
		if(bShowInfo)
			ShowInfo();
	}

	//
	// Lower the draw() cycle rate
	//
	// For audio, set higher that the framerate
	// of the video file used in audioOut
	// If no audio, hold the video frame rate
	if(m_audioPipe)
		sender.HoldFps(m_FrameRate + 2);
	else
		sender.HoldFps(m_FrameRate);


}

void ofApp::ApplyShaders()
{
	// readTexture and myTexture are global
	GLuint sourceID = readTexture.getTextureData().textureID;
	GLuint textureID = myTexture.getTextureData().textureID;
	unsigned int width = (unsigned int)myTexture.getWidth();
	unsigned int height = (unsigned int)myTexture.getHeight();

	// Temperature : 3500 - 9500  (default 6500 daylight)
	// Apply first to copy from the read texture to the draw texture
	shaders.Temperature(sourceID, textureID, width, height, Temp);

	// Brightness    -1 - 1   default 0
	// Contrast       0 - 4   default 1
	// Saturation     0 - 4   default 1
	// Gamma          0 - 4   default 1
	if (Brightness != 0.0
		|| Contrast != 1.0
		|| Saturation != 1.0
		|| Gamma != 1.0) {
		shaders.Adjust(textureID, textureID, width, height,
			Brightness, Contrast, Saturation, Gamma);
	}
	// Sharpness 0 - 1  (default 0)
	// 0.001 - 0.002 msec
	if (Sharpness > 0.0) {
		if (bAdaptive) {
			// Sharpness width radio buttons
			// 3x3, 5x5, 7x7 : 3.0, 5.0, 7.0
			float caswidth = 1.0f + (Sharpwidth - 3.0f) / 2.0f; // 1.0, 2.0, 3.0
			// Sharpness; // 0.0 - 1.0
			shaders.AdaptiveSharpen(textureID, width, height, caswidth, Sharpness);
		}
		else {
			shaders.Sharpen(textureID, textureID, width, height, Sharpwidth, Sharpness);
		}
	}

}


//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer &buffer)
{
	// Do not process audio for menu selection,
	// mouse click on the caption, or if paused
	// or if no audio in the video file
	if (bNCmousePressed || bPaused || !m_audioPipe) {
		if(buffer.size() > 0)
			buffer.set(0.0f); // silence
		return;
	}

	//
	// Read the next lot of audio frames from the file.
	// This is a separate thread to Draw so the audio is not
	// limited by the video rate. The number of bytes required
	// by the audio callback and the PCM data buffer are
	// established when soundstream is set up.
	//
	if (m_audioPipe && m_FramesRead > 0) { // wait until draw reads a frame
		size_t bytesRead = fread(m_pcmBuffer.data(), 1, m_pcmBuffer.size()*sizeof(int16_t), m_audioPipe);
		if (bytesRead == 0) {
			// fread = 0 means the end of the file
			// Let video read and start again
			bReadVideo = true;
			return;
		}
		else if (bytesRead > 0) {
			// Sync video with audio
			// Calculate the required video frame based on audio time
			double audioTime = (double)m_audioFramesPlayed.load()/m_sampleRate;
			long requiredFrame = (int)(audioTime*m_FrameRate);
			if (requiredFrame > m_FramesRead) {
				// Signal Draw to get the next video frame
				bReadVideo = true;
			}
			// For the sound to come from the speakers
			// Silence if mute or syncing video with audio
			if (!bMute && !bVideoSync) {
				size_t samplesRead = bytesRead / sizeof(int16_t);
				for (size_t i = 0; i < samplesRead; i++) {
					// A signed 16-bit sample ranges from : -32768 ... +32767
					// OpenFrameworks expects : -1.0 ... +1.0 float
					buffer[i] = static_cast<float>(m_pcmBuffer[i] / 32768.0f);
				}
				// Zero-fill any remaining samples if EOF reached
				for (size_t i = samplesRead; i < buffer.size(); i++)
					buffer[i] = 0.0f;
			}
			else {
				buffer.set(0.0f); // set silence
			}
			// For "Go to" time
			m_frameSec = audioTime;
			// Update the audio frame counter
			m_audioFramesPlayed += buffer.getNumFrames();
		}
	}
	else {
		buffer.set(0.0f);
	}
}

//--------------------------------------------------------------
void ofApp::exit()
{
	// Save menu settings
	menu->Save("sender-video-audio", true);
	// Release FFmpeg resources
	CloseFFmpeg();
	// Release the sender
	if (m_pixelBuffer) delete[] m_pixelBuffer;
	m_pixelBuffer = nullptr;
	sender.ReleaseSender();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
	// Show controls on-screen
	if (key == ' ' && !m_videopath.empty()) {
		bShowInfo = !bShowInfo;
		menu->SetPopupItem("Show controls - Space", bShowInfo);
	}

	// Escape key exit full screen
	if (key == VK_ESCAPE && (bFullScreen || bPreview)) {
		bFullScreen = false;
		bPreview = false;
		doFullScreen(bFullScreen, bPreview);
	}

	// v - toggle preview
	if ((key == 'v' || key == 'V') && !bFullScreen && !m_videopath.empty()) {
		bPreview = !bPreview;
		doFullScreen(bPreview, true); // enable/preview mode
	}

	// f - toggle fullscreen
	if ((key == 'f' || key == 'F') && !bPreview && !m_videopath.empty()) {
		bFullScreen = !bFullScreen;
		doFullScreen(bFullScreen);
		// Do not check this item because
		// there is no menu full screen
	}

	// Pause/Play
	if (key == 'p' || key == 'P') {
		bPaused = !bPaused;
		// Update "Go to" time
		m_frameSec = m_progress*m_Duration;
		// Handle menu item
		if (bPaused)
			menu->EnablePopupItem("Go to 'g'", true);
		else
			menu->EnablePopupItem("Go to 'g'", false);
	}

	// m - Mute
	if (key == 'm' || key == 'M') {
		bMute = !bMute;
		menu->SetPopupItem("Mute 'm'", bMute);
	}

	// g - Go to
	if (key == 'g' || key == 'G') {
		// Only enabled if paused
		if (menu->GetEnabled("Go to 'g'")) {
			if (m_frameSec > 0.0) {
				std::string str = "Enter the seconds to go to\n";
				str += "Current time is ";
				str += std::format("{:.2f}", m_frameSec);
				str += " seconds";
				std::string text;
				if (SpoutMessageBox(m_hWnd, str.c_str(), "Go to", MB_OKCANCEL, text) == IDOK) {
					if (!text.empty()) {
						double time = atof(text.c_str());
						if (time < m_Duration) {
							m_progress = time / m_Duration;
							RestartVideo(time);
							if (bPaused) {
								// Read the next frame and load the read texture with pixels
								if (m_pipein && m_pixelBuffer && m_SenderWidth > 0 && m_SenderHeight > 0) {
									if (fread(m_pixelBuffer, 1, m_SenderWidth * m_SenderHeight * 4, m_pipein)) {
										sender.LoadTexturePixels(readTexture.getTextureData().textureID,
											readTexture.getTextureData().textureTarget,
											m_SenderWidth, m_SenderHeight, m_pixelBuffer, GL_BGRA);
										// Draw the frame
										bPosition = true;
										bReadVideo = true;
									}
								}
							}
						}
						else {
							SpoutMessageBox("Seconds entered exceeds video duration\n");
						}
					}
				}
			}
		}
	}

	// c - Copy
	if ((key == 'c' || key == 'C') && menu->GetEnabled("Copy 'c'")) {
		if (myTexture.isAllocated()) {
			ofPixels myPixels;
			myTexture.readToPixels(myPixels);
			myPixels.setImageType(OF_IMAGE_COLOR_ALPHA); // Ensure RGBA pixel format
			// Flip image data for clipboard DIB
			sender.spoutcopy.FlipBuffer((unsigned char *)myPixels.getData(), m_SenderWidth, m_SenderHeight, GL_RGBA);
			if (CopyToClipBoard(m_hWnd, myPixels.getData(), GL_RGBA, m_SenderWidth, m_SenderHeight)) {
				SpoutMessageBox(m_hWnd, "Image copied to the clipboard", "Information", MB_OK | MB_ICONINFORMATION, 1200);
			}
			else {
				SpoutMessageBox(m_hWnd, "Error copying image to the clipboard", "Warning", MB_OK | MB_ICONWARNING);
			}
			myPixels.clear();
		}
	}

	// a - Adjust
	if (key == 'a' || key == 'A' && menu->GetEnabled("Adjust 'a'")) {
		if (sender.IsInitialized()) {
			if (!hwndAdjust) {
				// Open the adjust menu
				hwndAdjust = adjust->Open("Adjust");
				menu->SetPopupItem("Adjust 'a'", true);
				SetFocus(m_hWnd);
			}
			else {
				adjust->Close();
				menu->SetPopupItem("Adjust 'a'", false);
			}
		}
	}


	// r - Restart the same video
	if (key == 'r' || key == 'R') {
		if (bPaused) {
			// Mouse press at the start of the progess bar
			mousePressed(1, ofGetHeight()-15, 0);
		}
		else {
			RestartVideo();
		}
	}

	// e - End of the video
	if (key == 'e' || key == 'E') {
		if (bPaused) {
			// Mouse press at the end of the progess bar
			double width = (double)ofGetWidth();
			double interval = width/m_Duration;
			int xpos = (int)(width-interval)+(int)interval-1;
			mousePressed(xpos, ofGetHeight()-15, 0);
		}
		else {
			// Same behaviour as VLC - starts again
			RestartVideo();
		}
	}

	// s - Stop and close video
	if (key == 's' || key == 'S') {
		// Stop audio and draw
		bNCmousePressed = true;
		// Stop soundstream
		soundStream.stop();
		// Release FFmpeg resources
		CloseFFmpeg();
		// Release the sender
		if (m_pixelBuffer) delete[] m_pixelBuffer;
		m_pixelBuffer = nullptr;
		sender.ReleaseSender();
		// Clear the video path
		m_videopath.clear();
		// Start audio and draw
		bNCmousePressed = false;
		// Cancel paused
		bPaused = false;
		soundStream.start();
		// Quit full screen if set
		if (bFullScreen) {
			bFullScreen = false;
			doFullScreen(bFullScreen);
		}
		// Close adjust dialog
		if (hwndAdjust) {
			adjust->Close();
			hwndAdjust = nullptr;
		}
		menu->EnablePopupItem("Adjust 'a'", false);
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
	// Mouse press on progress bar
	int ypos = ofGetHeight()-15;
	if (y >= ypos && y < ypos+10) {
		// X position in seconds
		double position = (double)x*m_Duration/(double)ofGetWidth();
		// Back up one frame time
		// to allow for reading from the pipe again
		// e.g. for 30 frames per second, 1 frame = 1/30 seconds
		position -= 1.0/(double)m_FrameRate;
		RestartVideo(position);
		if (bPaused) {
			// Read the next frame and load the read texture with pixels
			if (m_pipein && m_pixelBuffer && m_SenderWidth > 0 && m_SenderHeight > 0) {
				if (fread(m_pixelBuffer, 1, m_SenderWidth * m_SenderHeight * 4, m_pipein)) {
					sender.LoadTexturePixels(readTexture.getTextureData().textureID,
						readTexture.getTextureData().textureTarget,
						m_SenderWidth, m_SenderHeight, m_pixelBuffer, GL_BGRA);
					// Draw the frame
					bPosition = true;
					bReadVideo = true;
				}
			}
		}
		else {
			// Signal draw to read straight away
			bReadVideo = true;
		}

		// "Go to" time is updated in RestartVideo and audioOut
		// Handle menu item
		if (bPaused)
			menu->EnablePopupItem("Go to 'g'", true);
		else
			menu->EnablePopupItem("Go to 'g'", false);
	}

	// Mouse press on icons
	// 0 reverse, 1 Pause, 2 Play, 3 stop 4 forward
	// 5 fullscreen, 6, sound, 7 mute

	// Icon 0 position
	int xpos = 10;
	ypos = ofGetHeight()-icon_size - 20;

	// 1/2 - pause/play
	if (x > xpos && x <= (xpos + icon_size)
	&& y > ypos && y <= (ypos + icon_size)) {
		keyPressed('p');
	}

	// 0 - reverse - start again
	xpos += icon_size*3/2;
	if (x > xpos && x <= (xpos + icon_size)
	&& y > ypos && y <= (ypos + icon_size)) {
		keyPressed('r');
	}

	// 3 - stop and close
	xpos += icon_size*1.1;
	if (x > xpos && x <= (xpos + icon_size)
	&& y > ypos && y <= (ypos + icon_size)) {
		keyPressed('s');
	}

	// 4 - fast forward
	xpos += icon_size*1.1;
	if (x > xpos && x <= (xpos + icon_size)
	&& y > ypos && y <= (ypos + icon_size)) {
		keyPressed('e');
	}

	// 5 full screen
	xpos += icon_size*3/2;
	if (x > xpos && x <= (xpos + icon_size)
	&& y > ypos && y <= (ypos + icon_size)) {
		keyPressed('f');
		m_iconColor[9] = icon_background_color;
	}

	// 6/7 sound/mute
	xpos += icon_size*3/2;
	if (x > xpos && x <= (xpos + icon_size)
	&& y > ypos && y <= (ypos + icon_size)) {
		keyPressed('m');
	}


}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{
	if (bShowInfo) {

		// 0 reverse, 1 Pause, 2 Play, 3 stop 4 forward
		// 5 fullscreen, 6, sound, 7 mute

		// Icon 0 position
		int xpos = 10;
		int ypos = ofGetHeight()-icon_size - 20;

		// 1/2 - pause/play
		if (x > xpos && x <= (xpos + icon_size)
		&&	y > ypos && y <= (ypos + icon_size)) {
			m_iconColor[1] = icon_highlight_color;
			m_iconColor[2] = icon_highlight_color;
		}
		else {
			m_iconColor[2] = icon_background_color;
			m_iconColor[3] = icon_background_color;
		}

		// 0 - reverse
		xpos += icon_size*3/2;
		if (x > xpos && x <= (xpos+icon_size)
		&&	y > ypos && y <= (ypos+icon_size))
			m_iconColor[0] = icon_highlight_color;
		else m_iconColor[0] = icon_background_color;
		
		// 3 - stop
		xpos += icon_size*1.1;
		if (x > xpos && x <= (xpos + icon_size)
		&&	y > ypos && y <= (ypos + icon_size))
			m_iconColor[3] = icon_highlight_color;
		else m_iconColor[3] = icon_background_color;

		// 4 - forward
		xpos += icon_size*1.1;
		if (x > xpos && x <= (xpos + icon_size)
		&&	y > ypos && y <= (ypos + icon_size))
			m_iconColor[4] = icon_highlight_color;
		else m_iconColor[4] = icon_background_color;

		// 5 fullscreen
		xpos += icon_size*3/2;
		if (x > xpos && x <= (xpos + icon_size)
		&& y > ypos && y <= (ypos + icon_size)) {
			m_iconColor[5] = icon_highlight_color;
		}
		else m_iconColor[5] = icon_background_color;

		// 6/7 sound/mute
		xpos += icon_size*3/2;
		if (x > xpos && x <= (xpos + icon_size)
		&&	y > ypos && y <= (ypos + icon_size)) {
			m_iconColor[7] = icon_highlight_color;
			m_iconColor[6] = icon_highlight_color;
		}
		else {
			m_iconColor[6] = icon_background_color;
			m_iconColor[7] = icon_background_color;
		}
	}
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{
	if (OpenVideo(dragInfo.files[0].string())) {
		OpenSender();
	}
}

//--------------------------------------------------------------
// Open FFmpeg Video and Audio pipes
bool ofApp::OpenVideo(std::string filePath, double seconds)
{
	if (filePath.empty() || _access(filePath.c_str(), 0) == -1)
		return false;

	// Close adjust dialog
	if (hwndAdjust) {
		adjust->Close();
		hwndAdjust = nullptr;
	}
	menu->EnablePopupItem("Adjust 'a'", false);

	// Stop audioOut
	soundStream.stop();

	// Cancel paused
	bPaused = false;

	// Reset counters
	m_Frames = 0;
	m_FramesRead = 0;
	m_Duration = 0.0;
	m_audioFramesPlayed = 0;
	m_progress = 0.0; // Progress bar position

	// Get information from the movie file using ffprobe
	// Sets the width, height and duration globals
	if (!ffprobe(filePath)) {
		MessageBoxA(NULL, "FFprobe error", "Warning", MB_OK);
		return false;
	}

	//
	// _popen for FFmpeg and FFprobe will open a console window.
	// To hide the output, open a console first and then hide it.
	// An application can have only one console window.
	// If one exists, leave management to the application.
	// This project does not open a console window - see main.cpp
	//
	if (!GetConsoleWindow()) {
		if (AllocConsole()) {
			FILE* pCout = nullptr;
			freopen_s(&pCout, "CONOUT$", "w", stdout);
		}
		HWND hwnd = GetConsoleWindow();
		if (hwnd) {
			ShowWindow(hwnd, SW_HIDE);
			ShowWindow(hwnd, SW_MINIMIZE);
			ShowWindow(hwnd, SW_HIDE);
		}
	}

	return OpenFFmpeg(filePath, seconds);

}

bool ofApp::OpenFFmpeg(std::string filePath, double seconds)
{
	// Safety for future revisions
	if (filePath.empty() || _access(filePath.c_str(), 0) == -1)
		return false;

	// Stop draw() read
	bReadVideo = false;

	if (m_pipein) {
		_pclose(m_pipein);
		m_pipein = nullptr;
	}
	m_input = m_ffmpegPath;

	// Auto detect hardware acceleration
	m_input += " -hwaccel auto";

	// Auto thread count
	m_input += " -threads 0";

	// Seek video to startseconds
	// Input seeking : -ss before -i
	// HH:MM:SS.Msec (e.g. 01:23:45.678)
	// FFmpeg starts at the requested timestamp.
	if (seconds > 0.0) {
		m_input += " -ss ";
		int hrs  = (int)(seconds/3600.0);
		int mins = (int)(seconds/60.0)-hrs*60;
		double secs = seconds - (double)(hrs*3600+mins*60);
		std::string time = std::format("{:02}:{:02}:{:04.2f}", hrs, mins, secs);
		m_input += time;
		// For "Go to" time
		m_frameSec = seconds;
	}

	// Quiet console output
	m_input += " -loglevel quiet";

	// Enable decoding with transparency for VP9/WebM videos.
	// Use the libvpx-vp9 codec for decoding the input.
	if (!m_codecName.empty() && m_codecName == "vp9") {
		m_input += " -c:v libvpx-vp9";
	}
	//
	m_input += " -i ";
	m_input += "\"";
	m_input += filePath;
	m_input += "\"";
	// 60 fps can be too high for FFmpeg pipe read
	// so reduce the frame rate here.
	// FFmpeg will drop frames if the video file frame rate is higher
	double frate = m_FrameRate;
	if (frate > 30.0) {
		frate = 30.0;
		m_FrameRate = frate;
	}
	m_input += " -vf fps=";
	m_input += std::to_string(frate);

	//
	// FFmpeg pipe read (fread) may be too slow with large image data
	// (typically 10-14 msec at 1920x1080, 3-4 msec at 1280x720)
	// and audio can drift out of sync. Reduce the output width to 1280
	// while preserving aspect ratio
	//
	if (bScale) { // Resize menu option
		unsigned int width = 1280;
		if (m_SenderWidth > width) {
			// Calculate from m_SenderWidth/m_SenderHeight
			m_SenderHeight = width * m_SenderHeight / m_SenderWidth;
			m_SenderWidth = width;
		}
		m_input += ",scale=";
		m_input += to_string(m_SenderWidth);
		m_input += ":";
		m_input += to_string(m_SenderHeight);
	}
	// Specify BGRA pixel format to match the sender format.
	m_input += " -f image2pipe -vcodec rawvideo -pix_fmt bgra -";
	m_pipein = _popen(m_input.c_str(), "rb");
	if (m_pipein) {
		if (m_pixelBuffer) delete[] m_pixelBuffer;
		unsigned int buffersize = m_SenderWidth * m_SenderHeight * 4;
		m_pixelBuffer = new unsigned char[buffersize];
	}
	else {
		MessageBoxA(NULL, "FFmpeg open failed", "Warning", MB_OK | MB_TOPMOST);
		return false;
	}

	// Check for an audio stream returned by FFprobe
	if (m_sampleRate > 0 && m_nChannels > 0) {

		// Audio pipe
		if (m_audioPipe) {
			_pclose(m_audioPipe);
			m_audioPipe = nullptr;
		}

		m_audioInput = m_ffmpegPath; // FFmpeg.exe path
		// Seek audio to seconds
		if (seconds > 0.0) {
			m_audioInput += " -ss ";
			m_audioInput += std::to_string(seconds);
		}
		// Quiet console output
		m_audioInput += " -loglevel quiet";
		m_audioInput += " -i ";
		m_audioInput += "\"";
		m_audioInput += filePath; // Video file path
		m_audioInput += "\"";
		// Output raw PCM
		m_audioInput += " -f s16le";
		m_audioInput += " -acodec pcm_s16le"; // PCM signed 16-bit little-endian
		m_audioInput += " -ac ";
		m_audioInput += std::to_string(m_nChannels); // Number of channels (2 = stereo)
		m_audioInput += " -ar ";
		m_audioInput += std::to_string(m_sampleRate);  // ouput sample rate e.g. 44100 Hz
		m_audioInput += " -";
		m_audioPipe = _popen(m_audioInput.c_str(), "rb");
	}

	if (m_pipein) {
		m_videopath = filePath;
		// Reset video frame counter
		m_FramesRead = 0;
		// Reset audio frame counter
		m_audioFramesPlayed = 0;
		// Signal draw() to read video from the pipe
		bReadVideo = true;
		// Handle menu items
		if (bPaused)
			menu->EnablePopupItem("Go to 'g'", true);
		else
			menu->EnablePopupItem("Go to 'g'", false);
		menu->EnablePopupItem("Adjust 'a'", true);
		menu->EnablePopupItem("Copy 'c'", true);
		menu->EnablePopupItem("Capture", true);
		menu->EnablePopupItem("Save as", true);
		return true;
	}
	else
		return false;

} // end OpenFFmpeg


//--------------------------------------------------------------
bool ofApp::OpenSender()
{
	// Stop audio and draw
	bNCmousePressed = true;
	
	sender.ReleaseSender();

	// Allocate a texture for read
	readTexture.allocate(m_SenderWidth, m_SenderHeight, GL_RGBA);
	// Allocate a texture for draw
	myTexture.allocate(m_SenderWidth, m_SenderHeight, GL_RGBA);

	// Set up soundstream
	soundStream.stop(); // Stop and close for repeats
	soundStream.close();

	// Set up soundstream if there is audio
	if (m_audioPipe) {

		ofSoundStreamSettings settings;
		auto devices = soundStream.getDeviceList();
		if (!devices.empty()) {
			// Select the device number as required by the system
			settings.setOutDevice(devices[0]); // Speakers
			settings.setOutListener(this);
			settings.sampleRate = m_sampleRate;
			settings.numOutputChannels = m_nChannels;
			settings.numInputChannels = 0;
			settings.bufferSize = 1024; // Can be adjusted
			if (soundStream.setup(settings)) {
				// PCM data buffer used in audioOut
				m_pcmBuffer.resize(settings.bufferSize * settings.numOutputChannels);
				// printf("\nSoundstream setup\n");
				// printf("  nSamples     = %d\n", soundStream.getBufferSize());
				// printf("  Sample rate  = %d\n", soundStream.getSampleRate());
				// printf("  N channels   = %d\n", soundStream.getNumOutputChannels());
			}
			else {
				printf("OpenSender : Soundstream setup failed\n");
				return false;
			}
		}
	}

	// Allow audio and draw
	bNCmousePressed = false;

	return true;

}

//--------------------------------------------------------------
// Release FFmpeg resources
void ofApp::CloseFFmpeg()
{
	if (m_pipein) {
		// stop sound
		fflush(m_pipein);
		_pclose(m_pipein);
	}
	m_pipein = nullptr;
	if (m_audioPipe) {
		 fflush(m_audioPipe);
		_pclose(m_audioPipe);
	}
	m_audioPipe = nullptr;
	// Handle menu items
	if (bPaused)
		menu->EnablePopupItem("Go to 'g'", true);
	else
		menu->EnablePopupItem("Go to 'g'", false);
	menu->EnablePopupItem("Adjust 'a'", false);
	menu->EnablePopupItem("Copy 'c'", false);
	menu->EnablePopupItem("Capture", false);
	menu->EnablePopupItem("Save as", false);
}

//--------------------------------------------------------------
// Close and restart at startseconds using the same video file
void ofApp::RestartVideo(double startseconds)
{
	// Do not cancel paused to allow positioning
	// by click on the control bar or "Go to".

	// No audio while syncing video (reset in draw)
	bVideoSync = true;
	// Stop audio (reset in draw)
	bNCmousePressed = true; // Reset in Draw
	// Stop soundstream
	if(m_audioPipe)
		soundStream.stop();
	// Release FFmpeg resources
	CloseFFmpeg();
	// Start again at startseconds
	OpenFFmpeg(m_videopath, startseconds);
	if(m_audioPipe)
		soundStream.start();
	// Update progress bar position
	m_progress = startseconds/m_Duration; // Progress position
}

//--------------------------------------------------------------
//
// Menu function callback
//
// This function is called by ofxWinMenu when an item is selected.
// The the title and state can be checked for required action.
// 
void ofApp::appMenuFunction(string title, bool bChecked)
{
	ofFileDialogResult result;
	string filePath;

	// Keep the audio in sync with video when menu selection
	// or mouse click on the title bar stops drawing.
	// WM_ENTERMENULOOP and WM_EXITMENULOOP are returned by ofxWinMenu
	// but are not required if WM_NCLBUTTONDOWN is tested.
	if (title == "WM_NCLBUTTONDOWN") {
		// WM_NCLBUTTONUP is not generated if the
		// mouse is released on the title bar.
		// The flag is reset when when Draw resumes and is
		// also used when video or audio has to be stopped
		bNCmousePressed = true;
		return;
	}

	//
	// File menu
	//
	if (title == "Open video") {
		// Move to the video folder
		std::string str;
		if (!m_videopath.empty()) {
			size_t pos = m_videopath.rfind("/");
			if (pos == std::string::npos) pos = m_videopath.rfind("\\");
			str = m_videopath.substr(0, pos);
		}
		else {
			str = m_exePath;
			str += "/data/videos/";
		}
		result = ofSystemLoadDialog("Select a video file", false, str.c_str());
		if (result.bSuccess) {
			if(OpenVideo(result.filePath)) {
				OpenSender();
			}
		}
	}

	if (title == "Video folder") {
		std::string str;
		if (!m_videopath.empty()) {
			size_t pos = m_videopath.rfind("/");
			if (pos == std::string::npos) pos = m_videopath.rfind("\\");
			str = m_videopath.substr(0, pos);
		}
		else {
			str = m_exePath;
			str += "/data/videos/";
		}

		// Does the video folder exist ?
		if (_access(str.c_str(), 0) == -1) {
			// Use the executable path as default
			str = m_exePath;
		}
		if(!ShellExecuteA(m_hWnd, "open", str.c_str(), NULL, NULL, SW_SHOWNORMAL)) {
			MessageBoxA(NULL, "No video loaded", "Warning", MB_ICONWARNING | MB_OK);
		}
	}

	if (title == "Image folder") {
		std::string str = m_exePath;
		str += "/data/images/";
		// Does the video folder exist ?
		if (_access(str.c_str(), 0) == -1) {
			// Use the executable path as default
			str = m_exePath;
		}
		if(!ShellExecuteA(m_hWnd, "open", str.c_str(), NULL, NULL, SW_SHOWNORMAL)) {
			MessageBoxA(NULL, "No image folder", "Warning", MB_ICONWARNING | MB_OK);
		}
	}

	if (title == "Exit") {
		ofExit();
	}

	//
	// Output menu
	//

	// Activate the adjust dialog
	if (title == "Adjust 'a'") {
		keyPressed('a');
	}

	if (title == "Mute 'm'") {
		keyPressed('m');
	}

	if (title == "Show on top") {
		bTopmost = bChecked;
		doTopmost(bTopmost);
		menu->SetPopupItem("Show on top", bTopmost);
	}

	if (title == "Resize") {
		bScale = bChecked;
		menu->SetPopupItem("Resize", bScale);
		// Release FFmpeg resources
		// and close the video playing
		CloseFFmpeg();
		if (!m_videopath.empty()) {
			// Release the sender
			if (m_pixelBuffer) delete[] m_pixelBuffer;
			m_pixelBuffer = nullptr;
			sender.ReleaseSender();
			// Start again
			if(OpenVideo(m_videopath))
				OpenSender();
		}
	}

	if (title == "Go to 'g'") {
		keyPressed('g');
	}

	if (title == "Copy 'c'" && !m_videopath.empty()) {
		keyPressed('c');
	}

	if ((title == "Capture" || title == "Save as") && !m_videopath.empty()) {
		std::string savepath;
		if (title == "Capture") {
			// Make a timestamped image file name
			std::string imagename = ofGetTimestampString() + ".png";
			// Save png image to bin>data>captures
			savepath = m_exePath;
			savepath += "\\data\\images\\";
			savepath += imagename;
		}
		else {
			savepath = EnterFileName();
			if (savepath.empty())
				return;
			// Enter a file extension if none entered
			if (ofFilePath::getFileExt(savepath).empty())
					savepath += ".png";
		}
		// Get pixels from the rgba texture
		ofImage myimage;
		myTexture.readToPixels(myimage.getPixels());
		myimage.save(savepath); // save image
		std::string str = "Image saved to\n" + savepath;
		SpoutMessageBox(m_hWnd, str.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION | MB_TOPMOST, 2000);
	}

	if (title == "Preview 'v'") {
		keyPressed('v');
	}

	if (title == "Full screen 'f'") {
		keyPressed('f');
	}

	if (title == "Show controls - Space") {
		bShowInfo = bChecked;
		menu->SetPopupItem("Show controls - Space", bShowInfo);
	}

	//
	// Help menu
	//

	if (title == "About") {

		// Spout version
		std::string about = "                       Spout video sender with audio\n";
		about += "                   using Openframeworks and FFmpeg\n";
		about += "                                <a href=\"http://spout.zeal.co\">http://spout.zeal.co</a>\n";
		about += "                            Spout Version ";
		about += GetSDKversion();
		about += "\n\n";

		about += "      An example of a sender for video files using FFmpeg with\n";
		about += "      two pipes, one for video and the other for audio.\n\n";
		about += "      ofSoundStream and audioOut enable sound output and Draw is\n";
		about += "      kept in sync with audio by timing and and frame count matching.\n";
		about += "      This is a simple method compared to using FFmpeg libraries.\n";
		about += "      Seeking is achieved by specifying the start time for video pipe read.\n";
		about += "      Performance varies depending on the encoder used for the video.\n\n";

		about += "      Uses the <a href=\"https://github.com/leadedge/ofxWinMenu\">ofxWinMenu</a> addon to create a menu and to manage\n";
		about += "      caption mouse press, and <a href=\"https://github.com/leadedge/ofxWinDialog\">ofxWinDialog</a> for an image adjust dialog.\n";
		about += "      Uses a <a href=\"https://github.com/leadedge/Spout2/blob/master/Building%20the%20libraries.pdf\">static library</a> for Spout functions, generated using Cmake.\n\n";
		about += "      FFmpeg.exe and FFprobe.exe are required.\n";
		about += "      Select the \"FFmpeg\" button below for more information\n";

		// Icon in the caption rather than the dialog window
		SpoutMessageBoxIconSmall();
		SpoutMessageBoxButton(1000, L"FFmpeg");
		SpoutMessageBoxButton(2000, L"Options");

		int iRet = SpoutMessageBox(NULL, about.c_str(), "FFmpeg", MB_ICONINFORMATION | MB_OK | MB_TOPMOST);
		if(iRet == 1000) {
			// FFmpeg download instructions
			// Keep the dialog open with "?noclose" in the url
			// and topmost so that the instructions remain visible
			// See ffdownloadstr()
			std::string str = "Downloading FFmpeg\n\n" + ffdownloadstr();
			SpoutMessageBoxIconSmall();
			SpoutMessageBox(NULL, str.c_str(), "FFmpeg", MB_ICONINFORMATION | MB_TOPMOST | MB_OK);
		}
		else if (iRet == 2000) {
			std::string str = "        File > Open video - Select a video file\n";
			str += "        File > Video folder - open folder of the last video\n";
			str += "        File > Image folder - open folder for image captures\n\n";
			str += "        Output > Adjust (a) - open adjust dialog\n";
			str += "        Output > Go to (g) - go to position in seconds\n";
			str += "        Output > Copy (c) - copy the current frame to the clipboard\n";
			str += "        Output > Capture - save the current frame as timestamp image file\n";
			str += "        Output > Save as - save the current frame as an image file\n";
			str += "        Output > Mute (m) - mute speakers\n";
			str += "        Output > Resize - limit video to 1280 width (resets)\n";
			str += "            FFmpeg pipe read (fread) can be slow with large images,\n";
			str += "            typically 10-14 msec at 1920x1080 compared to 3-4 msec\n";
			str += "            at 1280x720, and audio can drift out of sync. This option\n";
			str += "            limits output width to 1280 while preserving aspect ratio.\n";
			str += "            The output frame rate is also limited to 30fps and FFmpeg\n";
			str += "            drops frames to keep that rate.\n\n";
			str += "        View > Show on top - set window topmost\n";
			str += "        View > Show controls (space bar) - show video controls\n";
			str += "        View > Preview (v) - show minimal preview window\n";
			str += "        View > Full screen (f) - show full screen (ESC to exit)\n";
			SpoutMessageBoxIconSmall();
			SpoutMessageBox(NULL, str.c_str(), "Options", MB_ICONINFORMATION | MB_OK | MB_TOPMOST);
		}

	}

} // end appMenuFunction

//
// ================= Adjust dialog ===================
//
void ofApp::CreateAdjustDialog()
{
	int ypos = 10;

	adjust->TextColor(0x0F0000);
	adjust->AddGroup("Colour",        15, ypos, 420, 245);
	ypos += 30;

	adjust->AddText("Brightness",     30, ypos,  95, 25);
	adjust->AddSlider("Brightness",  120, ypos, 250, 25, -1.0, 1.0, Brightness, true);
	ypos += 30;
	adjust->AddText("Contrast",       30, ypos,  95, 25);
	adjust->AddSlider("Contrast",    120, ypos, 250, 25, 0.0, 2.0, Contrast, true);
	ypos += 30;
	adjust->AddText("Saturation",     30, ypos,  95, 25);
	adjust->AddSlider("Saturation",  120, ypos, 250, 25, 0.0, 4.0, Saturation, true);
	ypos += 30;
	adjust->AddText("Gamma",          30, ypos,  95, 25);
	adjust->AddSlider("Gamma",       120, ypos, 250, 25, 0.0, 2.0, Gamma, true);
	ypos += 30;
	adjust->AddText("Temperature",    30, ypos,  95, 25);
	adjust->AddSlider("Temperature", 120, ypos, 250, 25, 3500.0, 9500.0, Temp, true);
	ypos += 30;
	adjust->AddText("Sharpen",         30, ypos,  95, 25);
	adjust->AddSlider("Sharpen",      120, ypos, 250, 25, 0.0, 1.0, Sharpness, true);
	ypos += 30;
	// SharpWidth - 3x3, 5x5, 7x7 : 3.0, 5.0, 7.0
	adjust->AddRadioGroup();
	adjust->AddRadioButton("IDC_SHARPNESS_3x3", "3 x 3",  50, ypos,  80, 25, b3x3);
	adjust->AddRadioButton("IDC_SHARPNESS_5x5", "5 x 5", 140, ypos,  80, 25, b5x5);
	adjust->AddRadioButton("IDC_SHARPNESS_7x7", "7 x 7", 230, ypos,  80, 25, b7x7);
	adjust->AddCheckBox("IDC_ADAPTIVE", "Adaptive",      320, ypos,  80, 25, bAdaptive);
	ypos += 45;

	adjust->AddButton("IDC_RESTORE", "Restore",  90, ypos, 70, 30);
	adjust->AddButton("IDC_RESET",   "Reset",   165, ypos, 70, 30);
	adjust->AddButton("IDC_OK",      "OK",      240, ypos, 70, 30);
	adjust->AddButton("IDC_CANCEL",  "Cancel",  315, ypos, 70, 30);

	// Open to the left of the main window
	adjust->SetPosition(-472, 0, 472, 350);

}

//
// Adjust dialog callback function
//
void ofApp::AdjustCallback(std::string title, std::string text, int value)
{
	if (title == "WM_DESTROY") {
		hwndAdjust = nullptr;
		// Uncheck menu item
		menu->SetPopupItem("Adjust 'a'", false);
		return;
	}

	if (title == "Brightness") {
		Brightness = (float)value/100.0;
	}
	if (title == "Contrast") {
		Contrast = (float)value/100.0;
	}
	if (title == "Saturation") {
		Saturation = (float)value/100.0;
	}
	if (title == "Gamma") {
		Gamma = (float)value/100.0;
	}
	if (title == "Temperature") {
		Temp = (float)value/100.0;
	}
	if (title == "Sharpen") {
		Sharpness = (float)value/100.0;
	}
	if (title == "IDC_SHARPNESS_3x3") {
		if (value == 1) {
			b3x3 = true;
			b5x5 = false;
			b7x7 = false;
			Sharpwidth = 3.0;
		}
	}
	if (title == "IDC_SHARPNESS_5x5") {
		if (value == 1) {
			b3x3 = false;
			b5x5 = true;
			b7x7 = false;
			Sharpwidth = 5.0;
		}
	}
	if (title == "IDC_SHARPNESS_7x7") {
		if (value == 1) {
			b3x3 = false;
			b5x5 = false;
			b7x7 = true;
			Sharpwidth = 7.0;
		}
	}
	if (title == "IDC_ADAPTIVE") {
		bAdaptive = (value == 1);
	}

	if (title == "IDC_RESTORE") {
		// Soft reset to old pre-open values
		adjust->Restore();
		// Return values to ofApp
		// Necessary for restore
		adjust->GetControls();
	}

	if (title == "IDC_RESET") {
		// Hard reset to defaults
		Brightness = 0.0; // -1 - 1 default 0
		Contrast   = 1.0; //  0 - 2 default 1
		Saturation = 1.0; //  0 - 4 default 1
		Gamma      = 1.0; //  0 - 2 default 1
		Temp       = 6500.0; // 3500 - 9500 default 6500 (daylight)
		// Reset all dialog controls
		adjust->Reset();
	}

	if (title == "IDC_OK") {
		// Get current values of all controls
		adjust->GetControls();
		// Save settings
		adjust->Save("Adjust");
		adjust->Close();
	}

	if (title == "IDC_CANCEL") {
		// Restore all controls with old values
		adjust->Restore();
		// Return values to ofApp
		adjust->GetControls();
		adjust->Close();
	}
} // end Adjust callback


// Run FFprobe on a movie file and produce
// an ini file with the stream information
bool ofApp::ffprobe(std::string videoPath)
{
	// Get information from the movie file using ffprobe and write to an ini file
	// Use a batch file with the required ffprobe options and pass the path to ShellExecute
	std::string probepath = m_exePath;
	probepath += "/data/ffmpeg/probe.bat";

	// New video file
	if (videoPath != m_videopath) {
		// Does the batch file myprobe.ini exist ?
		if (_access(probepath.c_str(), 0) == -1) {
			// Create the probe.bat file
			std::ofstream batchfile(probepath);
			if (!batchfile) {
				printf("Could not create\n%s\n", probepath.c_str());
				return false;
			}
			// File created OK
			std::string str = "%~dp0/ffprobe.exe -v error -show_streams -of default=noprint_wrappers=1:nokey=1 -print_format ini -i %1 > \"%~dp0/myprobe.ini\"\n";
			batchfile << str;
			batchfile.close();
		}

		// Input to ffprobe
		std::string input = "\"";
		input += videoPath;
		input += "\"";

		// In the batch file, %~dp0 returns the Drive and Path to the batch script

		// Open ffprobe and wait for completion
		STARTUPINFOA si = { sizeof(STARTUPINFOA) };
		si = { sizeof(STARTUPINFOA) };
		DWORD dwExitCode = 0;
		ZeroMemory((void*)&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE; // hide the ffprobe console window
		PROCESS_INFORMATION pi{};
		std::string cmdstring = probepath + " " + input;
		if (CreateProcessA(NULL, (LPSTR)cmdstring.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			if (pi.hProcess) {
				do {
					GetExitCodeProcess(pi.hProcess, &dwExitCode);
				} while (dwExitCode == STILL_ACTIVE);
				CloseHandle(pi.hProcess);
			}
			if (pi.hThread)	CloseHandle(pi.hThread);
		}
		else {
			MessageBoxA(NULL, "FFprobe CreateProcess failed", "Warning", MB_OK | MB_TOPMOST);
			return false;
		}
	} // endif new video file

	// Read the ini file produced by FFprobe to get the video information
	char initfile[MAX_PATH]{};
	strcpy_s(initfile, MAX_PATH, m_exePath.c_str());
	strcat_s(initfile, MAX_PATH, "\\DATA\\FFMPEG\\myprobe.ini");
	if (_access(initfile, 0) == -1) {
		MessageBoxA(NULL, "FFprobe ini file not found", "Warning", MB_OK | MB_TOPMOST);
		return false;
	}

	char tmp[MAX_PATH]{};
	DWORD dwResult = 0;
	m_SenderWidth = 0;
	m_SenderHeight = 0;

	// Find the first video stream
	char stream[100]{};
	for (int i=0; i<10; i++) { // arbritrary maximum
		sprintf_s(stream, 100, "streams.stream.%d", i);
		if (GetPrivateProfileStringA((LPCSTR)stream, (LPSTR)"codec_type", (LPSTR)"0", (LPSTR)tmp, 8, initfile) > 0) {
			if (strcmp(tmp, "video") == 0) {
				if (GetPrivateProfileStringA((LPCSTR)stream, (LPSTR)"width", NULL, (LPSTR)tmp, 8, initfile) > 0)
					m_SenderWidth = atoi(tmp);
				if (GetPrivateProfileStringA((LPCSTR)stream, (LPSTR)"height", NULL, (LPSTR)tmp, 8, initfile) > 0)
					m_SenderHeight = atoi(tmp);
				if (GetPrivateProfileStringA((LPCSTR)stream, (LPSTR)"duration", NULL, (LPSTR)tmp, 10, initfile) > 0)
					m_Duration = atof(tmp);
				// Duration is in seconds
				// If N/A - try tags
				if (strcmp(tmp, "N/A") == 0) {
					// [streams.stream.0.tags]
					// DURATION=00\:55\:15.314000000
					if (GetPrivateProfileStringA((LPCSTR)"streams.stream.0.tags", (LPSTR)"DURATION", (LPSTR)"-1", (LPSTR)tmp, MAX_PATH, initfile) > 0) {
						// Remove FFprobe escaping: "\:" -> ":"
						std::string str = tmp;
						size_t pos = 0;
						while ((pos = str.find("\\:")) != std::string::npos) {
							str.replace(pos, 2, ":");
						}
						// strip the fractional seconds
						str = str.substr(0, str.find('.'));
						// Convert to seconds
						int hrs = 0;
						int mins = 0;
						int secs = 0;
						char colon1, colon2;
						std::stringstream ss(str);
						ss >> hrs >> colon1 >> mins >> colon2 >> secs;
						m_Duration = (hrs*3600.0+mins*60.0+secs);
					}
				}
				// Total number of frames
				if (m_Duration > 0.0 && m_FrameRate > 0.0) {
					m_Frames = (long)(m_Duration*m_FrameRate);
				}

				dwResult = GetPrivateProfileStringA((LPCSTR)"streams.stream.0", (LPSTR)"r_frame_rate", (LPSTR)"30/1", (LPSTR)tmp, 11, initfile);
				if (dwResult == 0)
					dwResult = GetPrivateProfileStringA((LPCSTR)"streams.stream.0", (LPSTR)"avm_frame_rate", (LPSTR)"30/1", (LPSTR)tmp, 11, initfile);
				if (dwResult > 0) {
					std::string iniValue = tmp;
					auto pos = iniValue.find("/");
					double num = atof(iniValue.substr(0, pos).c_str());
					double den = atof(iniValue.substr(pos + 1, iniValue.npos).c_str());
					if (num > 0.0 && den > 0.0) {
						m_FrameRate = num/den;
					}
				}
				// Video codec name
				if(GetPrivateProfileStringA((LPCSTR)"streams.stream.0", (LPSTR)"codec_name", (LPSTR)"0", (LPSTR)tmp, 20, initfile))
					m_codecName = tmp;
					break;
			}
		}

	} // end all streams

	// Audio
	for (int i=0; i<10; i++) { // arbritrary maximum
		sprintf_s(stream, 32, "streams.stream.%d", i);
		if (GetPrivateProfileStringA((LPCSTR)stream, (LPSTR)"codec_type", (LPSTR)"0", (LPSTR)tmp, 8, initfile) > 0) {
			if (strcmp(tmp, "audio") == 0) {
				if (GetPrivateProfileStringA((LPCSTR)stream, (LPSTR)"sample_rate", NULL, (LPSTR)tmp, 20, initfile) > 0)
					m_sampleRate = atoi(tmp);
				if (GetPrivateProfileStringA((LPCSTR)stream, (LPSTR)"channels", NULL, (LPSTR)tmp, 8, initfile) > 0)
					m_nChannels = atoi(tmp);
				// Downmix 5/6 channels to stereo
				if(m_nChannels > 2) m_nChannels = 2;
				break;
			}
		} // end audio
	}

	if (m_SenderWidth == 0 || m_SenderHeight == 0)
		return false;

	return true;
}

//--------------------------------------------------------------
// Reset the window size
void ofApp::ResetWindow(int windowWidth, int windowHeight)
{
	// Adjust window to desired client size allowing for the menu
	RECT rect{};
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = windowWidth;
	rect.bottom = windowHeight;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW | WS_BORDER, true);

	// Full window size
	windowWidth  = rect.right - rect.left;
	windowHeight = rect.bottom - rect.top;

	// Get current position
	GetWindowRect(m_hWnd, &rect);

	// Set size and centre on the screen
	SetWindowPos(m_hWnd, NULL,
		(ofGetScreenWidth() - windowWidth)/2,
		(ofGetScreenHeight() - windowHeight)/2,
		windowWidth, windowHeight, SWP_SHOWWINDOW);

}

//--------------------------------------------------------------
// Keep topmost
void ofApp::doTopmost(bool bTop)
{
	if (bTop) {
		// Get the current top window for return
		m_hWndForeground = GetForegroundWindow();
		// Set this window topmost
		SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	else {
		SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		// Reset the window that was topmost before
		if (GetWindowLong(m_hWndForeground, GWL_EXSTYLE) & WS_EX_TOPMOST)
			SetWindowPos(m_hWndForeground, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		else
			SetWindowPos(m_hWndForeground, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
} // end doTopmost

//--------------------------------------------------------------
// Full screen or preview
void ofApp::doFullScreen(bool bEnable, bool bPreviewMode)
{
	char tmp[256]={};
	RECT rectTaskBar{};
	HWND hWndTaskBar{};
	HWND hwndTopmost = NULL;

	if (bEnable) {

		// Set to full screen or preview

		// m_hwndTop is set by user selection "Show on top"
		if (m_hwndTop) {
			hwndTopmost = m_hwndTop;
		}
		else {
			//
			// Find the current topmost window - if any.
			//
			// Get the first visible window in the Z order
			hwndTopmost = GetTopWindow(NULL);
			GetWindowTextA(hwndTopmost, (LPSTR)tmp, 256); // hwnd can be null
			do {
				if (hwndTopmost && tmp[0] && IsWindowVisible(m_hwndTop)
					&& GetWindowLong(hwndTopmost, GWL_EXSTYLE) & WS_EX_TOPMOST) {
					break;
				}
				else {
					if (hwndTopmost) {
						// Get next window
						hwndTopmost = GetNextWindow(hwndTopmost, GW_HWNDNEXT);
						if (hwndTopmost) {
							// Save the title
							GetWindowTextA(hwndTopmost, (LPSTR)tmp, 256);
						}
						else {
							break; // no more windows
						}
					}
					else {
						break;
					}
				}
			} while (hwndTopmost != NULL); // hwndTopmost is NULL if GetNextWindow finds no more windows
		}

		// Get the client/window adjustment values
		GetWindowRect(m_hWnd, &m_windowRect);
		GetClientRect(m_hWnd, &m_clientRect);
		m_AddX = (m_windowRect.right - m_windowRect.left) - (m_clientRect.right - m_clientRect.left);
		m_AddY = (m_windowRect.bottom - m_windowRect.top) - (m_clientRect.bottom - m_clientRect.top);
		// Current client window size for return to windowed
		m_nonFullScreenX = ofGetWidth();
		m_nonFullScreenY = ofGetHeight();

		// Save current size values
		GetWindowRect(m_hWnd, &m_windowRect);
		GetClientRect(m_hWnd, &m_clientRect);

		// Current window style
		m_dwStyle = GetWindowLongPtrA(m_hWnd, GWL_STYLE);

		// Remove the caption and frame
		SetWindowLongPtr(m_hWnd, GWL_STYLE, m_dwStyle & ~(WS_CAPTION | WS_THICKFRAME));

		// Remove the menu but don't destroy it
		menu->RemoveWindowMenu();

		hWndTaskBar = FindWindowA("Shell_TrayWnd", "");
		GetWindowRect(hWndTaskBar, &rectTaskBar);

		// Hide the System Task Bar
		SetWindowPos(hWndTaskBar, HWND_NOTOPMOST,
			0, 0, (rectTaskBar.right - rectTaskBar.left),
			(rectTaskBar.bottom - rectTaskBar.top),
			SWP_NOMOVE | SWP_NOSIZE);

		int x = 0; int y = 0; int w = 0; int h = 0;
		if (bPreviewMode) { // PREVIEW
			x = (int)m_windowRect.left
				+ GetSystemMetrics(SM_CXBORDER) * 2
				+ GetSystemMetrics(SM_CXFRAME)
				+ GetSystemMetrics(SM_CXDLGFRAME);
			y = (int)m_windowRect.top
				+ GetSystemMetrics(SM_CYCAPTION)
				+ GetSystemMetrics(SM_CYMENU)
				+ GetSystemMetrics(SM_CYBORDER) * 2
				+ GetSystemMetrics(SM_CYFRAME)
				+ GetSystemMetrics(SM_CYDLGFRAME);
			w = (int)(m_clientRect.right - m_clientRect.left);
			h = (int)(m_clientRect.bottom - m_clientRect.top);
		}
		else {
			// FULL SCREEN
			// Allow for multiple monitors
			HMONITOR monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO mi{};
			mi.cbSize = sizeof(mi);
			GetMonitorInfoA(monitor, &mi);
			x = (int)mi.rcMonitor.left;
			y = (int)mi.rcMonitor.top;
			w = (int)(mi.rcMonitor.right - mi.rcMonitor.left); // rcMonitor dimensions are LONG
			h = (int)(mi.rcMonitor.bottom - mi.rcMonitor.top);
		}

		// Rendering is slow if resized to the monitor extents.
		// Making it 1 pixel larger seems to fix it. Reason unknown.
		// Hide the window while re-sizing to avoid a flash effect
		SetWindowPos(m_hWnd, HWND_TOPMOST, x-1, y-1, w+2, h+2,
			SWP_HIDEWINDOW | SWP_NOREDRAW | SWP_FRAMECHANGED);

		ShowWindow(m_hWnd, SW_SHOW);
		SetFocus(m_hWnd);

	} // endif full screen
	else {
		// Exit full screen

		// Restore original style
		SetWindowLongPtrA(m_hWnd, GWL_STYLE, m_dwStyle);

		// Restore the menu
		menu->SetWindowMenu();

		// Restore the application window
		SetWindowPos(m_hWnd, NULL, m_windowRect.left, m_windowRect.top,
			m_nonFullScreenX+m_AddX, m_nonFullScreenY+m_AddY,
			SWP_SHOWWINDOW | SWP_NOZORDER | SWP_FRAMECHANGED);

		// Show the cursor
		ShowCursor(TRUE);

		// Show the menu
		DrawMenuBar(m_hWnd);

	} // endif not full screen

}

//--------------------------------------------------------------
// Show controls, progress bar, frame and total time
void ofApp::ShowInfo()
{
	// Progress bar
	if (m_Frames > 0L && m_progress > 0.0) {
		// Progress bar - m_progress is the position
		ofSetColor(204); // Light grey total
		ofDrawRectRounded(0, ofGetHeight()-15, ofGetWidth(), 10, 3);
		ofSetColor(40, 125, 204); // VLC blue
		ofDrawRectRounded(0, ofGetHeight()-15, ofGetWidth()*m_progress, 10, 3);
		ofSetColor(255);
		ofEnableAlphaBlending();
		int x = 10;
		int y = ofGetHeight()-icon_size - 20;

		// Pause/Play
		if (bPaused) {
			// If paused, draw Play
			ofSetColor(m_iconColor[2]);
			m_icons[2].draw(x, y, icon_size, icon_size);
		}
		else {
			// If playing, draw Pause
			ofSetColor(m_iconColor[1]);
			m_icons[1].draw(x, y, icon_size, icon_size);
		}
		x += icon_size*3/2; // 1.5

		// reverse
		ofSetColor(m_iconColor[0]);
		m_icons[0].draw(x, y, icon_size, icon_size);
		x += icon_size*1.1;

		// stop
		ofSetColor(m_iconColor[3]);
		m_icons[3].draw(x, y, icon_size, icon_size);
		x += icon_size*1.1;

		// forward
		ofSetColor(m_iconColor[4]);
		m_icons[4].draw(x, y, icon_size, icon_size);
		x += icon_size*3/2;

		// fullscreen
		ofSetColor(m_iconColor[5]);
		m_icons[5].draw(x, y, icon_size, icon_size);
		x += icon_size*3/2;

		// sound/mute
		if (!bMute) {
			ofSetColor(m_iconColor[6]);
			m_icons[6].draw(x, y, icon_size, icon_size);
		}
		else {
			ofSetColor(m_iconColor[7]);
			m_icons[7].draw(x, y, icon_size, icon_size);
		}
		x += icon_size*3/2;

	}

	std::string totaltime = std::format("{:02}:{:02}", (int)(m_Duration/60.0), (int)(std::fmod(m_Duration, 60)));
	if (!totaltime.empty()) {
		double framesec = m_Duration*m_progress;
		std::string frametime = std::format("{:02}:{:02}", (int)(framesec/60.0), (int)(std::fmod(framesec, 60)));
		std::string str = frametime + " of " + totaltime;
		int strwidth = myFont.stringWidth(str);
		int xpos = ofGetWidth() - strwidth-10;
		int ypos = ofGetHeight()-23; // mid icons
		ofSetColor(255);
		myFont.drawString(str, xpos, ypos);
	}
}

//--------------------------------------------------------------
// FFmpeg download string for messagebox
std::string ofApp::ffdownloadstr()
{
	// Keep the dialog open by using "?noclose" in the url
	std::string str = "        * Go to <a href=\"https://github.com/GyanD/codexffmpeg/releases?noclose\">https://github.com/GyanD/codexffmpeg/releases</a>\n";
	str += "        * Choose the \"Essentials\" build.\n";
	str += "          for example : ffmpeg-8.1.2-essentials_build.zip\n";
	str += "        * Download the archive and unzip to a convenient folder.\n";
	str += "        * Copy bin/FFmpeg.exe and bin/FFprobe.exe to\n";
	str += "          the application \"data/ffmpeg\" folder.\n\n";
	return str;
}

//--------------------------------------------------------------
// User file name entry
std::string ofApp::EnterFileName()
{
	// Prevent topmost window hiding file entry dialog
	if (bTopmost)
		SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	char fileName[MAX_PATH] {};
	char filePath[MAX_PATH]{};
	sprintf_s(filePath, MAX_PATH, m_exePath.c_str()); // exe folder
	strcat_s(filePath, MAX_PATH, "\\data\\images\\");

	OPENFILENAMEA ofn={};
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	HWND hwnd = NULL;
	ofn.hwndOwner = hwnd;
	ofn.hInstance = GetModuleHandle(0);
	ofn.nMaxFileTitle = 31;
	// ofn.lpstrInitialDir is the initial directory
	ofn.lpstrInitialDir = (LPSTR)filePath;
	// ofn.lpstrFile is the initial file name and is returned by the entry
	ofn.lpstrFile = (LPSTR)fileName;
	ofn.nMaxFile = MAX_PATH;
	// Image type supported .png
	ofn.lpstrFilter = "PNG (Portable Network Graphics)\0*.png\0TIF (Tagged Image File Format)\0*.tif\0JPG (JPEG file interchange format)\0*.jpg\0BMP (Windows bitmap)\0*.bmp\0All files (*.*)\0*.*\0";
	ofn.lpstrDefExt = "";
	// OFN_OVERWRITEPROMPT prompts for over-write
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.lpstrTitle = "Output File";
	BOOL bRet = GetSaveFileNameA(&ofn);
	if (bTopmost)
		SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// Return to try again for fail or cancel
	if(!bRet) return "";

	// User file name entry
	return fileName;

}

//--------------------------------------------------------------
void ofApp::SaveImageFile(std::string name)
{
	// Add the extension if none entered
	std::size_t pos = name.rfind('.');
	if (pos == std::string::npos)
		name += ".png";

	// Get pixels from the rgba texture
	pos = name.rfind(".");
	ofImage myimage; // Bit depth 8 bits
	myTexture.readToPixels(myimage.getPixels());
	myimage.save(name); // save png image

}

// ... the end

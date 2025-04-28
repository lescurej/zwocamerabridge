#pragma once

#include "ofMain.h"
#include "ASICamera2.h"
#include "ofxSyphonServer.h"
#include "ofxASICamera.h"
#include "ofxASICameraGui.h"
#include "ofxOSCControl.h"

class ofApp : public ofBaseApp
{

public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;

private:
	ofxASICamera camera;
	ofxSyphonServer syphonServer;
	ofxASICameraGui cameraGui;
	ofxOSCControl oscControl;
};

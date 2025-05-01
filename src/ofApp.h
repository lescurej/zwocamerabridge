#pragma once

#include "ofMain.h"
#include "ASICamera2.h"
#include "ofxASICamera.h"
#include "ofxOSCControl.h"
#include "LogPanel.h"
#include "ofxASICameraManagerGui.h"

class ofApp : public ofBaseApp
{

public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;

private:
	ofxASICameraManagerGui cameraManager;
	LogPanel logPanel;

	ofxOSCControl oscControl;

	ofTexture cameraTexture;
};

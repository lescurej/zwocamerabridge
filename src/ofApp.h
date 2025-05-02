#pragma once

#include "ofMain.h"
#include "ASICamera2.h"
#include "camera/ofxASICamera.h"
#include "osc/ofxOSCControl.h"
#include "log/LogPanel.h"
#include "camera/ofxASICameraManagerGui.h"

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

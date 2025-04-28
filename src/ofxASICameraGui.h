
// ofxASICameraGui.h
#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxASICamera.h"

class ofxASICameraGui
{
public:
    ofxASICameraGui();
    ~ofxASICameraGui();
    void setup(ofxASICamera &camera);
    void draw();
    void update();

private:
    ofxPanel panel;
    std::map<ASI_CONTROL_TYPE, ofParameter<int>> sliders;
    std::map<ASI_CONTROL_TYPE, ofxToggle> autoToggles;
    std::map<ASI_CONTROL_TYPE, std::string> controlNames;

    ofxASICamera *cam = nullptr;
    bool initialized = false;

    void onSliderChanged(int &value);
    void onToggleChanged(bool &value);
    void saveSettings();
    void loadSettings();
};
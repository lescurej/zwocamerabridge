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
    void saveSettings();

private:
    ofxPanel panel;
    std::map<ASI_CONTROL_TYPE, ofParameter<int>> sliders;
    std::map<ASI_CONTROL_TYPE, ofxToggle> autoToggles;
    std::map<ASI_CONTROL_TYPE, std::string> controlNames;

    ofxASICamera *cam = nullptr;
    bool initialized = false;

    void onSliderChanged(int &value);
    void onToggleChanged(bool &value);

    // Ajout pour infos, binning et ROI
    ofParameter<std::string> cameraInfo;
    ofParameter<int> binParam;
    ofParameter<int> roiX, roiY, roiW, roiH;
    void onBinChanged(int &value);
    void onROIChanged(int &);

    // Ajout pour le mode caméra
    ofParameter<int> cameraMode;
    ofxButton softTriggerButton;
    void onModeChanged(int &mode);
    void onSoftTriggerPressed();
    std::map<int, std::string> modeNames;
    void setupModeNames();
};
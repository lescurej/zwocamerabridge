// ofxASICameraGui.h
#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxASICamera.h"
#include "ofxExclusiveToggleGroup.h"

class ofxASICameraGui
{
public:
    void setup(LogPanel *logPanel);
    void connect(int _cameraIndex);
    void disconnect();
    void draw();
    void update();

private:
    LogPanel *logPanel = nullptr;
    void log(ofLogLevel level, const std::string &message)
    {
        if (logPanel)
        {
            logPanel->addLog(message, level);
        }
    }

    void onParamIntChanged(int &value);
    void onParamBoolChanged(bool &value);
    void onAutoParamChanged(bool &value);
    void onResolutionChanged(int &);
    void onImageTypeChanged(int &);
    void onModeChanged(int &mode);
    void onSoftTriggerPressed();
    void onStartCapturePressed();
    void onStopCapturePressed();

    ofxPanel panel;

    std::map<ASI_CONTROL_TYPE, ofParameter<int>> intParams;
    std::map<ASI_CONTROL_TYPE, ofParameter<bool>> boolParams;
    std::map<ASI_CONTROL_TYPE, ofParameter<bool>> autoParams;

    ofxASICamera camera;

    ofxButton connectButton;
    ofxButton disconnectButton;
    ofxToggle autoConnectToggle;

    // Paramètres de configuration
    ofParameter<int> resolutionMaxWidth;
    ofParameter<int> resolutionMaxHeight;
    ofParameter<int> binParam;
    ofxExclusiveToggleGroup imageTypeToggleGroup;
    ofxExclusiveToggleGroup binToggleGroup;
    ofxExclusiveToggleGroup modeToggleGroup;
    ofxButton applySettingsButton;

    // Camera info
    ofxLabel cameraInfo;

    bool isConnected = false;

    // Camera mode
    ofParameter<int> cameraMode;
    ofxButton softTriggerButton;
    std::string settingsFileName;

    ofxButton startCaptureButton;
    ofxButton stopCaptureButton;
};
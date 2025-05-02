#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxASICamera.h"
#include "ofxASICameraGui.h"
#include "ofxExclusiveToggleGroup.h"

class ofxASICameraManagerGui
{
public:
    void setup(LogPanel *logPanel);
    void draw();
    void update();

    void exit();

private:
    void deleteCameraGui();

    LogPanel *logPanel = nullptr;
    void log(ofLogLevel level, const std::string &message)
    {
        if (logPanel)
        {
            logPanel->addLog(message, level);
        }
    }

    void saveSettings();
    ofxPanel panel;

    // Paramètres de connexion
    ofParameter<string> cameraConnectionState;

    ofxButton connectButton;
    ofxButton disconnectButton;

    ofxToggle autoConnectToggle;
    void onConnectPressed();
    void onDisconnectPressed();

    ofxASICameraGui *cameraGui;
    ofxExclusiveToggleGroup cameraToggleGroup;

    ofParameter<float> fps;
};
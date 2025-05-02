#pragma once

#include "ofxOsc.h"
#include "ofxGui.h"
#include "ASICamera2.h"
#include "ofThread.h"
#include "LogPanel.h"
#include "ofxASICameraManagerGui.h"
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <shared_mutex>

#define OFXASI_LOG_CAMERA 1

class ofxOSCControl : public ofThread
{
public:
    ofxOSCControl();
    ~ofxOSCControl();

    void setup(LogPanel *logPanel, ofxASICameraManagerGui *cameraManagerGui);
    void update();
    void draw();

    // Envoi des paramètres OSC
    void sendControlValue(int controlID, float value);

    void exit();

protected:
    void threadedFunction() override;

private:
    void saveSettings();
    ofxASICameraManagerGui *cameraManagerGui;

    ofxOscReceiver receiver;
    ofxOscSender sender;
    std::atomic<int> cameraID{-1};
    int receivePort = 12345;

    int sendPort = 54321;

    std::string receiveHost = "127.0.0.1";

    // GUI pour configurer les ports et l'adresse
    ofxPanel gui;
    ofxIntSlider receivePortSlider;
    ofxIntSlider sendPortSlider;
    ofxInputField<std::string> sendHostInput;

    LogPanel *logPanel;
    mutable std::shared_mutex logMutex;
    void log(ofLogLevel level, const std::string &message) const
    {
        std::shared_lock lock(logMutex);
        if (logPanel)
        {
            logPanel->addLog(message, level);
        }
    }

    void onReceivePortChanged(int &value);
    void onSendPortChanged(int &value);
    void onSendHostInputHostChanged(std::string &value);

    // Flag pour contrôler la réception
    std::atomic<bool> bNewMessage = false;
};

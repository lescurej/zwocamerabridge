#pragma once

#include "ofxOsc.h"
#include "ofxGui.h"
#include "ASICamera2.h"
#include "ofThread.h"
#include <vector>
#include <string>
#include <atomic>
#include <mutex>

class ofxOSCControl : public ofThread
{
public:
    ofxOSCControl();
    ~ofxOSCControl();

    void setup();
    void update();
    void draw();

    // Envoi des paramètres OSC
    void sendControlValue(int controlID, float value);

    // Fonction pour récupérer les paramètres de la caméra
    void fetchCameraControls();

    // Appliquer un contrôle à la caméra
    void setCameraControl(int controlID, float value);

protected:
    void threadedFunction() override;

private:
    ofxOscReceiver receiver;
    ofxOscSender sender;
    std::vector<ASI_CONTROL_CAPS> controls;
    std::mutex controlsMutex;
    int cameraID; // ID de la caméra
    int receivePort;
    int sendPort;
    std::string receiveHost;

    // GUI pour configurer les ports et l'adresse
    ofxPanel gui;
    ofxIntSlider receivePortSlider;
    ofxIntSlider sendPortSlider;
    ofxInputField<std::string> receiveHostInput;

    // Flag pour contrôler la réception
    std::atomic<bool> bNewMessage;
};

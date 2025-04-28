#include "ofxOSCControl.h"

// Constructeur
ofxOSCControl::ofxOSCControl()
{
    receivePort = 12345;
    sendPort = 54321;
    receiveHost = "127.0.0.1";
    bNewMessage = false;
}

ofxOSCControl::~ofxOSCControl()
{
    stopThread();
    receiver.stop();
    sender.clear();
}

// Setup de l'OSC et du GUI
void ofxOSCControl::setup()
{
    gui.setup("OSC controls");
    gui.add(receivePortSlider.setup("Receive Port", 12345, 1024, 65535));
    gui.add(sendPortSlider.setup("Send Port", 54321, 1024, 65535));
    gui.add(receiveHostInput.setup("Receive Host", "127.0.0.1"));

    receiver.setup(receivePort);
    sender.setup(receiveHost, sendPort);

    if (!receiver.isListening())
    {
        ofLogError() << "Receiver not initialized correctly!";
        return;
    }

    fetchCameraControls();
    startThread();
}

// Mise à jour de l'interface graphique
void ofxOSCControl::update()
{
    if (bNewMessage)
    {
        // Traiter ici si nécessaire, vous pouvez aussi stocker les messages reçus pour les utiliser dans l'update.
        bNewMessage = false;
    }
}

// Dessiner l'interface graphique
void ofxOSCControl::draw()
{
    gui.draw();
}

// Envoi des messages OSC pour contrôler la caméra
void ofxOSCControl::sendControlValue(int controlID, float value)
{
    ofxOscMessage m;
    m.setAddress("/camera/control");
    m.addIntArg(controlID);
    m.addFloatArg(value);
    sender.sendMessage(m, false);
}

// Récupérer les paramètres de contrôle de la caméra
void ofxOSCControl::fetchCameraControls()
{
    int numControls = 0;
    ASIGetNumOfControls(cameraID, &numControls);

    std::lock_guard<std::mutex> lock(controlsMutex);
    controls.clear();
    for (int i = 0; i < numControls; ++i)
    {
        ASI_CONTROL_CAPS cap;
        ASIGetControlCaps(cameraID, i, &cap);
        controls.push_back(cap);
    }
}

// Appliquer un contrôle à la caméra
void ofxOSCControl::setCameraControl(int controlID, float value)
{
    std::lock_guard<std::mutex> lock(controlsMutex);
    if (controlID >= 0 && controlID < controls.size())
    {
        ASI_CONTROL_CAPS cap = controls[controlID];
        ASISetControlValue(cameraID, cap.ControlType, value, ASI_TRUE);
    }
}

// Thread d'exécution pour gérer la réception des messages OSC
void ofxOSCControl::threadedFunction()
{
    while (isThreadRunning())
    {
        try
        {
            while (receiver.hasWaitingMessages())
            {
                ofxOscMessage m;
                receiver.getNextMessage(m);
                if (m.getAddress() == "/camera/control")
                {
                    int controlID = m.getArgAsInt(0);
                    float value = m.getArgAsFloat(1);
                    setCameraControl(controlID, value);
                    bNewMessage = true;
                }
            }
        }
        catch (const std::exception &e)
        {
            ofLogError() << "Exception caught in OSC thread: " << e.what();
        }

        ofSleepMillis(10);
    }
}

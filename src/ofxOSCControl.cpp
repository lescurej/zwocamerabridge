#include "ofxOSCControl.h"
#define OSC_SETTINGS_FILE "osc.xml"

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
}

void ofxOSCControl::saveSettings()
{
    gui.saveToFile(OSC_SETTINGS_FILE);
}

// Setup de l'OSC et du GUI
void ofxOSCControl::setup()
{
    gui.setup("OSC controls", OSC_SETTINGS_FILE, 300, 10);
    gui.add(receivePortSlider.setup("Receive Port", 12345, 1024, 65535));
    gui.add(sendPortSlider.setup("Send Port", 54321, 1024, 65535));
    gui.add(receiveHostInput.setup("Receive Host", "127.0.0.1"));
    gui.loadFromFile(OSC_SETTINGS_FILE);

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
    bool expected = true;
    if (std::atomic_compare_exchange_strong(&bNewMessage, &expected, false))
    {
        // Traiter ici si nécessaire
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

                    // Traiter le message de manière thread-safe
                    {
                        std::lock_guard<std::mutex> lock(controlsMutex);
                        if (controlID >= 0 && controlID < controls.size())
                        {
                            ASI_CONTROL_CAPS cap = controls[controlID];
                            ASISetControlValue(cameraID, cap.ControlType, value, ASI_TRUE);
                            std::atomic_store(&bNewMessage, true);
                        }
                    }
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

void ofxOSCControl::exit()
{
    stopThread();
    waitForThread(true, 2000); // Timeout de 2 secondes pour la fermeture propre

    try
    {
        saveSettings();
        receiver.stop();
        sender.clear();

        std::lock_guard<std::mutex> lock(controlsMutex);
        controls.clear();
    }
    catch (const std::exception &e)
    {
        ofLogError() << "Exception during exit: " << e.what();
    }
}
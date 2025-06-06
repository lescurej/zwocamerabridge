#include "ofxOSCControl.h"
#define OSC_SETTINGS_FILE "osc.json"
#include "cameraTools.h"

const std::string OSC_PREFIX = "/camera/astro/";

// Constructeur
ofxOSCControl::ofxOSCControl()
{
    ofLogNotice() << "[OSC] >>> Constructeur ofxOSCControl";
}

ofxOSCControl::~ofxOSCControl()
{
    ofLogNotice() << "[OSC] >>> Destructeur ~ofxOSCControl";
}

void ofxOSCControl::saveSettings()
{
    ofLogNotice() << "[OSC] >>> saveSettings()";
    gui.saveToFile(OSC_SETTINGS_FILE);
    ofLogNotice() << "[OSC] <<< saveSettings()";
}

// Setup de l'OSC et du GUI
void ofxOSCControl::setup(LogPanel *logPanel, ofxASICameraManagerGui *cameraManagerGui)
{
    ofLogNotice() << "[OSC] >>> setup()";
    this->logPanel = logPanel;
    this->cameraManagerGui = cameraManagerGui;
    gui.setup("OSC controls", OSC_SETTINGS_FILE, 250, 10);
    gui.add(receivePortSlider.setup("Receive Port", receivePort, 1024, 65535));
    receivePortSlider.addListener(this, &ofxOSCControl::onReceivePortChanged);
    gui.add(sendPortSlider.setup("Send Port", sendPort, 1024, 65535));
    sendPortSlider.addListener(this, &ofxOSCControl::onSendPortChanged);
    gui.add(sendHostInput.setup("Send Host", receiveHost));
    sendHostInput.addListener(this, &ofxOSCControl::onSendHostInputHostChanged);
    gui.loadFromFile(OSC_SETTINGS_FILE);

    receiver.setup(receivePort);
    sender.setup(receiveHost, sendPort);

    if (!receiver.isListening())
    {
        ofLogError() << "[OSC] Receiver not initialized correctly!";
        return;
    }

    startThread();
    ofLogNotice() << "[OSC] <<< setup()";
}

// Mise à jour de l'interface graphique
void ofxOSCControl::update()
{
    // ofLogNotice() << "[OSC] >>> update()";
    bool expected = true;
    if (std::atomic_compare_exchange_strong(&bNewMessage, &expected, false))
    {
        // Traiter ici si nécessaire
    }
    auto frameNum = ofGetFrameNum();
    if (frameNum % 50 == 0)
    {
        ofxOscMessage m;
        m.setAddress(OSC_PREFIX + "heartbeat");
        m.addIntArg(frameNum);
        sender.sendMessage(m, false);
    }
    // ofLogNotice() << "[OSC] <<< update()";
}

// Dessiner l'interface graphique
void ofxOSCControl::draw()
{
    // ofLogNotice() << "[OSC] >>> draw()";
    gui.draw();

    if (gui.isMinimized())
    {
        // ofLogNotice() << "[OSC] <<< draw() (panel minimized)";
        return; // ne rien dessiner si le panel est caché
    }

    float startX = gui.getPosition().x + 10;
    float startY = gui.getPosition().y + gui.getHeight() + 10;
    float lineHeight = 15;

    ofDrawBitmapString("arg1:value in float", startX, startY);
    ofDrawBitmapString("arg2:auto in bool (optional)", startX, startY + lineHeight);
    for (size_t i = 0; i < sizeof(control_type) / sizeof(control_type[0]); i++)
    {
        ofDrawBitmapString(OSC_PREFIX + control_type[i], startX, startY + (i + 2) * lineHeight);
    }

    ofSetColor(255); // reset color
    // ofLogNotice() << "[OSC] <<< draw()";
}

// Thread d'exécution pour gérer la réception des messages OSC
void ofxOSCControl::threadedFunction()
{
    ofLogNotice() << "[OSC] >>> threadedFunction() (thread démarré)";
    while (isThreadRunning())
    {
        try
        {
            while (receiver.hasWaitingMessages())
            {
                ofxOscMessage m;
                receiver.getNextMessage(m);
                if (m.getAddress().find(OSC_PREFIX) == 0)
                {
                    auto fullAddress = m.getAddress();
                    std::vector<std::string> addressParts;
                    std::stringstream ss(fullAddress);
                    std::string part;
                    while (std::getline(ss, part, '/'))
                    {
                        if (!part.empty())
                        {
                            addressParts.push_back(part);
                        }
                    }
                    if (m.getNumArgs() == 0)
                        continue;

                    std::string controlTypeString = addressParts[2];
                    auto controlType = getControlTypeFromString(controlTypeString);

                    float value = m.getArgAsFloat(0);
                    if (m.getNumArgs() > 1)
                    {
                        bool autoMode = m.getArgAsInt(1) == 1;
                        cameraManagerGui->setControlValue(controlType, value, autoMode);
                    }
                    else
                    {
                        cameraManagerGui->setControlValue(controlType, value);
                    }
                    std::atomic_store(&bNewMessage, true);
                }
            }
        }
        catch (const std::exception &e)
        {
            ofLogError() << "[OSC] Exception caught in OSC thread: " << e.what();
        }

        ofSleepMillis(10);
    }
    ofLogNotice() << "[OSC] <<< threadedFunction() (thread terminé)";
}

void ofxOSCControl::exit()
{
    ofLogNotice() << "[OSC] >>> exit()";
    log(OF_LOG_NOTICE, "[EXIT] ofxOSCControl::exit: Début");
    stopThread();
    log(OF_LOG_NOTICE, "[EXIT] ofxOSCControl::exit: Thread arrêté");
    waitForThread(true, 500);
    cameraManagerGui = nullptr;
    logPanel = nullptr;
    try
    {
        saveSettings();
        receiver.stop();
        sender.clear();
        log(OF_LOG_NOTICE, "[EXIT] ofxOSCControl::exit: OSC stopped and cleared");
    }
    catch (const std::exception &e)
    {
        ofLogError() << "[OSC] Exception during exit: " << e.what();
    }
    log(OF_LOG_NOTICE, "[EXIT] ofxOSCControl::exit: Fin");
    ofLogNotice() << "[OSC] <<< exit()";
}

void ofxOSCControl::onReceivePortChanged(int &value)
{
    receivePort = value;
    receiver.stop();
    receiver.setup(receiveHost, receivePort);
}

void ofxOSCControl::onSendPortChanged(int &value)
{
    sendPort = value;
    sender.setup(receiveHost, sendPort);
}

void ofxOSCControl::onSendHostInputHostChanged(std::string &value)
{
    receiveHost = value;
    sender.setup(receiveHost, sendPort);
}

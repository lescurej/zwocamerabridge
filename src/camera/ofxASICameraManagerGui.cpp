#include "ofxASICameraManagerGui.h"
#include "cameraTools.h"
#define CAMERA_SETTINGS_FILE "camera_manager.json"

void ofxASICameraManagerGui::setup(LogPanel *_logPanel)
{
    ofLogNotice() << "[MANAGER] >>> setup()";
    this->logPanel = _logPanel;

    panel.setup("ASI Camera Connection", CAMERA_SETTINGS_FILE, 10, 10);

    // Paramètres de connexion
    auto numCamera = getNumConnectedCameras();
    std::vector<std::string> cameraLabels;
    for (int i = 0; i < numCamera; i++)
    {
        cameraLabels.push_back("Camera " + ofToString(i + 1));
    }
    cameraToggleGroup.setup("Available camera number", cameraLabels, 0);

    panel.add(&cameraToggleGroup);

    connectButton.setup("Connect");
    disconnectButton.setup("Disconnect");

    autoConnectToggle.setup("Auto Connect", false);
    connectButton.addListener(this, &ofxASICameraManagerGui::onConnectPressed);
    disconnectButton.addListener(this, &ofxASICameraManagerGui::onDisconnectPressed);

    panel.add(&connectButton);
    panel.add(&disconnectButton);

    panel.add(&autoConnectToggle);
    panel.add(cameraConnectionState);
    cameraConnectionState = "disconnected";

    fps.set("FPS", 0, 0, 240); // Nom, valeur initiale, min, max
    panel.add(fps);

    panel.loadFromFile(CAMERA_SETTINGS_FILE);
    auto isAutoConnect = autoConnectToggle ? "1" : "0";
    log(OF_LOG_NOTICE, "autoConnect: " + ofToString(isAutoConnect));
    if (autoConnectToggle && numCamera > 0)
    {
        onConnectPressed();
    }
    ofLogNotice() << "[MANAGER] <<< setup()";
}

void ofxASICameraManagerGui::onConnectPressed()
{
    ofLogNotice() << "[MANAGER] >>> onConnectPressed()";
    if (getNumConnectedCameras() == 0)
        return;
    log(OF_LOG_NOTICE, "connect camera: " + ofToString(cameraToggleGroup.getSelectedIndex()));
    onDisconnectPressed();
    cameraGui = new ofxASICameraGui();
    cameraGui->setup(logPanel);
    cameraGui->connect(cameraToggleGroup.getSelectedIndex());
    log(OF_LOG_NOTICE, "camera connected: " + ofToString(cameraToggleGroup.getSelectedIndex()));
    cameraConnectionState = "connected";
    ofLogNotice() << "[MANAGER] <<< onConnectPressed()";
}

void ofxASICameraManagerGui::onDisconnectPressed()
{
    ofLogNotice() << "[MANAGER] >>> onDisconnectPressed()";
    log(OF_LOG_NOTICE, "[EXIT] ofxASICameraManagerGui::onDisconnectPressed: Début");
    cameraConnectionState = "disconnected";
    if (cameraGui)
    {
        log(OF_LOG_NOTICE, "[EXIT] ofxASICameraManagerGui::onDisconnectPressed: cameraGui->disconnect()");
        cameraGui->disconnect();
        log(OF_LOG_NOTICE, "[EXIT] ofxASICameraManagerGui::onDisconnectPressed: delete cameraGui");
        delete cameraGui;
        cameraGui = nullptr;
    }
    log(OF_LOG_NOTICE, "[EXIT] ofxASICameraManagerGui::onDisconnectPressed: Fin");
    ofLogNotice() << "[MANAGER] <<< onDisconnectPressed()";
}

void ofxASICameraManagerGui::update()
{
    // ofLogNotice() << "[MANAGER] >>> update()";
    if (cameraGui)
    {
        cameraGui->update();
    }
    fps = ofGetFrameRate();
    // ofLogNotice() << "[MANAGER] <<< update()";
}

void ofxASICameraManagerGui::draw()
{
    // ofLogNotice() << "[MANAGER] >>> draw()";
    if (cameraGui)
    {
        cameraGui->draw();
    }
    panel.draw();
    // ofLogNotice() << "[MANAGER] <<< draw()";
}

void ofxASICameraManagerGui::saveSettings()
{
    ofLogNotice() << "[MANAGER] >>> saveSettings()";
    panel.saveToFile(CAMERA_SETTINGS_FILE);
    ofLogNotice() << "[MANAGER] <<< saveSettings()";
}

void ofxASICameraManagerGui::exit()
{
    ofLogNotice() << "[MANAGER] >>> exit()";
    log(OF_LOG_NOTICE, "[EXIT] ofxASICameraManagerGui::exit: Début");
    saveSettings();
    onDisconnectPressed();
    log(OF_LOG_NOTICE, "[EXIT] ofxASICameraManagerGui::exit: Fin");
    ofLogNotice() << "[MANAGER] <<< exit()";
}

void ofxASICameraManagerGui::setControlValue(ASI_CONTROL_TYPE type, float value, bool autoMode)
{
    ofLogNotice() << "[MANAGER] setControlValue(type=" << type << ", value=" << value << ", autoMode=" << autoMode << ")";
    log(OF_LOG_NOTICE, "setControlValue: " + ofToString(type) + " " + ofToString(value) + " " + ofToString(autoMode));
    if (cameraGui)
    {
        cameraGui->setControlValue(type, value, autoMode);
    }
}

void ofxASICameraManagerGui::setControlValue(ASI_CONTROL_TYPE type, float value)
{
    ofLogNotice() << "[MANAGER] setControlValue(type=" << type << ", value=" << value << ")";
    log(OF_LOG_NOTICE, "setControlValue: " + ofToString(type) + " " + ofToString(value));
    if (cameraGui)
    {
        cameraGui->setControlValue(type, value);
    }
}

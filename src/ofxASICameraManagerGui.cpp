#include "ofxASICameraManagerGui.h"
#include "cameraTools.h"
#define CAMERA_SETTINGS_FILE "camera_manager.xml"

void ofxASICameraManagerGui::setup(LogPanel *_logPanel)
{
    this->logPanel = _logPanel;

    panel.setup("ASI Camera Controls", CAMERA_SETTINGS_FILE, 10, 10);

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
}

void ofxASICameraManagerGui::onConnectPressed()
{
    if (getNumConnectedCameras() == 0)
        return;
    log(OF_LOG_NOTICE, "connect camera: " + ofToString(cameraToggleGroup.getSelectedIndex()));
    onDisconnectPressed();
    cameraGui = new ofxASICameraGui();
    cameraGui->setup(logPanel);
    cameraGui->connect(cameraToggleGroup.getSelectedIndex());
    log(OF_LOG_NOTICE, "camera connected: " + ofToString(cameraToggleGroup.getSelectedIndex()));
    cameraConnectionState = "connected";
}

void ofxASICameraManagerGui::onDisconnectPressed()
{
    log(OF_LOG_NOTICE, "disconnect camera");
    cameraConnectionState = "disconnected";
    if (cameraGui)
    {
        cameraGui->disconnect();
        delete cameraGui;
        cameraGui = nullptr;
    }
}

void ofxASICameraManagerGui::update()
{
    if (cameraGui)
    {
        cameraGui->update();
    }
    fps = ofGetFrameRate();
}

void ofxASICameraManagerGui::draw()
{
    if (cameraGui)
    {
        cameraGui->draw();
    }
    panel.draw();
}

void ofxASICameraManagerGui::saveSettings()
{
    panel.saveToFile(CAMERA_SETTINGS_FILE);
}

void ofxASICameraManagerGui::exit()
{
    saveSettings();
    onDisconnectPressed();
}

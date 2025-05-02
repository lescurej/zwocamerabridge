#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSetWindowTitle("ASI Camera");
    ofSetVerticalSync(true);
    ofSetFrameRate(60);

    // Initialiser le panneau de logs
    logPanel.setup("Log", 200);
    logPanel.addLog("App started", OF_LOG_NOTICE);
    cameraManager.setup(&logPanel);
    oscControl.setup(&logPanel, &cameraManager);

    // Crée un chemin absolu vers le dossier à partir de data/
    std::string fullPath = ofToDataPath(".", true);
    // Utilise ofDirectory pour vérifier et créer si besoin
    ofDirectory dir(fullPath);
    if (!dir.exists())
    {
        dir.create(true); // true = création récursive si besoin
        ofLogNotice() << "Folder '" << fullPath << "' created";
    }
    else
    {
        ofLogNotice() << "Folder '" << fullPath << "' already exists";
    }
}

//--------------------------------------------------------------
void ofApp::update()
{
    cameraManager.update();
    oscControl.update();
}

//--------------------------------------------------------------
void ofApp::draw()
{
    ofBackground(40);
    // Dessiner le panneau de logs
    cameraManager.draw();
    oscControl.draw();
    logPanel.draw();
}

//--------------------------------------------------------------
void ofApp::exit()
{

    cameraManager.exit();
    oscControl.exit();
    logPanel.addLog("Camera disconnected", OF_LOG_NOTICE);
}
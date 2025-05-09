#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
    ofLogNotice() << "[APP] >>> setup()";
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
        ofLogNotice() << "[APP] Folder '" << fullPath << "' created";
    }
    else
    {
        ofLogNotice() << "[APP] Folder '" << fullPath << "' already exists";
    }
    ofLogNotice() << "[APP] <<< setup()";
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
    ofLogNotice() << "[APP] >>> exit()";
    logPanel.addLog("[EXIT] App: Début de la fermeture", OF_LOG_NOTICE);
    logPanel.addLog("[EXIT] App: cameraManager.exit()", OF_LOG_NOTICE);
    cameraManager.exit();
    logPanel.addLog("[EXIT] App: oscControl.exit()", OF_LOG_NOTICE);
    oscControl.exit();
    logPanel.addLog("[EXIT] App: Fin de la fermeture", OF_LOG_NOTICE);
    logPanel.addLog("Camera disconnected", OF_LOG_NOTICE);
    ofLogNotice() << "[APP] <<< exit()";
}
#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
    syphonServer.setName("ZWOCameraBridge");
    cameraGui.setup(camera);
    oscControl.setup();
}

//--------------------------------------------------------------
void ofApp::update()
{
    cameraGui.update();
    oscControl.update();
    if (!camera.isConnected())
    {
        camera.setup(0, 1920, 1080, ASI_IMG_RAW8, &syphonServer);
    }
}

//--------------------------------------------------------------
void ofApp::draw()
{
    oscControl.draw();
    cameraGui.draw();
    if (camera.isConnected())
    {
        camera.draw(0, 0);
    }
    else
    {
        ofDrawBitmapString("Camera not connected", 10, 10);
    }
}

//--------------------------------------------------------------
void ofApp::exit()
{
}

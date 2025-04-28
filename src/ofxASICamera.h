#pragma once

#include "ofMain.h"
#include "ASICamera2.h"
#include "ofxSyphonServer.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

class ofxASICamera
{
public:
    ofxASICamera();
    ~ofxASICamera();

    bool setup(int cameraIndex = 0, int width = 640, int height = 480, ASI_IMG_TYPE imgType = ASI_IMG_RAW8, ofxSyphonServer *syphonServer = nullptr);
    void close();

    void draw(float x, float y);
    bool isConnected() const { return connected; }
    bool isFrameNew() const;

    std::string getCameraName() const;

    // Get/set camera parameters
    int getControlValue(ASI_CONTROL_TYPE type, bool *autoMode = nullptr);
    void setControlValue(ASI_CONTROL_TYPE type, int value, bool autoMode = false);
    std::vector<ASI_CONTROL_CAPS> getAllControls() const;

    // Callback sur nouvelle image
    std::function<void()> onNewFrame;

    int getCameraID() const { return cameraID; }
    ASI_CAMERA_INFO getCameraInfo() const;
    std::vector<int> getSupportedBins() const;
    void setBinning(int bin);
    int getBinning() const;
    void setROI(int x, int y, int w, int h);

    // Méthodes pour le mode caméra
    bool isTriggerCamera() const;
    std::vector<ASI_CAMERA_MODE> getSupportedModes() const;
    ASI_CAMERA_MODE getCurrentMode() const;
    bool setMode(ASI_CAMERA_MODE mode);
    bool sendSoftTrigger(bool start = true);

private:
    int cameraID = -1;
    bool connected = false;

    int width = 0, height = 0;
    ASI_IMG_TYPE imgType = ASI_IMG_RAW8;

    std::vector<unsigned char> pixelBuffer;
    ofTexture texture;

    std::vector<ASI_CONTROL_CAPS> controls;
    void fetchControlCaps();

    // Thread de capture
    std::thread captureThread;
    std::atomic<bool> bCaptureRunning{false};
    std::atomic<bool> bFrameNew{false};
    std::mutex bufferMutex;

    void captureLoop();
    void startCaptureThread();
    void stopCaptureThread();

    ofxSyphonServer *syphonServer = nullptr;

    int currentBin = 1;
    ASI_CAMERA_MODE currentMode = ASI_MODE_NORMAL;
};

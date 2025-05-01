#pragma once

#include "ofMain.h"
#include "ASICamera2.h"
#include "ofxSyphonServer.h"
#include "LogPanel.h"
#include <thread>
#include <atomic>
#include <shared_mutex>
#include <memory>
#include <optional>
#include <future>

class ofxASICamera
{
public:
    ofxASICamera();
    ~ofxASICamera();

    // Empêcher la copie et le déplacement
    ofxASICamera(const ofxASICamera &) = delete;
    ofxASICamera &operator=(const ofxASICamera &) = delete;
    ofxASICamera(ofxASICamera &&) = delete;
    ofxASICamera &operator=(ofxASICamera &&) = delete;

    void setup(LogPanel *logPanel);
    std::optional<ASI_CAMERA_INFO> connect(int index);
    void close();
    void update();

    void draw(float x, float y);

    // Getters thread-safe
    bool isConnected() const noexcept { return connected.load(std::memory_order_acquire); }
    bool isCaptureRunning() const noexcept { return bCaptureRunning.load(std::memory_order_acquire); }

    struct Dimensions
    {
        int width;
        int height;
    };

    Dimensions getDimensions() const noexcept
    {
        std::shared_lock lock(dimensionMutex);
        return {width, height};
    }

    // Camera control methods
    float getControlValue(ASI_CONTROL_TYPE type, bool *autoMode = nullptr);
    void setControlValue(ASI_CONTROL_TYPE type, float value, bool autoMode = false);
    std::vector<ASI_CONTROL_CAPS> getAllControls();

    int getCameraID() const noexcept
    {
        std::shared_lock lock(infoMutex);
        return info.CameraID;
    }

    int getBinning() const noexcept
    {
        std::shared_lock lock(configMutex);
        return currentBin;
    }

    // Camera mode methods
    bool isTriggerCamera();
    std::vector<ASI_CAMERA_MODE> getSupportedModes();
    ASI_CAMERA_MODE getCurrentMode();
    bool setMode(ASI_CAMERA_MODE mode);
    bool sendSoftTrigger(bool start = true);

    std::future<void> startCaptureThread(ASI_IMG_TYPE type, int bin);
    void stopCaptureThread();

private:
    std::vector<ASI_CONTROL_CAPS> getCameraControlCaps();

    mutable std::shared_mutex infoMutex;
    ASI_CAMERA_INFO info{};

    struct CameraFrame
    {
        std::vector<unsigned char> data;
        bool ready{false};
    };

    std::unique_ptr<ofxSyphonServer> syphonServer;
    std::atomic<bool> connected{false};

    mutable std::shared_mutex dimensionMutex;
    int width{0}, height{0};

    mutable std::shared_mutex imgTypeMutex;
    ASI_IMG_TYPE imgType{ASI_IMG_RAW8};

    // Syphon buffer
    mutable std::shared_mutex pixelsMutex;
    std::unique_ptr<ofTexture> syphonTexture;

    mutable std::shared_mutex controlsMutex;
    std::vector<ASI_CONTROL_CAPS> controls;

    // Capture thread
    std::unique_ptr<std::thread> captureThread;
    std::atomic<bool> bCaptureRunning{false};
    std::promise<void> captureStartPromise;

    void captureLoop();

    mutable std::shared_mutex configMutex;
    int currentBin{1};
    ASI_CAMERA_MODE currentMode{ASI_MODE_NORMAL};
    std::atomic<long> cachedExposure{10000}; // Default in µs

    LogPanel *logPanel;
    mutable std::shared_mutex logMutex;
    void log(ofLogLevel level, const std::string &message) const;

    std::atomic<bool> textureUpdateNeeded{false};

    ofPixels latestPixels;
    std::atomic<bool> newFrameAvailable = false;

    // Helpers
    void setupTextures();

    // Thread-safe resource management
    class ResourceGuard
    {
        std::atomic<bool> &flag;

    public:
        explicit ResourceGuard(std::atomic<bool> &f) : flag(f) { flag.store(true, std::memory_order_release); }
        ~ResourceGuard() { flag.store(false, std::memory_order_release); }
    };
};

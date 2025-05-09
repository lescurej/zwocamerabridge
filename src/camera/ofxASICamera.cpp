#ifndef OFXASI_LOG_CAMERA
#define OFXASI_LOG_CAMERA 1
#endif

#include "ofxASICamera.h"
#include <string>
#include "cameraTools.h"

ofxASICamera::ofxASICamera()
{
    log(OF_LOG_NOTICE, "[CAMERA] >>> Constructeur ofxASICamera");
}

ofxASICamera::~ofxASICamera()
{
    log(OF_LOG_NOTICE, "[CAMERA] >>> Destructeur ~ofxASICamera");
    log(OF_LOG_NOTICE, "[EXIT] ~ofxASICamera: Début destruction");
    close();
    log(OF_LOG_NOTICE, "[EXIT] ~ofxASICamera: Fin destruction");
    log(OF_LOG_NOTICE, "[CAMERA] <<< Destructeur ~ofxASICamera");
}

void ofxASICamera::setup(LogPanel *logPanel)
{
    log(OF_LOG_NOTICE, "[CAMERA] setup() called");
    this->logPanel = logPanel;
}

std::optional<ASI_CAMERA_INFO> ofxASICamera::connect(int index)
{
    log(OF_LOG_NOTICE, "[CAMERA] >>> connect(" + ofToString(index) + ")");
    int numCams = getNumConnectedCameras();
    log(OF_LOG_NOTICE, "[CAMERA] getNumConnectedCameras()=" + ofToString(numCams));
    if (numCams <= index)
    {
        log(OF_LOG_ERROR, "[CAMERA] No camera at index " + ofToString(index));
        return std::nullopt;
    }
    ASI_CAMERA_INFO tempInfo;
    log(OF_LOG_NOTICE, "[CAMERA] Appel ASIGetCameraProperty");
    ASI_ERROR_CODE err = ASIGetCameraProperty(&tempInfo, index);
    log(OF_LOG_NOTICE, "[CAMERA] Retour ASIGetCameraProperty: " + ofToString(err));
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "[CAMERA] Failed to get camera property : " + decodeASIErrorCode(err));
        return std::nullopt;
    }
    log(OF_LOG_NOTICE, "[CAMERA] Appel ASIOpenCamera");
    err = ASIOpenCamera(index);
    log(OF_LOG_NOTICE, "[CAMERA] Retour ASIOpenCamera: " + ofToString(err));
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "[CAMERA] ASIOpenCamera failed: " + decodeASIErrorCode(err));
        return std::nullopt;
    }
    log(OF_LOG_NOTICE, "[CAMERA] Appel ASIInitCamera");
    err = ASIInitCamera(index);
    log(OF_LOG_NOTICE, "[CAMERA] Retour ASIInitCamera: " + ofToString(err));
    if (err != ASI_SUCCESS)
    {
        ASICloseCamera(index);
        log(OF_LOG_ERROR, "[CAMERA] ASIInitCamera failed: " + decodeASIErrorCode(err));
        return std::nullopt;
    }
    {
        std::unique_lock lock(infoMutex);
        info = tempInfo;
    }
    connected.store(true, std::memory_order_release);
    log(OF_LOG_NOTICE, "[CAMERA] Camera ID " + ofToString(tempInfo.CameraID) + " (" + std::string(tempInfo.Name) + "), " + ofToString(tempInfo.MaxWidth) + "x" + ofToString(tempInfo.MaxHeight));
    controls = getCameraControlCaps();
    log(OF_LOG_NOTICE, "[CAMERA] <<< connect(" + ofToString(index) + ")");
    return tempInfo;
}

std::future<void> ofxASICamera::startCaptureThread(int _width, int _height, ASI_IMG_TYPE type, int bin)
{
    log(OF_LOG_NOTICE, "[CAMERA] >>> startCaptureThread width=" + ofToString(_width) + ", height=" + ofToString(_height) + ", type=" + ofToString(type) + ", bin=" + ofToString(bin));
    if (!isConnected())
    {
        log(OF_LOG_ERROR, "[CAMERA] startCaptureThread() : camera not connected.");
        return std::future<void>();
    }
    ASI_CAMERA_INFO currentInfo;
    {
        std::shared_lock lock(infoMutex);
        currentInfo = info;
    }
    {
        std::unique_lock dimLock(dimensionMutex);
        log(OF_LOG_NOTICE, "[CAMERA] Bin " + ofToString(bin));
        width = _width;
        height = _height;
        log(OF_LOG_NOTICE, "[CAMERA] Setting ROI to " + ofToString(width) + "x" + ofToString(height));
    }
    {
        std::unique_lock typeLock(imgTypeMutex);
        imgType = type;
    }
    log(OF_LOG_NOTICE, "[CAMERA] Appel ASISetROIFormat");
    auto err = ASISetROIFormat(currentInfo.CameraID, width, height, bin, type);
    log(OF_LOG_NOTICE, "[CAMERA] Retour ASISetROIFormat: " + ofToString(err));
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "[CAMERA] Failed to set ROI format : " + decodeASIErrorCode(err));
        return std::future<void>();
    }
    setupTextures();
    log(OF_LOG_NOTICE, "[CAMERA] Appel ASIStartVideoCapture");
    err = ASIStartVideoCapture(currentInfo.CameraID);
    log(OF_LOG_NOTICE, "[CAMERA] Retour ASIStartVideoCapture: " + ofToString(err));
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "[CAMERA] Failed to start video capture : " + decodeASIErrorCode(err));
        return std::future<void>();
    }
    captureStartPromise = std::promise<void>();
    auto future = captureStartPromise.get_future();
    log(OF_LOG_NOTICE, "[CAMERA] Lancement du thread de capture...");
    captureThread = std::make_unique<std::thread>(&ofxASICamera::captureLoop, this);
    bCaptureRunning = true;
    log(OF_LOG_NOTICE, "[CAMERA] <<< startCaptureThread");
    return future;
}

void ofxASICamera::captureLoop()
{
    log(OF_LOG_NOTICE, "[CAMERA] >>> captureLoop (thread de capture démarré)");
    auto lastTime = std::chrono::steady_clock::now();
    int frameCount = 0;
    float fps = 0.0f;
    ResourceGuard guard(bCaptureRunning);
    const auto dimensions = getDimensions();
    ASI_IMG_TYPE localImgType;
    {
        std::shared_lock typeLock(imgTypeMutex);
        localImgType = imgType;
    }
    size_t bufferSize = dimensions.width * dimensions.height * bytesPerPixel(localImgType);
    auto buffer = std::make_unique<unsigned char[]>(bufferSize);
    log(OF_LOG_NOTICE, "[CAMERA] captureLoop: bufferSize=" + ofToString(bufferSize));
    captureStartPromise.set_value();
    while (bCaptureRunning)
    {
        if (!isConnected())
        {
            log(OF_LOG_NOTICE, "[CAMERA] captureLoop: not connected, sleep 100ms");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        int waitMs = static_cast<int>(cachedExposure.load(std::memory_order_relaxed) * 2 / 1000 + 500);
        // log(OF_LOG_NOTICE, "[CAMERA] captureLoop: Appel ASIGetVideoData (waitMs=" + ofToString(waitMs) + ")");
        ASI_ERROR_CODE err = ASIGetVideoData(getCameraID(), buffer.get(), bufferSize, waitMs);
        // log(OF_LOG_NOTICE, "[CAMERA] captureLoop: Retour ASIGetVideoData: " + ofToString(err));
        if (err == ASI_SUCCESS)
        {
            auto processedBuffer = std::make_unique<unsigned char[]>(bufferSize);
            std::memcpy(processedBuffer.get(), buffer.get(), bufferSize);
            if (localImgType == ASI_IMG_RAW16)
            {
                auto buffer8 = std::make_unique<unsigned char[]>(dimensions.width * dimensions.height);
                for (size_t i = 0; i < bufferSize; i += 2)
                {
                    buffer8[i / 2] = processedBuffer[i + 1];
                }
                processedBuffer = std::move(buffer8);
            }
            {
                std::unique_lock displayLock(pixelsMutex);
                latestPixels.setFromPixels(processedBuffer.get(), dimensions.width, dimensions.height, imageTypeToNumChannels(localImgType));
                newFrameAvailable = true;
            }
            frameCount++;
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> elapsed = now - lastTime;
            if (elapsed.count() >= 1.0f)
            {
                fps = frameCount / elapsed.count();
                frameCount = 0;
                lastTime = now;
                currentFPS.store(fps, std::memory_order_relaxed);
            }
        }
        else
        {
            log(OF_LOG_ERROR, "[CAMERA] captureLoop: Failed to get video data: " + decodeASIErrorCode(err));
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    log(OF_LOG_NOTICE, "[CAMERA] <<< captureLoop (thread de capture terminé)");
}

void ofxASICamera::setupTextures()
{
    log(OF_LOG_NOTICE, "[CAMERA] setupTextures() called");
    syphonServer.reset();
    if (syphonTexture)
    {
        syphonTexture->clear();
    }
    syphonTexture.reset();
    syphonTexture = std::make_unique<ofTexture>();
    syphonServer = std::make_unique<ofxSyphonServer>();
    log(OF_LOG_NOTICE, "[CAMERA] Allocating texture for " + ofToString(width) + "x" + ofToString(height));
    syphonTexture->allocate(width, height, getGLInternalFormat(imgType));
    syphonServer->setName("ASI Camera:" + ofToString(info.CameraID));
}

void ofxASICamera::stopCaptureThread()
{
    log(OF_LOG_NOTICE, "[CAMERA] >>> stopCaptureThread");
    log(OF_LOG_NOTICE, "[EXIT] stopCaptureThread: Début");
    if (!isCaptureRunning())
    {
        log(OF_LOG_NOTICE, "[EXIT] stopCaptureThread: Pas de capture en cours");
        log(OF_LOG_NOTICE, "[CAMERA] <<< stopCaptureThread (pas de capture)");
        return;
    }
    bCaptureRunning.store(false, std::memory_order_release);
    if (captureThread && captureThread->joinable())
    {
        try
        {
            log(OF_LOG_NOTICE, "[EXIT] stopCaptureThread: join captureThread...");
            captureThread->join();
            log(OF_LOG_NOTICE, "[EXIT] stopCaptureThread: captureThread joinée");
        }
        catch (const std::exception &e)
        {
            log(OF_LOG_ERROR, std::string("[EXIT] Error stopping capture thread: ") + e.what());
        }
    }
    {
        if (syphonTexture)
        {
            log(OF_LOG_NOTICE, "[EXIT] stopCaptureThread: Resetting syphon texture");
            syphonTexture->clear();
            syphonTexture.reset();
        }
        if (syphonServer)
        {
            log(OF_LOG_NOTICE, "[EXIT] stopCaptureThread: Resetting syphon server");
            syphonServer.reset();
        }
    }
    {
        std::unique_lock dimLock(dimensionMutex);
        width = height = 0;
    }
    log(OF_LOG_NOTICE, "[CAMERA] Appel ASIStopVideoCapture");
    ASI_ERROR_CODE err = ASIStopVideoCapture(getCameraID());
    log(OF_LOG_NOTICE, "[CAMERA] Retour ASIStopVideoCapture: " + ofToString(err));
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_WARNING, std::string("[EXIT] Error stopping video capture: ") + decodeASIErrorCode(err));
    }
    captureThread.reset();
    log(OF_LOG_NOTICE, "[EXIT] stopCaptureThread: Fin");
    log(OF_LOG_NOTICE, "[CAMERA] <<< stopCaptureThread");
}

void ofxASICamera::close()
{
    log(OF_LOG_NOTICE, "[CAMERA] >>> close()");
    log(OF_LOG_NOTICE, "[EXIT] close: Début fermeture caméra...");
    stopCaptureThread();
    if (isConnected())
    {
        log(OF_LOG_NOTICE, "[EXIT] close: Appel à ASICloseCamera");
        auto err = ASICloseCamera(getCameraID());
        log(OF_LOG_NOTICE, "[CAMERA] Retour ASICloseCamera: " + ofToString(err));
        if (err != ASI_SUCCESS)
        {
            log(OF_LOG_WARNING, std::string("[EXIT] Error closing camera: ") + decodeASIErrorCode(err));
        }
    }
    {
        std::unique_lock controlLock(controlsMutex);
        controls.clear();
    }
    {
        std::unique_lock infoLock(infoMutex);
        info = ASI_CAMERA_INFO{};
    }
    connected.store(false, std::memory_order_release);
    log(OF_LOG_NOTICE, "[EXIT] close: Camera disconnected");
    log(OF_LOG_NOTICE, "[EXIT] close: Fin fermeture caméra");
    log(OF_LOG_NOTICE, "[CAMERA] <<< close()");
}

std::__1::vector<ASI_CONTROL_CAPS> ofxASICamera::getCameraControlCaps()
{
    log(OF_LOG_NOTICE, "[CAMERA] getCameraControlCaps() called");
    if (!connected)
    {
        log(OF_LOG_ERROR, "[CAMERA] getCameraControlCaps() : camera not connected.");
        return controls;
    }
    int numControls = 0;
    log(OF_LOG_NOTICE, "[CAMERA] Appel ASIGetNumOfControls");
    auto err = ASIGetNumOfControls(info.CameraID, &numControls);
    log(OF_LOG_NOTICE, "[CAMERA] Retour ASIGetNumOfControls: " + ofToString(err) + ", numControls=" + ofToString(numControls));
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "[CAMERA] Failed to get number of controls : " + decodeASIErrorCode(err));
        return controls;
    }
    controls.clear();
    for (int i = 0; i < numControls; ++i)
    {
        ASI_CONTROL_CAPS cap;
        ASIGetControlCaps(info.CameraID, i, &cap);
        log(OF_LOG_NOTICE, "[CAMERA] Control " + ofToString(i) + ": " + std::string(cap.Name) + " [" + ofToString(cap.MinValue) + " - " + ofToString(cap.MaxValue) + "], default: " + ofToString(cap.DefaultValue));
        controls.push_back(cap);
    }
    log(OF_LOG_NOTICE, "[CAMERA] getCameraControlCaps() done");
    return controls;
}

void ofxASICamera::update()
{
    // log(OF_LOG_NOTICE, "[CAMERA] update() called");
    if (!connected)
        return;
    if (newFrameAvailable.exchange(false))
    {
        if (!syphonTexture || !syphonServer)
        {
            log(OF_LOG_ERROR, "[CAMERA] syphonTexture or syphonServer is not initialized");
            return;
        }
        {
            std::lock_guard lock(pixelsMutex);
            syphonTexture->loadData(latestPixels);
        }
        syphonServer->publishTexture(syphonTexture.get());
    }
}

void ofxASICamera::draw(float x, float y)
{
    // log(OF_LOG_NOTICE, "[CAMERA] draw() called");
    if (!connected || syphonTexture == nullptr || !syphonTexture->isAllocated())
        return;
    auto dimensions = getDimensions();
    float drawWidth = dimensions.width;
    float drawHeight = dimensions.height;
    float maxWidth = ofGetWidth() * 0.9;
    float maxHeight = ofGetHeight() * 0.9;
    if (drawWidth > maxWidth || drawHeight > maxHeight)
    {
        float scale = std::min(maxWidth / drawWidth, maxHeight / drawHeight);
        drawWidth *= scale;
        drawHeight *= scale;
    }
    ofPushStyle();
    ofSetColor(255);
    syphonTexture->draw(x, y, drawWidth, drawHeight);
    ofPopStyle();
}

float ofxASICamera::getControlValue(ASI_CONTROL_TYPE type, bool *autoMode)
{
    // log(OF_LOG_NOTICE, "[CAMERA] getControlValue(" + ofToString(type) + ") called");
    if (!connected)
    {
        log(OF_LOG_ERROR, "[CAMERA] getControlValue() : camera not connected.");
        return false;
    }
    long val = 0;
    ASI_BOOL bAuto;
    // log(OF_LOG_NOTICE, "[CAMERA] Appel ASIGetControlValue");
    auto err = ASIGetControlValue(info.CameraID, type, &val, &bAuto);
    // log(OF_LOG_NOTICE, "[CAMERA] Retour ASIGetControlValue: " + ofToString(err));
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "[CAMERA] Failed to get control value: " + decodeASIErrorCode(err));
        return -1;
    }
    if (autoMode)
        *autoMode = (bAuto == ASI_TRUE);
    return (float)val;
}

void ofxASICamera::setControlValue(ASI_CONTROL_TYPE type, float value, bool autoMode)
{
    // log(OF_LOG_NOTICE, "[CAMERA] setControlValue(" + ofToString(type) + ", value=" + ofToString(value) + ", auto=" + ofToString(autoMode) + ") called");
    if (!connected)
    {
        log(OF_LOG_ERROR, "[CAMERA] setControlValue() : camera not connected.");
        return;
    }
    if (type == ASI_COOLER_POWER_PERC)
    {
        log(OF_LOG_NOTICE, "[CAMERA] setControlValue: ignore ASI_COOLER_POWER_PERC");
        return;
    }
    // log(OF_LOG_NOTICE, "[CAMERA] Appel ASISetControlValue");
    auto err = ASISetControlValue(info.CameraID, type, (long)value, autoMode ? ASI_TRUE : ASI_FALSE);
    // log(OF_LOG_NOTICE, "[CAMERA] Retour ASISetControlValue: " + ofToString(err));
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "[CAMERA] Failed to set control value: " + getControlType(type) + " to " + ofToString(value) + " : " + decodeASIErrorCode(err));
        return;
    }
    if (type == ASI_EXPOSURE)
    {
        cachedExposure = value;
    }
}

void ofxASICamera::setControlValue(ASI_CONTROL_TYPE type, float value)
{
    // log(OF_LOG_NOTICE, "[CAMERA] setControlValue(" + ofToString(type) + ", value=" + ofToString(value) + ") called");
    bool autoMode = false;
    getControlValue(type, &autoMode);
    setControlValue(type, value, autoMode);
}

std::vector<ASI_CONTROL_CAPS> ofxASICamera::getAllControls()
{
    if (!connected)
    {
        log(OF_LOG_ERROR, "getAllControls() : camera not connected.");
        return {};
    }
    std::lock_guard<std::shared_mutex> lock(controlsMutex);
    return controls;
}

bool ofxASICamera::isTriggerCamera()
{
    if (!connected)
    {
        log(OF_LOG_ERROR, "isTriggerCamera() : camera not connected.");
        return false;
    }
    return info.IsTriggerCam;
}

std::vector<ASI_CAMERA_MODE> ofxASICamera::getSupportedModes()
{
    if (!connected)
    {
        log(OF_LOG_ERROR, "getSupportedModes() : camera not connected.");
        return {};
    }
    std::vector<ASI_CAMERA_MODE> modes;
    if (!isTriggerCamera())
    {
        std::vector<ASI_CAMERA_MODE> modes = {ASI_MODE_NORMAL};
        return modes;
    }

    ASI_SUPPORTED_MODE supportedMode;
    if (ASIGetCameraSupportMode(info.CameraID, &supportedMode) == ASI_SUCCESS)
    {
        for (int i = 0; i < 16 && supportedMode.SupportedCameraMode[i] != ASI_MODE_END; ++i)
        {
            modes.push_back(supportedMode.SupportedCameraMode[i]);
        }
    }
    return modes;
}

ASI_CAMERA_MODE ofxASICamera::getCurrentMode()
{
    if (!isTriggerCamera())
        return ASI_MODE_NORMAL;

    ASI_CAMERA_MODE mode;
    auto err = ASIGetCameraMode(info.CameraID, &mode);
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "Failed to get camera mode: " + decodeASIErrorCode(err));
    }

    return mode;
}

bool ofxASICamera::setMode(ASI_CAMERA_MODE mode)
{
    if (!isTriggerCamera())
        return false;

    if (ASISetCameraMode(info.CameraID, mode) == ASI_SUCCESS)
    {
        currentMode = mode;
        return true;
    }
    return false;
}

bool ofxASICamera::sendSoftTrigger(bool start)
{
    if (!isTriggerCamera())
        return false;
    return ASISendSoftTrigger(info.CameraID, start ? ASI_TRUE : ASI_FALSE) == ASI_SUCCESS;
}

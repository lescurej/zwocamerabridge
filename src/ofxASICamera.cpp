#ifndef OFXASI_LOG_CAMERA
#define OFXASI_LOG_CAMERA 1
#endif

#include "ofxASICamera.h"
#include <string>
#include "cameraTools.h"

ofxASICamera::ofxASICamera() : syphonServer(std::make_unique<ofxSyphonServer>()),
                               syphonTexture(std::make_unique<ofTexture>())
{
}

ofxASICamera::~ofxASICamera()
{
    close();
}

void ofxASICamera::setup(LogPanel *logPanel)
{
    this->logPanel = logPanel;
}

void ofxASICamera::log(ofLogLevel level, const std::string &message) const
{
    std::shared_lock lock(logMutex);
    if (logPanel)
    {
        logPanel->addLog(message, level);
    }
}

std::optional<ASI_CAMERA_INFO> ofxASICamera::connect(int index)
{
    log(OF_LOG_NOTICE, "Connecting to camera " + ofToString(index));

    int numCams = getNumConnectedCameras();
    if (numCams <= index)
    {
        log(OF_LOG_ERROR, "No camera at index " + ofToString(index));
        return std::nullopt;
    }

    ASI_CAMERA_INFO tempInfo;
    ASI_ERROR_CODE err = ASIGetCameraProperty(&tempInfo, index);
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "Failed to get camera property : " + decodeASIErrorCode(err));
        return std::nullopt;
    }

    err = ASIOpenCamera(index);
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "ASIOpenCamera failed: " + decodeASIErrorCode(err));
        return std::nullopt;
    }

    err = ASIInitCamera(index);
    if (err != ASI_SUCCESS)
    {
        ASICloseCamera(index);
        log(OF_LOG_ERROR, "ASIInitCamera failed: " + decodeASIErrorCode(err));
        return std::nullopt;
    }

    {
        std::unique_lock lock(infoMutex);
        info = tempInfo;
    }

    connected.store(true, std::memory_order_release);

    log(OF_LOG_NOTICE, "Camera ID " + ofToString(tempInfo.CameraID) + " (" + std::string(tempInfo.Name) + "), " +
                           ofToString(tempInfo.MaxWidth) + "x" + ofToString(tempInfo.MaxHeight));

    controls = getCameraControlCaps();
    return tempInfo;
}

std::future<void> ofxASICamera::startCaptureThread(ASI_IMG_TYPE type, int bin)
{
    if (!isConnected())
    {
        log(OF_LOG_ERROR, "startCaptureThread() : camera not connected.");
        return std::future<void>();
    }

    ASI_CAMERA_INFO currentInfo;
    {
        std::shared_lock lock(infoMutex);
        currentInfo = info;
    }

    {
        std::unique_lock dimLock(dimensionMutex);
        width = int(currentInfo.MaxWidth / bin);
        height = int(currentInfo.MaxHeight / bin);
        adjust_roi_size(width, height);
    }

    {
        std::unique_lock typeLock(imgTypeMutex);
        imgType = type;
    }

    {
        std::unique_lock configLock(configMutex);
        currentBin = bin;
    }

    // int iWidth,  the width of the ROI area. Make sure iWidth%8 == 0.
    // int iHeight,  the height of the ROI area. Make sure iHeight%2 == 0,
    // further, for USB2.0 camera ASI120, please make sure that iWidth*iHeight%1024=0.

    auto err = ASISetROIFormat(currentInfo.CameraID, width, height, bin, type);
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "Failed to set ROI format : " + decodeASIErrorCode(err));
        return std::future<void>();
    }

    setupTextures();

    err = ASIStartVideoCapture(currentInfo.CameraID);
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "Failed to start video capture : " + decodeASIErrorCode(err));
        return std::future<void>();
    }

    captureStartPromise = std::promise<void>();
    auto future = captureStartPromise.get_future();

    log(OF_LOG_NOTICE, "Starting capture thread...");
    captureThread = std::make_unique<std::thread>(&ofxASICamera::captureLoop, this);
    bCaptureRunning = true;

    return future;
}

void ofxASICamera::captureLoop()
{
    ResourceGuard guard(bCaptureRunning);

    const auto dimensions = getDimensions();

    ASI_IMG_TYPE localImgType;
    {
        std::shared_lock typeLock(imgTypeMutex);
        localImgType = imgType;
    }

    size_t bufferSize = dimensions.width * dimensions.height * bytesPerPixel(localImgType);
    auto buffer = std::make_unique<unsigned char[]>(bufferSize);

    captureStartPromise.set_value();

    while (bCaptureRunning)
    {
        if (!isConnected())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        int waitMs = static_cast<int>(cachedExposure.load(std::memory_order_relaxed) * 2 / 1000 + 500);

        ASI_ERROR_CODE err = ASIGetVideoData(getCameraID(), buffer.get(), bufferSize, waitMs);

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

            // {
            //     std::unique_lock syphonLock(syphonMutex);
            //     syphonTexture->loadData(processedBuffer.get(), dimensions.width, dimensions.height,
            //                             getGLInternalFormat(localImgType));
            //     syphonServer->publishTexture(syphonTexture.get());
            // }

            // {
            //     std::unique_lock displayLock(displayMutex);
            //     displayTexture->loadData(processedBuffer.get(), dimensions.width, dimensions.height,
            //                              getGLInternalFormat(localImgType));
            // }
        }
        else
        {
            log(OF_LOG_ERROR, "Failed to get video data: " + decodeASIErrorCode(err));
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void ofxASICamera::setupTextures()
{

    syphonTexture->allocate(width, height, getGLInternalFormat(imgType));
    syphonServer->setName("ASI Camera:" + ofToString(info.CameraID));
}

void ofxASICamera::stopCaptureThread()
{
    if (!isCaptureRunning())
        return;

    bCaptureRunning.store(false, std::memory_order_release);

    if (captureThread && captureThread->joinable())
    {
        try
        {
            captureThread->join();
        }
        catch (const std::exception &e)
        {
            log(OF_LOG_ERROR, "Error stopping capture thread: " + std::string(e.what()));
        }
    }
    syphonTexture->clear();
    captureThread.reset();
}

void ofxASICamera::close()
{
    log(OF_LOG_NOTICE, "Closing camera...");

    stopCaptureThread();

    if (isConnected())
    {
        const int cameraId = getCameraID();

        ASI_ERROR_CODE err = ASIStopVideoCapture(cameraId);
        if (err != ASI_SUCCESS)
        {
            log(OF_LOG_WARNING, "Error stopping video capture: " + decodeASIErrorCode(err));
        }

        err = ASICloseCamera(cameraId);
        if (err != ASI_SUCCESS)
        {
            log(OF_LOG_WARNING, "Error closing camera: " + decodeASIErrorCode(err));
        }
    }

    {
        if (syphonTexture && syphonTexture->isAllocated())
        {
            syphonTexture->clear();
        }
    }

    {
        std::unique_lock dimLock(dimensionMutex);
        width = height = 0;
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
    log(OF_LOG_NOTICE, "Camera disconnected");
}

std::__1::vector<ASI_CONTROL_CAPS> ofxASICamera::getCameraControlCaps()
{
    log(OF_LOG_NOTICE, "Fetching control caps...");
    if (!connected)
    {
        log(OF_LOG_ERROR, "getCameraControlCaps() : camera not connected.");
        return controls;
    }
    int numControls = 0;
    auto err = ASIGetNumOfControls(info.CameraID, &numControls);
    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "Failed to get number of controls : " + decodeASIErrorCode(err));
        return controls;
    }

    controls.clear();
    for (int i = 0; i < numControls; ++i)
    {
        ASI_CONTROL_CAPS cap;
        ASIGetControlCaps(info.CameraID, i, &cap);
        log(OF_LOG_NOTICE, "Contrôle " + ofToString(i) + ": " + std::string(cap.Name) +
                               " [" + ofToString(cap.MinValue) + " - " + ofToString(cap.MaxValue) +
                               "], défaut: " + ofToString(cap.DefaultValue));
        controls.push_back(cap);
    }
    log(OF_LOG_NOTICE, "Control caps fetched");

    return controls;
}

void ofxASICamera::update()
{
    if (!connected)
        return;

    if (newFrameAvailable.exchange(false))
    {
        {
            std::lock_guard lock(pixelsMutex);
            syphonTexture->loadData(latestPixels);
        }
        syphonServer->publishTexture(syphonTexture.get());
    }
}

void ofxASICamera::draw(float x, float y)
{
    if (!connected || !syphonTexture->isAllocated())
        return;

    auto dimensions = getDimensions();

    float drawWidth = dimensions.width;
    float drawHeight = dimensions.height;

    // Ajustement à la taille de la fenêtre
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
    // syphonTexture->draw(x, y, drawWidth, drawHeight);
    ofPopStyle();
}

float ofxASICamera::getControlValue(ASI_CONTROL_TYPE type, bool *autoMode)
{
    if (!connected)
    {
        log(OF_LOG_ERROR, "getControlValue() : camera not connected.");
        return false;
    }
    long val = 0;
    ASI_BOOL bAuto;

    auto err = ASIGetControlValue(info.CameraID, type, &val, &bAuto);

    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "Failed to get control value: " + decodeASIErrorCode(err));
        return -1;
    }

    if (autoMode)
        *autoMode = (bAuto == ASI_TRUE);
    // log(OF_LOG_NOTICE, "Control value: " + ofToString(val) + " for type: " + ofToString(type));
    return (float)val;
}

void ofxASICamera::setControlValue(ASI_CONTROL_TYPE type, float value, bool autoMode)
{
    if (!connected)
    {
        log(OF_LOG_ERROR, "setControlValue() : camera not connected.");
        return;
    }

    // log(OF_LOG_NOTICE, "Setting control value: " + ofToString(value) + " for type: " + ofToString(type));
    auto err = ASISetControlValue(info.CameraID, type, (long)value, autoMode ? ASI_TRUE : ASI_FALSE);

    if (err != ASI_SUCCESS)
    {
        log(OF_LOG_ERROR, "Failed to set control value: " + decodeASIErrorCode(err));
        return;
    }

    if (type == ASI_EXPOSURE)
    {
        cachedExposure = value;
    }
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

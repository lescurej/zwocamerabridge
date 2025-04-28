#include "ofxASICamera.h"

ofxASICamera::ofxASICamera() {}

ofxASICamera::~ofxASICamera()
{
    close();
}

bool ofxASICamera::setup(int index, int w, int h, ASI_IMG_TYPE type, ofxSyphonServer *syphonServer)
{
    int numCams = ASIGetNumOfConnectedCameras();
    if (numCams <= index)
    {
        // ofLogError("ofxASICamera") << "No camera at index " << index;
        return false;
    }

    ASI_CAMERA_INFO info;
    ASIGetCameraProperty(&info, index);
    cameraID = info.CameraID;

    if (ASIOpenCamera(cameraID) != ASI_SUCCESS)
        return false;
    if (ASIInitCamera(cameraID) != ASI_SUCCESS)
        return false;

    width = w;
    height = h;
    imgType = type;

    this->syphonServer = syphonServer;

    if (ASISetROIFormat(cameraID, width, height, 1, imgType) != ASI_SUCCESS)
    {
        ofLogError("ofxASICamera") << "Failed to set ROI format.";
        return false;
    }

    pixelBuffer.resize(width * height); // RAW8 = 1 byte/pixel
    texture.allocate(width, height, GL_LUMINANCE);

    fetchControlCaps();

    if (ASIStartVideoCapture(cameraID) != ASI_SUCCESS)
    {
        ofLogError("ofxASICamera") << "Failed to start video capture.";
        return false;
    }

    connected = true;
    startCaptureThread();
    return true;
}

void ofxASICamera::fetchControlCaps()
{
    int numControls = 0;
    ASIGetNumOfControls(cameraID, &numControls);

    controls.clear();
    for (int i = 0; i < numControls; ++i)
    {
        ASI_CONTROL_CAPS cap;
        ASIGetControlCaps(cameraID, i, &cap);
        controls.push_back(cap);
    }
}

void ofxASICamera::startCaptureThread()
{
    bCaptureRunning = true;
    captureThread = std::thread(&ofxASICamera::captureLoop, this);
}

void ofxASICamera::stopCaptureThread()
{
    bCaptureRunning = false;
    if (captureThread.joinable())
        captureThread.join();
}

void ofxASICamera::captureLoop()
{
    while (bCaptureRunning)
    {
        ASI_ERROR_CODE err = ASIGetVideoData(cameraID, pixelBuffer.data(), pixelBuffer.size(), 200 /*exposure*2+500ms*/);
        if (err == ASI_SUCCESS)
        {
            {
                std::lock_guard<std::mutex> lock(bufferMutex);
                texture.loadData(pixelBuffer.data(), width, height, GL_LUMINANCE);
            }
            bFrameNew = true;
            if (onNewFrame)
                onNewFrame();
            if (syphonServer)
                syphonServer->publishTexture(&texture);
        }
        else
        {
            bFrameNew = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void ofxASICamera::close()
{
    stopCaptureThread();

    if (connected)
    {
        ASIStopVideoCapture(cameraID);
        ASICloseCamera(cameraID);
        connected = false;
    }
}

bool ofxASICamera::isFrameNew() const
{
    return bFrameNew;
}

void ofxASICamera::draw(float x, float y)
{
    std::lock_guard<std::mutex> lock(bufferMutex);
    if (texture.isAllocated())
        texture.draw(x, y, width, height);
}

std::string ofxASICamera::getCameraName() const
{
    ASI_CAMERA_INFO info;
    ASIGetCameraProperty(&info, cameraID);
    return std::string(info.Name);
}

int ofxASICamera::getControlValue(ASI_CONTROL_TYPE type, bool *autoMode)
{
    long val = 0;
    ASI_BOOL bAuto;
    if (ASIGetControlValue(cameraID, type, &val, &bAuto) == ASI_SUCCESS)
    {
        if (autoMode)
            *autoMode = (bAuto == ASI_TRUE);
        return static_cast<int>(val);
    }
    return -1;
}

void ofxASICamera::setControlValue(ASI_CONTROL_TYPE type, int value, bool autoMode)
{
    ASISetControlValue(cameraID, type, value, autoMode ? ASI_TRUE : ASI_FALSE);
}

std::vector<ASI_CONTROL_CAPS> ofxASICamera::getAllControls() const
{
    return controls;
}

ASI_CAMERA_INFO ofxASICamera::getCameraInfo() const
{
    ASI_CAMERA_INFO info;
    ASIGetCameraProperty(&info, cameraID);
    return info;
}

std::vector<int> ofxASICamera::getSupportedBins() const
{
    ASI_CAMERA_INFO info = getCameraInfo();
    std::vector<int> bins;
    for (int i = 0; i < 16 && info.SupportedBins[i] != 0; ++i)
        bins.push_back(info.SupportedBins[i]);
    return bins;
}

void ofxASICamera::setBinning(int bin)
{
    currentBin = bin;
    ASISetROIFormat(cameraID, width, height, bin, imgType);
}

int ofxASICamera::getBinning() const
{
    return currentBin;
}

void ofxASICamera::setROI(int x, int y, int w, int h)
{
    ASISetStartPos(cameraID, x, y);
    ASISetROIFormat(cameraID, w, h, getBinning(), imgType);
}

bool ofxASICamera::isTriggerCamera() const
{
    ASI_CAMERA_INFO info = getCameraInfo();
    return info.IsTriggerCam;
}

std::vector<ASI_CAMERA_MODE> ofxASICamera::getSupportedModes() const
{
    std::vector<ASI_CAMERA_MODE> modes;
    if (!isTriggerCamera())
        return modes;

    ASI_SUPPORTED_MODE supportedMode;
    if (ASIGetCameraSupportMode(cameraID, &supportedMode) == ASI_SUCCESS)
    {
        for (int i = 0; i < 16 && supportedMode.SupportedCameraMode[i] != ASI_MODE_END; ++i)
        {
            modes.push_back(supportedMode.SupportedCameraMode[i]);
        }
    }
    return modes;
}

ASI_CAMERA_MODE ofxASICamera::getCurrentMode() const
{
    if (!isTriggerCamera())
        return ASI_MODE_NORMAL;

    ASI_CAMERA_MODE mode;
    if (ASIGetCameraMode(cameraID, &mode) == ASI_SUCCESS)
    {
        return mode;
    }
    return ASI_MODE_NORMAL;
}

bool ofxASICamera::setMode(ASI_CAMERA_MODE mode)
{
    if (!isTriggerCamera())
        return false;

    if (ASISetCameraMode(cameraID, mode) == ASI_SUCCESS)
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
    return ASISendSoftTrigger(cameraID, start ? ASI_TRUE : ASI_FALSE) == ASI_SUCCESS;
}

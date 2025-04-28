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

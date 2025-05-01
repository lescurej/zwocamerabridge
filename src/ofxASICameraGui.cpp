// ofxASICameraGui.cpp
#include "ofxASICameraGui.h"
#include "cameraTools.h"

void ofxASICameraGui::setup(LogPanel *logPanel)
{
    this->logPanel = logPanel;
    camera.setup(logPanel);
}

void ofxASICameraGui::connect(int _cameraIndex)
{
    this->settingsFileName = "camera_" + ofToString(_cameraIndex) + ".xml";
    panel.setup("ASI Camera Controls:" + ofToString(_cameraIndex), settingsFileName, 10, 200);

    startCaptureButton.setup("Start Capture");
    panel.add(&startCaptureButton);
    startCaptureButton.addListener(this, &ofxASICameraGui::onStartCapturePressed);

    stopCaptureButton.setup("Stop Capture");
    panel.add(&stopCaptureButton);
    stopCaptureButton.addListener(this, &ofxASICameraGui::onStopCapturePressed);

    // Paramètres de configuration
    auto info = camera.connect(_cameraIndex);
    log(OF_LOG_NOTICE, "info.CameraID: " + ofToString(info->CameraID));
    resolutionMaxWidth.set("Max Width", info->MaxWidth, info->MaxWidth, info->MaxWidth);
    panel.add(resolutionMaxWidth);

    resolutionMaxHeight.set("Max Height", info->MaxHeight, info->MaxHeight, info->MaxHeight);
    panel.add(resolutionMaxHeight);

    // Image type
    std::vector<std::string> imageTypeVec(std::begin(imageType), std::end(imageType));
    imageTypeToggleGroup.setup("Image Type", imageTypeVec, 1);
    panel.add(&imageTypeToggleGroup);
    imageTypeToggleGroup.getParameter().addListener(this, &ofxASICameraGui::onImageTypeChanged);

    // Binning
    std::vector<int> bins;
    for (int i = 0; i < 16 && info->SupportedBins[i] != 0; ++i)
        bins.push_back(info->SupportedBins[i]);
    std::vector<std::string> binVec;
    for (auto bin : bins)
    {
        binVec.push_back(getBin(bin - 1));
    }
    binToggleGroup.setup("Binning", binVec, camera.getBinning());
    panel.add(&binToggleGroup);

    // Affichage des infos caméra
    std::string infoStr = "Nom: " + std::string(info->Name) +
                          "\nID: " + ofToString(info->CameraID) +
                          "\nMax: " + ofToString(info->MaxWidth) + "x" + ofToString(info->MaxHeight) +
                          "\nTrigger: " + (info->IsTriggerCam ? "Oui" : "Non");
    cameraInfo.setup("Infos", infoStr);
    panel.add(&cameraInfo);

    log(OF_LOG_NOTICE, "info.IsTriggerCam: " + ofToString(info->IsTriggerCam));
    // Mode caméra (si supporté)
    if (info->IsTriggerCam)
    {
        auto modes = camera.getSupportedModes();
        log(OF_LOG_NOTICE, "modes: " + ofToString(modes.size()));
        if (!modes.empty())
        {
            std::vector<std::string> modeVec;
            for (auto mode : modes)
            {
                modeVec.push_back(getMode(mode));
            }
            log(OF_LOG_NOTICE, "modeVec: " + ofToString(modeVec));
            modeToggleGroup.setup("Mode", modeVec, 0);
            panel.add(&modeToggleGroup);
            modeToggleGroup.getParameter().addListener(this, &ofxASICameraGui::onModeChanged);

            softTriggerButton.setup("Soft Trigger");
            softTriggerButton.addListener(this, &ofxASICameraGui::onSoftTriggerPressed);
            panel.add(&softTriggerButton);
        }
    }

    auto controls = camera.getAllControls();
    log(OF_LOG_NOTICE, "number of controls: " + ofToString(controls.size()));
    intParams.clear();
    boolParams.clear();
    autoParams.clear();
    for (const auto &cap : controls)
    {
        bool bAuto;
        auto currentValue = camera.getControlValue(cap.ControlType, &bAuto);

        const std::vector<ASI_CONTROL_TYPE> intParamsType = {
            ASI_GAIN,
            ASI_EXPOSURE,
            ASI_GAMMA,
            ASI_WB_R,
            ASI_WB_B,
            ASI_OFFSET,
            ASI_BANDWIDTHOVERLOAD,
            ASI_TEMPERATURE,
            ASI_FLIP,
            ASI_AUTO_MAX_GAIN,
            ASI_AUTO_MAX_EXP,
            ASI_AUTO_TARGET_BRIGHTNESS,
            ASI_TARGET_TEMP,
            ASI_COOLER_POWER_PERC,
        };

        auto isIntControl = [&intParamsType](ASI_CONTROL_TYPE type) -> bool
        {
            return std::find(intParamsType.begin(), intParamsType.end(), type) != intParamsType.end();
        };

        const std::vector<ASI_CONTROL_TYPE> boolParamsType = {ASI_HARDWARE_BIN, ASI_HIGH_SPEED_MODE, ASI_COOLER_ON, ASI_MONO_BIN, ASI_FAN_ON, ASI_ANTI_DEW_HEATER};

        auto isBooleanControl = [&boolParamsType](ASI_CONTROL_TYPE type) -> bool
        {
            return std::find(boolParamsType.begin(), boolParamsType.end(), type) != boolParamsType.end();
        };

        if (isBooleanControl(cap.ControlType))
        {
            auto &param = boolParams[cap.ControlType];
            param.set(cap.Name, currentValue);
            param.addListener(this, &ofxASICameraGui::onParamBoolChanged);
            panel.add(param);
        }
        else
        {
            auto &param = intParams[cap.ControlType];
            if (cap.ControlType == ASI_EXPOSURE)
            {
                param.set(cap.Name, currentValue, cap.MinValue, 5000);
            }
            else
            {
                param.set(cap.Name, currentValue, cap.MinValue, cap.MaxValue);
            }
            param.addListener(this, &ofxASICameraGui::onParamIntChanged);
            panel.add(param);
        }

        if (cap.IsAutoSupported)
        {
            auto name = std::string("Auto_") + cap.Name;
            auto &toggle = autoParams[cap.ControlType];
            toggle.set(name, bAuto);
            toggle.addListener(this, &ofxASICameraGui::onAutoParamChanged);
            panel.add(toggle);
        }
    }
    panel.loadFromFile(settingsFileName);
    isConnected = true;
}

void ofxASICameraGui::disconnect()
{
    isConnected = false;
    if (camera.isConnected())
    {
        camera.close();
    }
    panel.saveToFile(settingsFileName);
}

void ofxASICameraGui::onResolutionChanged(int &)
{
    // Les changements seront appliqués lors de l'appui sur le bouton Appliquer
}

void ofxASICameraGui::onImageTypeChanged(int &)
{
    // Les changements seront appliqués lors de l'appui sur le bouton Appliquer
}

void ofxASICameraGui::update()
{
    if (!isConnected)
        return;
    if (camera.isCaptureRunning())
    {
        camera.update();
    }

    // // Update mode display if needed
    // if (camera.isTriggerCamera())
    // {
    //     ASI_CAMERA_MODE currentMode = camera.getCurrentMode();
    //     cameraMode.setWithoutEventNotifications(static_cast<int>(currentMode));
    // }

    // for (auto &[type, param] : sliders)
    // {
    //     bool autoMode;
    //     int val = camera.getControlValue(type, &autoMode);
    //     param.setWithoutEventNotifications(val);

    //     if (autoToggles.count(type))
    //     {
    //         autoToggles[type] = autoMode;
    //     }
    // }
}

void ofxASICameraGui::draw()
{
    if (!isConnected)
        return;
    if (camera.isCaptureRunning())
    {
        camera.draw(0, 0);
    }
    panel.draw();
}

void ofxASICameraGui::onParamIntChanged(int &value)
{

    for (auto &[type, param] : intParams)
    {
        if (&param.get() == &value)
        {
            bool autoMode = false;
            if (autoParams.count(type))
            {
                autoMode = autoParams[type];
            }
            camera.setControlValue(type, (float)value, autoMode);
            break;
        }
    }
}

void ofxASICameraGui::onParamBoolChanged(bool &value)
{
    for (auto &[type, param] : boolParams)
    {
        if (&param.get() == &value)
        {
            bool autoMode = false;
            if (autoParams.count(type))
            {
                autoMode = autoParams[type];
            }
            camera.setControlValue(type, value ? 1 : 0, autoMode);
            break;
        }
    }
}

void ofxASICameraGui::onAutoParamChanged(bool &value)
{

    for (auto &[type, toggle] : autoParams)
    {
        if (&toggle.get() == &value)
        {
            int val = intParams[type].get();
            camera.setControlValue(type, val, value);
            break;
        }
    }
}

void ofxASICameraGui::onModeChanged(int &mode)
{
    if (camera.isConnected())
    {
        camera.setMode(static_cast<ASI_CAMERA_MODE>(mode));
    }
}

void ofxASICameraGui::onSoftTriggerPressed()
{
    if (camera.isConnected())
    {
        camera.sendSoftTrigger(true);
        // Pour les modes "level", il faudrait un second appel avec false
        // après un certain délai, selon le cas d'usage
    }
}

void ofxASICameraGui::onStartCapturePressed()
{
    if (camera.isConnected())
    {
        camera.startCaptureThread(getImageTypeFromInt(imageTypeToggleGroup.getParameter()), binToggleGroup.getParameter() + 1);
    }
}

void ofxASICameraGui::onStopCapturePressed()
{
    if (camera.isConnected())
    {
        camera.stopCaptureThread();
    }
}
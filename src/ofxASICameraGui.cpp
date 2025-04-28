// ofxASICameraGui.cpp
#include "ofxASICameraGui.h"
#define CAMERA_SETTINGS_FILE "camera.xml"

ofxASICameraGui::ofxASICameraGui()
{
}

ofxASICameraGui::~ofxASICameraGui()
{
}

void ofxASICameraGui::saveSettings()
{
    panel.saveToFile(CAMERA_SETTINGS_FILE);
}

void ofxASICameraGui::setupModeNames()
{
    modeNames[ASI_MODE_NORMAL] = "Normal";
    modeNames[ASI_MODE_TRIG_SOFT_EDGE] = "Soft Edge Trigger";
    modeNames[ASI_MODE_TRIG_RISE_EDGE] = "Rising Edge Trigger";
    modeNames[ASI_MODE_TRIG_FALL_EDGE] = "Falling Edge Trigger";
    modeNames[ASI_MODE_TRIG_SOFT_LEVEL] = "Soft Level Trigger";
    modeNames[ASI_MODE_TRIG_HIGH_LEVEL] = "High Level Trigger";
    modeNames[ASI_MODE_TRIG_LOW_LEVEL] = "Low Level Trigger";
}

void ofxASICameraGui::setup(ofxASICamera &camera)
{
    cam = &camera;
    panel.setup("ASI Camera Controls", CAMERA_SETTINGS_FILE, 10, 10);
    controlNames.clear();
    setupModeNames();

    // Affichage des infos caméra
    auto info = cam->getCameraInfo();
    std::string infoStr = "Nom: " + std::string(info.Name) +
                          "\nID: " + ofToString(info.CameraID) +
                          "\nMax: " + ofToString(info.MaxWidth) + "x" + ofToString(info.MaxHeight) +
                          "\nTrigger: " + (info.IsTriggerCam ? "Oui" : "Non");
    cameraInfo.set("Infos", infoStr);
    panel.add(cameraInfo);

    // Mode caméra (si supporté)
    if (cam->isTriggerCamera())
    {
        auto modes = cam->getSupportedModes();
        if (!modes.empty())
        {
            cameraMode.set("Mode Caméra",
                           static_cast<int>(cam->getCurrentMode()),
                           static_cast<int>(modes.front()),
                           static_cast<int>(modes.back()));
            cameraMode.addListener(this, &ofxASICameraGui::onModeChanged);
            panel.add(cameraMode);

            // Bouton soft trigger
            softTriggerButton.setup("Soft Trigger");
            softTriggerButton.addListener(this, &ofxASICameraGui::onSoftTriggerPressed);
            panel.add(&softTriggerButton);
        }
    }

    // Binning
    auto bins = cam->getSupportedBins();
    int maxBin = bins.empty() ? 1 : *std::max_element(bins.begin(), bins.end());
    binParam.set("Binning", 1, 1, maxBin);
    binParam.addListener(this, &ofxASICameraGui::onBinChanged);
    panel.add(binParam);

    // ROI
    roiX.set("ROI X", 0, 0, info.MaxWidth - 1);
    roiY.set("ROI Y", 0, 0, info.MaxHeight - 1);
    roiW.set("ROI W", info.MaxWidth, 1, info.MaxWidth);
    roiH.set("ROI H", info.MaxHeight, 1, info.MaxHeight);
    roiX.addListener(this, &ofxASICameraGui::onROIChanged);
    roiY.addListener(this, &ofxASICameraGui::onROIChanged);
    roiW.addListener(this, &ofxASICameraGui::onROIChanged);
    roiH.addListener(this, &ofxASICameraGui::onROIChanged);
    panel.add(roiX);
    panel.add(roiY);
    panel.add(roiW);
    panel.add(roiH);

    auto controls = cam->getAllControls();
    for (const auto &cap : controls)
    {
        ofParameter<int> param;
        param.set(cap.Name, cap.DefaultValue, cap.MinValue, cap.MaxValue);
        param.addListener(this, &ofxASICameraGui::onSliderChanged);
        sliders[cap.ControlType] = param;

        if (cap.IsAutoSupported)
        {
            ofxToggle toggle;
            toggle.setup(std::string("Auto ") + cap.Name, 200, 30);
            toggle.addListener(this, &ofxASICameraGui::onToggleChanged);
            autoToggles[cap.ControlType] = toggle;
        }

        controlNames[cap.ControlType] = cap.Name;

        panel.add(param);
        if (cap.IsAutoSupported)
        {
            panel.add(&autoToggles[cap.ControlType]);
        }
    }
    panel.loadFromFile(CAMERA_SETTINGS_FILE);

    initialized = true;
}

void ofxASICameraGui::update()
{
    if (!initialized || !cam)
        return;

    // Update mode display if needed
    if (cam->isTriggerCamera())
    {
        ASI_CAMERA_MODE currentMode = cam->getCurrentMode();
        cameraMode.setWithoutEventNotifications(static_cast<int>(currentMode));
    }

    for (auto &[type, param] : sliders)
    {
        bool autoMode;
        int val = cam->getControlValue(type, &autoMode);
        param.setWithoutEventNotifications(val);

        if (autoToggles.count(type))
        {
            autoToggles[type] = autoMode;
        }
    }
}

void ofxASICameraGui::draw()
{
    if (initialized)
        panel.draw();
}

void ofxASICameraGui::onSliderChanged(int &value)
{
    for (auto &[type, param] : sliders)
    {
        if (&param.get() == &value)
        {
            bool autoMode = false;
            if (autoToggles.count(type))
            {
                autoMode = autoToggles[type];
            }
            cam->setControlValue(type, value, autoMode);
            break;
        }
    }
}

void ofxASICameraGui::onToggleChanged(bool &value)
{
    for (auto &[type, toggle] : autoToggles)
    {
        if (&toggle.getParameter().cast<bool>().get() == &value && sliders.count(type))
        {
            int val = sliders[type].get();
            cam->setControlValue(type, val, value);
            break;
        }
    }
}

void ofxASICameraGui::onBinChanged(int &value)
{
    if (cam)
        cam->setBinning(value);
}

void ofxASICameraGui::onROIChanged(int &)
{
    if (cam)
        cam->setROI(roiX, roiY, roiW, roiH);
}

void ofxASICameraGui::onModeChanged(int &mode)
{
    if (cam)
    {
        cam->setMode(static_cast<ASI_CAMERA_MODE>(mode));
    }
}

void ofxASICameraGui::onSoftTriggerPressed()
{
    if (cam)
    {
        cam->sendSoftTrigger(true);
        // Pour les modes "level", il faudrait un second appel avec false
        // après un certain délai, selon le cas d'usage
    }
}

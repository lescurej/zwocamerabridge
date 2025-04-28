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

void ofxASICameraGui::setup(ofxASICamera &camera)
{
    cam = &camera;
    panel.setup("ASI Camera Controls", CAMERA_SETTINGS_FILE, 10, 10);
    controlNames.clear();

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

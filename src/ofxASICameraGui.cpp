// ofxASICameraGui.cpp
#include "ofxASICameraGui.h"

ofxASICameraGui::ofxASICameraGui()
{
}

ofxASICameraGui::~ofxASICameraGui()
{
    saveSettings();
}

void ofxASICameraGui::setup(ofxASICamera &camera)
{
    cam = &camera;
    panel.setup("ASI Camera Controls");
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
    loadSettings();
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

void ofxASICameraGui::saveSettings()
{
    ofJson json;
    for (auto &[type, param] : sliders)
    {
        json[controlNames[type]] = param.get();
    }
    for (auto &[type, toggle] : autoToggles)
    {
        json[controlNames[type]] = toggle.getParameter().cast<bool>().get();
    }
    ofSaveJson("camera_settings.json", json);
}

void ofxASICameraGui::loadSettings()
{
    ofFile file("camera_settings.json");
    if (file.exists())
    {
        ofJson json = ofLoadJson(file);
        for (auto &[type, param] : sliders)
        {
            param.set(json[controlNames[type]]);
        }
        for (auto &[type, toggle] : autoToggles)
        {
            toggle.getParameter().cast<bool>().set(json[controlNames[type]]);
        }
    }
}

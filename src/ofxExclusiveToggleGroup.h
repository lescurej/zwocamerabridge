#pragma once
#include "ofMain.h"
#include "ofxGui.h"

class ofxExclusiveToggleGroup : public ofxGuiGroup
{
public:
    void setup(const std::string &name, const std::vector<std::string> &labels, int defaultIndex = 0)
    {
        this->setName(name);
        parameters.clear();
        toggles.clear();

        if (labels.empty())
        {
            ofLogWarning() << "ofxExclusiveToggleGroup::setup - No labels provided";
            return;
        }

        for (size_t i = 0; i < labels.size(); ++i)
        {
            ofParameter<bool> p;
            p.set(labels[i], i == defaultIndex);
            p.addListener(this, &ofxExclusiveToggleGroup::onToggleChanged);
            parameters.add(p);
            toggles.push_back(p);
        }

        ofxGuiGroup::setup(parameters);
        currentIndex = defaultIndex;

        selectedLabel.set("Selected", toggles[currentIndex].getName());
        parameters.add(selectedLabel);
    }

    int getSelectedIndex() const
    {
        return currentIndex;
    }

    std::string getSelectedLabel() const
    {
        return selectedLabel.get();
    }

    ofParameter<int> &getParameter()
    {
        return currentIndex;
    }

    void clear()
    {
        for (auto &p : toggles)
        {
            p.removeListener(this, &ofxExclusiveToggleGroup::onToggleChanged);
        }
        toggles.clear();
        parameters.clear();
        ofxGuiGroup::clear();
    }

private:
    ofParameterGroup parameters;
    std::vector<ofParameter<bool>> toggles;
    ofParameter<std::string> selectedLabel;
    ofParameter<int> currentIndex = 0;

    void onToggleChanged(bool &value)
    {
        if (!value)
            return;

        for (size_t i = 0; i < toggles.size(); ++i)
        {
            if (toggles[i].get() && i != currentIndex)
            {
                toggles[currentIndex].removeListener(this, &ofxExclusiveToggleGroup::onToggleChanged);
                toggles[currentIndex].set(false);
                toggles[currentIndex].addListener(this, &ofxExclusiveToggleGroup::onToggleChanged);
                currentIndex = i;
                selectedLabel = toggles[i].getName(); // mise à jour automatique de l'affichage
            }
            else if (i != currentIndex)
            {
                toggles[i].removeListener(this, &ofxExclusiveToggleGroup::onToggleChanged);
                toggles[i].set(false);
                toggles[i].addListener(this, &ofxExclusiveToggleGroup::onToggleChanged);
            }
        }
    }
};

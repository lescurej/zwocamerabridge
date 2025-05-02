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
        comboBox.clear();

        // Initialiser la sélection par défaut
        currentIndex.set(name, defaultIndex, 0, labels.size() - 1);
        currentIndex.addListener(this, &ofxExclusiveToggleGroup::onIndexChanded);
        parameters.add(currentIndex);
        this->add(currentIndex);

        // Créer les éléments du comboBox
        for (size_t i = 0; i < labels.size(); ++i)
        {
            ofParameter<bool> p;
            p.set(labels[i], i == currentIndex);
            p.addListener(this, &ofxExclusiveToggleGroup::onOptionChanged);
            parameters.add(p);
            comboBox.push_back(p);
        }

        // Setup du groupe de paramètres
        ofxGuiGroup::setup(parameters);
    }

    ofParameter<int> &getParameter() { return currentIndex; }
    int getSelectedIndex() const { return currentIndex.get(); }
    std::string getSelectedLabel() const { return comboBox[currentIndex.get()].getName(); }

private:
    ofParameterGroup parameters;
    std::vector<ofParameter<bool>> comboBox;
    ofParameter<int> currentIndex;

    void onOptionChanged(bool &value)
    {
        if (value)
        {
            for (size_t i = 0; i < comboBox.size(); ++i)
            {
                if (&comboBox[i].get() == &value) // comparer l'adresse du bool
                {
                    currentIndex = static_cast<int>(i);
                    break;
                }
            }
        }
    }

    void onIndexChanded(int &value)
    {
        for (size_t i = 0; i < comboBox.size(); ++i)
        {
            comboBox[i].removeListener(this, &ofxExclusiveToggleGroup::onOptionChanged);
            comboBox[i].set(i == static_cast<size_t>(value));
            comboBox[i].addListener(this, &ofxExclusiveToggleGroup::onOptionChanged);
        }
    }
};

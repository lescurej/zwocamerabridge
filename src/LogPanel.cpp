#include "LogPanel.h"

void LogPanel::setup(string panelName, int maxLines)
{
    this->maxLines = maxLines;
    panel.setup(panelName, "", 600, 10);
    panel.add(clearButton.setup("Clear"));
    clearButton.addListener(this, &LogPanel::clear);
}

void LogPanel::addLog(const string &message, ofLogLevel level)
{
    ofLog(level, message);
    logs.push_back({message, level});
    if (logs.size() > maxLines)
    {
        logs.erase(logs.begin()); // remove oldest
    }
}

ofColor LogPanel::getColorForLevel(ofLogLevel level)
{
    switch (level)
    {
    case OF_LOG_VERBOSE:
        return ofColor(150, 150, 150); // Grey
    case OF_LOG_NOTICE:
        return ofColor(255); // White
    case OF_LOG_WARNING:
        return ofColor(255, 165, 0); // Orange
    case OF_LOG_ERROR:
        return ofColor(255, 0, 0); // Red
    case OF_LOG_FATAL_ERROR:
        return ofColor(255, 0, 255); // Magenta
    case OF_LOG_SILENT:
        return ofColor(0); // Black (silent)
    default:
        return ofColor(255);
    }
}

void LogPanel::draw()
{
    panel.draw();

    if (panel.isMinimized())
    {
        return; // ne rien dessiner si le panel est caché
    }

    float startX = panel.getPosition().x + 10;
    float startY = panel.getPosition().y + panel.getHeight() + 10;
    float lineHeight = 15;

    for (size_t i = 0; i < logs.size(); ++i)
    {
        ofSetColor(getColorForLevel(logs[i].level));
        ofDrawBitmapString(logs[i].text, startX, startY + i * lineHeight);
    }

    ofSetColor(255); // reset color
}

void LogPanel::clear()
{
    logs.clear();
}

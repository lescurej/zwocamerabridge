#pragma once
#include "ofMain.h"
#include "ofxGui.h"

struct LogMessage
{
    string text;
    ofLogLevel level;
};

class LogPanel
{
public:
    void setup(string panelName, int maxLines = 20);
    void addLog(const string &message, ofLogLevel level = OF_LOG_NOTICE);
    void draw();
    void clear();

private:
    ofxPanel panel;
    vector<LogMessage> logs;
    int maxLines;
    ofxButton clearButton;

    ofColor getColorForLevel(ofLogLevel level);
};

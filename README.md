# ZWOCameraBridge

ZWOCameraBridge is an openFrameworks-based application that allows control and operation of ZWO ASI cameras from a macOS computer. It provides a graphical interface for camera management, real-time display, parameter configuration, as well as a control interface via OSC (Open Sound Control).

Example camera:
https://www.zwoastro.com/product/asi2600/

## Main Features
- Connection and management of ZWO ASI cameras
- Graphical interface for camera parameter control
- OSC protocol support for remote control
- Real-time video stream display
- Log management and parameter saving

## Requirements
- macOS
- openFrameworks 0.12.0: https://openframeworks.cc/download/
- ofxSyphon addon: https://github.com/astellato/ofxSyphon (must be installed in the openFrameworks addons folder)

## Installation and Compilation

Important: The `ZWOCameraBridge` project folder must be placed in the following directory of your openFrameworks installation:

/of_v0.12.0_osx_release/apps/myApps/ZWOCameraBridge



### Compilation

Open a terminal, navigate to the project folder, then run one of the following commands depending on the desired mode:

- To compile in Debug mode:
  ./build.sh Debug

- To compile in Release mode:
  ./build.sh Release

The binary will be generated in the `bin/` folder.

## Usage

Launch the application from the `bin/ZWOCameraBridge.app` folder or via the terminal after compilation.

---

© 2025 - Johan Lescure.

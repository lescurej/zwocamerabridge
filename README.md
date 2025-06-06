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
   - the build script copies `Syphon.framework` from `OF_ROOT/addons/ofxSyphon/libs/Syphon/lib/osx`. If the framework is missing at this location the build will fail.

**Important**: The application depends on the proprietary `ASICamera2` library
which only provides x86_64 binaries. If you are on Apple Silicon, make sure the
build happens under Rosetta so the executable remains in x86_64. The `build.sh`
script detects Apple Silicon and invokes `make` using `arch -x86_64`
automatically.

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

## Continuous Integration

GitHub Actions and GitLab CI pipelines build the application when a tag starting with `v` is pushed. The workflows download openFrameworks, run `build.sh` and create a DMG that is attached to the release.

---

© 2025 - Johan Lescure.

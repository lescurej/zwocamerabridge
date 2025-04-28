#!/bin/bash
MODE=${1:-Release}

# Chemin du dossier du script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "SCRIPT_DIR: $SCRIPT_DIR"
# Nom du dossier courant = nom du projet
APP_NAME="$(basename "$SCRIPT_DIR")"
echo "APP_NAME: $APP_NAME"

if [[ "$1" == "Debug" ]]; then
    APP_NAME="${APP_NAME}_debug"
fi

DYLIB_NAME="libASICamera2.dylib"
DYLIB_PATH="libs/ASICamera2/${DYLIB_NAME}"

APP_BUNDLE="bin/${APP_NAME}.app"
FRAMEWORKS_PATH="${APP_BUNDLE}/Contents/Frameworks"

echo "Building in $MODE mode..."
make $MODE || exit 1

echo "[Post-Build] Copie de ${DYLIB_NAME}"
mkdir -p "$FRAMEWORKS_PATH"
cp "$DYLIB_PATH" "$FRAMEWORKS_PATH"

BIN_PATH="${APP_BUNDLE}/Contents/MacOS/${APP_NAME}"
NEW_PATH="@loader_path/../Frameworks/${DYLIB_NAME}"

# Re-link .dylib itself
echo "[Post-Build] Fixing install_name in ${DYLIB_NAME}"
install_name_tool -id "@loader_path/../Frameworks/${DYLIB_NAME}" "$FRAMEWORKS_PATH/$DYLIB_NAME"

# Re-link in the binary
echo "[Post-Build] Relinking $DYLIB_NAME in $BIN_PATH"
install_name_tool -change "${DYLIB_NAME}" "@loader_path/../Frameworks/${DYLIB_NAME}" "$BIN_PATH"

install_name_tool -change "@loader_path/${DYLIB_NAME}" \
  "@loader_path/../Frameworks/${DYLIB_NAME}" \
  "${BIN_PATH}"

LIBUSB_DYLIB_NAME="libusb-1.0.0.dylib"
LIBUSB_DYLIB_PATH="libs/libusb/${LIBUSB_DYLIB_NAME}"

#copier la librairie libusb
cp "$LIBUSB_DYLIB_PATH" "$FRAMEWORKS_PATH"

#linnker la librairie libusb
install_name_tool -change "@loader_path/libusb-1.0.0.dylib" \
  "@loader_path/../Frameworks/libusb-1.0.0.dylib" \
  "${FRAMEWORKS_PATH}/${DYLIB_NAME}"

# Chemin vers Syphon.framework (fourni par ofxSyphon)
SYPHON_SOURCE="$SCRIPT_DIR/../../../addons/ofxSyphon/libs/Syphon/lib/osx/Syphon.framework"
SYPHON_DEST="$FRAMEWORKS_PATH/Syphon.framework"

# Copie si non déjà présent
if [ ! -d "$SYPHON_DEST" ]; then
    echo "[Post-Build] Copying Syphon.framework..."
    cp -R "$SYPHON_SOURCE" "$SYPHON_DEST"
fi

install_name_tool -change \
  "@rpath/Syphon.framework/Versions/A/Syphon" \
  "@loader_path/../Frameworks/Syphon.framework/Versions/A/Syphon" \
  "${BIN_PATH}"

otool -L "${BIN_PATH}"


#pragma once

#include "ofMain.h"
#include "ASICamera2.h"

static int getNumConnectedCameras()
{
    auto numCams = ASIGetNumOfConnectedCameras();
    return numCams;
}

static size_t imageTypeToNumChannels(ASI_IMG_TYPE type)
{
    switch (type)
    {
    case ASI_IMG_RAW8:
    case ASI_IMG_RAW16:
    case ASI_IMG_Y8:
        return 1; // Monochrome
    case ASI_IMG_RGB24:
        return 3; // RGB
    default:
        return 1; // Valeur par défaut sûre
    }
}

static GLint getGLInternalFormat(ASI_IMG_TYPE imgType)
{
    switch (imgType)
    {
    case ASI_IMG_RAW16:
        return GL_R16; // 16-bit mono
    case ASI_IMG_RGB24:
        return GL_RGB8; // 8-bit per channel RGB
    case ASI_IMG_RAW8:
        return GL_R8; // 8-bit mono
    case ASI_IMG_Y8:
        return GL_R8; // 8-bit mono (identique à RAW8 pour OpenGL)
    default:
        return GL_R8; // valeur par défaut sûre pour mono 8-bit
    }
}

// Fonction utilitaire pour décoder les codes d'erreur ASI
static std::string decodeASIErrorCode(ASI_ERROR_CODE code)
{
    switch (code)
    {
    case ASI_SUCCESS:
        return "Success";
    case ASI_ERROR_INVALID_INDEX:
        return "Invalid camera index";
    case ASI_ERROR_INVALID_ID:
        return "Invalid camera ID";
    case ASI_ERROR_INVALID_CONTROL_TYPE:
        return "Invalid control type";
    case ASI_ERROR_CAMERA_CLOSED:
        return "Camera closed";
    case ASI_ERROR_CAMERA_REMOVED:
        return "Camera removed";
    case ASI_ERROR_INVALID_PATH:
        return "Invalid file path";
    case ASI_ERROR_INVALID_FILEFORMAT:
        return "Invalid file format";
    case ASI_ERROR_INVALID_SIZE:
        return "Invalid size";
    case ASI_ERROR_INVALID_IMGTYPE:
        return "Invalid image type";
    case ASI_ERROR_OUTOF_BOUNDARY:
        return "Out of boundary value";
    case ASI_ERROR_TIMEOUT:
        return "Timeout";
    case ASI_ERROR_INVALID_SEQUENCE:
        return "Invalid sequence";
    case ASI_ERROR_BUFFER_TOO_SMALL:
        return "Buffer too small";
    case ASI_ERROR_VIDEO_MODE_ACTIVE:
        return "Video mode active";
    case ASI_ERROR_EXPOSURE_IN_PROGRESS:
        return "Exposure in progress";
    case ASI_ERROR_GENERAL_ERROR:
        return "General error";
    case ASI_ERROR_INVALID_MODE:
        return "Invalid mode";
    case ASI_ERROR_GPS_NOT_SUPPORTED:
        return "GPS not supported";
    case ASI_ERROR_GPS_VER_ERR:
        return "GPS FPGA version error";
    case ASI_ERROR_GPS_FPGA_ERR:
        return "GPS FPGA error";
    case ASI_ERROR_GPS_PARAM_OUT_OF_RANGE:
        return "GPS parameter out of range";
    case ASI_ERROR_GPS_DATA_INVALID:
        return "GPS data invalid";
    default:
        return "Unknown error";
    }
}

static size_t bytesPerPixel(ASI_IMG_TYPE imgType) noexcept(false)
{
    switch (imgType)
    {
    case ASI_IMG_RAW16:
        return 2;
    case ASI_IMG_RGB24:
        return 3;
    case ASI_IMG_RAW8:
    case ASI_IMG_Y8:
        return 1;
    default:
        throw std::runtime_error("Type d'image invalide");
    }
}

static std::string bayer[] = {"RG", "BG", "GR", "GB"};

static std::string getBayerPattern(ASI_BAYER_PATTERN pattern)
{
    return bayer[pattern];
}

static std::string imageType[] = {"RAW8",
                                  "RGB24",
                                  "RAW16",
                                  "Y8",
                                  "END"};

static std::string getStringFromImageType(ASI_IMG_TYPE type)
{
    return imageType[type];
}

static ASI_IMG_TYPE getImageTypeFromInt(int type)
{
    switch (type)
    {
    case 0:
        return ASI_IMG_RAW8;
    case 1:
        return ASI_IMG_RGB24;
    case 2:
        return ASI_IMG_RAW16;
    case 3:
        return ASI_IMG_Y8;
    default:
        throw std::runtime_error("Type d'image invalide");
    }
}

static std::string bin[] = {"1x1", "2x2", "3x3", "4x4"};

static std::string getBin(int _bin)
{
    return bin[_bin];
}

static std::string camera_mode[] = {"Auto", "Soft Edge Trigger", "Rising Edge Trigger", "Falling Edge Trigger", "Soft Level Trigger", "High Level Trigger", "Low Level Trigger"};

static std::string getMode(ASI_CAMERA_MODE mode)
{
    return camera_mode[mode];
}

static std::string control_type[] = {"Gain", "Exposure", "Gamma", "WB_R", "WB_B", "Offset", "BandwidthOverload", "Overclock", "Temperature", "Flip", "Auto_Max_Gain", "Auto_Max_Exp", "Auto_Target_Brightness", "Hardware_Bin", "High_Speed_Mode", "Cooler_Power_Perc", "Target_Temp", "Cooler_On", "Mono_Bin", "Fan_On", "Pattern_Adjust", "Anti_Dew_Heater", "Fan_Adjust", "PWRLED_BRIGNT", "USBHUB_Reset", "GPS_Support", "GPS_Start_Line", "GPS_End_Line", "Rolling_Interval"};

static std::string getControlType(ASI_CONTROL_TYPE type)
{
    return control_type[type];
}

static std::string exposure_status[] = {"Idle", "Working", "Success", "Failed"};

static std::string getExposureStatus(ASI_EXPOSURE_STATUS status)
{
    return exposure_status[status];
}

static std::string flipMode[] = {"None", "Horizontal", "Vertical", "Both"};

static std::string getFlip(ASI_FLIP_STATUS flip)
{
    return flipMode[flip];
}

static std::string rollingInterval[] = {"1000", "2000", "3000", "4000", "5000", "6000", "7000", "8000", "9000", "10000"};

static std::string getRollingInterval(int interval)
{
    return rollingInterval[interval];
}

static void adjust_roi_size(int &iWidth, int &iHeight)
{
    // Round width down to nearest multiple of 8
    iWidth = (iWidth / 8) * 8;

    // Round height down to nearest even number
    iHeight = (iHeight / 2) * 2;
}
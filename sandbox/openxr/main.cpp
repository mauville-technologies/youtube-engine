//
// Created by ozzadar on 2022-12-26.
//

#include <vulkan/vulkan.h>
#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <iostream>

using namespace std;

PFN_xrVoidFunction getXRFunction(XrInstance instance, const char* name) {
    PFN_xrVoidFunction func;

    XrResult result = xrGetInstanceProcAddr(instance, name, &func);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to load OpenXR extension function '" << name << "': " << result << endl;
        return nullptr;
    }

    return func;
}

XrBool32 handleXRError(
        XrDebugUtilsMessageSeverityFlagsEXT severity,
        XrDebugUtilsMessageTypeFlagsEXT type,
        const XrDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData
) {
    cerr << "OpenXR ";

    switch (type)
    {
        case XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT :
            cerr << "general ";
            break;
        case XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT :
            cerr << "validation ";
            break;
        case XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT :
            cerr << "performance ";
            break;
        case XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT :
            cerr << "conformance ";
            break;
    }

    switch (severity)
    {
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT :
            cerr << "(verbose): ";
            break;
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT :
            cerr << "(info): ";
            break;
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT :
            cerr << "(warning): ";
            break;
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT :
            cerr << "(error): ";
            break;
    }

    cerr << callbackData->message << endl;

    return XR_FALSE;
}

XrDebugUtilsMessengerEXT createDebugMessenger(XrInstance instance) {
    XrDebugUtilsMessengerEXT debugMessenger;

    XrDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
    debugMessengerCreateInfo.type = XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerCreateInfo.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugMessengerCreateInfo.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
    debugMessengerCreateInfo.userCallback = handleXRError;
    debugMessengerCreateInfo.userData = nullptr;

    auto xrCreateDebugUtilsMessengerEXT = (PFN_xrCreateDebugUtilsMessengerEXT)getXRFunction(instance, "xrCreateDebugUtilsMessengerEXT");

    XrResult result = xrCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, &debugMessenger);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to create OpenXR debug messenger: " << result << endl;
        return XR_NULL_HANDLE;
    }

    return debugMessenger;
}

XrInstance createInstance() {
    XrInstance instance;

    const char* const layerNames[] = { "XR_APILAYER_LUNARG_core_validation" };

    const char* const extensionNames[] {
        "XR_KHR_vulkan_enable",
        "XR_KHR_vulkan_enable2",
        "XR_EXT_debug_utils"
    };

    uint32_t majorVersion { 0 };
    uint32_t minorVersion { 0 };
    uint32_t patchVersion { 0 };
    XrInstanceCreateInfo instanceCreateInfo { XR_TYPE_INSTANCE_CREATE_INFO };
    instanceCreateInfo.createFlags = 0;
    strcpy_s(instanceCreateInfo.applicationInfo.applicationName, "OpenXR Example");
    instanceCreateInfo.applicationInfo.applicationVersion = XR_MAKE_VERSION(majorVersion, minorVersion, patchVersion);
    strcpy_s(instanceCreateInfo.applicationInfo.engineName, "OpenXR Example");
    instanceCreateInfo.applicationInfo.engineVersion = XR_MAKE_VERSION(majorVersion, minorVersion, patchVersion);
    instanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    instanceCreateInfo.enabledApiLayerCount = 0;
    instanceCreateInfo.enabledApiLayerNames = nullptr;
    instanceCreateInfo.enabledExtensionCount = sizeof(extensionNames) / sizeof(const char*);
    instanceCreateInfo.enabledExtensionNames = extensionNames;

    auto result = xrCreateInstance(&instanceCreateInfo, &instance);

    if ( result != XR_SUCCESS ) {
        cerr << " Failed to create OpenXR instance " << result << endl;
        return XR_NULL_HANDLE;
    }

    return instance;
}

void destroyDebugMessenger(XrInstance instance, XrDebugUtilsMessengerEXT debugMessenger)
{
    auto xrDestroyDebugUtilsMessengerEXT = (PFN_xrDestroyDebugUtilsMessengerEXT)getXRFunction(instance, "xrDestroyDebugUtilsMessengerEXT");

    xrDestroyDebugUtilsMessengerEXT(debugMessenger);
}

void destroyInstance(XrInstance instance) {
    xrDestroyInstance(instance);
}

XrSystemId getSystem(XrInstance instance) {
    XrSystemId systemID;

    XrSystemGetInfo systemGetInfo{ XR_TYPE_SYSTEM_GET_INFO };
    systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    auto result = xrGetSystem(instance, &systemGetInfo, &systemID);

    if (result != XR_SUCCESS) {
        cerr << "Failed to get system: " << result << endl;
        return XR_NULL_SYSTEM_ID;
    }

    return systemID;
}

int main(int, char**)
{
    auto instance = createInstance();
    auto debugMessenger = createDebugMessenger(instance);
    auto systemID = getSystem(instance);

    cout << systemID << endl;

    destroyDebugMessenger(instance, debugMessenger);
    destroyInstance(instance);
    return 0;
}
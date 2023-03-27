//
// Created by ozzadar on 2023-01-01.
//

#include <youtube_engine/rendering/renderer.h>
#include <vr/openxr/open_xr_subsystem.h>

#include <iostream>
#include <vector>

namespace OZZ {
    void OpenXRSubsystem::Init(VRSettings settings) {
        std::cout << "Initializing OpenXR Subsystem" << std::endl;
        _initialized = true;
        _renderer = settings.Renderer;

        createInstance(settings);
        createDebugMessenger();
        getHMDSystem();
        getGraphicsInstanceRequirements(settings);

    }

    void OpenXRSubsystem::Shutdown() {
        if (_renderer == RendererAPI::Vulkan) {
            _vulkanSwapchains.clear();
            EndSessionVulkan();
        }

        if (_debugMessenger != XR_NULL_HANDLE) {
            auto destroyDebugMessengerFunc = reinterpret_cast<PFN_xrDestroyDebugUtilsMessengerEXT>(getXRFunction(
                    "xrDestroyDebugUtilsMessengerEXT"));
            destroyDebugMessengerFunc(_debugMessenger);
            _debugMessenger = XR_NULL_HANDLE;
        }

        if (_instance != XR_NULL_HANDLE) {
            xrDestroyInstance(_instance);
            _instance = XR_NULL_HANDLE;
        }

        _initialized = false;
    }


    bool OpenXRSubsystem::Update() {
        XrEventDataBuffer eventData { XR_TYPE_EVENT_DATA_BUFFER };

        while (true) {
            auto result = xrPollEvent(_instance, &eventData);
            if (result == XR_EVENT_UNAVAILABLE) break;

            if (result != XR_SUCCESS) {
                std::cerr << "Failed to poll for XR events: " << result << std::endl;
                break;
            }

            switch (eventData.type) {
                default: {
                    std::cerr << "Unknown OpenXR Event Type received: " << eventData.type << std::endl;
                    break;
                }
                case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
                    std::cerr << "OpenXR Event queue overflowed and events were lost." << std::endl;
                    break;
                }
                case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                    std::cout << "OpenXR instance is shutting down." << std::endl;
                    break;
                }
                case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
                    std::cout << "The interaction profile has changed." << std::endl;
                    break;
                }
                case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
                    std::cout << "The reference space is changing." << std::endl;
                    break;
                }
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                    handleSessionStateChanged(reinterpret_cast<XrEventDataSessionStateChanged*>(&eventData));
                }
            }
        }

        return false;
    }

    void OpenXRSubsystem::createInstance(const VRSettings &settings) {
        std::vector<const char *> extensionNames {
                "XR_EXT_debug_utils"
        };

        switch (settings.Renderer) {
            case RendererAPI::Vulkan: {
                std::cout << "Adding Vulkan OpenXR extensions" << std::endl;
                extensionNames.emplace_back("XR_KHR_vulkan_enable");
            }
        }

        // enumerate api layer properties
        uint32_t capacity;
        xrEnumerateApiLayerProperties(0, &capacity, nullptr);

        std::vector<XrApiLayerProperties> layerProperties;
        layerProperties.resize(capacity);
        xrEnumerateApiLayerProperties(capacity, &capacity, layerProperties.data());

        std::vector<const char *> layerNames{};
        for (auto &property: layerProperties) {
            layerNames.push_back(property.layerName);
        }

        std::cout << "XR API Layer Count: " << layerNames.size() << std::endl;

        uint32_t majorVersion{0};
        uint32_t minorVersion{1};
        uint32_t patchVersion{0};
        XrInstanceCreateInfo instanceCreateInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        instanceCreateInfo.createFlags = 0;
        strcpy_s(instanceCreateInfo.applicationInfo.applicationName, settings.ApplicationName.c_str());
        instanceCreateInfo.applicationInfo.applicationVersion = XR_MAKE_VERSION(majorVersion, minorVersion,
                                                                                patchVersion);
        strcpy_s(instanceCreateInfo.applicationInfo.engineName, "OZZ Engine");
        instanceCreateInfo.applicationInfo.engineVersion = XR_MAKE_VERSION(majorVersion, minorVersion, patchVersion);
        instanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        instanceCreateInfo.enabledApiLayerCount = layerNames.size();
        instanceCreateInfo.enabledApiLayerNames = layerNames.data();
        instanceCreateInfo.enabledExtensionCount = extensionNames.size();
        instanceCreateInfo.enabledExtensionNames = extensionNames.data();

        if (auto result = xrCreateInstance(&instanceCreateInfo, &_instance) != XR_SUCCESS) {
            std::cerr << " Failed to create OpenXR instance " << result << std::endl;
            _instance = XR_NULL_HANDLE;
            _initialized = false;
            return;
        }

        std::cout << "OpenXR instance created" << std::endl;
    }

    void OpenXRSubsystem::createDebugMessenger() {
        XrDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoExt{
                XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        debugUtilsMessengerCreateInfoExt.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                             XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                                             XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                             XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugUtilsMessengerCreateInfoExt.messageTypes =
                XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugUtilsMessengerCreateInfoExt.userCallback = OpenXRSubsystem::handleDebugMessengerError;
        debugUtilsMessengerCreateInfoExt.userData = this;

        auto xrCreateDebugMessengerFunc = reinterpret_cast<PFN_xrCreateDebugUtilsMessengerEXT>(getXRFunction(
                "xrCreateDebugUtilsMessengerEXT"));

        auto createMessengerResult = xrCreateDebugMessengerFunc(_instance, &debugUtilsMessengerCreateInfoExt,
                                                                &_debugMessenger);

        if (createMessengerResult != XR_SUCCESS) {
            std::cerr << "Failed to create debug messenger for OpenXR Subsystem: " << createMessengerResult
                      << std::endl;
        }
    }

    PFN_xrVoidFunction OpenXRSubsystem::getXRFunction(const std::string functionName) {
        if (_instance == XR_NULL_HANDLE) {
            std::cerr << "Cannot get XR function on null instance!" << std::endl;
            return XR_NULL_HANDLE;
        }

        PFN_xrVoidFunction func;
        auto result = xrGetInstanceProcAddr(_instance, functionName.c_str(), &func);

        if (result != XR_SUCCESS) {
            std::cerr << "Failed to load OpenXR extension function '" << functionName << "': " << result << std::endl;
            return XR_NULL_HANDLE;
        }

        return func;
    }

    XrBool32 OpenXRSubsystem::handleDebugMessengerError(XrDebugUtilsMessageSeverityFlagsEXT severity,
                                                        XrDebugUtilsMessageTypeFlagsEXT type,
                                                        const XrDebugUtilsMessengerCallbackDataEXT *callbackData,
                                                        void *userData) {

        auto subsystem = reinterpret_cast<OpenXRSubsystem *>(userData);

        switch (type) {
            case XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: {
                break;
            }
            case XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: {
                break;
            }
            case XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: {
                break;
            }
            case XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT: {
                break;
            }
        }

        switch (severity) {
            case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
                std::cerr << "(verbose): ";
                break;
            }
            case XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
                std::cerr << "(info): ";
                break;
            }
            case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
                std::cerr << "(warning): ";
                break;
            }
            case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
                std::cerr << "(error): ";
                break;
            }
        }

        std::cerr << callbackData->message << std::endl;
        return XR_FALSE;
    }

    void OpenXRSubsystem::handleSessionStateChanged(XrEventDataSessionStateChanged *stateChangedData) {
        switch(stateChangedData->state) {
            case XR_SESSION_STATE_MAX_ENUM:
            case XR_SESSION_STATE_UNKNOWN: {
                std::cerr << "Unknown OpenXR session state entered: " << stateChangedData->state << std::endl;
                break;
            }
            case XR_SESSION_STATE_IDLE: {
                std::cout << "OpenXR session created." << std::endl;
                break;
            }
            case XR_SESSION_STATE_READY: {
                std::cout << "OpenXR beginSession" << std::endl;
                XrSessionBeginInfo beginInfo { XR_TYPE_SESSION_BEGIN_INFO };
                beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

                auto result = xrBeginSession(_session, &beginInfo);

                if (result != XR_SUCCESS) {
                    std::cerr << "Failed to begin OpenXR session: " << result << std::endl;
                    return;
                }

                _ready = true;
                break;
            }
            case XR_SESSION_STATE_SYNCHRONIZED:
            case XR_SESSION_STATE_VISIBLE:
            case XR_SESSION_STATE_FOCUSED: {
                std::cout << "Entered new ready state: " << stateChangedData->state << std::endl;
                break;
            }
            case XR_SESSION_STATE_STOPPING: {
                std::cout << "OpenXR endSession" << std::endl;
                auto result = xrEndSession(_session);

                if (result != XR_SUCCESS) {
                    std::cerr << "Failed to end OpenXR session: " << result << std::endl;
                }
                break;
            }
            case XR_SESSION_STATE_LOSS_PENDING: {
                std::cout << "OpenXR session is shutting down" << std::endl;
                break;
            }
            case XR_SESSION_STATE_EXITING: {
                std::cout << "OpenXR runtime requested shutdown." << std::endl;
                break;
            }
        }
    }

    void OpenXRSubsystem::getHMDSystem() {
        XrSystemGetInfo systemGetInfo { XR_TYPE_SYSTEM_GET_INFO };
        systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

        auto result = xrGetSystem(_instance, &systemGetInfo, &_systemId);

        if (result != XR_SUCCESS) {
            std::cerr << "Failed to get HMD system: " << result << std::endl;
            _initialized = false;
        }
    }

    void OpenXRSubsystem::getGraphicsInstanceRequirements(const VRSettings &settings) {
        switch (settings.Renderer) {
            case RendererAPI::Vulkan: {
                auto xrGetVulkanGraphicsRequirementsFunc =
                        (PFN_xrGetVulkanGraphicsRequirementsKHR)getXRFunction("xrGetVulkanGraphicsRequirementsKHR");

                if (!xrGetVulkanGraphicsRequirementsFunc) {
                    std::cerr << "Cannot find vulkan graphics requirements function!" << std::endl;
                    _initialized = false;
                    return;
                }

                auto xrGetVulkanInstanceExtensionsFunc =
                        (PFN_xrGetVulkanInstanceExtensionsKHR) getXRFunction("xrGetVulkanInstanceExtensionsKHR");

                if (!xrGetVulkanInstanceExtensionsFunc) {
                    std::cerr << "Cannot find vulkan instance requirements function!" << std::endl;
                    _initialized = false;
                    return;
                }

                XrGraphicsRequirementsVulkanKHR graphicsRequirements { XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR };

                XrResult result = xrGetVulkanGraphicsRequirementsFunc(_instance, _systemId, &graphicsRequirements);
                if (result != XR_SUCCESS) {
                    std::cerr << "Failed to get Vulkan graphics requirements: " << result << std::endl;
                    _initialized = false;
                    return;
                }

                _vulkanGraphicsRequirements = graphicsRequirements;

                uint32_t instanceExtensionSize;
                result = xrGetVulkanInstanceExtensionsFunc(_instance, _systemId, 0, &instanceExtensionSize, nullptr);

                if (result != XR_SUCCESS) {
                    std::cerr << "Failed to get Vulkan Instance Extensions count: " << result << std::endl;
                    _initialized = false;
                    return;
                }

                auto instanceExtensionsChars = std::vector<char> ();
                instanceExtensionsChars.resize(instanceExtensionSize);

                result = xrGetVulkanInstanceExtensionsFunc(_instance, _systemId, instanceExtensionSize, &instanceExtensionSize, instanceExtensionsChars.data());
                if (result != XR_SUCCESS) {
                    std::cerr << "Failed to get vulkan instance extensions: " << result << std::endl;
                    _initialized = false;
                    return;
                }

                uint32_t last = 0;
                for (uint32_t i = 0; i <= instanceExtensionSize; i++) {
                    if (i == instanceExtensionSize || instanceExtensionsChars[i] == ' ') {
                        _vulkanInstanceExtensions.insert(std::string(instanceExtensionsChars.data() + last, i - last));
                        last = i + 1;
                    }
                }

                break;
            }
        }
    }

    void OpenXRSubsystem::createSpace() {
        XrReferenceSpaceCreateInfo spaceCreateInfo { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        spaceCreateInfo.poseInReferenceSpace = { {0, 0, 0, 1}, {0, 0, 0}};

        auto result = xrCreateReferenceSpace(_session, &spaceCreateInfo, &_space);
        if (result != XR_SUCCESS) {
            std::cerr << "Failed to create OpenXR reference space: " << result << std::endl;
        }
    }

    void OpenXRSubsystem::createSwapchains() {
        if (_session == XR_NULL_HANDLE) {
            std::cerr << "Requested OpenXR swapchain creation without an active session" << std::endl;
            return;
        }

        if (_renderer == RendererAPI::Vulkan) {
            createVulkanSwapchains();
        }
    }

    VkPhysicalDevice OpenXRSubsystem::GetVulkanPhysicalDevice(VkInstance vulkanInstance) {
        auto xrGetVulkanGraphicsDevice = (PFN_xrGetVulkanGraphicsDeviceKHR)getXRFunction("xrGetVulkanGraphicsDeviceKHR");

        VkPhysicalDevice physDevice;

        auto result = xrGetVulkanGraphicsDevice(_instance, _systemId, vulkanInstance, &physDevice);
        if (result != XR_SUCCESS) {
            std::cerr << "Failed to get Vulkan graphics device: " << result << std::endl;
            return VK_NULL_HANDLE;
        }

        return physDevice;
    }

    std::set<std::string>
    OpenXRSubsystem::GetVulkanDeviceExtensions() {
        auto xrGetVulkanDeviceExtensionsFunc = (PFN_xrGetVulkanDeviceExtensionsKHR) getXRFunction("xrGetVulkanDeviceExtensionsKHR");

        uint32_t deviceExtensionSize;
        auto result = xrGetVulkanDeviceExtensionsFunc(_instance, _systemId, 0, &deviceExtensionSize, nullptr);

        if (result != XR_SUCCESS) {
            std::cerr << "Failed to get Vulkan device extension count: " << result << std::endl;
            return {};
        }

        std::vector<char> deviceExtensionsData {};
        deviceExtensionsData.resize(deviceExtensionSize);

        result = xrGetVulkanDeviceExtensionsFunc(_instance, _systemId, deviceExtensionSize, &deviceExtensionSize, deviceExtensionsData.data());

        if (result != XR_SUCCESS) {
            std::cerr << "Failed to get vulkan device extensions: " << result << std::endl;
            return {};
        }

        std::set<std::string> deviceExtensions;

        uint32_t last = 0;
        for (uint32_t i = 0; i <= deviceExtensionsData.size(); i++) {
            if (i == deviceExtensionsData.size() || deviceExtensionsData[i] == ' ') {
                deviceExtensions.insert(std::string(deviceExtensionsData.data() + last, i - last));
                last = i + 1;
            }
        }

        return deviceExtensions;
    }

    void OpenXRSubsystem::CreateSessionVulkan(VkInstance vulkanInstance, VkPhysicalDevice physDevice, VkDevice device, uint32_t queueFamilyIndex) {
        XrGraphicsBindingVulkanKHR graphicsBinding { XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR };
        graphicsBinding.instance = vulkanInstance;
        graphicsBinding.physicalDevice = physDevice;
        graphicsBinding.device = device;
        graphicsBinding.queueFamilyIndex = queueFamilyIndex;
        graphicsBinding.queueIndex = 0;

        XrSessionCreateInfo sessionCreateInfo { XR_TYPE_SESSION_CREATE_INFO };
        sessionCreateInfo.next = &graphicsBinding;
        sessionCreateInfo.systemId = _systemId;

        auto result = xrCreateSession(_instance, &sessionCreateInfo, &_session);

        if (result != XR_SUCCESS) {
            std::cerr << "Failed to create OpenXR session: " << result << std::endl;
            return;
        }

        // TODO: This space will likely need customizing?
        createSpace();
    }

    void OpenXRSubsystem::EndSessionVulkan() {
        if (_session != XR_NULL_HANDLE) {
            xrDestroySession(_session);
            _session = XR_NULL_HANDLE;
        }
    }

    void OpenXRSubsystem::createVulkanSwapchains() {
        uint32_t configViewsCount = EyeCount;

        std::vector<XrViewConfigurationView> configViews{
            configViewsCount,
            {.type = XR_TYPE_VIEW_CONFIGURATION_VIEW }
        };

        auto result = xrEnumerateViewConfigurationViews(
            _instance,
            _systemId,
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            configViewsCount,
            &configViewsCount,
            configViews.data()
        );

        if (result != XR_SUCCESS) {
            std::cerr << "Failed to enumerate view configuration views: " << result << std::endl;
            return;
        }

        uint32_t formatCount { 0 };
        result = xrEnumerateSwapchainFormats(_session, 0, &formatCount, nullptr);

        if (result != XR_SUCCESS) {
            std::cerr << "Failed to enumerate swapchain format (acquire count): " << result << std::endl;
            return;
        }

        std::vector<int64_t> formats {};
        formats.resize(formatCount);

        result = xrEnumerateSwapchainFormats(_session, formatCount, &formatCount, formats.data());
        if (result != XR_SUCCESS) {
            std::cerr << "Failed to enumerate swapchain format: " << result << std::endl;
            return;
        }

        int64_t chosenFormat = formats.front();

        for (auto format : formats) {
            if (format == VK_FORMAT_R8G8B8A8_SRGB) {
                chosenFormat = format;
                break;
            }
        }

        _vulkanSwapchains.clear();
        _vulkanSwapchains.resize(EyeCount);

        for (uint32_t i = 0; i < EyeCount; i++) {
            _swapchainWidth = configViews[i].recommendedImageRectWidth;
            _swapchainHeight = configViews[i].recommendedImageRectHeight;

            XrSwapchainCreateInfo swapchainCreateInfo { XR_TYPE_SWAPCHAIN_CREATE_INFO };
            swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            swapchainCreateInfo.format = chosenFormat;
            swapchainCreateInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
            swapchainCreateInfo.width = configViews[i].recommendedImageRectWidth;
            swapchainCreateInfo.height = configViews[i].recommendedImageRectHeight;
            swapchainCreateInfo.faceCount = 1;
            swapchainCreateInfo.arraySize = 1;
            swapchainCreateInfo.mipCount = 1;

            result = xrCreateSwapchain(_session, &swapchainCreateInfo, &_vulkanSwapchains[i].Swapchain);
            if (result != XR_SUCCESS) {
                std::cerr << "Failed to create xr swapchain for eye " << i << ": " << result << std::endl;
                _vulkanSwapchains.clear();
                return;
            }

            _vulkanSwapchains[i].Format = static_cast<VkFormat>(chosenFormat);
            _vulkanSwapchains[i].Width = configViews[i].recommendedImageRectWidth;
            _vulkanSwapchains[i].Height = configViews[i].recommendedImageRectHeight;

            // Store references to the swapchain images
            uint32_t imageCount;
            result = xrEnumerateSwapchainImages(_vulkanSwapchains[i].Swapchain, 0, &imageCount, nullptr);
            if (result != XR_SUCCESS) {
                std::cerr << "Failed to enumerate XR swapchain images (acquire count): " << result << std::endl;
                _vulkanSwapchains[i].Images.clear();
                return;
            }

            std::vector<XrSwapchainImageVulkanKHR> images{
                imageCount,
                {.type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR }
            };

            result = xrEnumerateSwapchainImages(_vulkanSwapchains[i].Swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)images.data());
            if (result != XR_SUCCESS) {
                std::cerr << "Failed to enumerate XR swapchain images: " << result << std::endl;
                _vulkanSwapchains[i].Images.clear();
                return;
            }

            _vulkanSwapchains[i].Images = images;
            _vulkanSwapchains[i].BufferDepth = images.size();
        }
    }

    std::tuple<XrFrameWaitInfo, XrFrameState> OpenXRSubsystem::WaitForNextVulkanFrame() {
        XrFrameWaitInfo frameWaitInfo { XR_TYPE_FRAME_WAIT_INFO };
        XrFrameState frameState { XR_TYPE_FRAME_STATE };

        auto result = xrWaitFrame(_session, &frameWaitInfo, &frameState);

        if (result != XR_SUCCESS) {
            std::cerr << "OpenXR Failed to Wait for vulkan frame: " << std::endl;
            return {};
        }

        return {frameWaitInfo, frameState};
    };

    std::vector<EyePoseInfo> OpenXRSubsystem::BeginVulkanFrame(XrFrameWaitInfo frameWaitInfo, XrFrameState frameState) {
        XrFrameBeginInfo beginFrameInfo { XR_TYPE_FRAME_BEGIN_INFO };
        auto result = xrBeginFrame(_session, &beginFrameInfo);

        if (result != XR_SUCCESS) {
            std::cerr << "OpenXR failed to begin Vulkan Frame: " << result << std::endl;
            return {};
        }

        _nextPredictedDisplayTime = frameState.predictedDisplayTime;

        XrViewLocateInfo viewLocateInfo { XR_TYPE_VIEW_LOCATE_INFO };
        viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        viewLocateInfo.displayTime = frameState.predictedDisplayTime;
        viewLocateInfo.space = _space;

        XrViewState viewState { XR_TYPE_VIEW_STATE };

        uint32_t viewCount { EyeCount };
        std::vector<XrView> views {
            viewCount,
            { .type = XR_TYPE_VIEW }
        };

        result = xrLocateViews(_session, &viewLocateInfo, &viewState, viewCount, &viewCount, views.data());
        if (result != XR_SUCCESS) {
            std::cerr << "Failed to locate OpenXR views" << std::endl;
            return {};
        }

        std::vector<EyePoseInfo> eyePoses (EyeCount);
        eyePoses[1] = {
            .FOV = {views[1].fov.angleDown, views[1].fov.angleLeft, views[1].fov.angleRight, views[1].fov.angleUp},
            .Orientation = glm::quat{views[1].pose.orientation.w, views[1].pose.orientation.x, views[1].pose.orientation.y, views[1].pose.orientation.z},
            .Position = { views[1].pose.position.x, views[1].pose.position.y, views[1].pose.position.z }
        };
        eyePoses[0] = {
            .FOV = {views[0].fov.angleDown, views[0].fov.angleLeft, views[0].fov.angleRight, views[0].fov.angleUp},
            .Orientation = glm::quat{views[0].pose.orientation.w, views[0].pose.orientation.x, views[0].pose.orientation.y, views[0].pose.orientation.z},
            .Position = { views[0].pose.position.x, views[0].pose.position.y, views[0].pose.position.z }
        };

        return eyePoses;
    }

    void OpenXRSubsystem::EndVulkanFrame(const std::vector<EyePoseInfo>& poseInfo) {
        XrCompositionLayerProjectionView projectedViews[EyeCount] {};

        for (auto i = 0; i < poseInfo.size(); i++) {
            XrPosef pose = {
                .orientation = {
                    .x = poseInfo[i].Orientation.x,
                    .y = poseInfo[i].Orientation.y,
                    .z = poseInfo[i].Orientation.z,
                    .w = poseInfo[i].Orientation.w,
                },
                .position = {
                    .x = poseInfo[i].Position.x,
                    .y = poseInfo[i].Position.y,
                    .z = poseInfo[i].Position.z
                }
            };

            XrFovf fov = {
                .angleLeft = poseInfo[i].FOV.AngleLeft,
                .angleRight = poseInfo[i].FOV.AngleRight,
                .angleUp = poseInfo[i].FOV.AngleUp,
                .angleDown = poseInfo[i].FOV.AngleDown,
            };

            projectedViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
            projectedViews[i].pose = pose;
            projectedViews[i].fov = fov;
            projectedViews[i].subImage = {
                .swapchain = _vulkanSwapchains[i].Swapchain,
                .imageRect = {
                    .offset = {0, 0},
                    .extent = { static_cast<int32_t>(_vulkanSwapchains[i].Width), static_cast<int32_t>(_vulkanSwapchains[i].Height) },
                },
                .imageArrayIndex = 0,
            };
        }

        XrCompositionLayerProjection layer { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        layer.space = _space;
        layer.viewCount = EyeCount;
        layer.views = projectedViews;

        auto pLayer = (const XrCompositionLayerBaseHeader*)&layer;
        XrFrameEndInfo endFrameInfo { XR_TYPE_FRAME_END_INFO };
        endFrameInfo.displayTime = _nextPredictedDisplayTime;
        endFrameInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endFrameInfo.layerCount = 1;
        endFrameInfo.layers = &pLayer;

        auto result = xrEndFrame(_session, &endFrameInfo);

        if (result != XR_SUCCESS) {
            std::cerr << "Failed to end OpenXR frame: " << result << std::endl;
            return;
        }
    }

    uint32_t OpenXRSubsystem::AcquireVulkanSwapchainImage(uint32_t eyeIndex) {
        XrSwapchainImageAcquireInfo imageAcquireInfo { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };

        const auto& swapchain = _vulkanSwapchains[eyeIndex];

        uint32_t activeIndex;

        auto result = xrAcquireSwapchainImage(swapchain.Swapchain, &imageAcquireInfo, &activeIndex);
        if (result != XR_SUCCESS) {
            std::cerr << "Failed to acquire swapchain image: " << result << std::endl;
            return BAD_INDEX;
        }

        XrSwapchainImageWaitInfo imageWaitInfo { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        imageWaitInfo.timeout = std::numeric_limits<int64_t>::max();

        result = xrWaitSwapchainImage(swapchain.Swapchain, &imageWaitInfo);

        if (result != XR_SUCCESS) {
//            std::cerr << "Failed to wait for OpenXR swapchain image: " << result << std::endl;
            return BAD_INDEX;
        }

        return activeIndex;
    }

    OpenXRSubsystem::OpenXRVulkanSwapchain& OpenXRSubsystem::GetVulkanSwapchain(int eyeIndex) {
        return _vulkanSwapchains[eyeIndex];
    }

    void OpenXRSubsystem::ReleaseVulkanSwapchainImage(int eyeIndex) {
        XrSwapchainImageReleaseInfo imageReleaseInfo { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };

        auto result = xrReleaseSwapchainImage(_vulkanSwapchains[eyeIndex].Swapchain, &imageReleaseInfo);
        if (result != XR_SUCCESS) {
            std::cerr << "Failed to release swapchain image: " << result << std::endl;
        }
    }
}
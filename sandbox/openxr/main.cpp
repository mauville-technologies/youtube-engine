#include <vulkan/vulkan.h>
#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <cstring>
#include <csignal>
#include <tuple>
#include <set>
#include <vector>
#include <fstream>
#include <streambuf>
#include <limits>
#include <cmath>

using namespace std;

static const char* const applicationName = "OpenXR Example";
static const unsigned int majorVersion = 0;
static const unsigned int minorVersion = 1;
static const unsigned int patchVersion = 0;
static const char* const layerNames[] = { "XR_APILAYER_LUNARG_core_validation" };
static const char* const extensionNames[] = {
        "XR_KHR_vulkan_enable",
        "XR_EXT_debug_utils"
};
static const char* const vulkanLayerNames[] = { "VK_LAYER_KHRONOS_validation" };
static const char* const vulkanExtensionNames[] = { "VK_EXT_debug_utils" };

static const size_t bufferSize = sizeof(float) * 4 * 4 * 3;

static const size_t eyeCount = 2;

static const float nearDistance = 0.01;
static const float farDistance = 1'000;

static bool quit = false;

struct Swapchain
{
    Swapchain(XrSwapchain swapchain, VkFormat format, uint32_t width, uint32_t height)
            : swapchain(swapchain)
            , format(format)
            , width(width)
            , height(height)
    {

    }

    ~Swapchain()
    {
        xrDestroySwapchain(swapchain);
    }

    XrSwapchain swapchain;
    VkFormat format;
    uint32_t width;
    uint32_t height;
};

struct SwapchainImage
{
    SwapchainImage(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkRenderPass renderPass,
            VkCommandPool commandPool,
            VkDescriptorPool descriptorPool,
            VkDescriptorSetLayout descriptorSetLayout,
            const Swapchain* swapchain,
            XrSwapchainImageVulkanKHR image
    )
            : device(device)
            , commandPool(commandPool)
            , descriptorPool(descriptorPool)
            , image(image)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = image.image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = swapchain->format;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);

        if (result != VK_SUCCESS)
        {
            cerr << "Failed to create Vulkan image view: " << result << endl;
        }

        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &imageView;
        framebufferCreateInfo.width = swapchain->width;
        framebufferCreateInfo.height = swapchain->height;
        framebufferCreateInfo.layers = 1;

        result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer);

        if (result != VK_SUCCESS)
        {
            cerr << "Failed to create Vulkan framebuffer: " << result << endl;
        }

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);

        if (result != VK_SUCCESS)
        {
            cerr << "Failed to allocate Vulkan command buffers: " << result << endl;
        }
    }

    ~SwapchainImage()
    {
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        vkDestroyImageView(device, imageView, nullptr);
    }

    XrSwapchainImageVulkanKHR image;
    VkImageView imageView;
    VkFramebuffer framebuffer;
    VkCommandBuffer commandBuffer;

private:
    VkDevice device;
    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;
};

PFN_xrVoidFunction getXRFunction(XrInstance instance, const char* name)
{
    PFN_xrVoidFunction func;

    XrResult result = xrGetInstanceProcAddr(instance, name, &func);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to load OpenXR extension function '" << name << "': " << result << endl;
        return nullptr;
    }

    return func;
}

PFN_vkVoidFunction getVKFunction(VkInstance instance, const char* name)
{
    PFN_vkVoidFunction func = vkGetInstanceProcAddr(instance, name);

    if (!func)
    {
        cerr << "Failed to load Vulkan extension function '" << name << "'." << endl;
        return nullptr;
    }

    return func;
}

XrBool32 handleXRError(
        XrDebugUtilsMessageSeverityFlagsEXT severity,
        XrDebugUtilsMessageTypeFlagsEXT type,
        const XrDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData
)
{
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

    cout << callbackData->message << endl;

    return XR_FALSE;
}

VkBool32 handleVKError(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData
)
{
    cout << "Vulkan ";

    switch (type)
    {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT :
            cout << "general ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT :
            cout << "validation ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT :
            cout << "performance ";
            break;
    }

    switch (severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT :
            cout << "(verbose): ";
            break;
        default :
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT :
            cout << "(info): ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT :
            cout << "(warning): ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT :
            cout << "(error): ";
            break;
    }

    cout << callbackData->pMessage << endl;

    return VK_FALSE;
}

XrInstance createInstance()
{
    XrInstance instance;

    XrInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.createFlags = 0;
    strcpy(instanceCreateInfo.applicationInfo.applicationName, applicationName);
    instanceCreateInfo.applicationInfo.applicationVersion = XR_MAKE_VERSION(majorVersion, minorVersion, patchVersion);
    strcpy(instanceCreateInfo.applicationInfo.engineName, applicationName);
    instanceCreateInfo.applicationInfo.engineVersion = XR_MAKE_VERSION(majorVersion, minorVersion, patchVersion);
    instanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    instanceCreateInfo.enabledApiLayerCount = 0;
    instanceCreateInfo.enabledApiLayerNames = nullptr;
    instanceCreateInfo.enabledExtensionCount = sizeof(extensionNames) / sizeof(const char*);
    instanceCreateInfo.enabledExtensionNames = extensionNames;

    XrResult result = xrCreateInstance(&instanceCreateInfo, &instance);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to create OpenXR instance: " << result << endl;
        return XR_NULL_HANDLE;
    }

    return instance;
}

void destroyInstance(XrInstance instance)
{
    xrDestroyInstance(instance);
}

XrDebugUtilsMessengerEXT createDebugMessenger(XrInstance instance)
{
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

void destroyDebugMessenger(XrInstance instance, XrDebugUtilsMessengerEXT debugMessenger)
{
    auto xrDestroyDebugUtilsMessengerEXT = (PFN_xrDestroyDebugUtilsMessengerEXT)getXRFunction(instance, "xrDestroyDebugUtilsMessengerEXT");

    xrDestroyDebugUtilsMessengerEXT(debugMessenger);
}

XrSystemId getSystem(XrInstance instance)
{
    XrSystemId systemID;

    XrSystemGetInfo systemGetInfo{};
    systemGetInfo.type = XR_TYPE_SYSTEM_GET_INFO;
    systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrResult result = xrGetSystem(instance, &systemGetInfo, &systemID);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to get system: " << result << endl;
        return XR_NULL_SYSTEM_ID;
    }

    return systemID;
}

tuple<XrGraphicsRequirementsVulkanKHR, set<string>> getVulkanInstanceRequirements(XrInstance instance, XrSystemId system)
{
    auto xrGetVulkanGraphicsRequirementsKHR = (PFN_xrGetVulkanGraphicsRequirementsKHR)getXRFunction(instance, "xrGetVulkanGraphicsRequirementsKHR");
    auto xrGetVulkanInstanceExtensionsKHR = (PFN_xrGetVulkanInstanceExtensionsKHR)getXRFunction(instance, "xrGetVulkanInstanceExtensionsKHR");

    XrGraphicsRequirementsVulkanKHR graphicsRequirements{};
    graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR;

    XrResult result = xrGetVulkanGraphicsRequirementsKHR(instance, system, &graphicsRequirements);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to get Vulkan graphics requirements: " << result << endl;
        return { graphicsRequirements, {} };
    }

    uint32_t instanceExtensionsSize;

    result = xrGetVulkanInstanceExtensionsKHR(instance, system, 0, &instanceExtensionsSize, nullptr);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to get Vulkan instance extensions: " << result << endl;
        return { graphicsRequirements, {} };
    }

    char* instanceExtensionsData = new char[instanceExtensionsSize];

    result = xrGetVulkanInstanceExtensionsKHR(instance, system, instanceExtensionsSize, &instanceExtensionsSize, instanceExtensionsData);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to get Vulkan instance extensions: " << result << endl;
        return { graphicsRequirements, {} };
    }

    set<string> instanceExtensions;

    uint32_t last = 0;
    for (uint32_t i = 0; i <= instanceExtensionsSize; i++)
    {
        if (i == instanceExtensionsSize || instanceExtensionsData[i] == ' ')
        {
            instanceExtensions.insert(string(instanceExtensionsData + last, i - last));
            last = i + 1;
        }
    }

    delete[] instanceExtensionsData;

    return { graphicsRequirements, instanceExtensions };
}

tuple<VkPhysicalDevice, set<string>> getVulkanDeviceRequirements(XrInstance instance, XrSystemId system, VkInstance vulkanInstance)
{
    auto xrGetVulkanGraphicsDeviceKHR = (PFN_xrGetVulkanGraphicsDeviceKHR)getXRFunction(instance, "xrGetVulkanGraphicsDeviceKHR");
    auto xrGetVulkanDeviceExtensionsKHR = (PFN_xrGetVulkanDeviceExtensionsKHR)getXRFunction(instance, "xrGetVulkanDeviceExtensionsKHR");

    VkPhysicalDevice physicalDevice;

    XrResult result = xrGetVulkanGraphicsDeviceKHR(instance, system, vulkanInstance, &physicalDevice);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to get Vulkan graphics device: " << result << endl;
        return { VK_NULL_HANDLE, {} };
    }

    uint32_t deviceExtensionsSize;

    result = xrGetVulkanDeviceExtensionsKHR(instance, system, 0, &deviceExtensionsSize, nullptr);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to get Vulkan device extensions: " << result << endl;
        return { VK_NULL_HANDLE, {} };
    }

    char* deviceExtensionsData = new char[deviceExtensionsSize];

    result = xrGetVulkanDeviceExtensionsKHR(instance, system, deviceExtensionsSize, &deviceExtensionsSize, deviceExtensionsData);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to get Vulkan device extensions: " << result << endl;
        return { VK_NULL_HANDLE, {} };
    }

    set<string> deviceExtensions;

    uint32_t last = 0;
    for (uint32_t i = 0; i <= deviceExtensionsSize; i++)
    {
        if (i == deviceExtensionsSize || deviceExtensionsData[i] == ' ')
        {
            deviceExtensions.insert(string(deviceExtensionsData + last, i - last));
            last = i + 1;
        }
    }

    delete[] deviceExtensionsData;

    return { physicalDevice, deviceExtensions };
}

XrSession createSession(
        XrInstance instance,
        XrSystemId systemID,
        VkInstance vulkanInstance,
        VkPhysicalDevice physDevice,
        VkDevice device,
        uint32_t queueFamilyIndex
)
{
    XrSession session;

    XrGraphicsBindingVulkanKHR graphicsBinding{};
    graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
    graphicsBinding.instance = vulkanInstance;
    graphicsBinding.physicalDevice = physDevice;
    graphicsBinding.device = device;
    graphicsBinding.queueFamilyIndex = queueFamilyIndex;
    graphicsBinding.queueIndex = 0;

    XrSessionCreateInfo sessionCreateInfo{};
    sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
    sessionCreateInfo.next = &graphicsBinding;
    sessionCreateInfo.createFlags = 0;
    sessionCreateInfo.systemId = systemID;

    XrResult result = xrCreateSession(instance, &sessionCreateInfo, &session);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to create OpenXR session: " << result << endl;
        return XR_NULL_HANDLE;
    }

    return session;
}

void destroySession(XrSession session)
{
    xrDestroySession(session);
}

tuple<Swapchain*, Swapchain*> createSwapchains(XrInstance instance, XrSystemId system, XrSession session)
{
    uint32_t configViewsCount = eyeCount;
    vector<XrViewConfigurationView> configViews(
            configViewsCount,
            { .type = XR_TYPE_VIEW_CONFIGURATION_VIEW }
    );

    XrResult result = xrEnumerateViewConfigurationViews(
            instance,
            system,
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            configViewsCount,
            &configViewsCount,
            configViews.data()
    );

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to enumerate view configuration views: " << result << endl;
        return { nullptr, nullptr };
    }

    uint32_t formatCount = 0;

    result = xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to enumerate swapchain formats: " << result << endl;
        return { nullptr, nullptr };
    }

    vector<int64_t> formats(formatCount);

    result = xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data());

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to enumerate swapchain formats: " << result << endl;
        return { nullptr, nullptr };
    }

    int64_t chosenFormat = formats.front();

    for (int64_t format : formats)
    {
        if (format == VK_FORMAT_R8G8B8A8_SRGB)
        {
            chosenFormat = format;
            break;
        }
    }

    XrSwapchain swapchains[eyeCount];

    for (size_t i = 0; i < eyeCount; i++)
    {
        XrSwapchainCreateInfo swapchainCreateInfo{};
        swapchainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.format = chosenFormat;
        swapchainCreateInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        swapchainCreateInfo.width = configViews[i].recommendedImageRectWidth;
        swapchainCreateInfo.height = configViews[i].recommendedImageRectHeight;
        swapchainCreateInfo.faceCount = 1;
        swapchainCreateInfo.arraySize = 1;
        swapchainCreateInfo.mipCount = 1;

        result = xrCreateSwapchain(session, &swapchainCreateInfo, &swapchains[i]);

        if (result != XR_SUCCESS)
        {
            cerr << "Failed to create swapchain: " << result << endl;
            return { nullptr, nullptr };
        }
    }

    return {
            new Swapchain(
                    swapchains[0],
                    (VkFormat)chosenFormat,
                    configViews[0].recommendedImageRectWidth,
                    configViews[0].recommendedImageRectHeight
            ),
            new Swapchain(
                    swapchains[1],
                    (VkFormat)chosenFormat,
                    configViews[1].recommendedImageRectWidth,
                    configViews[1].recommendedImageRectHeight
            )
    };
}

XrSpace createSpace(XrSession session)
{
    XrSpace space;

    XrReferenceSpaceCreateInfo spaceCreateInfo{};
    spaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    spaceCreateInfo.poseInReferenceSpace = { { 0, 0, 0, 1 }, { 0, 0, 0 } };

    XrResult result = xrCreateReferenceSpace(session, &spaceCreateInfo, &space);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to create space: " << result << endl;
        return XR_NULL_HANDLE;
    }

    return space;
}

void destroySpace(XrSpace space)
{
    xrDestroySpace(space);
}

vector<XrSwapchainImageVulkanKHR> getSwapchainImages(XrSwapchain swapchain)
{
    uint32_t imageCount;

    XrResult result = xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to enumerate swapchain images: " << result << endl;
        return {};
    }

    vector<XrSwapchainImageVulkanKHR> images(
            imageCount,
            { .type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR }
    );

    result = xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)images.data());

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to enumerate swapchain images: " << result << endl;
        return {};
    }

    return images;
}

VkInstance createVulkanInstance(XrGraphicsRequirementsVulkanKHR graphicsRequirements, set<string> instanceExtensions)
{
    VkInstance instance;

    size_t extensionCount = 1 + instanceExtensions.size();
    const char** extensionNames = new const char*[extensionCount];

    size_t i = 0;
    extensionNames[i] = vulkanExtensionNames[0];
    i++;

    for (const string& instanceExtension : instanceExtensions)
    {
        extensionNames[i] = instanceExtension.c_str();
        i++;
    }

    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = applicationName;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(majorVersion, minorVersion, patchVersion);
    applicationInfo.pEngineName = applicationName;
    applicationInfo.engineVersion = VK_MAKE_VERSION(majorVersion, minorVersion, patchVersion);
    applicationInfo.apiVersion = graphicsRequirements.minApiVersionSupported;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensionNames;
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = vulkanLayerNames;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    delete[] extensionNames;

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to create Vulkan instance: " << result << endl;
        return VK_NULL_HANDLE;
    }

    return instance;
}

void destroyVulkanInstance(VkInstance instance)
{
    vkDestroyInstance(instance, nullptr);
}

VkDebugUtilsMessengerEXT createVulkanDebugMessenger(VkInstance instance)
{
    VkDebugUtilsMessengerEXT debugMessenger;

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
    debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerCreateInfo.pfnUserCallback = handleVKError;

    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)getVKFunction(instance, "vkCreateDebugUtilsMessengerEXT");

    VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger);

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to create Vulkan debug messenger: " << result << endl;
        return VK_NULL_HANDLE;
    }

    return debugMessenger;
}

void destroyVulkanDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
{
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)getVKFunction(instance, "vkDestroyDebugUtilsMessengerEXT");

    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
}

int32_t getDeviceQueueFamily(VkPhysicalDevice physicalDevice)
{
    int32_t graphicsQueueFamilyIndex = -1;

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (int32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsQueueFamilyIndex = i;
            break;
        }
    }

    if (graphicsQueueFamilyIndex == -1)
    {
        cerr << "No graphics queue found." << endl;
        return graphicsQueueFamilyIndex;
    }

    return graphicsQueueFamilyIndex;
}

tuple<VkDevice, VkQueue> createDevice(
        VkPhysicalDevice physicalDevice,
        int32_t graphicsQueueFamilyIndex,
        set<string> deviceExtensions
)
{
    VkDevice device;

    size_t extensionCount = deviceExtensions.size();
    const char** extensions = new const char*[extensionCount];

    size_t i = 0;
    for (const string& deviceExtension : deviceExtensions)
    {
        extensions[i] = deviceExtension.c_str();
        i++;
    }

    float priority = 1;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);

    delete[] extensions;

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to create Vulkan device: " << result << endl;
        return { VK_NULL_HANDLE, VK_NULL_HANDLE };
    }

    VkQueue queue;
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &queue);

    return { device, queue };
}

void destroyDevice(VkDevice device)
{
    vkDestroyDevice(device, nullptr);
}

VkRenderPass createRenderPass(VkDevice device)
{
    VkRenderPass renderPass;

    VkAttachmentDescription attachment{};
    attachment.format = VK_FORMAT_R8G8B8A8_SRGB;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachmentRef{};
    attachmentRef.attachment = 0;
    attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentRef;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &attachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;

    VkResult result = vkCreateRenderPass(device, &createInfo, nullptr, &renderPass);

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to create Vulkan render pass: " << result << endl;
        return VK_NULL_HANDLE;
    }

    return renderPass;
}

void destroyRenderPass(VkDevice device, VkRenderPass renderPass)
{
    vkDestroyRenderPass(device, renderPass, nullptr);
}

VkCommandPool createCommandPool(VkDevice device, int32_t graphicsQueueFamilyIndex)
{
    VkCommandPool commandPool;

    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(device, &createInfo, nullptr, &commandPool);

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to create Vulkan command pool: " << result << endl;
        return VK_NULL_HANDLE;
    }

    return commandPool;
}

void destroyCommandPool(VkDevice device, VkCommandPool commandPool)
{
    vkDestroyCommandPool(device, commandPool, nullptr);
}

VkDescriptorPool createDescriptorPool(VkDevice device)
{
    VkDescriptorPool descriptorPool;

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 32;

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.maxSets = 32;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &poolSize;

    VkResult result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool);

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to create Vulkan descriptor pool: " << result << endl;
        return VK_NULL_HANDLE;
    }

    return descriptorPool;
}

void destroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayout descriptorSetLayout;

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = 1;
    createInfo.pBindings = &binding;

    VkResult result = vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout);

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to create Vulkan descriptor set layout: " << result << endl;
        return VK_NULL_HANDLE;
    }

    return descriptorSetLayout;
}

void destroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

VkShaderModule createShader(VkDevice device, string path)
{
    VkShaderModule shader;

    ifstream file(path);
    string source = string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());

    VkShaderModuleCreateInfo shaderCreateInfo{};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.codeSize = source.size();
    shaderCreateInfo.pCode = (const uint32_t*)source.data();

    VkResult result = vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shader);

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to create Vulkan shader: " << result << endl;
    }

    return shader;
}

void destroyShader(VkDevice device, VkShaderModule shader)
{
    vkDestroyShaderModule(device, shader, nullptr);
}

tuple<VkPipelineLayout, VkPipeline> createPipeline(
        VkDevice device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout descriptorSetLayout,
        VkShaderModule vertexShader,
        VkShaderModule fragmentShader
)
{
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;
    layoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    VkResult result = vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineLayout);

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to create Vulkan pipeline layout: " << result << endl;
        return { VK_NULL_HANDLE, VK_NULL_HANDLE };
    }

    VkPipelineVertexInputStateCreateInfo vertexInputStage{};
    vertexInputStage.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStage.vertexBindingDescriptionCount = 0;
    vertexInputStage.pVertexBindingDescriptions = nullptr;
    vertexInputStage.vertexAttributeDescriptionCount = 0;
    vertexInputStage.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStage{};
    inputAssemblyStage.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStage.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStage.primitiveRestartEnable = false;

    VkPipelineShaderStageCreateInfo vertexShaderStage{};
    vertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStage.module = vertexShader;
    vertexShaderStage.pName = "main";

    VkViewport viewport = {
            0, 0,
            1024, 1024,
            0, 1
    };

    VkRect2D scissor = {
            { 0, 0 },
            { 1024, 1024 }
    };

    VkPipelineViewportStateCreateInfo viewportStage{};
    viewportStage.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStage.viewportCount = 1;
    viewportStage.pViewports = &viewport;
    viewportStage.scissorCount = 1;
    viewportStage.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStage{};
    rasterizationStage.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStage.depthClampEnable = false;
    rasterizationStage.rasterizerDiscardEnable = false;
    rasterizationStage.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStage.lineWidth = 1;
    rasterizationStage.cullMode = VK_CULL_MODE_NONE;
    rasterizationStage.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStage.depthBiasEnable = false;
    rasterizationStage.depthBiasConstantFactor = 0;
    rasterizationStage.depthBiasClamp = 0;
    rasterizationStage.depthBiasSlopeFactor = 0;

    VkPipelineMultisampleStateCreateInfo multisampleStage{};
    multisampleStage.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStage.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStage.sampleShadingEnable = false;
    multisampleStage.minSampleShading = 0.25;

    VkPipelineDepthStencilStateCreateInfo depthStencilStage{};
    depthStencilStage.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStage.depthTestEnable = true;
    depthStencilStage.depthWriteEnable = true;
    depthStencilStage.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilStage.depthBoundsTestEnable = false;
    depthStencilStage.minDepthBounds = 0;
    depthStencilStage.maxDepthBounds = 1;
    depthStencilStage.stencilTestEnable = false;

    VkPipelineShaderStageCreateInfo fragmentShaderStage{};
    fragmentShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStage.module = fragmentShader;
    fragmentShaderStage.pName = "main";

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = true;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendStage{};
    colorBlendStage.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStage.logicOpEnable = false;
    colorBlendStage.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStage.attachmentCount = 1;
    colorBlendStage.pAttachments = &colorBlendAttachment;
    colorBlendStage.blendConstants[0] = 0;
    colorBlendStage.blendConstants[1] = 0;
    colorBlendStage.blendConstants[2] = 0;
    colorBlendStage.blendConstants[3] = 0;

    VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineShaderStageCreateInfo shaderStages[] = {
            vertexShaderStage,
            fragmentShaderStage
    };

    VkGraphicsPipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.stageCount = 2;
    createInfo.pStages = shaderStages;
    createInfo.pVertexInputState = &vertexInputStage;
    createInfo.pInputAssemblyState = &inputAssemblyStage;
    createInfo.pTessellationState = nullptr;
    createInfo.pViewportState = &viewportStage;
    createInfo.pRasterizationState = &rasterizationStage;
    createInfo.pMultisampleState = &multisampleStage;
    createInfo.pDepthStencilState = &depthStencilStage;
    createInfo.pColorBlendState = &colorBlendStage;
    createInfo.pDynamicState = &dynamicState;
    createInfo.layout = pipelineLayout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;
    createInfo.basePipelineHandle = VK_NULL_HANDLE;
    createInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(device, nullptr, 1, &createInfo, nullptr, &pipeline);

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to create Vulkan pipeline: " << result << endl;
        return { VK_NULL_HANDLE, VK_NULL_HANDLE };
    }

    return { pipelineLayout, pipeline };
}

void destroyPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline pipeline)
{
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

bool renderEye(
        Swapchain* swapchain,
        const vector<SwapchainImage*>& images,
        XrView view,
        VkDevice device,
        VkQueue queue,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout,
        VkPipeline pipeline
)
{
    XrSwapchainImageAcquireInfo acquireImageInfo{};
    acquireImageInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;

    uint32_t activeIndex;

    XrResult result = xrAcquireSwapchainImage(swapchain->swapchain, &acquireImageInfo, &activeIndex);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to acquire swapchain image: " << result << endl;
        return false;
    }

    XrSwapchainImageWaitInfo waitImageInfo{};
    waitImageInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
    waitImageInfo.timeout = numeric_limits<int64_t>::max();

    result = xrWaitSwapchainImage(swapchain->swapchain, &waitImageInfo);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to wait for swapchain image: " << result << endl;
        return false;
    }

    const SwapchainImage* image = images[activeIndex];

//    float* data;
//    VkResult vkResult = vkMapMemory(device, image->memory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
//
//    if (vkResult != VK_SUCCESS)
//    {
//        cerr << "Failed to map Vulkan memory: " << result << endl;
//    }
//
//    float angleWidth = tan(view.fov.angleRight) - tan(view.fov.angleLeft);
//    float angleHeight = tan(view.fov.angleDown) - tan(view.fov.angleUp);
//
//    float projectionMatrix[4][4]{0};
//
//    projectionMatrix[0][0] = 2.0f / angleWidth;
//    projectionMatrix[2][0] = (tan(view.fov.angleRight) + tan(view.fov.angleLeft)) / angleWidth;
//    projectionMatrix[1][1] = 2.0f / angleHeight;
//    projectionMatrix[2][1] = (tan(view.fov.angleUp) + tan(view.fov.angleDown)) / angleHeight;
//    projectionMatrix[2][2] = -farDistance / (farDistance - nearDistance);
//    projectionMatrix[3][2] = -(farDistance * nearDistance) / (farDistance - nearDistance);
//    projectionMatrix[2][3] = -1;
//
//    glm::mat4 viewMatrix = glm::inverse(
//            glm::translate(glm::mat4(1.0f), glm::vec3(view.pose.position.x, view.pose.position.y, view.pose.position.z))
//            * glm::mat4_cast(glm::quat(view.pose.orientation.w, view.pose.orientation.x, view.pose.orientation.y, view.pose.orientation.z))
//    );
//
//    float modelMatrix[4][4]{
//            { 1, 0, 0, 0 },
//            { 0, 1, 0, 0 },
//            { 0, 0, 1, 0 },
//            { 0, 0, 0, 1 }
//    };
//
//    memcpy(data, projectionMatrix, sizeof(float) * 4 * 4);
//    memcpy(data + (4 * 4), glm::value_ptr(viewMatrix), sizeof(float) * 4 * 4);
//    memcpy(data + (4 * 4) * 2, modelMatrix, sizeof(float) * 4 * 4);
//
//    vkUnmapMemory(device, image->memory);

    XrView;
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    auto vkResult = vkBeginCommandBuffer(image->commandBuffer, &beginInfo);

    VkClearValue clearValue{};
    clearValue.color = { { 0.0f, 1.0f, 1.0f, 1.0f } };

    VkRenderPassBeginInfo beginRenderPassInfo{};
    beginRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginRenderPassInfo.renderPass = renderPass;
    beginRenderPassInfo.framebuffer = image->framebuffer;
    beginRenderPassInfo.renderArea = {
            { 0, 0 },
            { (uint32_t)swapchain->width, (uint32_t)swapchain->height }
    };
    beginRenderPassInfo.clearValueCount = 1;
    beginRenderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(image->commandBuffer, &beginRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
            0, 0,
            (float)swapchain->width, (float)swapchain->height,
            0, 1
    };

    vkCmdSetViewport(image->commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
            { 0, 0 },
            { (uint32_t)swapchain->width, (uint32_t)swapchain->height }
    };

    vkCmdSetScissor(image->commandBuffer, 0, 1, &scissor);

//    vkCmdBindPipeline(image->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

//    vkCmdBindDescriptorSets(image->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &image->descriptorSet, 0, nullptr);

//    vkCmdDraw(image->commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(image->commandBuffer);

    vkResult = vkEndCommandBuffer(image->commandBuffer);

    if (vkResult != VK_SUCCESS)
    {
        cerr << "Failed to end Vulkan command buffer: " << vkResult << endl;
        return false;
    }

    VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &stageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &image->commandBuffer;

    vkResult = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    if (vkResult != VK_SUCCESS)
    {
        cerr << "Failed to submit Vulkan command buffer: " << result << endl;
        return false;
    }

    XrSwapchainImageReleaseInfo releaseImageInfo{};
    releaseImageInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;

    result = xrReleaseSwapchainImage(swapchain->swapchain, &releaseImageInfo);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to release swapchain image: " << result << endl;
        return false;
    }

    return true;
}

bool render(
        XrSession session,
        Swapchain* swapchains[2],
        vector<SwapchainImage*> swapchainImages[2],
        XrSpace space,
        XrTime predictedDisplayTime,
        VkDevice device,
        VkQueue queue,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout,
        VkPipeline pipeline
)
{
    XrFrameBeginInfo beginFrameInfo{};
    beginFrameInfo.type = XR_TYPE_FRAME_BEGIN_INFO;

    XrResult result = xrBeginFrame(session, &beginFrameInfo);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to begin frame: " << result << endl;
        return false;
    }

    XrViewLocateInfo viewLocateInfo{};
    viewLocateInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = predictedDisplayTime;
    viewLocateInfo.space = space;

    XrViewState viewState{};
    viewState.type = XR_TYPE_VIEW_STATE;

    uint32_t viewCount = eyeCount;
    vector<XrView> views(
            viewCount,
            { .type = XR_TYPE_VIEW }
    );

    result = xrLocateViews(
            session,
            &viewLocateInfo,
            &viewState,
            viewCount,
            &viewCount,
            views.data()
    );

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to locate views: " << result << endl;
        return false;
    }

    for (size_t i = 0; i < eyeCount; i++)
    {
        renderEye(
                swapchains[i],
                swapchainImages[i],
                views[i],
                device,
                queue,
                renderPass,
                pipelineLayout,
                pipeline
        );
    }

    XrCompositionLayerProjectionView projectedViews[2]{};

    for (size_t i = 0; i < eyeCount; i++)
    {
        projectedViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projectedViews[i].pose = views[i].pose;
        projectedViews[i].fov = views[i].fov;
        projectedViews[i].subImage = {
                swapchains[i]->swapchain,
                {
                        { 0, 0 },
                        { (int32_t)swapchains[i]->width, (int32_t)swapchains[i]->height }
                },
                0
        };
    }

    XrCompositionLayerProjection layer{};
    layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
    layer.space = space;
    layer.viewCount = eyeCount;
    layer.views = projectedViews;

    auto pLayer = (const XrCompositionLayerBaseHeader*)&layer;

    XrFrameEndInfo endFrameInfo{};
    endFrameInfo.type = XR_TYPE_FRAME_END_INFO;
    endFrameInfo.displayTime = predictedDisplayTime;
    endFrameInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endFrameInfo.layerCount = 1;
    endFrameInfo.layers = &pLayer;

    result = xrEndFrame(session, &endFrameInfo);

    if (result != XR_SUCCESS)
    {
        cerr << "Failed to end frame: " << result << endl;
        return false;
    }

    return true;
}

void onInterrupt(int)
{
    quit = true;
}

int main(int, char**)
{
    XrInstance instance = createInstance();
    XrDebugUtilsMessengerEXT debugMessenger = createDebugMessenger(instance);
    XrSystemId system = getSystem(instance);

    XrGraphicsRequirementsVulkanKHR graphicsRequirements;
    set<string> instanceExtensions;
    tie(graphicsRequirements, instanceExtensions) = getVulkanInstanceRequirements(instance, system);
    VkInstance vulkanInstance = createVulkanInstance(graphicsRequirements, instanceExtensions);
    VkDebugUtilsMessengerEXT vulkanDebugMessenger = createVulkanDebugMessenger(vulkanInstance);

    VkPhysicalDevice physicalDevice;
    set<string> deviceExtensions;
    tie(physicalDevice, deviceExtensions) = getVulkanDeviceRequirements(instance, system, vulkanInstance);
    int32_t graphicsQueueFamilyIndex = getDeviceQueueFamily(physicalDevice);
    VkDevice device;
    VkQueue queue;
    tie(device, queue) = createDevice(physicalDevice, graphicsQueueFamilyIndex, deviceExtensions);

    VkRenderPass renderPass = createRenderPass(device);
    VkCommandPool commandPool = createCommandPool(device, graphicsQueueFamilyIndex);
    VkDescriptorPool descriptorPool = createDescriptorPool(device);
    VkDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayout(device);
//    VkShaderModule vertexShader = createShader(device, "vertex.spv");
//    VkShaderModule fragmentShader = createShader(device, "fragment.spv");
//    VkPipelineLayout pipelineLayout;
//    VkPipeline pipeline;
//    tie(pipelineLayout, pipeline) = createPipeline(device, renderPass, descriptorSetLayout, vertexShader, fragmentShader);

    XrSession session = createSession(instance, system, vulkanInstance, physicalDevice, device, graphicsQueueFamilyIndex);

    Swapchain* swapchains[eyeCount];
    tie(swapchains[0], swapchains[1]) = createSwapchains(instance, system, session);

    vector<XrSwapchainImageVulkanKHR> swapchainImages[eyeCount];

    for (size_t i = 0; i < eyeCount; i++)
    {
        swapchainImages[i] = getSwapchainImages(swapchains[i]->swapchain);
    }

    vector<SwapchainImage*> wrappedSwapchainImages[eyeCount];

    for (size_t i = 0; i < eyeCount; i++)
    {
        wrappedSwapchainImages[i] = vector<SwapchainImage*>(swapchainImages[i].size(), nullptr);

        for (size_t j = 0; j < wrappedSwapchainImages[i].size(); j++)
        {
            wrappedSwapchainImages[i][j] = new SwapchainImage(
                    physicalDevice,
                    device,
                    renderPass,
                    commandPool,
                    descriptorPool,
                    descriptorSetLayout,
                    swapchains[i],
                    swapchainImages[i][j]
            );
        }
    }

    XrSpace space = createSpace(session);

    signal(SIGINT, onInterrupt);

    bool running = false;
    while (!quit)
    {
        XrEventDataBuffer eventData{};
        eventData.type = XR_TYPE_EVENT_DATA_BUFFER;

        XrResult result = xrPollEvent(instance, &eventData);

        if (result == XR_EVENT_UNAVAILABLE)
        {
            if (running)
            {
                XrFrameWaitInfo frameWaitInfo{};
                frameWaitInfo.type = XR_TYPE_FRAME_WAIT_INFO;

                XrFrameState frameState{};
                frameState.type = XR_TYPE_FRAME_STATE;

                XrResult result = xrWaitFrame(session, &frameWaitInfo, &frameState);

                if (result != XR_SUCCESS)
                {
                    cerr << "Failed to wait for frame: " << result << endl;
                    break;
                }

                if (!frameState.shouldRender)
                {
                    continue;
                }

                quit = !render(
                        session,
                        swapchains,
                        wrappedSwapchainImages,
                        space,
                        frameState.predictedDisplayTime,
                        device,
                        queue,
                        renderPass,
                        VK_NULL_HANDLE,
                        VK_NULL_HANDLE
                );
            }
        }
        else if (result != XR_SUCCESS)
        {
            cerr << "Failed to poll events: " << result << endl;
            break;
        }
        else
        {
            switch (eventData.type)
            {
                default :
                    cerr << "Unknown event type received: " << eventData.type << endl;
                    break;
                case XR_TYPE_EVENT_DATA_EVENTS_LOST :
                    cerr << "Event queue overflowed and events were lost." << endl;
                    break;
                case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING :
                    cout << "OpenXR instance is shutting down." << endl;
                    quit = true;
                    break;
                case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED :
                    cout << "The interaction profile has changed." << endl;
                    break;
                case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING :
                    cout << "The reference space is changing." << endl;
                    break;
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED :
                {
                    auto event = (XrEventDataSessionStateChanged*)&eventData;

                    switch (event->state)
                    {
                        case XR_SESSION_STATE_UNKNOWN :
                        case XR_SESSION_STATE_MAX_ENUM :
                            cerr << "Unknown session state entered: " << event->state << endl;
                            break;
                        case XR_SESSION_STATE_IDLE :
                            running = false;
                            break;
                        case XR_SESSION_STATE_READY :
                        {
                            XrSessionBeginInfo sessionBeginInfo{};
                            sessionBeginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
                            sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

                            result = xrBeginSession(session, &sessionBeginInfo);

                            if (result != XR_SUCCESS)
                            {
                                cerr << "Failed to begin session: " << result << endl;
                            }

                            running = true;
                            break;
                        }
                        case XR_SESSION_STATE_SYNCHRONIZED :
                        case XR_SESSION_STATE_VISIBLE :
                        case XR_SESSION_STATE_FOCUSED :
                            running = true;
                            break;
                        case XR_SESSION_STATE_STOPPING :
                            running = false;

                            result = xrEndSession(session);

                            if (result != XR_SUCCESS)
                            {
                                cerr << "Failed to end session: " << result << endl;
                            }
                            break;
                        case XR_SESSION_STATE_LOSS_PENDING :
                            cout << "OpenXR session is shutting down." << endl;
                            quit = true;
                            break;
                        case XR_SESSION_STATE_EXITING :
                            cout << "OpenXR runtime requested shutdown." << endl;
                            quit = true;
                            break;
                    }
                    break;
                }
            }
        }
    }

    VkResult result = vkDeviceWaitIdle(device);

    if (result != VK_SUCCESS)
    {
        cerr << "Failed to wait for device to idle: " << result << endl;
    }

    destroySpace(space);

    for (size_t i = 0; i < eyeCount; i++)
    {
        for (size_t j = 0; j < wrappedSwapchainImages[i].size(); j++)
        {
            delete wrappedSwapchainImages[i][j];
        }
    }

    for (size_t i = 0; i < eyeCount; i++)
    {
        delete swapchains[i];
    }

    destroySession(session);

//    destroyPipeline(device, pipelineLayout, pipeline);
//    destroyShader(device, fragmentShader);
//    destroyShader(device, vertexShader);
    destroyDescriptorSetLayout(device, descriptorSetLayout);
    destroyDescriptorPool(device, descriptorPool);
    destroyCommandPool(device, commandPool);
    destroyRenderPass(device, renderPass);

    destroyDevice(device);

    destroyVulkanDebugMessenger(vulkanInstance, vulkanDebugMessenger);
    destroyVulkanInstance(vulkanInstance);

    destroyDebugMessenger(instance, debugMessenger);
    destroyInstance(instance);

    return 0;
}
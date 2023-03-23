//
// Created by ozzadar on 2023-01-01.
//

#pragma once

#include <youtube_engine/vr/vr_subsystem.h>
#include <rendering/vulkan/vulkan_includes.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <any>
#include <set>
namespace OZZ {
    class OpenXRSubsystem : public VirtualRealitySubsystem {
        friend class VulkanRenderer;

    public:
        static inline uint32_t BAD_INDEX = 400;

        struct OpenXRVulkanSwapchain {
            XrSwapchain Swapchain { XR_NULL_HANDLE };
            VkFormat Format { VK_FORMAT_UNDEFINED };
            uint32_t Width { 0 };
            uint32_t Height { 0 };
            uint32_t BufferDepth { 0 };
            std::vector<XrSwapchainImageVulkanKHR> Images;

            ~OpenXRVulkanSwapchain() {
                if (Swapchain != XR_NULL_HANDLE) {
                    xrDestroySwapchain(Swapchain);
                }
            }

            bool IsValid() const {
                return Swapchain != XR_NULL_HANDLE && Format != VK_FORMAT_UNDEFINED &&
                    Width > 0 && Height > 0;
            }
        };

        void Init(VRSettings) override;
        void Shutdown() override;

        bool Update() override;

        bool IsInitialized() override { return _initialized; }
        std::tuple<int, int> GetSwapchainImageRectDimensions() override { return {_swapchainWidth, _swapchainHeight }; }

        VRBackend GetBackendType() override { return VRBackend::OpenXR; }


    private:
        void createInstance(const VRSettings& settings);
        void createDebugMessenger();
        void getHMDSystem();
        void getGraphicsInstanceRequirements(const VRSettings &settings);
        void createSpace();
        void createSwapchains();


        PFN_xrVoidFunction getXRFunction(std::string functionName);

        static XrBool32 handleDebugMessengerError(XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT type,
                                           const XrDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

        void handleSessionStateChanged(XrEventDataSessionStateChanged* stateChangedData);

    private:

        /* TODO: I can't think of a way to abstract away the Vulkan / OpenXR intermingling in a way that's not super annoying
            Therefore, the VulkanRenderer is a friend class and the OpenXR system is aware of Vulkan concepts. I'm attempting
            to let VulkanRenderer to not require any OpenXR specific concepts outside of this friend interface.
        */
        VkPhysicalDevice GetVulkanPhysicalDevice(VkInstance vulkanInstance);
        std::set<std::string> GetVulkanDeviceExtensions();
        std::set<std::string> GetVulkanInstanceExtensions() { return _vulkanInstanceExtensions; }
        XrGraphicsRequirementsVulkan2KHR GetVulkanGraphicsRequirements() { return _vulkanGraphicsRequirements; }
        void CreateSessionVulkan(VkInstance vulkanInstance, VkPhysicalDevice physDevice, VkDevice device, uint32_t queueFamilyIndex);
        void EndSessionVulkan();
        std::tuple<XrFrameWaitInfo, XrFrameState> WaitForNextVulkanFrame();
        std::vector<EyePoseInfo> BeginVulkanFrame(XrFrameWaitInfo, XrFrameState);
        void EndVulkanFrame(const std::vector<EyePoseInfo>& poseInfo);
        uint32_t AcquireVulkanSwapchainImage(uint32_t eyeIndex);

        size_t GetVulkanBufferDepth() { return _vulkanSwapchains.empty() ? 0 : _vulkanSwapchains.at(0).BufferDepth; }
        OpenXRVulkanSwapchain& GetVulkanSwapchain(int eyeIndex);
        void ReleaseVulkanSwapchainImage(int eyeIndex);

        void createVulkanSwapchains();
    private:
        RendererAPI _renderer;

        XrInstance _instance { XR_NULL_HANDLE };
        XrSystemId _systemId {};
        XrSession _session { XR_NULL_HANDLE };
        XrDebugUtilsMessengerEXT _debugMessenger { XR_NULL_HANDLE };

        XrSpace _space { XR_NULL_HANDLE };

        bool _initialized { false };

        // Vulkan entities
        XrGraphicsRequirementsVulkanKHR _vulkanGraphicsRequirements;
        std::set<std::string> _vulkanInstanceExtensions {};
        std::vector<OpenXRVulkanSwapchain> _vulkanSwapchains {};
        XrTime _nextPredictedDisplayTime { 0 };

        bool _ready { false };
        inline static const size_t EyeCount = 2;

        int _swapchainWidth {0};
        int _swapchainHeight {0};
    };

}

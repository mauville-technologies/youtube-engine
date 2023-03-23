//
// Created by ozzadar on 2022-12-25.
//

#pragma once
#include <youtube_engine/platform/window.h>
#include <youtube_engine/platform/serializable.h>
#include <youtube_engine/rendering/renderer.h>
#include <unordered_map>
#include <any>

namespace OZZ {
    enum class EngineSetting {
        WindowType,
        WindowDisplayMode,
        ResolutionX,
        ResolutionY,
        VR,
        RendererAPI
    };

    struct EngineConfiguration : Serializable {
        WindowType WinType { WindowType::GLFW };

        WindowDisplayMode WinDisplayMode { WindowDisplayMode::Windowed };
        uint32_t ResX { 800 };
        uint32_t ResY { 600 };

        bool VR { false };
        RendererAPI Renderer {RendererAPI::Vulkan };

        nlohmann::json ToJson() override {
            nlohmann::json json;
            json["windowType"] = static_cast<int>(WinType);
            json["windowDisplayMode"] = static_cast<int>(WinDisplayMode);
            json["resX"] = ResX;
            json["resY"] = ResY;
            json["vr"] = VR;
            json["rendererAPI"] = static_cast<int>(Renderer);
            return json;
        }

        void FromJson(const nlohmann::json& inJson) override {
            WinType = static_cast<WindowType>(inJson["windowType"]);
            WinDisplayMode = static_cast<WindowDisplayMode>(inJson["windowDisplayMode"]);
            ResX = inJson["resX"];
            ResY = inJson["resY"];
            VR = inJson["vr"];
            Renderer = inJson["rendererAPI"];
        }
    };

    class Configuration {
    public:
        using EngineSettingChangeCallbackFunc = std::function<void()>;

        struct EngineSettingChangeCallback {
            std::string Ref;
            EngineSettingChangeCallbackFunc Func;
        };

        virtual void Init() = 0;
        virtual const EngineConfiguration& GetEngineConfiguration() = 0;
        virtual const std::unordered_map<std::string, std::any>& GetUserConfiguration() = 0;

        virtual void SetEngineSetting(EngineSetting, std::any) = 0;
        virtual void ListenForEngineSettingChange(EngineSetting, EngineSettingChangeCallback func) = 0;
        virtual void StopListeningForEngineSettingChange(EngineSetting, std::string) = 0;

        virtual void WriteConfig() = 0;
        virtual void ReadConfig() = 0;
    };
}


//
// Created by ozzadar on 2022-12-25.
//

#pragma once
#include <youtube_engine/platform/window.h>
#include <unordered_map>
#include <any>
#include <youtube_engine/platform/serializable.h>

namespace OZZ {
    enum class EngineSetting {
        WindowType,
        WindowDisplayMode,
        ResolutionX,
        ResolutionY,
        VR,
    };

    struct EngineConfiguration : Serializable {
        WindowType WinType { WindowType::GLFW };

        WindowDisplayMode WinDisplayMode { false };
        uint32_t ResX { 1920 };
        uint32_t ResY { 1080 };

        bool VR { false };

        nlohmann::json ToJson() override {
            nlohmann::json json;
            json["windowType"] = static_cast<int>(WinType);
            json["windowDisplayMode"] = static_cast<int>(WinDisplayMode);
            json["resX"] = ResX;
            json["resY"] = ResY;
            json["vr"] = VR;
            return json;
        }

        void FromJson(const nlohmann::json& inJson) override {
            WinType = static_cast<WindowType>(inJson["windowType"]);
            WinDisplayMode = static_cast<WindowDisplayMode>(inJson["windowDisplayMode"]);
            ResX = inJson["resX"];
            ResY = inJson["resY"];
            VR = inJson["vr"];
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


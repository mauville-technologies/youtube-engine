//
// Created by ozzadar on 2022-12-25.
//

#pragma once
#include <youtube_engine/platform/window.h>
#include <unordered_map>
#include <any>
#include <youtube_engine/platform/serializable.h>

namespace OZZ {
    struct EngineConfiguration : Serializable {
        WindowType WinType { WindowType::GLFW };

        bool Fullscreen { false };
        uint32_t ResX { 800 };
        uint32_t ResY { 600 };

        bool VR { false };

        nlohmann::json ToJson() override {
            nlohmann::json json;
            json["windowType"] = static_cast<int>(WinType);
            json["fullscreen"] = Fullscreen;
            json["resX"] = ResX;
            json["resY"] = ResY;
            json["vr"] = VR;
            return json;
        }

        void FromJson(const nlohmann::json& inJson) override {
            WinType = static_cast<WindowType>(inJson["windowType"]);
            Fullscreen = inJson["fullscreen"];
            ResX = inJson["resX"];
            ResY = inJson["resY"];
            VR = inJson["vr"];
        }
    };

    class Configuration {
    public:
        virtual void Init() = 0;
        virtual const EngineConfiguration& GetEngineConfiguration() = 0;
        virtual const std::unordered_map<std::string, std::any>& GetUserConfiguration() = 0;
        virtual void WriteConfig() = 0;
        virtual void ReadConfig() = 0;
    };

}


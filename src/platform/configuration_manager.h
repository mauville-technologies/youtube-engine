//
// Created by ozzadar on 2022-12-25.
//

#pragma once
#include <youtube_engine/platform/configuration.h>
#include <youtube_engine/platform/filesystem.h>

namespace OZZ {

    class ConfigurationManager : public Configuration {
    public:
        ConfigurationManager() = default;

        void Init() override;

        const EngineConfiguration &GetEngineConfiguration() override;

        const std::unordered_map<std::string, std::any>& GetUserConfiguration() override;

        void WriteConfig() override;

        void ReadConfig() override;

        void SetEngineSetting(EngineSetting setting, std::any) override;

        void ListenForEngineSettingChange(EngineSetting setting, EngineSettingChangeCallback callback) override;

        void StopListeningForEngineSettingChange(EngineSetting setting, std::string callbackRef) override;

    private:
        void broadcastSettingChange(EngineSetting);

        Path ConfigFilename {};
        EngineConfiguration _engineConfig {};
        std::unordered_map<std::string, std::any> _userConfig {};

        std::unordered_map<EngineSetting, std::vector<EngineSettingChangeCallback>> _listeners {};
    };
}
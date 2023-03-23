//
// Created by ozzadar on 2022-12-25.
//

#include <platform/configuration_manager.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>

namespace OZZ {
    void ConfigurationManager::Init() {
        ConfigFilename = Filesystem::GetAppUserDataDirectory() / "config.ozzcfg";

        if (!Filesystem::DoesFileExist(Filesystem::GetAppUserDataDirectory())) {
            std::cout << "Created directory: " << static_cast<int>(Filesystem::CreateDirectory(Filesystem::GetAppUserDataDirectory())) << std::endl;
        }

        // First check if the configuration file exists
        if (Filesystem::DoesFileExist(ConfigFilename)) {
            // if they do, read the values and store engine and user values.
            ReadConfig();
        } else {
            // if not, create it with default values and store those defaults
            _engineConfig = {};
            WriteConfig();
        }
    }

    const EngineConfiguration &ConfigurationManager::GetEngineConfiguration() {
        return _engineConfig;
    }

    const std::unordered_map<std::string, std::any>& ConfigurationManager::GetUserConfiguration() {
        return _userConfig;
    }

    void ConfigurationManager::WriteConfig() {
        std::cout << "Writing config" << std::endl;

        // open the file
        std::ofstream outFile {ConfigFilename, std::ios_base::trunc};

        if (!outFile.is_open()) {
            std::cout << "Failed to open config file for writing." << std::endl;
            return;
        }
        // build the json object
        nlohmann::json jsonFile;

        // add environment
        jsonFile["engine"] = _engineConfig.ToJson();

        // we make it pretty to allow users to change it easily.
        outFile << std::setw(4) << jsonFile;

        outFile.close();
    }

    void ConfigurationManager::ReadConfig() {
        std::ifstream inFile {ConfigFilename};
        nlohmann::json jsonFile = nlohmann::json::parse(inFile);

        _engineConfig.FromJson(jsonFile["engine"]);
    }

    void ConfigurationManager::SetEngineSetting(EngineSetting setting, std::any value) {
        try {
            switch (setting) {
                case EngineSetting::WindowType: {
                    auto newValue = std::any_cast<WindowType>(value);

                    if (newValue != _engineConfig.WinType) {
                        _engineConfig.WinType = newValue;
                        broadcastSettingChange(setting);
                    }

                    break;
                }
                case EngineSetting::WindowDisplayMode: {
                    auto newValue = std::any_cast<WindowDisplayMode>(value);

                    if (newValue != _engineConfig.WinDisplayMode) {
                        _engineConfig.WinDisplayMode = newValue;
                        broadcastSettingChange(setting);
                    }

                    break;
                }
                case EngineSetting::ResolutionX: {
                    auto newValue = std::any_cast<uint32_t>(value);

                    if (newValue != _engineConfig.ResX) {
                        _engineConfig.ResX = newValue;
                        broadcastSettingChange(setting);
                    }

                    break;
                }
                case EngineSetting::ResolutionY: {
                    auto newValue = std::any_cast<uint32_t>(value);

                    if (newValue != _engineConfig.ResY) {
                        _engineConfig.ResY = newValue;
                        broadcastSettingChange(setting);
                    }

                    break;
                }
                case EngineSetting::VR: {
                    auto newValue = std::any_cast<bool>(value);

                    if (newValue != _engineConfig.VR) {
                        _engineConfig.VR = newValue;
                        broadcastSettingChange(setting);
                    }

                    break;
                }
                case EngineSetting::RendererAPI: {
                    auto newValue = std::any_cast<RendererAPI>(value);

                    if (newValue != _engineConfig.Renderer) {
                        _engineConfig.Renderer = newValue;
                        broadcastSettingChange(setting);
                    }

                    break;
                }
            }

            WriteConfig();
        } catch (std::bad_any_cast& e) {
            std::cout << "Invalid type supplied for EngineSetting " << static_cast<int>(setting) << std::endl;
        }
    }

    void ConfigurationManager::ListenForEngineSettingChange(EngineSetting setting,
                                                            Configuration::EngineSettingChangeCallback callback) {
        _listeners[setting].push_back(callback);
    }

    void ConfigurationManager::StopListeningForEngineSettingChange(EngineSetting setting, std::string callbackRef) {
        erase_if(_listeners[setting], [callbackRef](const EngineSettingChangeCallback& callback) {
            return callback.Ref == callbackRef;
        });
    }

    void ConfigurationManager::broadcastSettingChange(EngineSetting setting) {
        for (auto& callback : _listeners[setting]) {
            callback.Func();
        }
    }

}

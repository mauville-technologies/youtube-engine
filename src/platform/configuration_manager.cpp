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
}

//
// Created by ozzadar on 2022-12-25.
//
#pragma once
#include <nlohmann/json.hpp>

namespace OZZ {
    struct Serializable {
        virtual nlohmann::json ToJson() = 0;
        virtual void FromJson(const nlohmann::json& inJson) = 0;
    };
}

//
// Created by ozzadar on 2021-08-12.
//

#pragma once
#include <functional>
#include <youtube_engine/input/input_key.h>

namespace OZZ {
    struct InputDeviceState {
        float value { -99.f };
    };

    using InputDeviceStateCallbackFunc = std::function<std::unordered_map<InputKey, InputDeviceState>(int)>;

    struct InputDevice {
        InputSource Source;
        int Index;
        std::unordered_map<InputKey, InputDeviceState> CurrentState;
        InputDeviceStateCallbackFunc StateFunc;
    };
}


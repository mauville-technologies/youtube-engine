//
// Created by ozzadar on 2021-08-12.
//

#include "youtube_engine/input/input_manager.h"

#include <iostream>

namespace OZZ {
    InputSource GetInputSourceFromKey(InputKey key) {
        switch (key) {
        case InputKey::KEY_A:
        case InputKey::KEY_B:
        case InputKey::KEY_C:
        case InputKey::KEY_D:
        case InputKey::KEY_E:
            return InputSource::KEYBOARD;
        case InputKey::GAMEPAD_L_THUMB_X:
        case InputKey::GAMEPAD_L_THUMB_Y:
        case InputKey::GAMEPAD_R_THUMB_X:
        case InputKey::GAMEPAD_R_THUMB_Y:
        case InputKey::GAMEPAD_R_TRIGGER:
        case InputKey::GAMEPAD_L_TRIGGER:
        case InputKey::GAMEPAD_Y:
        case InputKey::GAMEPAD_X:
        case InputKey::GAMEPAD_B:
        case InputKey::GAMEPAD_A:
        case InputKey::GAMEPAD_START:
        case InputKey::GAMEPAD_SELECT:
        case InputKey::GAMEPAD_BUMPER_R:
        case InputKey::GAMEPAD_BUMPER_L:
        case InputKey::GAMEPAD_L3:
        case InputKey::GAMEPAD_R3:
        case InputKey::GAMEPAD_DPAD_UP:
        case InputKey::GAMEPAD_DPAD_RIGHT:
        case InputKey::GAMEPAD_DPAD_LEFT:
        case InputKey::GAMEPAD_DPAD_DOWN:
            return InputSource::GAMEPAD;
        case InputKey::MOUSE_LEFT:
        case InputKey::MOUSE_RIGHT:
        case InputKey::MOUSE_MIDDLE:
        case InputKey::MOUSE_MOVE_X:
        case InputKey::MOUSE_MOVE_Y:
            return InputSource::MOUSE;
        default:
            return InputSource::UNKNOWN;
        }
    }

    InputManager::InputManager() {
        _active = true;
        std::cout << "Input Manager intialized!\n";
    }

    InputManager::~InputManager() {
        _active = false;
    }

    void InputManager::RegisterActionCallback(const std::string& actionName, const InputManager::ActionCallback& callback) {
        _actionCallbacks[actionName].emplace_back(callback);
    }

    void InputManager::RemoveActionCallback(const std::string& actionName, const std::string& callbackRef) {
        erase_if(_actionCallbacks[actionName], [callbackRef](const ActionCallback& callback) {
            return callback.Ref == callbackRef;
        });
    }

    void InputManager::MapInputToAction(InputKey key, const InputAction& action) {
        // TODO: Check for duplicates
        _inputActionMapping[key].emplace_back(action);
    }

    void InputManager::UnmapInputFromAction(InputKey key, const std::string& action) {
        erase_if(_inputActionMapping[key], [action](const InputAction& inputAction) {
            return inputAction.actionName == action;
        });
    }

    void InputManager::processInput() {
        std::vector<ActionEvent> events {};
        for (auto& device : _devices) {
            // get new state for device
            auto newState = device.StateFunc(device.Index);

            // compare to old state for device
            for (auto& keyState : newState) {
                if (device.CurrentState[keyState.first].value != keyState.second.value) {
                    //TODO: Fix cases where conflicting mappings -- additive fashion?
                    auto generatedEvents = generateActionEvent(device.Index, keyState.first,keyState.second.value);
                    events.insert(events.end(), generatedEvents.begin(), generatedEvents.end());
                    // save new state value
                    device.CurrentState[keyState.first].value = keyState.second.value;
                }
            }
        }

        // propagate action events
        for (auto& event : events) {
            propagateActionEvent(event);
        }
    }

    std::vector<InputManager::ActionEvent> InputManager::generateActionEvent(int DeviceIndex, InputKey key, float newVal) {
        auto& actions = _inputActionMapping[key];

        std::vector<ActionEvent> actionEvents {};

        InputSource source = GetInputSourceFromKey(key);

        for (auto& action : actions) {
            actionEvents.emplace_back(ActionEvent {
                .ActionName = action.actionName,
                .Source = source,
                .SourceIndex = DeviceIndex,
                .Value = newVal * action.scale
            });
        }

        return actionEvents;
    }

    void InputManager::propagateActionEvent(ActionEvent event) {
        for (size_t i = _actionCallbacks[event.ActionName].size() - 1; i >= 0; i--) {
            auto& actionCallback = _actionCallbacks[event.ActionName][i];

            if (actionCallback.Func(event.Source, event.SourceIndex, event.Value)) break;
        }
    }

    void InputManager::RegisterDevice(const InputDevice& device) {
        std::cout << "Device registered of type: " << static_cast<int>(device.Type) << std::endl;
        _devices.emplace_back(device);
        std::cout << "Device #: " << _devices.size() << std::endl;
    }

    void InputManager::RemoveDevice(InputDeviceType type, int inputIndex) {
        erase_if(_devices, [type, inputIndex](const InputDevice& device) {
            return device.Type == type && device.Index == inputIndex;
        });
        std::cout << "Device unregistered of type: " << static_cast<int>(type) << std::endl;
        std::cout << "Device #: " << _devices.size() << std::endl;
    }
}
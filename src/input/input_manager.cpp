//
// Created by ozzadar on 2021-08-12.
//

#include "youtube_engine/input/input_manager.h"

#include <iostream>

namespace OZZ {
    InputSource GetInputSourceFromKey(InputKey key) {

        if (key < InputKey::KeyMax) {
            return InputSource::Keyboard;
        } else if (key < InputKey::ControllerMax) {
            return InputSource::Controller;
        } else if (key < InputKey::MouseMax) {
            return InputSource::Mouse;
        }

        return InputSource::Unknown;
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
        _inputKeyMapping[key].emplace_back(action);
        _inputActionMapping[action.ActionName].emplace_back(key);
    }

    void InputManager::UnmapInputFromAction(InputKey key, const std::string& action) {
        erase_if(_inputKeyMapping[key], [action](const InputAction& inputAction) {
            return inputAction.ActionName == action;
        });

        erase_if(_inputActionMapping[action], [key](const InputKey& inputKey) {
            return key == inputKey;
        });
    }

    float InputManager::GetActionValue(const std::string &actionName) {
        // TODO: This will need to be able to specify device indexes or sources or something later?
        auto& keys = _inputActionMapping[actionName];

        if (keys.empty()) {
            return 0.f;
        }

        // Get the largest value and return it
        float value = 0.f;
        InputKey theKey = InputKey::Unknown;

        for (auto& key : keys) {
            // get the device
            auto device = GetInputSourceFromKey(key);

            // search for devices with that value
            for (auto &iDevice: _devices) {
                if (iDevice.Source == device) {
                    // check value of key
                    auto state = iDevice.StateFunc(iDevice.Index);
                    auto newVal = state[key].value;

                    if (std::abs(newVal) > std::abs(value)) {
                        value = newVal;
                        theKey = key;
                    }
                }
            }
        }

        if (theKey == InputKey::Unknown) {
            return 0.f;
        }

        // get the weight for that key
        float weight = 1.f;

        for (auto& action : _inputKeyMapping[theKey]) {
            if (action.ActionName == actionName) {
                weight = action.Scale;
                break;
            }
        }

        return value * weight;
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
        auto& actions = _inputKeyMapping[key];

        std::vector<ActionEvent> actionEvents {};

        InputSource source = GetInputSourceFromKey(key);

        for (auto& action : actions) {
            actionEvents.emplace_back(ActionEvent {
                .ActionName = action.ActionName,
                .Source = source,
                .SourceIndex = DeviceIndex,
                .Value = newVal * action.Scale
            });
        }

        return actionEvents;
    }

    void InputManager::propagateActionEvent(const ActionEvent& event) {
        if (size_t i = _actionCallbacks[event.ActionName].empty()) return;
        for (size_t i = _actionCallbacks[event.ActionName].size() - 1; i >= 0; i--) {
            auto& actionCallback = _actionCallbacks[event.ActionName][i];

            if (actionCallback.Func(event.Source, event.SourceIndex, event.Value)) break;
        }
    }

    void InputManager::RegisterDevice(const InputDevice& device) {
        std::cout << "Device registered of type: " << static_cast<int>(device.Source) << std::endl;
        _devices.emplace_back(device);
        std::cout << "Device #: " << _devices.size() << std::endl;
    }

    void InputManager::RemoveDevice(InputSource type, int inputIndex) {
        erase_if(_devices, [type, inputIndex](const InputDevice& device) {
            return device.Source == type && device.Index == inputIndex;
        });
        std::cout << "Device unregistered of type: " << static_cast<int>(type) << std::endl;
        std::cout << "Device #: " << _devices.size() << std::endl;
    }
}
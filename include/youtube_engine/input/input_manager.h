//
// Created by ozzadar on 2021-08-12.
//

#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <functional>
#include <youtube_engine/input/input_key.h>
#include <youtube_engine/input/input_devices.h>


namespace OZZ {
    class InputManager {
    public:
        using ActionCallbackFunc = std::function<bool(InputSource, int, float)>;

        struct ActionCallback {
            std::string Ref;
            ActionCallbackFunc Func;
        };

    private:
        struct ActionEvent {
            std::string ActionName;
            InputSource Source;
            int SourceIndex;
            float Value;
        };

    public:
        InputManager();
        ~InputManager();

        void RegisterDevice(const InputDevice& device);
        void RemoveDevice(InputSource source, int inputIndex);

        void RegisterActionCallback(const std::string &actionName, const ActionCallback& callback);
        void RemoveActionCallback(const std::string& actionName, const std::string& callbackRef);

        float GetActionValue(const std::string& actionName);

        void MapInputToAction(InputKey key, const InputAction& action);
        void UnmapInputFromAction(InputKey key, const std::string& action);

    private:
        friend class Game;

        // processInput will get new device state and compare with old state; then generate action events
        void processInput();

        std::vector<ActionEvent> generateActionEvent(int deviceIndex, InputKey key, float newVal);
        void propagateActionEvent(const ActionEvent& event);


    private:
        bool _active { false };

        std::unordered_map<InputKey, std::vector<InputAction>> _inputKeyMapping {};

        std::unordered_map<std::string, std::vector<InputKey>> _inputActionMapping {};
        std::unordered_map<std::string, std::vector<ActionCallback>> _actionCallbacks {};

        std::vector<InputDevice> _devices;
    };
}



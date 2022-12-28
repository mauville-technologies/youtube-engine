//
// Created by ozzadar on 2021-05-29.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <string>
#include <youtube_engine/core/scene.h>
#include <chrono>

namespace OZZ {
    class Game {
    public:
        Game();

        explicit Game(std::string windowTitle);

        ~Game();

        // Will run the main game
        void Run(int argc, char **argv);

        std::string GetTitle() { return _title; }
        void Quit() { _running = false; }
    protected:
        Scene* GetScene() { return _currentScene.get(); }
        virtual void Init() {};
        virtual void PhysicsUpdate(float deltaTime) {};
        virtual void Update(float deltaTime) {};
        virtual void OnExit() {};

        virtual void ResetRenderer();
    private:
        void initializeServices();

        void shutdownServices();

    public:

    private:
        std::string _title {"Default Ozz Game"};
        bool _running;
        bool _rendererResetRequested { false };

        std::unique_ptr<Scene> _currentScene {};

        std::chrono::time_point<std::chrono::high_resolution_clock> _lastFrameTime {};
    };

    extern Game *CreateGame();
}
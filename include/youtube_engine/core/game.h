//
// Created by ozzadar on 2021-05-29.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <string>
#include <youtube_engine/core/scene.h>

namespace OZZ {
    class Game {
    public:
        Game();

        explicit Game(std::string windowTitle);

        ~Game();

        // Will run the main game
        void Run();

    protected:
        Scene* GetScene() { return _currentScene.get(); }
        virtual void PhysicsUpdate(float deltaTime) {};

        virtual void Update(float deltaTime) {};
        virtual void Render() {};
        virtual void OnExit() {};

    private:
        void initializeServices();

        void shutdownServices();

    public:

    private:
        std::string _title;
        bool _running;

        std::unique_ptr<Scene> _currentScene {};
    };

    extern Game *CreateGame();
}
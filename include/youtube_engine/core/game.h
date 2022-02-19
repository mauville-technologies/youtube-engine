//
// Created by ozzadar on 2021-05-29.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <string>
namespace OZZ {
    class Game {
    public:
        Game();

        explicit Game(std::string windowTitle);

        ~Game();

        // Will run the main game
        void Run();

    protected:
        virtual void PhysicsUpdate(float deltaTime) {};

        virtual void Update(float deltaTime) {};

    private:
        void initializeServices();

        void shutdownServices();

    public:


    private:
        std::string _title;
        bool _running;
    };

    extern Game *CreateGame();
}
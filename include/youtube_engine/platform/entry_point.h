//
// Created by ozzadar on 2021-05-29.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <youtube_engine/service_locator.h>

// Keep headers
#include <youtube_engine/core/game.h>

int main(int argc, char **argv) {
    // Create the game
    auto* theGame = OZZ::CreateGame();

    // Run the game
    theGame->Run();

    // once it's done, delete the game
    delete theGame;
}
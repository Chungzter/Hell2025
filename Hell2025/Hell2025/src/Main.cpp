/*

███    █▄  ███▄▄▄▄    ▄█        ▄██████▄   ▄█    █▄     ▄████████ ████████▄
███    ███ ███▀▀▀██▄ ███       ███    ███ ███    ███   ███    ███ ███   ▀███
███    ███ ███   ███ ███       ███    ███ ███    ███   ███    █▀  ███    ███
███    ███ ███   ███ ███       ███    ███ ███    ███  ▄███▄▄▄     ███    ███
███    ███ ███   ███ ███       ███    ███ ███    ███ ▀▀███▀▀▀     ███    ███
███    ███ ███   ███ ███       ███    ███ ███    ███   ███    █▄  ███    ███
███    ███ ███   ███ ███▌    ▄ ███    ███ ███    ███   ███    ███ ███   ▄███
████████▀   ▀█   █▀  █████▄▄██  ▀██████▀   ▀██████▀    ██████████ ████████▀

*/

#include "Hell/Backend/BackEnd.h"
#include "Unloved/Unloved.h"
#include "Renderer/Renderer.h"
#include <iostream>
#include "Hell/Logging.h"

#include "API/Vulkan/vk_backend.h"

#include "Hell/AssetLoader/AssetLoader.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

/*
int main2() {
    VulkanBackEnd::Init();
    return 0;
}*/

int main() {
    std::cout << "\x1B[2J\x1B[H";
    std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";

    Logging::EnableLevel(Logging::Level::INIT);
    Logging::EnableLevel(Logging::Level::DEBUG);
    Logging::EnableLevel(Logging::Level::ERROR);
    Logging::EnableLevel(Logging::Level::FATAL);
    Logging::EnableLevel(Logging::Level::TODO);
    Logging::EnableLevel(Logging::Level::WARNING);
	Logging::EnableLevel(Logging::Level::FUNCTION);
	Logging::EnableLevel(Logging::Level::SUPPORT);

    // Init the back-end, sub-systems, and the minimum to render loading screen
    if (!Hell::BackEnd::Init(API::OPENGL, WindowedMode::WINDOWED, "Unloved")) {
        std::cout << "Hell::BackEnd::Init() FAILED!\n";
        return -1;
    }
    if (!Unloved::Init()) {
        std::cout << "Unloved::Init() FAILED!\n";
        return -1;
    }

    // Program loop
    while (Hell::BackEnd::WindowIsOpen()) {

        Hell::BackEnd::BeginFrame();
        Unloved::UpdateSubSystems();
        Unloved::BeginFrame();

        // Render loading screen
        if (!Hell::AssetLoader::LoadingComplete()) {
            Hell::AssetLoader::Update();
            Unloved::UpdateLoadingScreen();
            Renderer::RenderLoadingScreen();

            // Loading complete?
            if (Hell::AssetLoader::LoadingComplete()) {
                Unloved::OnAssetLoadingComplete();
            }
        }
        // Update/render game
        else {
            //Timer timer("GameLoop");
            Unloved::Update();
            Unloved::Render();
        }
        Unloved::EndFrame();
        Hell::BackEnd::EndFrame();
    }

    Unloved::CleanUp();
    Hell::BackEnd::CleanUp();
    return 0;
}

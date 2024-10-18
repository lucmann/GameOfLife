#include <gui/interface.hpp>
#include <presets/modelpresets.hpp>


#include <iostream>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

//#include <SDL3/SDL.h>

void Interface::startDraw(
    bool& surfClear,
    int& desiredModelFPS,
    const int measuredModelFPS) 
{
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

    ImGui::Begin("Options");
    ImGui::SliderInt("Desired Model FPS", &desiredModelFPS, 1, 1000);
    ImGui::Text("Measured FPS: %d", measuredModelFPS);
    if (ImGui::SmallButton("Clear"))
        surfClear = true;
}

void Interface::endDraw(SDL_Renderer* renderer) {
    ////for testing:
    //SDL_GetCurrentVideoDriver();

    ImGui::End();
	ImGui::Render();


	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

Interface::~Interface() {

}
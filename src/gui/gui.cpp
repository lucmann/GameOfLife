#include "gui.hpp"

#include <iostream>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <imgui.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_video.h>
//#include <SDL3/SDL_timer.h>


GUI::GUI() {};

bool GUI::initialize(
	std::string windowTitle
)
{
	mainWindow.initialize(windowTitle.c_str());

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	std::string path;
	std::string file(__FILE__);
	std::replace(file.begin(), file.end(), '\\', '/');
	std::string::size_type lastSlash = file.rfind("/");
	file = file.substr(0, lastSlash);
	path = file + "/../../resources/fonts/JetBrainsMono-Regular.ttf";

	io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f);
	io.Fonts->Build();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
	io.FontGlobalScale = 1;

	if (!ImGui_ImplSDL3_InitForSDLRenderer(mainWindow.sdlWindow, mainWindow.sdlRenderer)) return false;//fails... mainwindow does not have a renderer
	if (!ImGui_ImplSDLRenderer3_Init(mainWindow.sdlRenderer)) return false;

	return true;
};

void GUI::shutdown()
{
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

GUI::~GUI() 
{
	
};
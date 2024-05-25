#include <core.hpp>

#include <iostream>
#include <vector>

#include <SDL2/SDL.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui.h>

Core::Core() {
}

bool Core::run() {
    if(!init()) return false;
    int lastDisplayUpdate, lastModelUpdate, now;
    now = lastDisplayUpdate = lastModelUpdate = SDL_GetTicks();
    //int lastTime = SDL_GetTicks();//returns milliseconds 

    while (coreAppRunning_) {  
        now = SDL_GetTicks();
        //if(now - lastTime < 1000/desiredFPS_) continue;
        //measuredFPS_ = 1000/(now - lastTime);
        //lastTime = now;

        processEvents();
        if (modelRunning_ && now - lastModelUpdate > 1000 / modelFPS_)
        {
            update();
            measuredModelFPS_ = 1000 / (now - lastModelUpdate);
            lastModelUpdate = now;
        }
        if (now - lastDisplayUpdate > 1000 / displayFPS_)
        {
            render();
            lastDisplayUpdate = now;
        }
        //wait function so we don't burn too much cpu
        int waitTime = std::min(1000 / displayFPS_ - (now - lastDisplayUpdate), 1000 / modelFPS_ - (now - lastModelUpdate));
        if (waitTime > 0) SDL_Delay(waitTime);
    }

    return true;
}

bool Core::init() {

    //Configure SDL2
    //Temporary surface 
    //surface_ = SDL_LoadBMP("C:/src/GameOfLife/resources/fern.bmp");
    surface_ = generateModelSurface(matrixWidth_, matrixHeight_, fillFactor_);

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Error intializing SDL2: " << SDL_GetError() << std::endl;
        coreAppRunning_ = false;
        return false;
    }
    std::cout << "SDL2 video initialized..." << std::endl;
    
    window_ = SDL_CreateWindow(
        "GameOfLife", 
        50, 
        50, 
        800, 
        600, 
        SDL_WINDOW_RESIZABLE);
        
    renderer_ = SDL_CreateRenderer(
        window_, 
        -1, 
        SDL_RENDERER_ACCELERATED);

    //Configure ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer2_Init(renderer_);

    coreAppRunning_ = true;
    return true;
}

void Core::processEvents() {
    SDL_Event event;

    while(SDL_PollEvent(&event)) {
        //SDL 
        switch(event.type)
        {
            case SDL_QUIT:
                coreAppRunning_ = false;
                break;
            case SDL_KEYDOWN:
                handleSDL_KEYDOWN(event);
                break;
        }
        //ImGui
        ImGui_ImplSDL2_ProcessEvent(&event);
    }
}

void Core::update() {
    SDL_Surface* previousState = SDL_ConvertSurface(surface_, surface_->format, 0);

    for (int row = 0; row < matrixHeight_; row++) {
        int livingNeighbors = 0;
        bool cellAlive = false;
        for (int column = 0; column < matrixWidth_; column++) {
            cellAlive = *((Uint8*)previousState->pixels + row * previousState->pitch + column);
            livingNeighbors = 0;
            for (int neighborRow = -1; neighborRow <= 1; neighborRow++) {
                for (int neighborColumn = -1; neighborColumn <= 1; neighborColumn++) {
                    //out of range rows
                    if (row + neighborRow < 0 || row + neighborRow >= matrixHeight_) continue;
                    //out of range columns
                    if (column + neighborColumn < 0 || column + neighborColumn >= matrixWidth_) continue;
                    ////center pixel
                    if (neighborRow == 0 && neighborColumn == 0) continue;
                    //count
                    if (*((Uint8*)previousState->pixels + ((row + neighborRow) * previousState->pitch) + column + neighborColumn) == 1) livingNeighbors++;
                }
            }
            
            //If not alive and has 3 neighbors, become alive
            if (!cellAlive) {
                if (livingNeighbors == 3) *((Uint8*)surface_->pixels + row * surface_->pitch + column) = 1;
            }
            //If neighbors are less than 2 or more than 3, kill it.
            else  if(livingNeighbors < 2 || livingNeighbors > 3) *((Uint8*)surface_->pixels + row * surface_->pitch + column) = 0;
		}  
        
    }
    SDL_FreeSurface(previousState);
}

void Core::render() {
    int windowWidth, windowHeight;
    SDL_GetRendererOutputSize(renderer_, &windowWidth, &windowHeight);
    SDL_Rect destinationRect{0, 0, windowWidth, windowWidth};

    //IMGUI
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Options");
    ImGui::SliderInt("Desired Model FPS", &modelFPS_, 1, 120);
    ImGui::Text("Measured FPS: %d", measuredModelFPS_);
    if (modelRunning_) {
        if (ImGui::Button("Pause Model")) {
            modelRunning_ = false;
        }
    } else {
        ImGui::SliderFloat("Model Fill Factor", &fillFactor_, 0.001, 1);
        if (ImGui::Button("Generate Model")) {
            surface_ = generateModelSurface(matrixWidth_, matrixHeight_, fillFactor_);
        }
        ImGui::SameLine();
        if (ImGui::Button("Start Model")) {
            modelRunning_ = true;
        }
    }

    ImGuiInputTextFlags modelInputFlags = modelRunning_ ? ImGuiInputTextFlags_ReadOnly : 0;

    //TODO:: limit to positive values
    //TODO:: ensure that input is 4-byte aligned
    ImGui::InputInt("Width", &matrixWidth_, 100, 100, modelInputFlags);
    ImGui::InputInt("Height", &matrixHeight_, 100, 100, modelInputFlags);

    ImGui::End();
    ImGui::Render();

    //Is this the right way, or should I be passing to an existing texture?
    //SDL_updateTexture could be the way.. 
    //I won't worry about this for now as this would change if I move to GPU. 
    auto texture = SDL_CreateTextureFromSurface(renderer_, surface_);

    SDL_SetRenderTarget(renderer_, texture);
    SDL_SetRenderDrawColor(renderer_, 0, 255, 0, 255);
    SDL_RenderClear(renderer_);
     SDL_RenderCopy(renderer_, texture, NULL, &destinationRect);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    
    SDL_RenderPresent(renderer_);

    SDL_DestroyTexture(texture);
}

void Core::handleSDL_KEYDOWN(SDL_Event& event) {
    switch(event.key.keysym.sym)
    {
        case SDLK_ESCAPE:
            coreAppRunning_ = false;
            std::cout << "Escape key pressed: Exiting application." << std::endl;
            break;
        default:
            break;
    }
}

SDL_Surface* Core::generateModelSurface(int width, int height, float fillFactor) {
    //Create and single bit surface to represent black and white values
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 1, SDL_PIXELFORMAT_INDEX8);//single bit per pixel image format
    if(surface == nullptr) {
        std::cout << "Error creating surface: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    //Set the color palette
    const SDL_Color colors[2] = {
        {0, 0, 0, 255},
        {255, 255, 255, 255}
    };
    SDL_SetPaletteColors(surface->format->palette, colors, 0, 2);

    //Fill with random data
    srand(static_cast<unsigned>(time(nullptr)));
    for(int row = 0; row < height; row++) {
        for(int column = 0; column < width; column++) {
            *((Uint8*)surface->pixels + row * surface->pitch + column) = (rand() < fillFactor * (float)RAND_MAX) ? 1 : 0;
        }
    }

    return surface;
}

//I used this for testing. I could delete it, but I'll leave it in just in case I need it in future.
SDL_Surface* Core::generateBlinkerTestSurface(){
    matrixHeight_ = 5;
    matrixWidth_ = 5;
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, matrixWidth_, matrixHeight_, 1, SDL_PIXELFORMAT_INDEX8);
    if (surface == nullptr) {
        std::cout << "Error creating surface: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    //Set the color palette
    const SDL_Color colors[2] = {
        {0, 0, 0, 255},
        {255, 255, 255, 255}
    };
    SDL_SetPaletteColors(surface->format->palette, colors, 0, 2);
    *((Uint8*)surface->pixels + 2 * surface->pitch + 1) = 1;
    *((Uint8*)surface->pixels + 2 * surface->pitch + 2) = 1;
    *((Uint8*)surface->pixels + 2 * surface->pitch + 3) = 1;

    return surface;

}

Core::~Core() {
    //Need to shutdown ImGui first
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    //shutdown SDL
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_FreeSurface(surface_);

    SDL_Quit();
}
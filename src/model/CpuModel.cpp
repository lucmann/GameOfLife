#include "CpuModel.hpp"
#include "presets/modelpresets.hpp"
#include "gui/WidgetFunctions.hpp"
#include "ImGuiScope/ImGuiScope.hpp"
#include "portable-file-dialogs/portable-file-dialogs.h"

#include <imgui.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

CpuModel::CpuModel() :
    glRenderer_(nullptr),
    gridBackBuffer_(nullptr, SDL_DestroyTexture)
{}

CpuModel::~CpuModel()
{
    glRenderer_->cleanup();
}

void CpuModel::initialize(const SDL_Rect& viewport)
{
    setViewPort(viewport);
    generateModel(activeModelParams_);
    recalcDrawRange_ = true;
}

void CpuModel::setViewPort(const SDL_Rect& viewPort)
{
	viewPort_ = viewPort;
	recalcDrawRange_ = true;
}

void CpuModel::setMouseMove(float x, float y, bool motion)
{
    if (motion)
        glRenderer_->setMotionPoint(
            x - screenSpaceDisplacementX_,
            y - screenSpaceDisplacementY_,
            activeModelParams_.zoomLevel);
    else
        glRenderer_->gatherPoint(
            x - screenSpaceDisplacementX_,
            y - screenSpaceDisplacementY_,
            activeModelParams_.zoomLevel);
    std::cout << x << ", " << y << std::endl;
}

void CpuModel::setBlendFactor(const BlendFactor& blendFactor)
{
    glRenderer_->setBlendFactor(
        blendFactor_.BlendFactorMap[static_cast<BlendFactor::Option>(blendFactor.selectedSrcColorFactorIndex)],
        blendFactor_.BlendFactorMap[static_cast<BlendFactor::Option>(blendFactor.selectedDstColorFactorIndex)],
        blendFactor_.BlendFactorMap[static_cast<BlendFactor::Option>(blendFactor.selectedSrcAlphaFactorIndex)],
        blendFactor_.BlendFactorMap[static_cast<BlendFactor::Option>(blendFactor.selectedDstAlphaFactorIndex)],
        blendFactor_.BlendEquationMap[static_cast<BlendFactor::Equation>(blendFactor.selectedBlendEquationIndex)],
        blendFactor_.constantColor
    );
}

void CpuModel::clear()
{
    glRenderer_->clearPoint();
}

void CpuModel::resizeGrid_()
{
	grid_.resize(activeModelParams_.modelHeight, std::vector<uint8_t>(activeModelParams_.modelWidth, 0));
    for (auto& row : grid_) {
		row.resize(activeModelParams_.modelWidth, 0);
	}
    recalcDrawRange_ = true;
}

void CpuModel::clearGrid_()
{
    for (auto& row : grid_) {
        row.assign(row.size(), 0);
	}
}

void CpuModel::initBackbuffer_(SDL_Renderer* renderer)
{
    gridBackBuffer_.reset(
        SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA4444,
            SDL_TEXTUREACCESS_STREAMING, // Note that must be created with this access mode
                                         // Otherwise it is not lockable which means that
                                         // SDL_Texture::pixels can't be accessed directly
            activeModelParams_.modelWidth,
            activeModelParams_.modelHeight
        )
    );
    SDL_SetTextureScaleMode(gridBackBuffer_.get(), SDL_SCALEMODE_NEAREST);
    initBackbufferRequired_ = false;

    glRenderer_.reset(new GL_Renderer(SDL_GetRenderWindow(renderer)));
    glRenderer_->prepare();
}

void CpuModel::setParameters(const ModelParameters& modelParameters)
{
	activeModelParams_ = modelParameters;
}

ModelParameters CpuModel::getParameters()
{
	return activeModelParams_;
}

void CpuModel::update()
{
    //I should just be swapping these between two allocated vectors.
    std::vector<std::vector<uint8_t>> previousState = grid_;
    int livingNeighbors = 0;
    bool cellAlive = false;
    int rowCount = grid_.size();
    int columnCount = grid_[0].size();
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
        for (int columnIndex = 0; columnIndex < columnCount; columnIndex++) {
            uint8_t* cellValue = &grid_[rowIndex][columnIndex];
            cellAlive = (*cellValue == aliveValue_) ? true: false;
            livingNeighbors = 0;
            //count living neighbors

            for (int neighborRow = -1; neighborRow <= 1; neighborRow++) {
                int neighborRowIndex = rowIndex + neighborRow;

                //wrap the rows
                if (neighborRowIndex < 0) neighborRowIndex = rowCount - 1;
                if (neighborRowIndex >= rowCount) neighborRowIndex = 0;

                for (int neighborColumn = -1; neighborColumn <= 1; neighborColumn++) {
                    //skip center pixel
                    if (neighborRow == 0 && neighborColumn == 0) continue;

                    int neighborColumnIndex = columnIndex + neighborColumn;

                    //wrap the columnsa
                    if (neighborColumnIndex < 0) neighborColumnIndex = columnCount - 1;
                    if (neighborColumnIndex >= columnCount) neighborColumnIndex = 0;

                    //count
                    if (previousState[neighborRowIndex][neighborColumnIndex] == aliveValue_) livingNeighbors++;
                }
            }

            //If not alive and has 3 neighbors, become alive
            if (!cellAlive) {
                if (livingNeighbors == activeModelParams_.rule4) *cellValue = aliveValue_;
                else *cellValue = (*cellValue >= deadValueDecrement_) ? *cellValue -= deadValueDecrement_ : 0;
            }
            //If neighbors are less than 2 or more than 3, kill it.
            else if (livingNeighbors < activeModelParams_.rule1 || livingNeighbors > activeModelParams_.rule3) {
                *cellValue = (*cellValue >= deadValueDecrement_) ? *cellValue -= deadValueDecrement_ : 0;
            }
        }
    }
}

void CpuModel::draw(SDL_Renderer* renderer)
{
    //TODO: It is redrawing the model every time. It might be better to draw it to a texture and reuse the texture if model is not updated.
    //I might even update the texture at the same time I update the model, if the view has drawn the last one. 
    //THe hard thing there is making sure that we aren't drawing the texture more often than we need. 
    //I could have a bool that says the texture needs updating before drawing and handle that in core.cpp.
    //This also shouldn't be calculated every frame.. 
    
    //This should just be what is written to activeModelParams_.displacementX and activeModelParams_.displacementY
    //For that I'll need to grab window resize events.
    if (initBackbufferRequired_) initBackbuffer_(renderer);

    if (recalcDrawRange_) {
        screenSpaceDisplacementX_ = (viewPort_.w / 2) - (activeModelParams_.modelWidth * activeModelParams_.zoomLevel / 2) + activeModelParams_.displacementX;
        screenSpaceDisplacementY_ = (viewPort_.h / 2) - (activeModelParams_.modelHeight * activeModelParams_.zoomLevel / 2) + activeModelParams_.displacementY;
        std::cout << "modelDisplacement: " << activeModelParams_.displacementX << ", " << activeModelParams_.displacementY<< std::endl;
        std::cout << "screenSpaceDisplacement: " << screenSpaceDisplacementX_ << ", " << screenSpaceDisplacementY_ << std::endl;
		drawRange_ = getDrawRange_();
		recalcDrawRange_ = false;
	}

    if (resetBlendFactor_) {
        setBlendFactor(blendFactor_);
        resetBlendFactor_ = false;
    }

    //****Drawing by swapping my backbuffer****
    // NEXT:
    //-I should have it check for changes in model, so I can skip rendering step when rendering is higher frequency than model.

    if (!gridBackBuffer_) {
        std::cout << "Invalid backbuffer!\n";
        return;
    }
    auto drawBackBufferTimer = std::make_optional<ImGuiScope::TimeScope>("Draw My Backbuffer");

    SDL_SetRenderTarget(renderer, gridBackBuffer_.get());

    // Uint16* pixels;
    // int pitch = 0;
    // SDL_LockTexture(gridBackBuffer_.get(), nullptr, (void**)&pixels, &pitch);

    // int rowCount = grid_.size();
    // int columnCount = grid_[0].size();
    // for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    // {
    //     for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
    //     //for (int columnIndex = drawRange_.columnBegin; columnIndex <= drawRange_.columnEnd; columnIndex++)
    //     {
    //         //I could optimize this a tiny bit by having pixel format defined at compile time,
    //         //and having pitch defined just one when model size changes.
    //         SDL_Color color = colorMapper_.getSDLColor(grid_[rowIndex][columnIndex]);

    //         pixels[(rowIndex ) * grid_[0].size() + columnIndex] = SDL_MapRGB(
    //             SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGB565),
    //             nullptr,
    //             color.r,
    //             color.g,
    //             color.b);
    //     }
    // }

    // SDL_UnlockTexture(gridBackBuffer_.get());

    glRenderer_->drawToSDLTexture(gridBackBuffer_.get());

    SDL_SetRenderTarget(renderer, nullptr);


    auto destRect = SDL_FRect{
        (float)screenSpaceDisplacementX_,
        (float)screenSpaceDisplacementY_,
        (float)grid_[0].size() * activeModelParams_.zoomLevel, 
        (float)grid_.size() * activeModelParams_.zoomLevel };
    SDL_RenderTexture(renderer, gridBackBuffer_.get(), nullptr, &destRect);

    drawBackBufferTimer.reset();
}

void CpuModel::drawImGuiWidgets(const bool& isModelRunning)
{
    WidgetFunctions::drawGOLRulesHeader(
        activeModelParams_, 
        [this](const ModelParameters& params) {generateModel(params);},
        isModelRunning);

    WidgetFunctions::drawBlendFuncHeader(blendFactor_, resetBlendFactor_);

    WidgetFunctions::drawVisualizationHeader(
		activeModelParams_,
		colorMapper_,
		deadValueDecrement_,
        recalcDrawRange_,
		isModelRunning);
    
    WidgetFunctions::drawPresetsHeader(
		activeModelParams_,
		[this](const ModelParameters& params) {generateModel(params);},
		[this](std::string filePath) {loadRLE_(filePath);},
        [this]() {populateFromRLEString_(inputString_);},
        inputString_,
        isModelRunning
        );
}

void CpuModel::handleSDLEvent(const SDL_Event& event)
{
//TODO:: Rather than if else statements I might have a map? Or a switch statement
    //I should profile it to see if it would make a difference before changing though. 
    
    //TODO:
    //We only want to get the mouse state if the right events have been called.

    if (!ImGui::IsWindowHovered(4) && !ImGui::IsAnyItemActive())
    {
        float mousePosX, mousePosY;
        // int mouseButtonState = SDL_GetMouseState(&mousePosX, &mousePosY);

        // if (mouseButtonState & SDL_BUTTON(SDL_BUTTON_LEFT) && event.type == SDL_EVENT_MOUSE_MOTION)
        // {
      
            // activeModelParams_.displacementX += event.motion.xrel;
            // activeModelParams_.displacementY += event.motion.yrel;
            // //TODO:Check that it is within bounds of a maximum displacement
            // recalcDrawRange_ = true;
        // }
        if (event.type == SDL_EventType::SDL_EVENT_MOUSE_WHEEL)
        {
            int cursorModelIndexX = (mousePosX - screenSpaceDisplacementX_) / activeModelParams_.zoomLevel;
            int cursorModelIndexY = (mousePosY - screenSpaceDisplacementY_) / activeModelParams_.zoomLevel;

            //Zoom
            if (event.wheel.y > 0) activeModelParams_.zoomLevel += 1;
            else if (event.wheel.y < 0)  activeModelParams_.zoomLevel -= 1;
            activeModelParams_.zoomLevel = std::clamp<double>(activeModelParams_.zoomLevel, MIN_ZOOM, MAX_ZOOM);

            activeModelParams_.displacementX = -cursorModelIndexX * activeModelParams_.zoomLevel + mousePosX  + (activeModelParams_.modelWidth * activeModelParams_.zoomLevel - viewPort_.w )/ 2;
            activeModelParams_.displacementY = -cursorModelIndexY * activeModelParams_.zoomLevel + mousePosY  + (activeModelParams_.modelHeight * activeModelParams_.zoomLevel / 2) - (viewPort_.h / 2);

            recalcDrawRange_ = true;
        }
    }
}

void CpuModel::generateModel(const ModelParameters& params) {
    //First set the members to correspond with the parameters
    activeModelParams_.modelWidth = std::max<int>(activeModelParams_.modelWidth, params.minWidth);
    activeModelParams_.modelHeight = std::max<int>(activeModelParams_.modelHeight, params.minHeight);
    activeModelParams_.minWidth = params.minWidth;
    activeModelParams_.minHeight = params.minHeight;

    if (params.modelWidth > 0) activeModelParams_.modelWidth = params.modelWidth;
    if (params.modelHeight > 0) activeModelParams_.modelHeight = params.modelHeight;


    if (params.fillFactor > 0) activeModelParams_.fillFactor = params.fillFactor;
    if (params.rule1 > 0) activeModelParams_.rule1 = params.rule1;
    if (params.rule3 > 0) activeModelParams_.rule3 = params.rule3;
    if (params.rule4 > 0) activeModelParams_.rule4 = params.rule4;

    if (grid_.size() != activeModelParams_.modelHeight || grid_[0].size() != activeModelParams_.modelWidth) {
		resizeGrid_();
	}
    else {
		clearGrid_();
	}

    if (params.random) {
        std::random_device randomDevice;
        std::mt19937 rng(randomDevice());
        std::uniform_real_distribution<double> distribution(0.0, 1.0);

        for (auto& row : grid_) {
            for (auto& cell : row) {
				cell = distribution(rng) < params.fillFactor ? aliveValue_ : deadValue_;
			}
		}
        std::cout << "Random model generated" << std::endl;
        recalcDrawRange_ = true;
        initBackbufferRequired_ = true;
        return;
    }

    else {
        if (!params.runLengthEncoding.empty()) {
            std::istringstream rleStream(params.runLengthEncoding);
            populateFromRLE_(rleStream);
        }
    }

    initBackbufferRequired_ = true;
}

void CpuModel::populateFromRLE_(std::istream& modelStream)
{
    std::string line = "";
    std::string RLEstring = "";
    while(std::getline(modelStream, line))
    {
        //Find the header line containing specifications, which begins with the char 'x'
		if (line[0] == '#') continue;
		if (line[0] == 'x') {
            std::string::iterator lineIterator = line.begin();
            std::string minWidthString = "";
            std::string minHeightString = "";
            int bornNeighborCount = 3;
            int surviveNeighborMin = 2;
            int surviveNeighborMax = 3;

            //minimum width
            while (!std::isdigit(*lineIterator)) lineIterator++;
            while (std::isdigit(*lineIterator)) {
				minWidthString += *lineIterator;
				lineIterator++;
			}
            activeModelParams_.minWidth = std::stoi(minWidthString);

            //minimum height
            while (!std::isdigit(*lineIterator)) lineIterator++;
            while (std::isdigit(*lineIterator)) {
                minHeightString += *lineIterator;
                lineIterator++;
            }
            activeModelParams_.minHeight = std::stoi(minHeightString);
            
            //Neighbor count to be born
            while (*lineIterator != 'B' && *lineIterator != 'b') lineIterator++;
            lineIterator++;
            activeModelParams_.rule4 = (int)(*lineIterator - '0');
            lineIterator++;
            //minimum and maximum neighbors to survive
            while (!std::isdigit(*lineIterator)) lineIterator++;
            activeModelParams_.rule1 = (int)(*lineIterator - '0');
            lineIterator++;
            activeModelParams_.rule3 = (int)(*lineIterator - '0');//lineiterator '/S23'
            continue;
		}

        //If lines don't start with # or X, they must be part of the RLE encoded model.
        RLEstring += line;
	}
    activeModelParams_.modelWidth = std::max<int>(activeModelParams_.modelWidth, activeModelParams_.minWidth);
    activeModelParams_.modelHeight= std::max<int>(activeModelParams_.modelHeight, activeModelParams_.minHeight);
    if (grid_.size() != activeModelParams_.modelHeight || grid_[0].size() != activeModelParams_.modelWidth) {
        resizeGrid_();
    }
    else {
        clearGrid_();
    }
    //minwidth is wrong!
    int startColumn = (activeModelParams_.modelWidth / 2) - (activeModelParams_.minWidth/2);
    int startRow = (activeModelParams_.modelHeight - activeModelParams_.minHeight) / 2;
    int row = startRow;
    int column = startColumn;

    std::string::iterator RLEit;
    for (std::string::iterator it = RLEstring.begin(); it != RLEstring.end(); ++it)
    {
        if (*it == '!') break;

        std::string stringInteger = "";
        while ( isdigit(*it) || *it == '\n')
        {
            if (*it != '\n') stringInteger += *it;
            it++;
        }
        //If there is no preceding integer, set the count to 1
        if (stringInteger.empty()) stringInteger = "1";
        int count = std::stoi(stringInteger);
        //b is dead, o is alive, $ is newline
        if (*it == 'b') {
            for (int i = 0; i < count; i++) {
                column++;
            }
        }
        else if (*it == 'o') {
            for (int i = 0; i < count; i++) {
                grid_[row][column] = aliveValue_;
                column++;
            }
        }
        else if (*it == '$')
        {
            column = startColumn;
            for (int i = 0; i < count; i++) {
                row++;
            }
        }
    }

    recalcDrawRange_ = true;
    initBackbufferRequired_ = true;
}

void CpuModel::loadRLE_(const std::string& filePath)
{
	std::ifstream filestream(filePath);
	if (filestream.is_open())
	{
		clearGrid_();
		populateFromRLE_(filestream);
		filestream.close();
	}
}

void CpuModel::populateFromRLEString_(const std::string& rleString)
{
	std::stringstream rleStream(rleString);
	populateFromRLE_(rleStream);
}

//I should only be calculating this when it changes.
CpuModel::GridDrawRange CpuModel::getDrawRange_()
{
    CpuModel::GridDrawRange drawRange;
    drawRange.rowBegin = -(screenSpaceDisplacementY_ + activeModelParams_.displacementY) / activeModelParams_.zoomLevel;
    drawRange.rowEnd = drawRange.rowBegin + (viewPort_.h / activeModelParams_.zoomLevel);
    drawRange.columnBegin = -(screenSpaceDisplacementX_ + activeModelParams_.displacementX) / activeModelParams_.zoomLevel;
    drawRange.columnEnd = drawRange.columnBegin + (viewPort_.w / activeModelParams_.zoomLevel);

    //Don't try and draw something not in grid_
    int gridRows = grid_.size();
    int gridColumns = grid_[0].size();
    if (drawRange.rowEnd >= gridRows) drawRange.rowEnd = gridRows - 1;
    if (drawRange.columnEnd >= gridColumns) drawRange.columnEnd = gridColumns - 1;
    if (drawRange.rowBegin < 0) drawRange.rowBegin = 0;
    if (drawRange.columnBegin < 0) drawRange.columnBegin = 0;

    return drawRange;
}
#ifndef CPU_MODEL_H
#define CPU_MODEL_H

#include "abstract_model.hpp"
#include "ColorMapper.hpp"
#include "GlRenderer.hpp"


#include <vector>
//#include <SDL.h>
#include <memory>

//Next:: make and sdl texture backbuffer system. Only modify the buffer if there has been a change.
struct SDL_Texture;

class CpuModel : public AbstractModel 
{
public:
	CpuModel();
	~CpuModel();

	void initialize(const SDL_Rect& viewport) override;

	void update() override;

	void handleSDLEvent(const SDL_Event& event) override;

	void draw(SDL_Renderer* renderer) override;

	void drawImGuiWidgets(const bool& isModelRunning) override;

	void setParameters(const ModelParameters& modelParameters);
	ModelParameters getParameters();

	void setViewPort(const SDL_Rect& viewPort) override;

	void setMouseMove(float x, float y, bool motion=false);

	void setBlendFactor(const BlendFactor& blendFactor);

	void clear();

	void generateModel(const ModelParameters& modelParameters);

private:
	
	//Take a stream representing the RLE encoded model and populate board.
	void populateFromRLE_(std::istream& modelStream);
	//Load an RLE file and populate the board. Intended as a callback sent to gui.
	void loadRLE_(const std::string& filePath);
	//Convert and RLE string to a stream and call populateFromRLE_
	void populateFromRLEString_(const std::string& rleString);
	void resizeGrid_();
	void clearGrid_();
	//Whenever the model size is changed, the backbuffer texture must be reinitialized.
	void initBackbuffer_(SDL_Renderer* renderer);

	struct GridDrawRange
	{
		int rowBegin = 0;
		int rowEnd = 1;
		int columnBegin = 0;
		int columnEnd = 1;
	};

	GridDrawRange getDrawRange_();

private:
	std::unique_ptr<GL_Renderer> glRenderer_;

	//I might change this to a struct that has a flag for if I should check it,
	//and an int with the value. 
	//Or I could do some bit shifting to have it all in an int.

	std::vector<std::vector<uint8_t>> grid_; //I use an 8 bit int so I can represent some other info for visualization.

	//Because the SDL_Texture type is obfuscated and requires an SDL deleter, 
	//we need a template that can accept that deleter.
	std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> gridBackBuffer_;

	ModelParameters activeModelParams_{
		true,
		1260,
		720
	};

	BlendFactor blendFactor_;

	ColorMapper colorMapper_;
	const int aliveValue_ = 255;
	const int deadValue_ = 0;
	float dualColorAliveColor_[3] = { 1.0, 1.0, 0 };
	float dualColorDeadColor_[3] = { 0.0, 0.0, 1.0 };
	int deadValueDecrement_ = 10;//how fast does teh dead value decrement

	//If any blend factor changes, reset blend factors
	bool resetBlendFactor_ = true;
	//If the zoom level or displacement changes, recalculates the draw range.
	bool recalcDrawRange_ = true;
	//On first pass or on resized model, backbuffer needs reinitialized.
	bool initBackbufferRequired_ = true;
	////On backbuffer reinitilization or zoom change, complete redraw of 
	//bool completeBackbufferRedrawRequired_ = true;

	GridDrawRange drawRange_;
	int screenSpaceDisplacementX_ = 0;
	int screenSpaceDisplacementY_ = 0;

	const double MAX_ZOOM = 100.0;
	const double MIN_ZOOM = 1.0;

	//for handling ImGui RLE user input
	std::string inputString_ = "";
};

#endif // CPU_MODEL_H

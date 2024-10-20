#pragma once

#include <array>
#include <vector>
#include <SDL3/SDL_video.h>
#include <GL/glew.h>

struct SDL_Texture;

class GL_Renderer {
public:
    GL_Renderer(SDL_Window *sdlWindow);

    void prepare();
    void cleanup();
    void drawToSDLTexture(SDL_Texture *sdlTexture);
    void gatherPoint(float x, float y, float zoomLevel);
    void setMotionPoint(float x, float y, float zoomLevel);
    void setBlendFactor(GLenum srcColor, GLenum dstColor, GLenum srcAlpha, GLenum dstAlpha, GLenum equation);
    void clearPoint();

    struct Point {
        float x;
        float y;
        float zoom;
    };

    enum Shader {
        Vertex,
        Geometry,
        Fragment
    };

    enum {
        SrcColor = 0,
        DstColor,
        SrcAlpha,
        DstAlpha,
        Equation
    };

    struct Program {
        GLuint prog;
        // Note that always push back in order of vs, tcs, tes, gs, fs
        std::array<GLuint, 3> shaders;
    };

private:
    SDL_GLContext glContext = nullptr;
    SDL_Window *window = nullptr;
    GLuint texture{0};
    GLuint fbo{0};
    Program drawPoint;
    Program drawTriangle;

    std::array<GLenum, 5> blendFactors;
    std::vector<Point> points;
};

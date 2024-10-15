#include <vector>
#include <SDL3/SDL_video.h>
#include <GL/glew.h>

struct SDL_Texture;

class GL_Renderer {
public:
    GL_Renderer(SDL_Window *sdlWindow);

    void bindSDLTextureToFBO(SDL_Texture *sdlTexture);
    void drawToSDLTexture(SDL_Texture *sdlTexture);
    void gatherPoint(float x, float y);

    struct Point {
        float x;
        float y;
    };

private:
    SDL_GLContext glContext = nullptr;
    SDL_Window *window = nullptr;
    GLuint texture{0};
    GLuint fbo{0};

    std::vector<Point> points;
};

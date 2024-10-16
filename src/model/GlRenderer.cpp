#include <iostream>
#include <SDL3/SDL_render.h>
#include "GlRenderer.hpp"

GL_Renderer::GL_Renderer(SDL_Window * sdlWindow):
                         window(sdlWindow)
{
	glContext = SDL_GL_CreateContext(sdlWindow);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
    }
}

void
GL_Renderer::gatherPoint(float x, float y)
{
    points.push_back({x, y});
}

void
GL_Renderer::clearPoint()
{
    points.clear();
}

void
GL_Renderer::bindSDLTextureToFBO(SDL_Texture *sdlTexture)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1260, 720,
                 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to create framebuffer" << std::endl;
        return;
    }

    glEnable(GL_POINT_SMOOTH);
    glPointSize(9.9);
}

// Vertex Shader source code
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;

uniform vec2 viewport;

void main() {
    vec2 pos_ndc = 2.0f * aPos / viewport - vec2(1.0, 1.0);
    gl_Position = vec4(pos_ndc, 0.0, 1.0);
}
)";

// Fragment Shader source code
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(0.596, 0.984, 0.596, 1.0); // Mint green
}
)";

void
GL_Renderer::drawToSDLTexture(SDL_Texture* sdlTexture)
{
    int pitch = 0;
    Uint16* pixels;
    int pointNum = points.size();

    // Create a vertex buffer object (VBO)
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, pointNum * sizeof(Point), points.data(), GL_STATIC_DRAW);

    // Create a vertex array object (VAO)
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // Link the shaders into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Delete the shaders as they're now linked into the program
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glClearColor(0.0, 0.188, 0.286, 1.0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1260, 720);

    // Retain the content unless all points have been cleared
    if (pointNum == 0)
        glClear(GL_COLOR_BUFFER_BIT);

    // Render loop
    glUseProgram(shaderProgram);
    GLfloat values[] = {1260, 720};
    GLuint viewport = glGetUniformLocation(shaderProgram, "viewport");
    glUniform2fv(viewport, 1, values);

    glBindVertexArray(VAO);

    // Always draw points
    glDrawArrays(GL_POINTS, 0, pointNum);

    // Draw independent triangles only when there are proper number of points
    if (pointNum % 3 == 0)
        glDrawArrays(GL_TRIANGLES, 0, pointNum);

    glBindVertexArray(0);
    // We are rendering to texture and glGetTexImage later
    // It is okay to omit glFlush at this point
    // glFlush();

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    SDL_LockTexture(sdlTexture, nullptr, (void **)&pixels, &pitch);

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);

    SDL_UnlockTexture(sdlTexture);
}

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
}

// Vertex Shader source code
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

void main() {
    gl_Position = vec4(aPos, 1.0);
}
)";

// Fragment Shader source code
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color
}
)";

void
GL_Renderer::drawToSDLTexture(SDL_Texture* sdlTexture)
{
    int pitch = 0;
    Uint16* pixels;

    // Define the triangle vertices
    GLfloat vertices[] = {
         0.0f,  0.5f, 0.0f,  // top vertex
        -0.5f, -0.5f, 0.0f,  // bottom left vertex
         0.5f, -0.5f, 0.0f   // bottom right vertex
    };

#if 1
    // Create a vertex buffer object (VBO)
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create a vertex array object (VAO)
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
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

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1260, 720);
    // Render loop
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    // We are rendering to texture and glGetTexImage later
    // It is okay to omit glFlush at this point
    // glFlush();

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
#endif
    SDL_LockTexture(sdlTexture, nullptr, (void **)&pixels, &pitch);

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);

    SDL_UnlockTexture(sdlTexture);
}

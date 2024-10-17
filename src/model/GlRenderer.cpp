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
GL_Renderer::gatherPoint(float x, float y, float zoomLevel)
{
    points.push_back({x, y, zoomLevel});
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
layout(location = 0) in vec3 aPos;

uniform vec2 viewport;

void main() {
    vec2 pos_ndc = 2.0f * aPos.xy / (viewport * aPos.z) - vec2(1.0, 1.0);
    gl_Position = vec4(pos_ndc, 0.0, 1.0);
}
)";

const char* geometryShaderSource = R"(
#version 330 core
layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 9) out;

void main() {
    gl_Position = gl_in[3].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position;
    gl_PrimitiveID = 0;
    EmitVertex();

    EndPrimitive();

    gl_Position = gl_in[3].gl_Position;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    gl_PrimitiveID = 1;
    EmitVertex();

    EndPrimitive();

    gl_Position = gl_in[3].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    gl_PrimitiveID = 2;
    EmitVertex();

    EndPrimitive();
}
)";

// Fragment Shader source code
const char* fragmentShaderSourcePoint = R"(
#version 330 core

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.2, 0.302, 1.0);
}
)";

const char* fragmentShaderSourceTriangle = R"(
#version 330 core

out vec4 FragColor;

void main() {
    if (gl_PrimitiveID == 0)
        FragColor = vec4(0.72, 0.13, 0.15, 1);  // Red
    else if (gl_PrimitiveID == 1)
        FragColor = vec4(0.14, 0.58, 0.27, 1);  // Green
    else if (gl_PrimitiveID == 2)
        FragColor = vec4(0.03, 0.4, 0.71, 1); // Blue
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometryShader, 1, &geometryShaderSource, nullptr);
    glCompileShader(geometryShader);

    // Compile the fragment shader for drawing points
    GLuint fragmentShaderPoint = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderPoint, 1, &fragmentShaderSourcePoint, nullptr);
    glCompileShader(fragmentShaderPoint);

    // Compile the fragment shader for drawing triangles
    GLuint fragmentShaderTriangle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderTriangle, 1, &fragmentShaderSourceTriangle, nullptr);
    glCompileShader(fragmentShaderTriangle);

    // Link the vs and fs into a shader program without gs
    GLuint shaderProgramNoGeom = glCreateProgram();
    glAttachShader(shaderProgramNoGeom, vertexShader);
    glAttachShader(shaderProgramNoGeom, fragmentShaderPoint);
    glLinkProgram(shaderProgramNoGeom);

    // Link the vs, gs and fs into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, geometryShader);
    glAttachShader(shaderProgram, fragmentShaderTriangle);
    glLinkProgram(shaderProgram);

    // Delete the shaders as they're now linked into the program
    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    glDeleteShader(fragmentShaderPoint);
    glDeleteShader(fragmentShaderTriangle);

    glClearColor(0.0, 0.188, 0.286, 1.0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1260, 720);

    // Retain the content unless all points have been cleared
    if (pointNum == 0)
        glClear(GL_COLOR_BUFFER_BIT);

    GLfloat values[] = { 1260, 720 };

    // Render loop
    glUseProgram(shaderProgramNoGeom);
    GLuint viewport = glGetUniformLocation(shaderProgramNoGeom, "viewport");
    glUniform2fv(viewport, 1, values);

    glBindVertexArray(VAO);

    // Always draw points
    glDrawArrays(GL_POINTS, 0, pointNum);

    // Draw independent triangles only when there are proper number of points
    if (pointNum == 4) {
        glUseProgram(shaderProgram);
        GLuint viewport = glGetUniformLocation(shaderProgram, "viewport");
        glUniform2fv(viewport, 1, values);

        glDrawArrays(GL_LINES_ADJACENCY, 0, pointNum);
    }

    glBindVertexArray(0);
    // We are rendering to texture and glGetTexImage later
    // It is okay to omit glFlush at this point
    // glFlush();

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(shaderProgramNoGeom);

    SDL_LockTexture(sdlTexture, nullptr, (void **)&pixels, &pitch);

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);

    SDL_UnlockTexture(sdlTexture);
}

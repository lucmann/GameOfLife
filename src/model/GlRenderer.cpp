#include <iostream>
#include <SDL3/SDL_render.h>
#include "GlRenderer.hpp"

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

bool edgeFunction(vec4 a, vec4 b, vec4 c)
{
    return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x) >= 0);
}

void main() {
    vec4 V0 = gl_in[0].gl_Position;
    vec4 V1 = gl_in[1].gl_Position;
    vec4 V2 = gl_in[2].gl_Position;
    vec4 P0 = gl_in[3].gl_Position;

    bool outside_02 = true;

    outside_02 = outside_02 && edgeFunction(V0, V1, P0);
    outside_02 = outside_02 && edgeFunction(V1, V2, P0);

    // Invariance
    // Primitive 0: Red
    // Primitive 1: Green
    // Primitive 2: Blue
    // Top primitive is always destination in blend
    //
    // Variation depends on where P0 locates
    if (outside_02) {
        // Red is destination in blend
        gl_PrimitiveID = 0;
        gl_Position = P0; EmitVertex();
        gl_Position = V2; EmitVertex();
        gl_Position = V0; EmitVertex();

        EndPrimitive();

        gl_PrimitiveID = 1;
        gl_Position = P0; EmitVertex();
        gl_Position = V0; EmitVertex();
        gl_Position = V1; EmitVertex();

        EndPrimitive();

        gl_PrimitiveID = 2;
        gl_Position = P0; EmitVertex();
        gl_Position = V2; EmitVertex();
        gl_Position = V1; EmitVertex();

        EndPrimitive();
    }
    else {
        bool outside_01 = true;
        outside_01 = outside_01 && edgeFunction(V2, V0, P0);
        outside_01 = outside_01 && edgeFunction(V1, V2, P0);

        if (outside_01) {
            // Green is destination in blend
            gl_PrimitiveID = 1;
            gl_Position = P0; EmitVertex();
            gl_Position = V0; EmitVertex();
            gl_Position = V1; EmitVertex();

            EndPrimitive();

            gl_PrimitiveID = 2;
            gl_Position = P0; EmitVertex();
            gl_Position = V2; EmitVertex();
            gl_Position = V1; EmitVertex();

            EndPrimitive();

            gl_PrimitiveID = 0;
            gl_Position = P0; EmitVertex();
            gl_Position = V2; EmitVertex();
            gl_Position = V0; EmitVertex();

            EndPrimitive();
        }
        else {
            // Blue is destination in blend
            gl_PrimitiveID = 2;
            gl_Position = P0; EmitVertex();
            gl_Position = V2; EmitVertex();
            gl_Position = V1; EmitVertex();

            EndPrimitive();

            gl_PrimitiveID = 1;
            gl_Position = P0; EmitVertex();
            gl_Position = V0; EmitVertex();
            gl_Position = V1; EmitVertex();

            EndPrimitive();

            gl_PrimitiveID = 0;
            gl_Position = P0; EmitVertex();
            gl_Position = V2; EmitVertex();
            gl_Position = V0; EmitVertex();

            EndPrimitive();
        }
    }
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
        FragColor = vec4(0.72, 0.13, 0.15, 1.0);  // Red
    else if (gl_PrimitiveID == 1)
        FragColor = vec4(0.14, 0.58, 0.27, 1.0);  // Green
    else if (gl_PrimitiveID == 2)
        FragColor = vec4(0.03, 0.4, 0.71, 1.0); // Blue
}
)";

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
    if (points.size() >= 4)
        return;

    points.emplace_back(x, y, zoomLevel);
}

void
GL_Renderer::setMotionPoint(float x, float y, float zoomLevel)
{
    if (points.size() != 4)
        return;

    points.back() = {x, y, zoomLevel};
}

void
GL_Renderer::setBlendFactor(GLenum srcColor, GLenum dstColor, GLenum srcAlpha, GLenum dstAlpha, GLenum equation)
{
    blendFactors[SrcColor] = srcColor;
    blendFactors[DstColor] = dstColor;
    blendFactors[SrcAlpha] = srcAlpha;
    blendFactors[DstAlpha] = dstAlpha;
    blendFactors[Equation] = equation;
}

void
GL_Renderer::clearPoint()
{
    points.clear();
}

void
GL_Renderer::prepare()
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1260, 720,
                 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, nullptr);
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

    glEnable(GL_BLEND);
    glBlendColor(0.0, 0.0, 0.0, 0.618);

    // Compile and link the shaders for drawing points
    drawPoint.shaders[Shader::Vertex] = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(drawPoint.shaders[0], 1, &vertexShaderSource, nullptr);
    glCompileShader(drawPoint.shaders[0]);

    drawPoint.shaders[Shader::Fragment] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(drawPoint.shaders[Shader::Fragment], 1, &fragmentShaderSourcePoint, nullptr);
    glCompileShader(drawPoint.shaders[Shader::Fragment]);

    drawPoint.prog = glCreateProgram();
    glAttachShader(drawPoint.prog, drawPoint.shaders[Shader::Vertex]);
    glAttachShader(drawPoint.prog, drawPoint.shaders[Shader::Fragment]);
    glLinkProgram(drawPoint.prog);

    // Compile and link the shaders for drawing triangles
    // We are sharing the vertex shader with drawPoint
    drawTriangle.shaders[Shader::Vertex] = drawPoint.shaders[Shader::Vertex];

    drawTriangle.shaders[Shader::Geometry] = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(drawTriangle.shaders[Shader::Geometry], 1, &geometryShaderSource, nullptr);
    glCompileShader(drawTriangle.shaders[Shader::Geometry]);

    drawTriangle.shaders[Shader::Fragment] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(drawTriangle.shaders[Shader::Fragment], 1, &fragmentShaderSourceTriangle, nullptr);
    glCompileShader(drawTriangle.shaders[Shader::Fragment]);

    // Link the vs, gs and fs into a shader program
    drawTriangle.prog = glCreateProgram();
    glAttachShader(drawTriangle.prog, drawTriangle.shaders[Shader::Vertex]);
    glAttachShader(drawTriangle.prog, drawTriangle.shaders[Shader::Geometry]);
    glAttachShader(drawTriangle.prog, drawTriangle.shaders[Shader::Fragment]);
    glLinkProgram(drawTriangle.prog);

    // Delete the shaders as they're now linked into the program
    glDeleteShader(drawPoint.shaders[Shader::Vertex]);
    glDeleteShader(drawPoint.shaders[Shader::Fragment]);
    glDeleteShader(drawTriangle.shaders[Shader::Geometry]);
    glDeleteShader(drawTriangle.shaders[Shader::Fragment]);
}

void
GL_Renderer::cleanup()
{
    glDeleteProgram(drawTriangle.prog);
    glDeleteProgram(drawPoint.prog);
}

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

    glBlendFuncSeparate(blendFactors[SrcColor], blendFactors[DstColor],
                        blendFactors[SrcAlpha], blendFactors[DstAlpha]);
    glBlendEquation(blendFactors[Equation]);

    glClearColor(0.0, 0.188, 0.286, 1.0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1260, 720);

    glClear(GL_COLOR_BUFFER_BIT);

    GLfloat values[] = { 1260, 720 };

    // Render loop
    glBindVertexArray(VAO);

    // Draw independent triangles only when there are proper number of points
    if (pointNum == 4) {
        glUseProgram(drawTriangle.prog);
        GLuint viewport = glGetUniformLocation(drawTriangle.prog, "viewport");
        glUniform2fv(viewport, 1, values);

        glDrawArrays(GL_LINES_ADJACENCY, 0, pointNum);
    }

    // Always draw points
    glUseProgram(drawPoint.prog);
    GLuint viewport = glGetUniformLocation(drawPoint.prog, "viewport");
    glUniform2fv(viewport, 1, values);
    glDrawArrays(GL_POINTS, 0, pointNum);

    glBindVertexArray(0);
    // We are rendering to texture and glGetTexImage later
    // It is okay to omit glFlush at this point
    // glFlush();

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    SDL_LockTexture(sdlTexture, nullptr, (void **)&pixels, &pitch);

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, pixels);

    SDL_UnlockTexture(sdlTexture);
}

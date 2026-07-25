#include "GLLoader.h"

#include <wx/log.h>

GLuint CompileShaderProgram(const char* vs_name, const char* vs_src,
                            const char* fs_name, const char* fs_src) {
    // Standard OpenGL shader lifecycle: compile vertex+fragment shaders, link
    // them into a program, then release the intermediate shader objects (the
    // program keeps its own copy once linked).
    auto compile = [](GLenum type, const char* name, const char* src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            char log[512];
            glGetShaderInfoLog(shader, 512, nullptr, log);
            wxLogError("Shader compile error (%s): %s", name, log);
        }
        return shader;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, vs_name, vs_src);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fs_name, fs_src);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint status = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        wxLogError("Shader link error (%s / %s): %s", vs_name, fs_name, log);
    }
    // The shaders stay attached to the program; deleting them here just drops
    // the standalone references so they are freed with the program.
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

bool InitGLLoader() {
    glewExperimental = GL_TRUE;
    const GLenum glew_status = glewInit();

    // glewInit may trigger a benign GL_INVALID_ENUM on core profiles.
    glGetError();

    if (glew_status != GLEW_OK) {
        wxLogError("GLEW initialization failed: %s",
                   reinterpret_cast<const char*>(glewGetErrorString(glew_status)));
        return false;
    }

    if (!GLEW_VERSION_2_0 ||
        glCreateShader == nullptr ||
        glShaderSource == nullptr ||
        glCompileShader == nullptr ||
        glCreateProgram == nullptr ||
        glAttachShader == nullptr ||
        glLinkProgram == nullptr ||
        glUseProgram == nullptr) {
        wxLogError("OpenGL 2.0 shader entry points are unavailable after GLEW initialization.");
        return false;
    }

    return true;
}

#include "GLLoader.h"

#include <wx/log.h>

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

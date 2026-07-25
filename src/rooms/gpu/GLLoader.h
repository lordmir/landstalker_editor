#ifndef GL_LOADER_H
#define GL_LOADER_H

#include <GL/glew.h>

// Initializes the OpenGL function loader after a valid GL context is current.
bool InitGLLoader();

// Compiles a vertex+fragment shader pair and links them into a program,
// logging any compile/link errors (named by vs_name/fs_name) and freeing the
// intermediate shader objects. Returns the linked program object.
GLuint CompileShaderProgram(const char* vs_name, const char* vs_src,
                            const char* fs_name, const char* fs_src);

#endif // GL_LOADER_H

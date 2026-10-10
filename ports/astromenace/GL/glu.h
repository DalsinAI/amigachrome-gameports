#ifndef AMIGACHROME_GLU_H
#define AMIGACHROME_GLU_H

#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
void gluLookAt(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ,
               GLdouble centerX, GLdouble centerY, GLdouble centerZ,
               GLdouble upX, GLdouble upY, GLdouble upZ);
GLint gluBuild2DMipmaps(GLenum target, GLint internalFormat,
                        GLsizei width, GLsizei height, GLenum format,
                        GLenum type, const void *data);

#ifdef __cplusplus
}
#endif

#endif

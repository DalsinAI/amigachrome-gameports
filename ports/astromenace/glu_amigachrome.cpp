#include "GL/glu.h"

#include <math.h>

#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP 0x8191
#endif

extern "C" void gluPerspective(GLdouble fovy, GLdouble aspect,
                                GLdouble zNear, GLdouble zFar)
{
    const GLdouble radians = fovy * 3.14159265358979323846 / 360.0;
    const GLdouble ymax = zNear * tan(radians);
    const GLdouble xmax = ymax * aspect;
    glFrustum(-xmax, xmax, -ymax, ymax, zNear, zFar);
}

extern "C" void gluLookAt(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ,
                           GLdouble centerX, GLdouble centerY, GLdouble centerZ,
                           GLdouble upX, GLdouble upY, GLdouble upZ)
{
    GLdouble fx = centerX - eyeX;
    GLdouble fy = centerY - eyeY;
    GLdouble fz = centerZ - eyeZ;
    GLdouble fl = sqrt(fx * fx + fy * fy + fz * fz);
    if (fl == 0.0) return;
    fx /= fl; fy /= fl; fz /= fl;

    GLdouble ul = sqrt(upX * upX + upY * upY + upZ * upZ);
    if (ul == 0.0) return;
    upX /= ul; upY /= ul; upZ /= ul;

    GLdouble sx = fy * upZ - fz * upY;
    GLdouble sy = fz * upX - fx * upZ;
    GLdouble sz = fx * upY - fy * upX;
    GLdouble sl = sqrt(sx * sx + sy * sy + sz * sz);
    if (sl == 0.0) return;
    sx /= sl; sy /= sl; sz /= sl;

    const GLdouble ux = sy * fz - sz * fy;
    const GLdouble uy = sz * fx - sx * fz;
    const GLdouble uz = sx * fy - sy * fx;

    const GLfloat m[16] = {
        (GLfloat)sx, (GLfloat)ux, (GLfloat)-fx, 0.0f,
        (GLfloat)sy, (GLfloat)uy, (GLfloat)-fy, 0.0f,
        (GLfloat)sz, (GLfloat)uz, (GLfloat)-fz, 0.0f,
        0.0f,        0.0f,        0.0f,        1.0f
    };
    glMultMatrixf(m);
    glTranslatef((GLfloat)-eyeX, (GLfloat)-eyeY, (GLfloat)-eyeZ);
}

extern "C" GLint gluBuild2DMipmaps(GLenum target, GLint internalFormat,
                                     GLsizei width, GLsizei height,
                                     GLenum format, GLenum type,
                                     const void *data)
{
    /*
     * OpenGPU's compatibility GL supports the legacy automatic mipmap flag.
     * This gives AstroMenace the only GLU mipmap behaviour it needs without
     * importing an unrelated GLU implementation.
     */
    glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
    return 0;
}

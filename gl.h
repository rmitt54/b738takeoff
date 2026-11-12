#pragma once
typedef unsigned int GLenum; typedef int GLsizei; typedef int GLint; typedef float GLfloat; typedef unsigned int GLbitfield;
#define GL_TEXTURE_2D 1
#define GL_TEXTURE_MIN_FILTER 2
#define GL_TEXTURE_MAG_FILTER 3
#define GL_LINEAR 4
#define GL_RGBA 5
#define GL_UNSIGNED_BYTE 6
#define GL_VIEWPORT 7
#define GL_ALL_ATTRIB_BITS 8
#define GL_CLIENT_VERTEX_ARRAY_BIT 9
#define GL_CULL_FACE 10
#define GL_SCISSOR_TEST 11
#define GL_TEXTURE_ENV 12
#define GL_TEXTURE_ENV_MODE 13
#define GL_MODULATE 14
#define GL_VERTEX_ARRAY 15
#define GL_TEXTURE_COORD_ARRAY 16
#define GL_COLOR_ARRAY 17
#define GL_MODELVIEW 18
#define GL_FLOAT 19
#define GL_TRIANGLES 20
#define GL_UNSIGNED_SHORT 21
#define GL_UNSIGNED_INT 22
extern "C" {
void glTexParameteri(GLenum, GLenum, GLint); void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
void glGetIntegerv(GLenum, GLint*); void glPushAttrib(GLbitfield); void glPopAttrib(void); void glPushClientAttrib(GLbitfield); void glPopClientAttrib(void);
void glDisable(GLenum); void glEnable(GLenum); void glTexEnvi(GLenum, GLenum, GLint); void glEnableClientState(GLenum);
void glMatrixMode(GLenum); void glPushMatrix(void); void glPopMatrix(void); void glTranslatef(GLfloat, GLfloat, GLfloat); void glScalef(GLfloat, GLfloat, GLfloat);
void glVertexPointer(GLint, GLenum, GLsizei, const void*); void glTexCoordPointer(GLint, GLenum, GLsizei, const void*); void glColorPointer(GLint, GLenum, GLsizei, const void*);
void glScissor(GLint, GLint, GLsizei, GLsizei); void glDrawElements(GLenum, GLsizei, GLenum, const void*);
}

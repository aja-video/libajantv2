/* SPDX-License-Identifier: MIT */

#ifndef _OPENGL_
#define _OPENGL_

#if defined( AJAMac )
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif


#include <ntv2debug.h>

void CheckGLErrors();

#endif


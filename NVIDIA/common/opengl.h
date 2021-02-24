/* SPDX-License-Identifier: MIT */

#ifndef _OPENGL_
#define _OPENGL_

#if defined( AJAMac )
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <gl/glew.h>
//#include <glut.h>
//#include <glu.h>
#include <gl/gl.h>
#endif


#include <ntv2debug.h>

void CheckGLErrors();

#endif


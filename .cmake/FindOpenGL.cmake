#
# Copyright (c) 2014 Politecnico di Milano
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTOPENCLLAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

INCLUDE (FindPackageHandleStandardArgs)

IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
	SET (_OPENGL_POSSIBLE_LIB_SUFFIXES lib/x86_64 lib/x64 lib64 lib)
ELSE (CMAKE_SIZEOF_VOID_P EQUAL 8)
	SET (_OPENGL_POSSIBLE_LIB_SUFFIXES lib/x86 lib32 lib)
ENDIF (CMAKE_SIZEOF_VOID_P EQUAL 8)

SET (_OPENGL_POSSIBLE_INC_SUFFIXES include include/GL)

FIND_PATH (OPENGL_INCLUDE_DIR
  NAMES GLU.h
	glut.h
  HINTS ${OPENCL_ROOT_DIR}
  PATH_SUFFIXES ${_OPENGL_POSSIBLE_INC_SUFFIXES}
  DOC "OpenGL include directory")

FIND_PATH (OPENGL_LIBRARY_DIR
  NAMES libGLEW.so
	libglut.so
  HINTS ${OPENCL_ROOT_DIR}
  PATH_SUFFIXES ${_OPENGL_POSSIBLE_LIB_SUFFIXES}
  DOC "OpenGL library directory")

MESSAGE(STATUS "Found OpenGL: ")
MESSAGE("**  OpenGL include directory...: " ${OPENGL_INCLUDE_DIR})
MESSAGE("**  OpenGL library directory...: " ${OPENGL_LIBRARY_DIR})

MARK_AS_ADVANCED (OPENGL_INCLUDE_DIR OPENGL_LIBRARY_DIR)

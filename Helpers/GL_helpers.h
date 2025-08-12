/*
 * GL_helpers.h
 *
 *  Created on: Aug 11, 2025
 *      Author: tugrul
 */

#ifndef HELPERS_GL_HELPERS_H_
#define HELPERS_GL_HELPERS_H_

#define RED 1.0f, 0.0f, 0.0f, 1.0f
#define BLUE 0.0f, 0.0f, 1.0f, 1.0f
#define GREEN  0.0f, 1.0f, 0.0f, 1.0f
#define YELLOW 1.0f, 1.0f, 0.0f, 1.0f
#define CYAN   0.0f, 1.0f, 1.0f, 1.0f
#define MAGENTA 1.0f, 0.0f, 1.0f, 1.0f
#define WHITE  1.0f, 1.0f, 1.0f, 1.0f
#define BLACK  0.0f, 0.0f, 0.0f, 1.0f
#define ORANGE 1.0f, 0.5f, 0.0f, 1.0f
#define PURPLE 0.5f, 0.0f, 0.5f, 1.0f
#define PINK   1.0f, 0.75f, 0.8f, 1.0f
#define GREY   0.5f, 0.5f, 0.5f, 1.0f
#define BROWN  0.6f, 0.3f, 0.0f, 1.0f

unsigned int createProgram(const char *vertexSource, const char *fragmentSource);

unsigned int createProgramFromFile(const char *vertexSourceFile, const char *fragmentSourceFile);

#endif /* HELPERS_GL_HELPERS_H_ */

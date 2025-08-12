#include <stdio.h>
#include <GLES2/gl2.h>
#include <stdlib.h>

#include "Helpers/Renderer_helpers.h"
#include "Helpers/GL_helpers.h"

// Vertex shader source code
static const char *vertexShaderSource =
		"		\
attribute vec4 a_Position;				\
void main() {							\
    gl_Position = a_Position;			\
}										\
";

// Fragment shader source code
static const char *fragmentShaderSource =
		" 	\
precision mediump float;				\
uniform vec4 u_Color;					\
void main() {							\
    gl_FragColor = u_Color;				\
}										\
";

static GLfloat vertices[] = {
		0.0f, 0.25f, 0.0f, // Top
		-0.25f, -0.25f, 0.0f, // Bottom left
		0.25f, -0.25f, 0.0f  // Bottom right
	};

static GLuint program;
static GLint positionAttrib;
static GLint colorUniform;
static GLuint vbo = 0;

static int dir = 0;
static float speed = 0.01;

static void init() {
//	program = createProgram(vertexShaderSource, fragmentShaderSource);
	program = createProgramFromFile("./Shaders/triangle.vert", "./Shaders/triangle.frag");
	if(!program){
		exit(0); // Error creating program
	}

	positionAttrib = glGetAttribLocation(program, "a_Position");
	colorUniform = glGetUniformLocation(program, "u_Color");

	// Create VBO
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void update_vertices() {
	if (dir) {
		if (vertices[1] > 1.0)
			dir = 0;
		else {
			vertices[1] += speed;
			vertices[4] += speed;
			vertices[7] += speed;
		}
	} else {
		if (vertices[7] < -1.0)
			dir = 1;
		else {
			vertices[1] -= speed;
			vertices[4] -= speed;
			vertices[7] -= speed;
		}
	}

	// Update vertex buffer data
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void draw() {
    glViewport(0, 0, renderer_get_width(), renderer_get_height());
    update_vertices();
    glClearColor(GREEN);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(positionAttrib);

    glUniform4f(colorUniform, 1.0, 0.0, 0.0, 1.0); // Red

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(positionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void cleanup() {
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    glDeleteProgram(program);
}

int main() {
	init_renderer(init, draw, cleanup);

	if (render_loop()) {
		// Error
		free_renderer();
	} else {
		// Success
		free_renderer();
	}

	return 0;
}

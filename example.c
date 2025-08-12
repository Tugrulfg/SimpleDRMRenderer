#include <stdio.h>
#include <GLES2/gl2.h>
#include <stdlib.h>

#include "Helpers/Renderer_helpers.h"
#include "Helpers/GL_helpers.h"
#include "Helpers/Input_helpers.h"

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

static float speed = 0.05;

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

static void update_vertices(int key) {
	float diff = 0;
	switch(key){
	case 'w':
		if (vertices[1]+speed < 1.0){
			diff = speed;
		}
		else{
			diff = 1.0 - vertices[1];
		}
		vertices[1] += diff;
		vertices[4] += diff;
		vertices[7] += diff;
		break;
	case 's':
		if (vertices[7]-speed > -1.0){
			diff = speed;
		}
		else{
			diff = vertices[7] + 1;
		}
		vertices[1] -= diff;
		vertices[4] -= diff;
		vertices[7] -= diff;
		break;
	case 'a':
		if (vertices[3]-speed > -1.0){
			diff = speed;
		}
		else{
			diff = vertices[3] + 1;
		}

		vertices[0] -= diff;
		vertices[3] -= diff;
		vertices[6] -= diff;
		break;
	case 'd':
		if (vertices[6]+speed < 1.0){
			diff = speed;
		}
		else{
			diff = 1 - vertices[6];
		}
		vertices[0] += diff;
		vertices[3] += diff;
		vertices[6] += diff;

		break;
	default:
		printf("Unknown key: %c: %d\n", key, key);
		return;
	}

	// Update vertex buffer data
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void draw() {
    glViewport(0, 0, renderer_get_width(), renderer_get_height());
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

int keyboard_callback(int key){
	if(key == 27)
		return 1;

    update_vertices(key);
	return 0;
}

void mouse_callback(int x_move, int y_move, int left, int right, int middle){
//	printf("Mouse: L=%d R=%d M=%d Move=(%d,%d)\n", left, right, middle, x_move, y_move);
	if (left) {
		for (int i = 0; i < 9; i++)
			vertices[i] *= 1.1;
	}
	if (right) {
		for (int i = 0; i < 9; i++)
			vertices[i] *= 0.9;
	}

	// Update vertex buffer data
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

int main() {
	init_renderer(init, draw, cleanup);
	init_input_handler(keyboard_callback, mouse_callback);

	if (render_loop()) {
		// Error
		free_renderer();
		free_input_handler();
	} else {
		// Success
		free_renderer();
		free_input_handler();
	}

	return 0;
}

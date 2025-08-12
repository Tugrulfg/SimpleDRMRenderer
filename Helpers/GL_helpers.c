#include <GLES2/gl2.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "GL_helpers.h"

static GLuint compileShader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char *infoLog = (char *)malloc(infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            printf("GL Error: Error compiling shader (%s):\n%s\n",
                    type == GL_VERTEX_SHADER ? "Vertex Shader" : "Fragment Shader", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

unsigned int createProgram(const char *vertexSource, const char *fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (!vertexShader || !fragmentShader) {
        return 0; // Shader compilation failed.
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char *infoLog = (char *)malloc(infoLen);
            glGetProgramInfoLog(program, infoLen, NULL, infoLog);
            printf("GL Error: Error linking program:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteProgram(program);
        return 0;
    }

    // Detach and delete shaders after linking
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

unsigned int createProgramFromFile(const char *vertexSourceFile, const char *fragmentSourceFile){
	if(!vertexSourceFile){
		printf("GL Error: Invalid vertex shader file\n");
		return 0;
	}
	else if (!fragmentSourceFile) {
		printf("GL Error: Invalid fragment shader file\n");
		return 0;
	}
	char* vertexSource = NULL, *fragmentSource = NULL;
	GLuint program = 0;
	struct stat st;
	size_t vert_size, frag_size;

	int vert_fd = open(vertexSourceFile, O_RDONLY);
	if(vert_fd < 0){
		printf("GL Error: Failed to open vertex shader file: %s\n", vertexSourceFile);
		return 0;
	}
	// Get file size
	if (fstat(vert_fd, &st) == -1) {
		perror("fstat");
		close(vert_fd);
		return 0;
	}

	vert_size = st.st_size;
	// Map file to memory
	vertexSource = mmap(NULL, vert_size, PROT_READ, MAP_PRIVATE, vert_fd, 0);
	if (vertexSource == MAP_FAILED) {
		perror("mmap");
		close(vert_fd);
		return 1;
	}

	int frag_fd = open(fragmentSourceFile, O_RDONLY);
	if (frag_fd < 0) {
		printf("GL Error: Failed to open fragment shader file: %s\n", fragmentSourceFile);
		munmap(vertexSource, vert_size);
		close(vert_fd);
		return 0;
	}

	// Get file size
	if (fstat(frag_fd, &st) == -1) {
		perror("fstat");
		close(frag_fd);
		munmap(vertexSource, vert_size);
		close(vert_fd);
		return 0;
	}
	frag_size = st.st_size;

	// Map file to memory
	fragmentSource = mmap(NULL, frag_size, PROT_READ, MAP_PRIVATE, frag_fd, 0);
	if (fragmentSource == MAP_FAILED) {
		perror("mmap");
		close(frag_fd);
		munmap(vertexSource, vert_size);
		close(vert_fd);
		return 1;
	}

	program = createProgram(vertexSource, fragmentSource);

	munmap(fragmentSource, frag_size);
	close(frag_fd);
	munmap(vertexSource, vert_size);
	close(vert_fd);

	return program;
}


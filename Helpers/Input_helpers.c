#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "Input_helpers.h"

// Store callbacks
static key_event_cb g_key_cb = NULL;
static mouse_event_cb g_mouse_cb = NULL;
static int mouse_fd = 0;
static unsigned char mouse_data[3];

static void set_raw_mode(int enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);  // raw mode, no echo
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

// Call this when you detect a key press/release
static int renderer_invoke_key_event(int key) {
	if (g_key_cb) {
		return g_key_cb(key);
	}

	return 0;
}

// Call this when you detect a mouse event
static void renderer_invoke_mouse_event(int x, int y, int left, int right, int middle) {
	if (g_mouse_cb) {
		g_mouse_cb(x, y, left, right, middle);
	}
}

int init_input_handler(key_event_cb key_cb, mouse_event_cb mouse_cb) {
	if(!key_cb && !mouse_cb){
		printf("Input Handler Error: Both callback functions are NULL\n");
		return 1;
	}
	if(g_key_cb || g_mouse_cb){
		printf("Input Handler Error: Input handler have already been initialized\n");
		return 1;
	}

	g_key_cb = key_cb;
	g_mouse_cb = mouse_cb;

	if (mouse_cb) {
		// Open mouse device
		mouse_fd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
		if (mouse_fd < 0) {
			printf("Input Handler Error: Failed to open \"/dev/input/mice\"\n");
			mouse_fd = 0;
			return 1;
		}
	}
	if (key_cb) {
		set_raw_mode(1);
		// Make read from stdin nonblock
		int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
	}

	return 0;
}

int process_inputs() {
	int ret = 0;
	if (g_key_cb) {
		static char buf[3];
		char ch;
		ssize_t n = read(STDIN_FILENO, &buf, 3);
		if (n > 0) {
			if (n == 3 && buf[0] == 27 && buf[1] == '[') {		// Handle arrow keys
				switch (buf[2]) {
				case 'A':
					ch = 'w';
					break;
				case 'B':
					ch = 's';
					break;
				case 'C':
					ch = 'd';
					break;
				case 'D':
					ch = 'a';
					break;
				}
			}
			else{
				ch = buf[0];
			}

			ret = renderer_invoke_key_event(ch);
		}
	}

	if(g_mouse_cb){
		// Mouse input
		ssize_t m = read(mouse_fd, mouse_data, sizeof(mouse_data));
		if (m > 0) {
			int left = mouse_data[0] & 0x1;
			int right = (mouse_data[0] & 0x2) >> 1;
			int middle = (mouse_data[0] & 0x4) >> 2;
			int x_move = (int) (signed char) mouse_data[1];
			int y_move = (int) (signed char) mouse_data[2];
			renderer_invoke_mouse_event(x_move, y_move, left, right, middle);
		}
	}

	return ret;
}

void free_input_handler(){
	set_raw_mode(0);
	if(mouse_fd){
		close(mouse_fd);
		mouse_fd = 0;
	}

	// Make read from stdin nonblock
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags ^ O_NONBLOCK);
}

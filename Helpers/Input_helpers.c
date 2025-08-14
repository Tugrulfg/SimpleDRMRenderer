#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <linux/input.h>
#include <termios.h>
#include "Input_helpers.h"

// Store callbacks
static key_event_cb g_key_cb = NULL;
static mouse_event_cb g_mouse_cb = NULL;
static int mouse_fd = 0;
static int kbd_fd = 0;
static unsigned char mouse_data[3];
static bool key_state[KEY_CNT] = {0};

static int open_keyboard_device(void) {
    struct dirent *entry;
    DIR *dir = opendir("/dev/input");
    if (!dir) {
        perror("opendir");
        return -1;
    }

    char path[256];
    char name[256];
    int fd = -1;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            snprintf(path, sizeof(path), "/sys/class/input/%s/device/name", entry->d_name);
            FILE *f = fopen(path, "r");
            if (f) {
                if (fgets(name, sizeof(name), f)) {
                    // remove newline
                    name[strcspn(name, "\n")] = 0;
                    if (strstr(name, "Keyboard") || strstr(name, "keyboard")) {
                        // Found a keyboard device
                        char dev_path[256];
                        snprintf(dev_path, sizeof(dev_path), "/dev/input/%s", entry->d_name);
                        fd = open(dev_path, O_RDONLY | O_NONBLOCK);
                        fclose(f);
                        break;
                    }
                }
                fclose(f);
            }
        }
    }

    closedir(dir);
    return fd;
}

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

bool is_key_pressed(int key){
	if(key < 0 || key >= KEY_CNT)
		return false;

	return key_state[key];
}

int init_input_handler(key_event_cb key_cb, mouse_event_cb mouse_cb) {
    if(!key_cb && !mouse_cb){
        printf("Input Handler Error: Both callback functions are NULL\n");
        return 1;
    }
    if(g_key_cb || g_mouse_cb){
        printf("Input Handler Error: Input handler already initialized\n");
        return 1;
    }

    g_key_cb = key_cb;
    g_mouse_cb = mouse_cb;

    if (mouse_cb) {
        mouse_fd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
        if (mouse_fd < 0) {
            printf("Input Handler Error: Failed to open /dev/input/mice\n");
            mouse_fd = 0;
            return 1;
        }
    }

    if (key_cb) {
        kbd_fd = open_keyboard_device();
        if (kbd_fd < 0) {
            printf("Input Handler Error: Failed to open keyboard device\n");
            kbd_fd = 0;
            return 1;
        }

        set_raw_mode(1);
    }

    return 0;
}

int process_inputs() {
    int ret = 0;

    // Keyboard events
    if (kbd_fd > 0) {
        struct input_event ev;
        while (read(kbd_fd, &ev, sizeof(ev)) > 0) {
			if (ev.type == EV_KEY && ev.code < KEY_CNT) {
				key_state[ev.code] = (ev.value != 0); // press or hold = true, release = false
			}
        }
        if(g_key_cb())
        	ret = 1;
    }

    // Mouse events
    if (mouse_fd > 0) {
        ssize_t m = read(mouse_fd, mouse_data, sizeof(mouse_data));
        if (m > 0) {
            int left = mouse_data[0] & 0x1;
            int right = (mouse_data[0] & 0x2) >> 1;
            int middle = (mouse_data[0] & 0x4) >> 2;
            int x_move = (int)(signed char) mouse_data[1];
            int y_move = (int)(signed char) mouse_data[2];
            g_mouse_cb(x_move, y_move, left, right, middle);
        }
    }

    return ret;
}

void free_input_handler() {
    if(mouse_fd){
        close(mouse_fd);
        mouse_fd = 0;
    }
    if(kbd_fd){
        close(kbd_fd);
        kbd_fd = 0;
        set_raw_mode(0);
    }
}

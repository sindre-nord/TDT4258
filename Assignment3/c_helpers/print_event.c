#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <time.h>

#define JOYSTICK_DEVICE "/dev/input/event5"
#define POLL_INTERVAL 500000  // 500ms in microseconds

int main() {
    struct input_event ie;
    int fd;

    // Open the joystick device
    if((fd = open(JOYSTICK_DEVICE, O_RDONLY)) == -1) {
        perror("Error opening device");
        exit(1);
    }

    while(1) {
        // Use select to wait for data to be available
        fd_set fds;
        struct timeval tv;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = POLL_INTERVAL;

        int ret = select(fd + 1, &fds, NULL, NULL, &tv);
        if(ret < 0) {
            perror("Error in select");
            close(fd);
            exit(1);
        } else if(ret > 0) {
            // Data is available
            ssize_t bytesRead = read(fd, &ie, sizeof(struct input_event));
            if(bytesRead != sizeof(struct input_event)) {
                perror("Error reading");
                close(fd);
                exit(1);
            }

            if(ie.type == EV_KEY && ie.value == 1) {
                printf("Button %d pressed\n", ie.code);
                if(ie.code == BTN_MIDDLE) {
                    break;
                }
            }
        } else {
            // Timeout occurred
            continue;
        }
    }

    close(fd);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main(int argc, char const *argv[]){
    int fb = open("/dev/fb1", O_RDWR); // dev/fb1 is harcoded here from testing
    if (fb == -1) {
        perror("Error opening framebuffer device");
        return 1;
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fb);
        return 1;
    }
    // Print all the information
    printf("Screen resolution: %dx%d\n", vinfo.xres, vinfo.yres);
    printf("Virtual resolution: %dx%d\n", vinfo.xres_virtual, vinfo.yres_virtual);
    printf("Bits per pixel: %d\n", vinfo.bits_per_pixel);
    printf("Red: offset %d, length %d, msb_right %d\n", vinfo.red.offset, vinfo.red.length, vinfo.red.msb_right);
    printf("Green: offset %d, length %d, msb_right %d\n", vinfo.green.offset, vinfo.green.length, vinfo.green.msb_right);
    printf("Blue: offset %d, length %d, msb_right %d\n", vinfo.blue.offset, vinfo.blue.length, vinfo.blue.msb_right);
    printf("Transp: offset %d, length %d, msb_right %d\n", vinfo.transp.offset, vinfo.transp.length, vinfo.transp.msb_right);
    // The rest of the info deos not seem to be useful ref: https://www.kernel.org/doc/html/latest/fb/api.html
     
     


    pclose(fb); // Compiler wants pclose() instead of close()
    return 0;
}
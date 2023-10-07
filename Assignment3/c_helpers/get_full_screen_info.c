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
    struct fb_fix_screeninfo finfo;
    if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        close(fb);
        return 1;
    }
    // Print all the variable information
    printf("Screen resolution: %dx%d\n", vinfo.xres, vinfo.yres);
    printf("Virtual resolution: %dx%d\n", vinfo.xres_virtual, vinfo.yres_virtual);
    printf("Bits per pixel: %d\n", vinfo.bits_per_pixel);
    printf("Red: offset %d, length %d, msb_right %d\n", vinfo.red.offset, vinfo.red.length, vinfo.red.msb_right);
    printf("Green: offset %d, length %d, msb_right %d\n", vinfo.green.offset, vinfo.green.length, vinfo.green.msb_right);
    printf("Blue: offset %d, length %d, msb_right %d\n", vinfo.blue.offset, vinfo.blue.length, vinfo.blue.msb_right);
    printf("Transp: offset %d, length %d, msb_right %d\n", vinfo.transp.offset, vinfo.transp.length, vinfo.transp.msb_right);
    // The rest of the info deos not seem to be useful ref: https://www.kernel.org/doc/html/latest/fb/api.html
     
    // Print the fixed information
    printf("id: %s\n", finfo.id);
    printf("smem_start: %lu\n", finfo.smem_start);
    printf("smem_len: %d\n", finfo.smem_len);
    printf("type: %d\n", finfo.type);
    printf("type_aux: %d\n", finfo.type_aux);
    printf("visual: %d\n", finfo.visual);
    printf("xpanstep: %d\n", finfo.xpanstep);
    printf("ypanstep: %d\n", finfo.ypanstep);
    printf("ywrapstep: %d\n", finfo.ywrapstep);
    printf("line_length: %d\n", finfo.line_length);
    printf("mmio_start: %lu\n", finfo.mmio_start);
    printf("mmio_len: %d\n", finfo.mmio_len);
    printf("accel: %d\n", finfo.accel);
    printf("capabilities: %d\n", finfo.capabilities);
    


    close(fb); // Compiler wants pclose() instead of close()
    return 0;
}
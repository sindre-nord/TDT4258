#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>

// Needed to fine the sense hat framebuffer
#include <dirent.h>
#include <fnmatch.h>

#define SENSE_HAT_FB_ID "RPi-Sense FB"

#define SENSE_HAT_FB_PATH "/dev"
#define SENSE_HAT_FB_PATTERN "fb*"


int is_fb_sense_hat(int fb){
    // Check if the framebuffer is the sense hat framebuffer
    // Return 1 if it is
    // Return 0 if it is not
    // Return -1 if there is an error
    struct fb_fix_screeninfo finfo;
    if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        close(fb);
        return -1;
    }
    if (finfo.id == SENSE_HAT_FB_ID){
        return 1;
    }
    return 0;
}

int find_frame_buffer_by_id(char *id){
    // Sear for all framebuffers
    // If id matches, return the file descriptor
    // Else return -1
    DIR *dir;
    struct dirent *entry;
    const char *directory_path = "/dev"; // default to /dev directory
    const char *pattern = "fb*"; // The pattern to match

    dir = opendir(directory_path);
    if (!dir) {
        perror("Unable to open directory");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        // printf("Found entry: %s\n", entry->d_name);
        if (fnmatch(pattern, entry->d_name, 0) == 0) {
            printf("Found framebuffer: %s\n", entry->d_name);
            // This is a hit, check if it is the one we are looking for
            int fb = open(entry->d_name, O_RDWR);
            if (fb == -1) {
                perror("Error opening framebuffer device");
                closedir(dir);
                return -1;
            }
            if (is_fb_sense_hat(fb)){
                closedir(dir);
                printf("Found sense hat framebuffer: %s\n", entry->d_name);
                return fb;
            }

        }
    }
    closedir(dir);
    return -1;
}
void print_sense_hat_info(int fb){
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fb);
        return;
    }
    struct fb_fix_screeninfo finfo;
    if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        close(fb);
        return;
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
}

int main(){
    printf("Attempting to find sense hat framebuffer...\n");
    // Find the sense hat frambuffer
    int fb = find_frame_buffer_by_id(SENSE_HAT_FB_ID);
    if (fb == -1){
        perror("Error finding sense hat framebuffer");
        return 1;
    }
    print_sense_hat_info(fb);

}
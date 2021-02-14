#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
 
#include <linux/gpio.h>
#include <signal.h> // signal functions
 
volatile sig_atomic_t ctrlcflag = 0;
static void my_handler(int sig){ // can be called asynchronously
  ctrlcflag = 1; // set flag
}

int main(int argc, char **argv)
{
    struct gpiohandle_request req;
    struct gpiohandle_data data;
    char chrdev_name[20];
    int fd, ret;
    
    
    signal(SIGINT, my_handler);
    strcpy(chrdev_name, "/dev/gpiochip0");
    
    fd = open(chrdev_name, 0);
    if (fd == -1) {
        ret = -errno;
        fprintf(stderr, "Failed to open %s\n", chrdev_name);
        
        return ret;
    }
    
    req.lineoffsets[0] = 15;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    memcpy(req.default_values, &data, sizeof(req.default_values));
    strcpy(req.consumer_label, "red_led");
    req.lines = 1;
    
    ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
    if (ret == -1) {
        ret = -errno;
        fprintf(stderr, "Failed to issue GET LINEHANDLE IOCTL (%d)\n", ret);
    }
    if (close(fd) == -1)
        perror("Failed to close GPIO character device file");
    
    /* Start led blinking */
    while(1) {
        if(ctrlcflag)
            data.values[0] = 0;
        else
            data.values[0] = !data.values[0];
        ret = ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
        if (ret == -1) {
            ret = -errno;
            fprintf(stderr, "Failed to issue %s (%d)\n", "GPIOHANDLE_SET_LINE_VALUES_IOCTL", ret);
        }
        if(ctrlcflag)
            break;
        sleep(1);
    }
    
    perror("maybe CTRL+C enter");
    /* release line */
    ret = close(req.fd);
    if (ret == -1) {
        perror("Failed to close GPIO LINEHANDLE device file");
        ret = -errno;
    }
    return ret;
}
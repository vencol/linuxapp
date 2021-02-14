#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
 
#include <linux/gpio.h>
 
int main(int argc, char **argv)
{
    struct gpiohandle_request req;
    struct gpiohandle_data data;
    char chrdev_name[20];
    int fd, ret;
    
	//如果不知道gpiochipN对应的是GPIO?，可以查看/sys/bus/gpio/devices/gpiochip0/of_node# cat st,bank-name 输出结果是GPIOA
    //或者查看dts中对应的驱动文件
    strcpy(chrdev_name, "/dev/gpiochip1");
    
    fd = open(chrdev_name, 0);
    if (fd == -1) {
        ret = -errno;
        fprintf(stderr, "Failed to open %s\n", chrdev_name);
        
        return ret;
    }
    
    req.lineoffsets[0] = 10;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    memcpy(req.default_values, &data, sizeof(req.default_values));
    strcpy(req.consumer_label, "green_led");
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
        data.values[0] = !data.values[0];
        ret = ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
        if (ret == -1) {
            ret = -errno;
            fprintf(stderr, "Failed to issue %s (%d)\n", "GPIOHANDLE_SET_LINE_VALUES_IOCTL", ret);
        }
        sleep(1);
    }
    
    /* release line */
    ret = close(req.fd);
    if (ret == -1) {
        perror("Failed to close GPIO LINEHANDLE device file");
        ret = -errno;
    }
    return ret;
}
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
 
#include <linux/gpio.h>
 
//  设备树配置如下
// 	leds {
// 		compatible = "gpio-leds";

// 		pwr_led {
// 			label = "orangepi:green:pwr";
// 			gpios = <&r_pio 0 10 GPIO_ACTIVE_HIGH>;
// 			default-state = "on";
// 		};

// 		status_led {
// 			label = "orangepi:red:status";
// 			gpios = <&pio 0 15 GPIO_ACTIVE_HIGH>;
// 		};
// 	};
#define MAX_DEV_NUM         64
#define BASE_DEV_NAME  "/sys/class/leds/orangepi:green:pwr"
#define CONTROL_BRIGHTNESS  "brightness"
#define CONTROL_TRIGGER     "trigger"
#define CONTROL_TRIGGER_WAY "timer"
#define CONTROL_TRIGGER_ON  "delay_on"
#define CONTROL_TRIGGER_OFF "delay_off"
#define TRIGGER_ON_TIME     "1000"
#define TRIGGER_OFF_TIME    "100"
static int setLedParm(char *devname, char *param, int delay)
{
    //trigger 占用线程，实际应用需要使用多线程
    char chrdev_name[MAX_DEV_NUM];
    int ret;
    if (devname == NULL || param == NULL)
        return -1;
    snprintf(chrdev_name, MAX_DEV_NUM, "%s/%s", BASE_DEV_NAME, devname);
    int trigerfd = open(chrdev_name, O_RDWR );
    if (trigerfd == -1) {
        ret = -errno;
        printf("Failed to set trigger open %s code:%d\n", chrdev_name, ret);
        return ret;
    }
    ret = write(trigerfd, param, strlen(param) + 1);
    if (ret < 0) {
        ret = -errno;
        printf("Failed to set trigger write %s code:%d\n", chrdev_name, ret);
        return ret;
    }
    do{
        sleep(1);
        printf("set led trigger way by timer, now time set :%s", param);
        delay--;
    }while(delay > 0);
    ret = close(trigerfd);
    if (ret == -1) {
        ret = -errno;
        printf("Failed to set trigger close %s code:%d\n", chrdev_name, ret);
        return ret;;
    }
}
int main(int argc, char **argv)
{
    char chrdev_name[MAX_DEV_NUM];
    int fd, ret, brightness=0;
    char buf[8];
    
    //使用led-class.ko和leds-gpio.ko可以实现开关亮度调节，再加上ledtrig-timer.ko可以使用定时器触发亮灭，或者使用其他的trigger方式，需要相关trigger驱动模块
    snprintf(chrdev_name, MAX_DEV_NUM, "%s/%s", BASE_DEV_NAME, CONTROL_BRIGHTNESS);
    fd = open(chrdev_name, O_RDWR, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP );
    if (fd == -1) {
        ret = -errno;
        printf("Failed to open %s code:%d\n", chrdev_name, ret);
        return ret;
    }
    
    buf[0] = '0';
    ret = write(fd, buf, 1);
    if (ret < 0) {
        ret = -errno;
        printf("Failed to issue GET write IOCTL (%d)\n", ret);
    }
    /* Start led blinking */
    buf[7]=0;
    while(1) {
        if(++buf[7] > 10)
            break;
        ret = read(fd, buf, 1);
        if (ret < 0) {
            ret = -errno;
            printf("Failed to issue GET write IOCTL (%d)\n", ret);
        }
        printf("read data is %d\n", buf[0]);
        buf[0] = (buf[0] == '0') ? '1' : '0';
        ret = write(fd, buf, 1);
        if (ret < 0) {
            ret = -errno;
            printf("Failed to issue write (%d)\n", ret);
        }
        sleep(1);
    }
    
    setLedParm(CONTROL_TRIGGER, CONTROL_TRIGGER_WAY, 10);
    setLedParm(CONTROL_TRIGGER_OFF, TRIGGER_OFF_TIME, 10);
    setLedParm(CONTROL_TRIGGER_ON, TRIGGER_ON_TIME, 10);

    
    while(1) {
        ret = read(fd, buf, 1);
        if (ret < 0) {
            ret = -errno;
            printf("Failed to issue GET write IOCTL (%d)\n", ret);
        }
        printf("read data is %d\n", buf[0]);
        buf[0] = (buf[0] == '0') ? '1' : '0';
        ret = write(fd, buf, 1);
        if (ret < 0) {
            ret = -errno;
            printf("Failed to issue write (%d)\n", ret);
        }
        sleep(1);
    }

    
    /* release line */
    ret = close(fd);
    if (ret == -1) {
        perror("Failed to close GPIO LINEHANDLE device file");
        ret = -errno;
    }
    return ret;
}
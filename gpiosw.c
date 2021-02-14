#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
 
#include <linux/gpio.h>
 
// 设备树配置
// 		r_pio: pinctrl@1f02c00 {
// 			compatible = "allwinner,sun8i-h3-r-pinctrl";
// 			reg = <0x01f02c00 0x400>;
// 			interrupt-parent = <&r_intc>;
// 			interrupts = <GIC_SPI 45 IRQ_TYPE_LEVEL_HIGH>;
// 			clocks = <&r_ccu CLK_APB0_PIO>, <&osc24M>, <&rtc 0>;
// 			clock-names = "apb", "hosc", "losc";
// 			gpio-controller;
// 			#gpio-cells = <3>;
// 			interrupt-controller;
// 			#interrupt-cells = <3>;

// 			r_ir_rx_pin: r-ir-rx-pin {
// 				pins = "PL11";
// 				function = "s_cir_rx";
// 			};

// 			r_i2c_pins: r-i2c-pins {
// 				pins = "PL0", "PL1";
// 				function = "s_i2c";
// 			};

// 			r_pwm_pin: r-pwm-pin {
// 				pins = "PL10";
// 				function = "s_pwm";
// 			};
// 		};


#define USE_POLL 0

#if USE_POLL
#include <sys/poll.h>
static void usePollEvent(struct gpioevent_request *event_req)
{
    int ret;
	struct pollfd poll_fd;
	struct gpioevent_data event_data;
	poll_fd.fd = event_req->fd;	//注意这个event_req.fd是ret = ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, &event_req);返回的
	poll_fd.events = POLLIN;
	
	while(1) {		
		/* event测试，即监控IO输入电平的高低，触发事件的方式有上升沿触发、下降沿触发等 */
		ret = poll(&poll_fd, 1, 3000);
		if(ret == 0)
			printf("poll time out \n");
		else{
            if( poll_fd.revents == POLLIN)  {
                event_data.timestamp = 0;
                event_data.id = 0;
                // 这里一定要将中断的事件读出来，否则会一直处于触发状态
                read(event_req->fd, &event_data, sizeof(event_data));
                // event_data.timestamp是以纳秒为单位的时间戳
                printf("poll event_data.timestamp:%lluns, event_data.id:%d \n", event_data.timestamp, event_data.id);
            }
		}
	}
}
#else
#include <sys/select.h>
static void useSelectEvent(struct gpioevent_request *event_req)
{
    int ret;
    struct timeval tv;
    fd_set key_fds;//定义一个pollfd结构体key_fds
	struct gpioevent_data event_data;
    tv.tv_sec = 5;
	tv.tv_usec = 0;
    
    FD_ZERO(&key_fds);
    FD_SET(event_req->fd , &key_fds);
    while(1)
    {
        ret = select(event_req->fd + 1, &key_fds, NULL, NULL, &tv);
        if(ret == 0 ){
            printf("select time out\n");
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            FD_ZERO(&key_fds);
            FD_SET(event_req->fd, &key_fds);//timeout will clean fdset
        }
        else if(ret == -1)  {//失败
            printf("fail to select!\n");
        }
        else    {
            event_data.timestamp = 0;
            event_data.id = 0;
            // 这里一定要将中断的事件读出来，否则会一直处于触发状态
            read(event_req->fd, &event_data, sizeof(event_data));
            // event_data.timestamp是以纳秒为单位的时间戳
            printf("select event_data.timestamp:%lluns, event_data.id:%d \n", event_data.timestamp, event_data.id);
        }
    }
}
#endif

int main(int argc, char **argv)
{
	char chrdev_name[20];
	int fd, ret;
	
	struct gpiochip_info chip_info;
	struct gpioline_info line_info;
	struct gpioevent_request event_req;
    
	 
	//如果不知道gpiochipN对应的是GPIO?，可以查看/sys/bus/gpio/devices/gpiochip0/of_node# cat st,bank-name 输出结果是GPIOA
    //或者查看dts中对应的驱动文件，或者cat /sys/bus/gpio/devices/gpiochip0/of_node/compatible
	strcpy(chrdev_name, "/dev/gpiochip1");
	 
	/* Open device: gpiochip11 for GPIO bank Z */
	fd = open(chrdev_name, 0);
	if (fd == -1) {
		ret = -errno;
		fprintf(stderr, "Failed to open %s\n", chrdev_name);
		 
		return ret;
	}
	
	/*
	获取gpio chip info，即打开的设备文件"/dev/gpiochip11"的chipinfo
	*/
	if ((ret = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &chip_info)) < 0){
         printf("GPIO_GET_CHIPINFO_IOCTL failed\n");
         return -errno;
     }
     printf("chip_info.name = %s, chip_info.label = %s, chip_info.lines = %d\n", \
			chip_info.name, chip_info.label, chip_info.lines);
	 
	/*
	获取gpio line info, 即获取chip"/dev/gpiochip11"里面第0个引脚的info，即pz0的info
	*/
	line_info.line_offset = 3;
     if ((ret = ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &line_info)) < 0){
         printf("GPIO_GET_LINEINFO_IOCTL failed\n");
         return -errno;
     }
     printf("line_info.line_offset = %d, line_info.flags = %d, line_info.name = %s, line_info.consumer = %s\n", line_info.line_offset, line_info.flags, line_info.name, line_info.consumer);
	 
	/* event request */
	event_req.lineoffset = 3;
	event_req.handleflags = GPIOHANDLE_REQUEST_INPUT;
	event_req.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;//GPIOEVENT_REQUEST_RISING_EDGE;
 
	ret = ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, &event_req);	// event测试的时候用这个
	if (ret == -1) {
		ret = -errno;
		fprintf(stderr, "Failed to issue GET LINEHANDLE IOCTL (%d)\n", ret);
	}
	if (close(fd) == -1)
		perror("Failed to close GPIO character device file");
	
#if USE_POLL
    usePollEvent(&event_req);
#else
    useSelectEvent(&event_req);
#endif
	 
	/* release line */
	ret = close(fd);
	if (ret == -1) {
		perror("Failed to close GPIO LINEHANDLE device file");
		ret = -errno;
	}
	return ret;
}
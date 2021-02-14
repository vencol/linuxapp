
#include <linux/input.h>       
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/select.h>


// 设备树配置
// 	r_gpio_keys {
// 		compatible = "gpio-keys";

// 		sw4 {
// 			label = "sw4";
// 			linux,code = <KEY_POWER>;
// 			gpios = <&r_pio 0 3 GPIO_ACTIVE_LOW>;
// 		};
// 	};

int main(int argc, char **argv)
{
    int fd;
    char* filename="/dev/input/event0";
    struct input_event ipenv;
    int ret, timecounter=0;
    fd_set key_fds;//定义一个pollfd结构体key_fds
    struct timeval tv;
    tv.tv_sec = 5;
	tv.tv_usec = 0;


    fd = open(filename, O_RDWR);//打开dev/firstdrv设备文件
    if (fd < 0) { //小于0说明没有成功
        printf("error, can't open %s\n", filename);
        return 0;
    }

    if(argc !=1)    {
        printf("Usage : %s ",argv[0]);
        return 0;
    }


    FD_ZERO(&key_fds);
    FD_SET(fd, &key_fds);
    while(1)
    {
        ret = select(fd + 1, &key_fds, NULL, NULL, &tv);
        if(ret == 0 ){
            printf("select time out\n");
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            FD_ZERO(&key_fds);
            FD_SET(fd, &key_fds);//timeout will clean fdset
        }
        else if(ret == -1)  {//失败
            printf("fail to select!\n");
        }
        else    {
            if(read(fd, &ipenv, sizeof(ipenv)) == sizeof(ipenv))  {
                if(ipenv.type == EV_KEY)  {
                    if(ipenv.value) {
                        printf("select key %d Pressed time:%d.%d ms\n", ipenv.code, ipenv.time.tv_sec, ipenv.time.tv_usec); 
                        timecounter = ipenv.time.tv_sec;
                    }
                    else    {
                        printf("select key %d Released time:%d.%d ms\n", ipenv.code, ipenv.time.tv_sec, ipenv.time.tv_usec); 
                        if(ipenv.time.tv_sec > timecounter + 3) {
                            timecounter = open("/dev/watchdog0", O_RDWR);
                            if (timecounter)    {
                                ret = 0;
                                write(timecounter, &ret, 1);
                                close(timecounter);
                            }
                            else
                                printf("open wdt fail");
                        }
                        timecounter = 0;
                    }
                }
            }
            else
                printf("select key_val read error\n");//打印
        }
    }
    close(fd);

    return 0;
}
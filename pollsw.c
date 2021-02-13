
#include <linux/input.h>      
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>

/*
  *usage ./buttonstest
  */
int main(int argc, char **argv)
{
    int fd;
    char* filename="/dev/input/event0";
    struct input_event ipenv;
    int ret, timecounter=0;
    struct pollfd key_fds;//定义一个pollfd结构体key_fds


    fd = open(filename, O_RDWR);//打开dev/firstdrv设备文件
    if (fd < 0)//小于0说明没有成功
    {
        printf("error, can't open %s\n", filename);
        return 0;
    }

    if(argc !=1)
    {
        printf("Usage : %s ",argv[0]);
        return 0;
    }
    key_fds.fd = fd;//文件
    key_fds.events = POLLIN;//poll直接返回需要的条件
    while(1)
    {
        ret =  poll(&key_fds, 1, 5000);//调用sys_poll系统调用，如果5S内没有产生POLLIN事件，那么返回，如果有POLLIN事件，直接返回
        if(!ret){
            printf("poll time out\n");
        }
        else    {
            if(key_fds.revents & POLLIN)//如果返回的值是POLLIN，说明有数据POLL才返回的
            {
                if(read(key_fds.fd, &ipenv, sizeof(ipenv)) == sizeof(ipenv))  {
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
                    printf("poll key_val read error\n");//打印
            }
        }
    }
    close(fd);

    return 0;
}
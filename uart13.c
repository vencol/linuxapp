

/* 包含的头文件 */
#include <stdio.h>        //标准输入输出,如printf、scanf以及文件操作
#include <stdlib.h>        //标准库头文件，定义了五种类型、一些宏和通用工具函数
#include <unistd.h>        //定义 read write close lseek 等Unix标准函数
#include <sys/types.h>    //定义数据类型，如 ssiz e_t off_t 等
#include <sys/stat.h>    //文件状态
#include <fcntl.h>        //文件控制定义
#include <termios.h>    //终端I/O
#include <errno.h>        //与全局变量 errno 相关的定义
#include <getopt.h>        //处理命令行参数
#include <string.h>        //字符串操作
#include <time.h>        //时间
#include <sys/select.h>    //select函数
#include <pthread.h>


// 设备树配置
// 		pio: pinctrl@1c20800 {
// 			/* compatible is in per SoC .dtsi file */
// 			reg = <0x01c20800 0x400>;
// 			interrupt-parent = <&r_intc>;
// 			interrupts = <GIC_SPI 11 IRQ_TYPE_LEVEL_HIGH>,
// 				     <GIC_SPI 17 IRQ_TYPE_LEVEL_HIGH>;
// 			clocks = <&ccu CLK_BUS_PIO>, <&osc24M>, <&rtc 0>;
// 			clock-names = "apb", "hosc", "losc";
// 			gpio-controller;
// 			#gpio-cells = <3>;
// 			interrupt-controller;
// 			#interrupt-cells = <3>;
            
// 			uart1_pins: uart1-pins {
// 				pins = "PG6", "PG7";
// 				function = "uart1";
// 			};
// 			uart3_pins: uart3-pins {
// 				pins = "PA13", "PA14";
// 				function = "uart3";
// 			};
//         }   
// 		uart1: serial@1c28400 {
// 			compatible = "snps,dw-apb-uart";
// 			reg = <0x01c28400 0x400>;
// 			interrupts = <GIC_SPI 1 IRQ_TYPE_LEVEL_HIGH>;
// 			reg-shift = <2>;
// 			reg-io-width = <4>;
// 			clocks = <&ccu CLK_BUS_UART1>;
// 			resets = <&ccu RST_BUS_UART1>;
// 			dmas = <&dma 7>, <&dma 7>;
// 			dma-names = "rx", "tx";
// 			status = "disabled";
// 		};   
// &uart1 {
// 	pinctrl-names = "default";
// 	pinctrl-0 = <&uart1_pins>;
// 	status = "okay";
// };
// 		uart3: serial@1c28c00 {
// 			compatible = "snps,dw-apb-uart";
// 			reg = <0x01c28c00 0x400>;
// 			interrupts = <GIC_SPI 3 IRQ_TYPE_LEVEL_HIGH>;
// 			reg-shift = <2>;
// 			reg-io-width = <4>;
// 			clocks = <&ccu CLK_BUS_UART3>;
// 			resets = <&ccu RST_BUS_UART3>;
// 			dmas = <&dma 9>, <&dma 9>;
// 			dma-names = "rx", "tx";
// 			status = "disabled";
// 		};
// &uart3 {
// 	pinctrl-names = "default";
// 	pinctrl-0 = <&uart3_pins>;
// 	status = "okay";
// };


//修改设备树使能对应的文件，相关的设备文件可以通过，dmesg | grep tty查看得知
#define DEV_NAME1    "/dev/ttyS1"    ///< 串口设备
#define DEV_NAME2    "/dev/ttyS2"    ///< 串口设备


/**@brief   设置串口参数：波特率，数据位，停止位和效验位
 * @param[in]  fd         类型  int      打开的串口文件句柄
 * @param[in]  nSpeed     类型  int     波特率
 * @param[in]  nBits     类型  int     数据位   取值 为 7 或者8
 * @param[in]  nParity     类型  int     停止位   取值为 1 或者2
 * @param[in]  nStop      类型  int      效验类型 取值为N,E,O,,S
 * @return     返回设置结果
 * - 0         设置成功
 * - -1     设置失败
 */
static int setOpt(int fd, int nSpeed, int nBits, int nParity, int nStop)
{
    struct termios newtio, oldtio;

    // 保存测试现有串口参数设置，在这里如果串口号等出错，会有相关的出错信息
    if (tcgetattr(fd, &oldtio) != 0)
    {
        perror("SetupSerial 1");
        return -1;
    }

    bzero(&newtio, sizeof(newtio));        //新termios参数清零
    newtio.c_cflag |= CLOCAL | CREAD;    //CLOCAL--忽略 modem 控制线,本地连线, 不具数据机控制功能, CREAD--使能接收标志
    // 设置数据位数
    newtio.c_cflag &= ~CSIZE;    //清数据位标志
    switch (nBits)
    {
        case 7:
            newtio.c_cflag |= CS7;
        break;
        case 8:
            newtio.c_cflag |= CS8;
        break;
        default:
            fprintf(stderr, "Unsupported data size\n");
            return -1;
    }
    // 设置校验位
    switch (nParity)
    {
        case 'o':
        case 'O':                     //奇校验
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'e':
        case 'E':                     //偶校验
            newtio.c_iflag |= (INPCK | ISTRIP);
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
            break;
        case 'n':
        case 'N':                    //无校验
            newtio.c_cflag &= ~PARENB;
            break;
        default:
            fprintf(stderr, "Unsupported parity\n");
            return -1;
    }
    // 设置停止位
    switch (nStop)
    {
        case 1:
            newtio.c_cflag &= ~CSTOPB;
        break;
        case 2:
            newtio.c_cflag |= CSTOPB;
        break;
        default:
            fprintf(stderr,"Unsupported stop bits\n");
            return -1;
    }
    // 设置波特率 2400/4800/9600/19200/38400/57600/115200/230400
    switch (nSpeed)
    {
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 19200:
            cfsetispeed(&newtio, B19200);
            cfsetospeed(&newtio, B19200);
            break;
        case 38400:
            cfsetispeed(&newtio, B38400);
            cfsetospeed(&newtio, B38400);
            break;
        case 57600:
            cfsetispeed(&newtio, B57600);
            cfsetospeed(&newtio, B57600);
            break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        case 230400:
            cfsetispeed(&newtio, B230400);
            cfsetospeed(&newtio, B230400);
            break;
        default:
            printf("\tSorry, Unsupported baud rate, set default 9600!\n\n");
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
    }
    // 设置read读取最小字节数和超时时间
    newtio.c_cc[VTIME] = 1;     // 读取一个字符等待1*(1/10)s
    newtio.c_cc[VMIN] = 1;        // 读取字符的最少个数为1

      tcflush(fd,TCIFLUSH);         //清空缓冲区
      if (tcsetattr(fd, TCSANOW, &newtio) != 0)    //激活新设置
      {
        perror("SetupSerial 3");
          return -1;
     }
      printf("Serial set done!\n");
    return 0;
}

/**@brief 串口读取函数
 * @param[in]  fd         打开的串口文件句柄
 * @param[in]  *rcv_buf 接收缓存指针
 * @param[in]  data_len    要读取数据长度
 * @param[in]  timeout     接收等待超时时间，单位ms
 * @return     返回设置结果
 * - >0      设置成功
 * - 其他      读取超时或错误
 */
static int UART_Recv(int fd, char *rcv_buf, int data_len, int timeout, char *devname)
{
    int len, fs_sel;
    fd_set fs_read;
    struct timeval time;

    time.tv_sec = timeout / 1000;              //set the rcv wait time
    time.tv_usec = timeout % 1000 * 1000;    //100000us = 0.1s

    FD_ZERO(&fs_read);        //每次循环都要清空集合，否则不能检测描述符变化
    FD_SET(fd, &fs_read);    //添加描述符

    // 超时等待读变化，>0：就绪描述字的正数目， -1：出错， 0 ：超时
    fs_sel = select(fd + 1, &fs_read, NULL, NULL, &time);
//    printf("fs_sel = %d\n", fs_sel);
    if(fs_sel)
    {
        len = read(fd, rcv_buf, data_len);
        printf("[%s] receive len:%d [%s] \n", devname, len, rcv_buf);
        return len;
    }
    else
    {
//        printf("Sorry,I am wrong!");
        return -1;
    }
}

/**@brief 串口发送函数
 * @param[in]  fd            打开的串口文件句柄
 * @param[in]  *send_buf     发送数据指针
 * @param[in]  data_len     发送数据长度
 * @return     返回结果
 * - data_len    成功
 * - -1            失败
 */
static int UART_Send(int fd, char *send_buf, int data_len, char *devname)
{
    ssize_t ret = 0;

    ret = write(fd, send_buf, data_len);
    if (ret == data_len)
    {
        printf("[%s] send is %s\n", devname, send_buf);
        return ret;
    }
    else
    {
        printf("write device error\n");
        tcflush(fd,TCOFLUSH);
        return -1;
    }
}

static void* uart1Task (void* arg)
{
    int fdSerial;
    pid_t pid = getpid();
    pthread_t tid = pthread_self();
    printf("uart1Task %s pid: %u, tid: %u (0x%x)\n", (char*)arg, (unsigned int)pid, (unsigned int)tid, (unsigned int)tid);

    // 打开串口设备
    fdSerial = open(DEV_NAME1, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fdSerial < 0)
    {
        perror(DEV_NAME1);
        exit(1);
    }
    // 设置串口阻塞， 0：阻塞， FNDELAY：非阻塞
    if (fcntl(fdSerial, F_SETFL, 0) < 0)    //阻塞，即使前面在open串口设备时设置的是非阻塞的
    {
        printf("fcntl failed!\n");
    }
    else
    {
        printf("fcntl=%d\n", fcntl(fdSerial, F_SETFL, 0));
    }
    if (isatty(fdSerial) == 0)
    {
        printf("standard input is not a terminal device\n");
        close(fdSerial);
        exit(1);
    }
    else
    {
        printf("is a tty success!\n");
    }
    printf("fd-open=%d\n", fdSerial);


    // 设置串口参数
    if (setOpt(fdSerial, 115200, 8, 'N', 1)== -1)    //设置8位数据位、1位停止位、无校验
    {
        fprintf(stderr, "Set opt Error\n");
        close(fdSerial);
        exit(1);
    }

    tcflush(fdSerial, TCIOFLUSH);    //清掉串口缓存
    fcntl(fdSerial, F_SETFL, 0);    //串口阻塞


    char rcv_buf[100];
    int len;

    while(1)    //循环读取数据
    {
        len = UART_Recv(fdSerial, rcv_buf, 99, 1000, DEV_NAME1);
        if(len > 0)
        {
            rcv_buf[len] = '\0';
            rcv_buf[len-2]++;
            UART_Send(fdSerial, rcv_buf, len, DEV_NAME1);
        }
        else
        {
            // UART_Send(fdSerial, "my test0", 9, DEV_NAME1);//self send and self receive
//            printf("cannot receive data\n");
        }
        // usleep(100000);    //休眠100ms
        sleep(1);
    }
}

static void* uart3Task (void* arg)
{
    int fdSerial;
    pid_t pid = getpid();
    pthread_t tid = pthread_self();
    printf("uart3Task %s pid: %u, tid: %u (0x%x)\n", (char*)arg, (unsigned int)pid, (unsigned int)tid, (unsigned int)tid);

    // 打开串口设备
    fdSerial = open(DEV_NAME2, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fdSerial < 0)
    {
        perror(DEV_NAME2);
        exit(1);
    }
    // 设置串口阻塞， 0：阻塞， FNDELAY：非阻塞
    if (fcntl(fdSerial, F_SETFL, 0) < 0)    //阻塞，即使前面在open串口设备时设置的是非阻塞的
    {
        printf("fcntl failed!\n");
    }
    else
    {
        printf("fcntl=%d\n", fcntl(fdSerial, F_SETFL, 0));
    }
    if (isatty(fdSerial) == 0)
    {
        printf("standard input is not a terminal device\n");
        close(fdSerial);
        exit(1);
    }
    else
    {
        printf("is a tty success!\n");
    }
    printf("fd-open=%d\n", fdSerial);


    // 设置串口参数
    if (setOpt(fdSerial, 115200, 8, 'N', 1)== -1)    //设置8位数据位、1位停止位、无校验
    {
        fprintf(stderr, "Set opt Error\n");
        close(fdSerial);
        exit(1);
    }

    tcflush(fdSerial, TCIOFLUSH);    //清掉串口缓存
    fcntl(fdSerial, F_SETFL, 0);    //串口阻塞


    char rcv_buf[100];
    int len;

    while(1)    //循环读取数据
    {
        len = UART_Recv(fdSerial, rcv_buf, 99, 1000, DEV_NAME2);
        if(len > 0)
        {
            rcv_buf[len] = '\0';
            rcv_buf[len-2]++;
            if(rcv_buf[len-2] == ':')
                rcv_buf[len-2] = '0';
            UART_Send(fdSerial, rcv_buf, len, DEV_NAME2);
        }
        else
        {
            UART_Send(fdSerial, "my test0", 9, DEV_NAME2);//self send and self receive
//            printf("cannot receive data\n");
        }
        // usleep(100000);    //休眠100ms
        sleep(1);
    }
}

/**@fn main
 * @brief main入口函数
 */
int main (int argc, char *argv[])
{
    pthread_t tid1, tid2;
    if (pthread_create(&tid1, NULL, (void*)uart1Task, "uart1Task") != 0) {
        printf("pthread_create error.");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&tid2, NULL, (void*)uart3Task, "uart3Task") != 0) {
        printf("pthread_create error.");
        exit(EXIT_FAILURE);
    }
    pthread_detach(tid2);

    char* rev = NULL;
    pthread_join(tid1, (void *)&rev);
    printf("%s return.\n", rev);
    pthread_cancel(tid2);

    printf("main thread end.\n");
    return 0;
}
#include <errno.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../oled/ssd1306.h"
#include "buttons.h"

#define MAX_LEN 2000
#define SPEED_ARR_SIZE 4

#define GPIO_LED "/sys/class/gpio/gpio10"
#define GPIO_K1 "/sys/class/gpio/gpio0"
#define GPIO_K2 "/sys/class/gpio/gpio2"
#define GPIO_K3 "/sys/class/gpio/gpio3"
#define LED "10"
#define K1 "0"
#define K2 "2"
#define K3 "3"

#define PATH_QUEUE "/screenqueue"
#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#define MODE_PATH "/sys/devices/platform/fandriver/mode"
#define SPEED_PATH "/sys/devices/platform/fandriver/speed"

enum mode { manual, automatic };

// current
enum mode current;
// speed
const int size            = SPEED_ARR_SIZE;
int speed[SPEED_ARR_SIZE] = {2, 5, 10, 20};
int idx;

static void set_drv_mode(enum mode mode_to_set) {
    char* curmode = (mode_to_set == manual) ? "manual\n" : "automatic\n";
    int fd = open(MODE_PATH, O_WRONLY);
    int ret = write(fd, curmode, strlen(curmode));
    close(fd);

}

static void set_drv_speed(int curr_speed) {
    char str_speed[10];
    sprintf(str_speed, "%d", curr_speed);
    int fd = open(SPEED_PATH, O_WRONLY);
    str_speed[10] = '\n';
    int ret = write(fd, str_speed, strlen(str_speed));
    close(fd);
}

static int get_drv_speed() {
    char sspeed[10];
    int ispeed = 0;
    int fd = open(SPEED_PATH, O_RDONLY);
    if (fd != -1) {
        read(fd, &sspeed, sizeof(sspeed));
        ispeed = atoi(sspeed);
    }
    close(fd);
    return ispeed;
}

static void set_auto() { current = automatic; }

static void set_manual() { current = manual; }

static enum mode get_mode() { return current; }

static int get_speed() { 
    if(get_mode()== automatic){
        return get_drv_speed();
    }
    return speed[idx]; 
    }

static int set_speed_up()
{
    if (idx == size - 1) {
        return speed[idx];
    } else {
        idx++;
    }
    return speed[idx];
}
static int set_speed_down()
{
    if (idx == 0) {
        return speed[idx];
    } else {
        idx--;
    }
    return speed[idx];
}

static int read_temp(int fd)
{
    char stemp[10];
    int temp = 0;
    if (fd != -1) {
        read(fd, &stemp, sizeof(stemp));
        temp = atoi(stemp);
        temp /= 1000;  // represent in degrees
    }
    return temp;
}

static void set_screen()
{
    ssd1306_set_position(0, 0);
    ssd1306_puts("-System Status-");
    ssd1306_set_position(0, 1);
    ssd1306_puts("-Nanopi Cooling-");
    ssd1306_set_position(0, 2);
    ssd1306_puts("--------------");
}

static void set_screen_mode(enum mode curr)
{
    char* curmode = (curr == manual) ? "manual   " : "automatic";
    char mode_buf[50];
    sprintf(mode_buf, "mode: %s", curmode);
    ssd1306_set_position(0, 3);
    ssd1306_puts(mode_buf);
}

static void set_screen_temp(int temp)
{
    ssd1306_set_position(0, 4);
    char temp_buf[50];
    sprintf(temp_buf, "Temp: %d  ", temp);
    ssd1306_puts(temp_buf);
    ssd1306_set_position(10, 4);
    ssd1306_puts("'C");
}

static void set_screen_freq(int freq)
{
    ssd1306_set_position(0, 5);
    char freq_buf[50];
    sprintf(freq_buf, "Freq: %d  ", freq);
    ssd1306_puts(freq_buf);
    ssd1306_set_position(10, 5);
    ssd1306_puts("Hz");
}

static void set_screen_duty(int duty)
{
    ssd1306_set_position(0, 6);
    char duty_buf[50];
    sprintf(duty_buf, "Duty: %d  ", duty);
    ssd1306_puts(duty_buf);
    ssd1306_set_position(10, 6);
    ssd1306_puts("%");
}

int main()
{
    int fd_temp = open(TEMP_PATH, O_RDONLY);
    if (fd_temp == -1) {
        perror("error: opening temp file");
    }
    ssd1306_init();

    set_screen();
    set_screen_freq(get_speed());
    set_screen_mode(get_mode());
    set_screen_temp(read_temp(fd_temp));
    set_screen_duty(0);

    int k1 = open_gpio(K1, GPIO_K1, "in", "rising");
    int k2 = open_gpio(K2, GPIO_K2, "in", "rising");
    int k3 = open_gpio(K3, GPIO_K3, "in", "rising");

    struct epoll_event ret, evk1, evk2, evk3, emq, etemp;

    // create epoll
    int epollfd = create_epoll();
    // create msg queue
    mqd_t mqd = mq_open(PATH_QUEUE, O_CREAT | O_RDWR | O_NONBLOCK, 0x660, NULL);

    if (mqd == -1) {
        printf("MQ open failed: %s\n", strerror(errno));
    }

    // register buttons to epoll
    register_fd_event(epollfd, k1, EPOLLIN | EPOLLET, &evk1);
    register_fd_event(epollfd, k2, EPOLLIN | EPOLLET, &evk2);
    register_fd_event(epollfd, k3, EPOLLIN | EPOLLET, &evk3);
    // register message queue to epoll
    register_fd_event(epollfd, mqd, EPOLLIN | EPOLLET, &emq);
    // register temperature file
    register_fd_event(epollfd, fd_temp, EPOLLIN | EPOLLET, &etemp);
    struct mq_attr mqatt;
    mq_getattr(mqd, &mqatt);

    while (true) {
        char* info = (char*)malloc(sizeof(char) * MAX_LEN);
        int nfds   = epoll_wait(epollfd, &ret, 1, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        if (ret.data.fd == mqd) {
            // receive from client program
            int recvlen = mq_receive(mqd, info, mqatt.mq_msgsize, NULL);
            if (recvlen != -1) {
                printf("%s\n", info);
            }

        } else if (ret.data.fd == fd_temp) {
            // update temperature
            set_screen_temp(read_temp(fd_temp));
        } else if (ret.data.fd == k1) {
            // put speed up
            uint64_t temp;
            read(k1, &temp, sizeof(temp));
            if (get_mode() == manual) {
                set_speed_up();
                set_drv_speed(get_speed());
                set_screen_freq(get_speed());
            }
        } else if (ret.data.fd == k2) {
            // put speed down
            uint64_t temp;
            read(k1, &temp, sizeof(temp));
            if (get_mode() == manual) {
                set_speed_down();
                set_drv_speed(get_speed());
                set_screen_freq(get_speed());
            }

        } else if (ret.data.fd == k3) {
            // change mode
            uint64_t temp;
            read(k1, &temp, sizeof(temp));
            (get_mode() == manual) ? set_auto() : set_manual();
            set_drv_mode(get_mode());
            set_screen_mode(get_mode());
            if(get_mode()==automatic){
                set_screen_freq(get_speed());
            }
        }

        free(info);
        close(fd_temp);
    }

    return 0;
}
/**
 * Copyright 2018 University of Applied Sciences Western Switzerland / Fribourg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Project: HEIA-FR / HES-SO MSE - MA-CSEL1 Laboratory
 *
 * Abstract: System programming -  file system
 *
 * Purpose: NanoPi status led control system
 *
 * Autĥor:  Julien Piguet
 * Date:    28.04.2023
 */

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED "/sys/class/gpio/gpio10"
#define GPIO_K1 "/sys/class/gpio/gpio0"
#define GPIO_K2 "/sys/class/gpio/gpio2"
#define GPIO_K3 "/sys/class/gpio/gpio3"
#define LED "10"
#define K1 "0"
#define K2 "2"
#define K3 "3"

#define MAX_EVENTS 10

 #define NB_FD 4
 int fds[NB_FD];

static int open_led()
{
    // unexport pin out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // config pin
    f = open(GPIO_LED "/direction", O_WRONLY);
    write(f, "out", 3);
    close(f);

    // open gpio value attribute
    f = open(GPIO_LED "/value", O_RDWR);
    return f;
}

static int open_gpio(char* id, char* file, char* direction, char* edge){
    char direction_str[48];
    char edge_str[48];
    char value_str[48];

    // unexport pin out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, id, strlen(id));
    close(f);

    // export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, id, strlen(id));
    close(f);

    // config direction
    strcat(strcpy(direction_str, file), "/direction");
    f = open(direction_str, O_WRONLY);
    write(f, direction, strlen(direction));
    close(f);

    // config edge
    strcat(strcpy(edge_str, file), "/edge");
    f = open(edge_str, O_WRONLY);
    write(f, edge, strlen(edge));
    close(f);

    // open gpio value attribute
    strcat(strcpy(value_str, file), "/value");
    f = open(value_str, O_RDWR);
    return f;
}

int register_fd_event(int epfd, int fd, enum EPOLL_EVENTS ev, struct epoll_event* event)
{
    event->data.fd = fd;
    event->events = ev;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event);
    if (ret == -1)
    {
        perror("error setting controller on file descriptor\n");
        return -1;
    }
    return 0;
}

int create_tfd(){
    int tfd;
    tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1) {
        perror("timerfd_create() failed: errno=%d\n", errno);
        return EXIT_FAILURE;
    }
    return tfd;
}

int set_tfd_interval(int tfd, long sec, long nsec){
    struct itimerspec new_value;
    struct timespec now;

    if (clock_gettime(CLOCK_REALTIME, &now) == -1){
        perror("clock_gettime() failed: errno=%d\n", errno);
        return EXIT_FAILURE;
    }

    new_value.it_interval.tv_sec = sec;
	new_value.it_interval.tv_nsec = nsec;
    new_value.it_value.tv_sec = now.tv_sec;
    new_value.it_value.tv_nsec = now.tv_nsec;

    if (timerfd_settime(tfd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1){
        perror("timerfd_settime() failed: errno=%d\n", errno);
        return EXIT_FAILURE;
    }
    return 0;
}

int set_tfd_freq(int tfd, long period){
    long sec = period / 1000;
    long nsec = (period % 1000) * 1000000;
    return set_tfd_interval(tfd, sec, nsec);
}

int main(int argc, char* argv[])
{
    (void)(argc);
    (void)(argv);
    int led, k1, k2, k3;
    long init_period = 250; // ms
    long period = init_period;

    led = open_led();
    k1 = open_gpio(K1, GPIO_K1, "in", "rising");
    k2 = open_gpio(K2, GPIO_K2, "in", "rising");
    k3 = open_gpio(K3, GPIO_K3, "in", "rising");

    int tfd;
    
    tfd = create_tfd();
    set_tfd_freq(tfd, init_period);

    int epollfd, nfds;
    struct epoll_event evt1, ret;
    struct epoll_event evk1, evk2, evk3;

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    register_fd_event(epollfd, tfd, EPOLLIN, &evt1);
    register_fd_event(epollfd, k1, EPOLLIN | EPOLLET, &evk1);
    register_fd_event(epollfd, k2, EPOLLIN | EPOLLET, &evk2);   
    register_fd_event(epollfd, k3, EPOLLIN | EPOLLET, &evk3);   

    int k = 0;
    while (1) {
        nfds = epoll_wait(epollfd, &ret, 1, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        if (ret.data.fd == tfd) {
            uint64_t temp;
            read(tfd, &temp, sizeof(temp));
            k  = (k + 1) % 2;
            if (k == 0)
                pwrite(led, "1", sizeof("1"), 0);
            else
                pwrite(led, "0", sizeof("0"), 0);
        } else if (ret.data.fd == k1){
            uint64_t temp;
            read(k1, &temp, sizeof(temp));
            perror("K1");
            period -= 10;
            if (period < 0) period = 0;
            set_tfd_freq(tfd, period);
        } else if (ret.data.fd == k2){
            uint64_t temp;
            read(k2, &temp, sizeof(temp));
            perror("K2");
            period = init_period;
            set_tfd_freq(tfd, period);
        } else if (ret.data.fd == k3){
            uint64_t temp;
            read(k3, &temp, sizeof(temp));
            perror("K3");
            period += 10;
            set_tfd_freq(tfd, period);
        } else {
            perror("Another fd");
            uint64_t temp;
            read(ret.data.fd, &temp, sizeof(temp));
        }
        
    }

    return 0;
}

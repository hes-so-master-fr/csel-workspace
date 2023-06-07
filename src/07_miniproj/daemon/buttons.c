#include "buttons.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"

int open_gpio(char* id, char* file, char* direction, char* edge)
{
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

int create_epoll()
{
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        printf("error creating epoll");
        exit(EXIT_FAILURE);
    }
    return epollfd;
}

int register_fd_event(int epfd,
                      int fd,
                      enum EPOLL_EVENTS ev,
                      struct epoll_event* event)
{
    event->data.fd = fd;
    event->events  = ev;
    int ret        = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event);
    if (ret == -1) {
        perror("error setting controller on file descriptor\n");
        return -1;
    }
    return 0;
}

int create_tfd()
{
    int tfd;
    tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1) {
        perror("timerfd_create() failed\n");
        return EXIT_FAILURE;
    }
    return tfd;
}

int set_tfd_interval(int tfd, long sec, long nsec)
{
    struct itimerspec new_value;
    struct timespec now;

    if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
        perror("clock_gettime() failed\n");
        return EXIT_FAILURE;
    }

    new_value.it_interval.tv_sec  = sec;
    new_value.it_interval.tv_nsec = nsec;
    new_value.it_value.tv_sec     = now.tv_sec;
    new_value.it_value.tv_nsec    = now.tv_nsec;

    if (timerfd_settime(tfd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1) {
        perror("timerfd_settime() failed\n");
        return EXIT_FAILURE;
    }
    return 0;
}

int set_tfd_freq(int tfd, long period)
{
    long sec  = period / 1000;
    long nsec = (period % 1000) * 1000000;
    syslog(LOG_INFO, "Set frequence to %f", (double)period / 1000.0);
    return set_tfd_interval(tfd, sec, nsec);
}
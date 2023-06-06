#include "buttons.h"
#include <stdbool.h>

#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"

int open_gpio(char* id, char* file, char* direction, char* edge){
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


int create_epoll(){
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    return epollfd;
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


#pragma once

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

int open_gpio(char* id, char* file, char* direction, char* edge);

int create_epoll();

int register_fd_event(int epfd,
                      int fd,
                      enum EPOLL_EVENTS ev,
                      struct epoll_event* event);

int create_tfd();
int set_tfd_interval(int tfd, long sec, long nsec);
int set_tfd_freq(int tfd, long period);
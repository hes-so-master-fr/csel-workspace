#include <mqueue.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PATH_QUEUE "/screenqueue"

void show_help()
{
    char* help = "-h  show this help file\n-a automatic mode\n-m manual mode";

    printf("%s\n", help);
}

int main(int argc, char* argv[])
{
    int opt;
    int set_a = 0;
    int set_m = 0;

    while ((opt = getopt(argc, argv, "ham")) != -1) {
        switch (opt) {
            case 'h':
                show_help();
                break;
            case 'a':
                set_a = 1;
                break;
            case 'm':
                set_m = 1;
                break;
            default:
                printf("error parsing arguments\n");
                break;
        }
    }
    char* msg = "none";
    if (set_a == 1) {
        msg = "automatic\n";
    } else {
        msg = "manual\n";
    }
    if (set_a == 1 ||set_m == 1) {
        mqd_t mqd = mq_open(PATH_QUEUE, O_CREAT | O_RDWR, 0x660, NULL);

        int prio = 1;
        mq_send(mqd, msg, strlen(msg), prio);
        printf("set application in mode %s\n", msg);
        mq_close(mqd);
    }
    return 0;
}
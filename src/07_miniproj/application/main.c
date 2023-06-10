#include <mqueue.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include  <stdlib.h>
#define PATH_QUEUE "/clientqueue"
#define ULTRA_LOW 2
#define LOW 5
#define MEDIUM 10
#define HIGH 20
struct Param {
    char mode[100];
    int value;
};

/*
@brief will show help
*/
void show_help()
{
    char* help = "-h  show this help file\n-a automatic mode\n-m manual mode [2,5,10,20]";

    printf("%s\n", help);
}



int main(int argc, char* argv[])
{
    int opt;
    int set_a = 0;
    int set_m = 0;
    int set_h = 0;
    int value = 0;

    while ((opt = getopt(argc, argv, "ham:")) != -1) {
        switch (opt) {
            case 'h':
                show_help();
                set_h = 1;
                break;
            case 'a':
                set_a = 1;
                break;
            case 'm':
                set_m = 1;
                // retrieve value
                value = atoi(optarg);
                break;
            default:
                printf("error parsing arguments\n");
                break;
        }
    }
    struct Param param;
    char* msg = "none";
    //set mode 
    if (set_a == 1) {
        msg = "automatic\n";
    } else {
        msg = "manual\n";
    }
    strcpy(param.mode, msg);
    param.value  = value;
    // if set in manual will parse the value
    if(strncmp("manual",param.mode, strlen("manual"))==0 && set_h==0){
    if(value== ULTRA_LOW || value== LOW || value== MEDIUM || value== HIGH){
        printf("set manual param %d\n", value);
        param.value = value;
    }else{
         printf("invalid manual param %d, will set default 2\n", value);
        param.value = 2;
    }}
    if (set_a == 1 ||set_m == 1) {
        mqd_t mqd = mq_open(PATH_QUEUE, O_CREAT | O_RDWR, 0x660, NULL);

        int prio = 1;
        //send to message queue
        mq_send(mqd, (const char *)&param, sizeof(param), prio);
        printf("set application in mode %s\n", msg);
        mq_close(mqd);
    }
    return 0;
}
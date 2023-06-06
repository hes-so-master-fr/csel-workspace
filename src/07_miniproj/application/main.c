#include <mqueue.h>
#include <string.h>


#define PATH_QUEUE "/screenqueue"

int main(){

    mqd_t mqd = mq_open(PATH_QUEUE, O_CREAT | O_RDWR, 0x660, NULL);
    
    char * msg = "automatic\n";
    int prio = 1;
    mq_send(mqd, msg, strlen(msg), prio);

   mq_close(mqd);

    return 0;
}
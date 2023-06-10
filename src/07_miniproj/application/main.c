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
 * Project: HEIA-FR / HES-SO MSE - MA-CSEL1 Mini-project
 *
 * Abstract: Application to use the fan deamon
 *
 * Purpose: This application will set the fan deamon in manual or automatic mode
 *
 * Autĥor:  Antoine Delabays & Julien Piguet
 * Date:    11.06.2023
 */

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
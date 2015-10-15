/*
 *  * File         : testchardriver.c
 *  * Description  : ELE784 Lab1 charDriver tester
 *  *
 *  * Etudiants:  JORA09019100 (Alexandru Jora)
 *  *             MUKM28098503 (Mukandila Mukandila)
 *  */

#include "charDriver.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define DEVICE_NODE     "/dev/charDriver_Node"

// lazy terminal clearing
#define CLEAR_TERM  printf("\033[2J\033[1;1H")

// global file descriptor var
static int fd;

// prototypes
int menu(void);
int read_mode(void);
int write_mode(void);
int read_non_blocking(void);
int read_blocking(void);
int write_non_blocking(void);
int write_blocking(void);
int ioctl_call(void);

int main(void) {
    int opened = 0;
    while(1){
        int choice = menu();
        switch(choice){

            case 1:
                read_mode();
                break;

            case 2:
                write_mode();
                break;

            case 3:
                ioctl_call();
                break;

            case 4:
                printf("Exiting\n");
                exit(-1);
                break;

            default:
                printf("Command not recognized! Exiting\n");
                exit(-1);
        }
        close(fd);
    }
}

int menu(void){
    CLEAR_TERM;
    int cmd;
    cmd = 0;
    printf("===========================\n");
    printf("charDriver test application\n");
    printf("===========================\n\n");
    printf("Choose a command to perform\n");
    printf("1. Read\n");
    printf("2. Write\n");
    printf("3. Send IOCTL command\n");
    printf("4. Exit\n");
    scanf("%d", &cmd);
    CLEAR_TERM;
    return cmd;
}

int read_blocking(void){
    int numBytes;
    printf("Reading in blocking mode\n");
    printf("How many bytes to read?\n");
    scanf("%d", &numBytes);
    //TODO
    return 0;
}

int read_non_blocking(void){
    int numBytes, i;
    int redo = 0;
    printf("Reading in non blocking mode\n");
    printf("How many bytes to read?\n");
    scanf("%d", &numBytes);
    char bufOut[numBytes];
    // flush buffer
    for(i=0; i<numBytes; i++){
        bufOut[i] = '\0';
    }
    //TODO
    if(fd){
        read(fd, &bufOut, numBytes);
        // add string terminating char
        bufOut[numBytes] = '\0';
        printf("Read from buffer: %s\n\n", bufOut);
    }
    else{
        printf("Error using file descriptor!\n");
        exit(-2);
    }
    printf("Read again?\n");
    printf("1. Yes\n");
    printf("2. No\n");
    scanf("%d", &redo);
    if(redo == 1)
        read_non_blocking();
    else
        return 0;
}

int read_mode(void){
    int read_mode;
    read_mode = 0;
    fd = open(DEVICE_NODE, O_RDONLY);
    if(fd){
        printf("Device succesfully openend in O_RDONLY mode\n\n");
    }
    else{
        printf("Error opening device in O_RDONLY mode\n");
        printf("Exiting...\n");
        exit(-1);
    }
    printf("Choose an opening mode\n");
    printf("1. Blocking\n");
    printf("2. Non-blocking\n");
    scanf("%d", &read_mode);
    CLEAR_TERM;

    switch(read_mode){
        case 1:
            read_blocking();
            break;

        case 2:
            read_non_blocking();
            break;
    }
    return 0;
}

int write_blocking(void){
    int numBytes;
    printf("Writing in blocking mode\n");
    printf("How many bytes to write?\n");
    scanf("%d", &numBytes);
    //TODO
    return 0;
}

int write_non_blocking(void){
    int numBytes, i;
    int redo = 0;
    printf("Writing in non blocking mode\n");
    printf("How many bytes to write?\n");
    scanf("%d", &numBytes);
    char bufIn[numBytes + 1];
    // flush buffer
    for(i=0; i<numBytes; i++){
        bufIn[i] = '\0';
    }
    printf("Data to be written?\n");
    scanf("%s", bufIn);

    if(fd){
        write(fd, &bufIn, numBytes);
        printf("Write to buffer: %s\n\n", bufIn);
    }
    else{
        printf("Error using file descriptor!\n");
        exit(-2);
    }
    printf("Write again?\n");
    printf("1. Yes\n");
    printf("2. No\n");
    scanf("%d", &redo);
    if(redo == 1)
        write_non_blocking();
    else
        return 0;
}

int write_mode(void){
    int write_mode;
    write_mode = 0;
    fd = open(DEVICE_NODE, O_WRONLY);
    if(fd){
        printf("Device succesfully openend in O_RWONLY mode\n\n");
    }
    else{
        printf("Error opening device in O_RWONLY mode\n");
        printf("Exiting...\n");
        exit(-1);
    }
    printf("Choose an opening mode\n");
    printf("1. Blocking\n");
    printf("2. Non-blocking\n");
    scanf("%d", &write_mode);
    CLEAR_TERM;

    switch(write_mode){
        case 1:
            write_blocking();
            break;

        case 2:
            write_non_blocking();
            break;
    }
    return 0;
}

int ioctl_call(void){
    int ioctl_ret, ioctl_cmd, redo, newBufSize;
    printf("Select an IOCTL command\n");
    printf("1. Get number of readers\n");
    printf("2. Get circular buffer data count\n");
    printf("3. Get circular buffer size\n");
    printf("4. Get magic number\n");
    printf("5. Set new circular buffer size\n");
    scanf("%d", &ioctl_cmd);
    CLEAR_TERM;

    switch(ioctl_cmd){
        case 1:
            fd = open(DEVICE_NODE, O_RDONLY);
            if(fd){
                printf("Device succesfully openend in O_RDONLY mode\n\n");
            }
            else{
                printf("Error opening device in O_RDONLY mode\n");
                printf("Exiting...\n");
                exit(-1);
            }
            ioctl(fd, CHARDRIVER_GETNUMREADER, &ioctl_ret);
            printf("Number of readers is: %i\n\n", ioctl_ret);
            printf("Send another command?\n");
            printf("1. Yes\n");
            printf("2. No\n");
            scanf("%d", &redo);
            if(redo == 1)
                ioctl_call();
            else
                return 0;
            break;

        case 2:
            fd = open(DEVICE_NODE, O_RDONLY);
            if(fd){
                printf("Device succesfully openend in O_RDONLY mode\n\n");
            }
            else{
                printf("Error opening device in O_RDONLY mode\n");
                printf("Exiting...\n");
                exit(-1);
            }
            ioctl(fd, CHARDRIVER_GETNUMDATA, &ioctl_ret);
            printf("Circular buffer data count is: %i\n\n", ioctl_ret);
            printf("Send another command?\n");
            printf("1. Yes\n");
            printf("2. No\n");
            scanf("%d", &redo);
            if(redo == 1)
                ioctl_call();
            else
                return 0;
            break;

        case 3:
            fd = open(DEVICE_NODE, O_RDONLY);
            if(fd){
                printf("Device succesfully openend in O_RDONLY mode\n\n");
            }
            else{
                printf("Error opening device in O_RDONLY mode\n");
                printf("Exiting...\n");
                exit(-1);
            }
            ioctl(fd, CHARDRIVER_GETBUFSIZE, &ioctl_ret);
            printf("Circular buffer size is: %i\n\n", ioctl_ret);
            printf("Send another command?\n");
            printf("1. Yes\n");
            printf("2. No\n");
            scanf("%d", &redo);
            if(redo == 1)
                ioctl_call();
            else
                return 0;
            break;

        case 4:
            fd = open(DEVICE_NODE, O_RDONLY);
            if(fd){
                printf("Device succesfully openend in O_RDONLY mode\n\n");
            }
            else{
                printf("Error opening device in O_RDONLY mode\n");
                printf("Exiting...\n");
                exit(-1);
            }
            ioctl(fd, CHARDRIVER_GETMAGICNUMBER, &ioctl_ret);
            printf("Magic number is: %c\n\n", (char)ioctl_ret);
            printf("Send another command?\n");
            printf("1. Yes\n");
            printf("2. No\n");
            scanf("%d", &redo);
            if(redo == 1)
                ioctl_call();
            else
                return 0;
            break;

        case 5:
            printf("Please input new circular buffer size\n");
            scanf("%d", &newBufSize);
            fd = open(DEVICE_NODE, O_WRONLY);
            if(fd){
                printf("Device succesfully openend in O_WRONLY mode\n\n");
            }
            else{
                printf("Error opening device in O_WRONLY mode\n");
                printf("Exiting...\n");
                exit(-1);
            }
            ioctl(fd, CHARDRIVER_SETBUFSIZE, newBufSize);
            printf("Set new circular buffer size to: %i\n\n", newBufSize);
            printf("Send another command?\n");
            printf("1. Yes\n");
            printf("2. No\n");
            scanf("%d", &redo);
            if(redo == 1)
                ioctl_call();
            else
                return 0;
            break;
    }
    return 0;
}
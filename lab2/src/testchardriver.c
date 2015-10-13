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
#define IN_BUF_SIZE     10
#define OUT_BUF_SIZE    10

// lazy terminal clearing
#define CLEAR_TERM  printf("\033[2J\033[1;1H");

int main(void) {
    int opened = 0;
    while(1){
        int choice = menu();
        switch(choice){
            case 1:
                open_mode();
                break;

            case 2:
                if(!opened){
                    printf("***Error you must open device first!***\n\n");
                    break;
                }
                read_mode();
                break;

            case 3:
                if(!opened){
                    printf("***Error you must open device first!***\n\n");
                    break;
                }
                write_mode();
                break;

            case 4:
                if(!opened){
                    printf("***Error you must open device first!***\n\n");
                    break;
                }
                printf("Not implemented!\n");
                break;

            case 5:
                printf("Exiting\n");
                exit(-1);
                break;
        }
    }
}

int menu(void){
    int cmd;
    cmd = 0;
    printf("===========================\n");
    printf("charDriver test application\n");
    printf("===========================\n\n");
    printf("Choose a command to perform\n");
    printf("1. Open\n");
    printf("2. Read\n");
    printf("3. Write\n");
    printf("4. Send IOCTL command\n");
    printf("5. Exit\n");
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
    int numBytes;
    printf("Reading in non blocking mode\n");
    printf("How many bytes to read?\n");
    scanf("%d", &numBytes);
    //TODO
    return 0;
}

int read_mode(void){
    int read_mode;
    read_mode = 0;
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
    int numBytes;
    printf("Writing in non blocking mode\n");
    printf("How many bytes to write?\n");
    scanf("%d", &numBytes);
    //TODO
    return 0;
}

int write_mode(void){
    int write_mode;
    write_mode = 0;
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

int open_mode(void){
    int open_mode;
    int fd;
    open_mode = 0;
    printf("Choose an opening mode\n");
    printf("1. O_RDONLY\n");
    printf("2. O_WRONLY\n");
    printf("3. O_RDRW\n");
    scanf("%d", &open_mode);
    CLEAR_TERM;

    switch(open_mode){
        case 1:
            // open read only
            fd = open(DEVICE_NODE, O_RDONLY);
            if(fd){
                printf("Device succesfully openend in O_RDONLY mode\n");
            }
            else{
                printf("Error opening device in O_RDONLY mode\n");
                printf("Exiting...\n");
                exit(-1);
            }
            read_mode();
            break;

        case 2:
            // open write only
            fd = open(DEVICE_NODE, O_WRONLY);
            if(fd){
                printf("Device succesfully openend in O_RWONLY mode\n");
            }
            else{
                printf("Error opening device in O_RWONLY mode\n");
                printf("Exiting...\n");
                exit(-1);
            }
            write_mode();
            break;

        case 3:
            // open read/write
            fd = open(DEVICE_NODE, O_RDWR);
            if(fd){
                printf("Device succesfully openend in O_RDRW mode\n");
            }
            else{
                printf("Error opening device in O_RDRW mode\n");
                printf("Exiting...\n");
                exit(-1);
            }
            rw_mode();
            break;
    }
    return 0;
}

int rw_mode(void){
    int rw_mode;
    rw_mode = 0;
    printf("Choose a command\n");
    printf("1. Read\n");
    printf("2. Write\n");
    printf("3. Exit\n");
    scanf("%d", &rw_mode);
    CLEAR_TERM;

    do{
        switch(rw_mode){
            case 1:
                read_mode();
                break;

            case 2:
                write_mode();
                break;

            case 3:
                printf("Exiting R/W mode\n");
                return 0;
        }
    } while(!rw_mode);
    return 0;
}


    /*
    // TODO: Tests
    //
    // OPEN READ
    // OPEN WRITE
    // OPEN READ_WRITE
    // READ in BLOCKING/NON_BLOCKING
    // WRITE in BLOCKING/NON_BLOCKING
    // IOCTL

    int fd1, fd2, fd3, fd4, fd5;
    char *bufOut;
    int ret, len;

    char bufIn[] = "test_data";
    len = strlen(bufIn);
    fd1 = open(DEVICE_NODE, O_RDWR); // Open in READ ONLY

    if(fd1)
        printf("Opened in READ/WRITE\n");
    else
        return fd1;

    printf("Writing %i bytes to device\n", len);
    ret = write(fd1, bufIn, len);
    if (ret != len){
        printf("Error writing to device!\n");
    }

    printf("Reading %i bytes from device\n", len);
    ret = read(fd1, &bufOut, len);
    if (ret != len){
        printf("Error reading from device!\n");
    }
    printf("%s\n", bufOut);

    close(fd1);

    exit(0);
    */

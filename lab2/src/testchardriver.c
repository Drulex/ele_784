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

#define DEVICE_NODE     "/dev/charDriver_Node"
#define IN_BUF_SIZE     10
#define OUT_BUF_SIZE    10

int main(void) {

    // TODO: Tests
    //
    // OPEN READ
    // OPEN WRITE
    // OPEN READ_WRITE
    // READ in BLOCKING/NON_BLOCKING
    // WRITE in BLOCKING/NON_BLOCKING
    // IOCTL

    int fd1, fd2, fd3, fd4, fd5;
    char bufOut[OUT_BUF_SIZE];
    char bufIn[IN_BUF_SIZE];
    int ret;
    int ioctl_ret;

    fd1 = open(DEVICE_NODE, O_RDONLY); // Open in READ ONLY
    if(fd1) {

        printf("Opened in READ ONLY\n");
        ioctl(fd1,CHARDRIVER_GETNUMREADER,&ioctl_ret);
        ioctl(fd1,CHARDRIVER_GETNUMDATA,&ioctl_ret);
        ioctl(fd1,CHARDRIVER_GETBUFSIZE,&ioctl_ret);
        ioctl(fd1,CHARDRIVER_GETMAGICNUMBER,&ioctl_ret);
        ioctl(fd1,CHARDRIVER_SETBUFSIZE,300);
        ioctl(fd1,CHARDRIVER_GETBUFSIZE,&ioctl_ret);

//        printf("RETURN is %i\n",ret);

//        printf("IOCTL returned: %i\n", ioctl_ret);

    } else
        return fd1;

    close(fd1);

//    fd2 = open(DEVICE_NODE, O_WRONLY); // Open in WRITE ONLY
//  if(fd2)
//        printf("Opened in WRITE ONLY\n");
//    else
//        return fd2;


    exit(0);
}

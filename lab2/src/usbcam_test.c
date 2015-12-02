/*
 * File         : usbcam.c
 * Description  : ELE784 Lab2 source
 *
 * Etudiants:  JORA09019100 (Alexandru Jora)
 *             MUKM28098503 (Mukandila Mukandila)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "usbcam.h"
#include "dht_data.h"

 /*
1) ouvrir le fichier dans lequel vous enregistrerez l’image
2) Effectuer la commande IOCTL_STREAMON
3) Effectuer la commande IOCTL_GRAB
4) Utiliser la commande read du pilote pour récupérer les données de l’image
5) Effectuer la commande IOCTL_STREAMOFF
6) Effectuer les modifications sur les données (voir exemple plus bas)
7) Écrire le résultat final dans le fichier ouvert à l’étape #1
8) Fermer le fichier ouvert à l’étape #1
 */

#define USBCAM_IMAGE 		"usbcam_image.jpg"
#define USBCAM_DEVICE 		"/dev/usbcam1"
#define USBCAM_BUF_SIZE		42666

#define CLEAR_TERM  printf("\033[2J\033[1;1H")
#define PANTILT_MAX 35

// function prototypes
int menu(void);
int capture_image(int fd);

int main(int argc, char *argv[]) {
	static int fd;
    int cmd, i;


    // open driver in READONLY
	fd = open(USBCAM_DEVICE, O_RDONLY);

    if(fd <= 0){
        printf("ERROR OPENING USB DEVICE!\n NOW EXITING\n");
        return -1;
    }

    printf("USB DEVICE successfully openened in O_RDONLY\n");

    while(cmd != 7) {
        cmd = menu();
        switch(cmd) {

            case 1:
                capture_image(fd);
                break;

            case 2:
                printf("PANTILT: DIR=UP, VALUE=%i\n", PANTILT_MAX);
			    for(i=0; i<PANTILT_MAX; i++) {
					ioctl(fd, IOCTL_PANTILT, 0);
                    sleep(0.1);
			    }
                sleep(1);
                break;

            case 3:
                printf("PANTILT: DIR=DOWN, VALUE=%i\n", PANTILT_MAX);
			    for(i=0; i<PANTILT_MAX; i++) {
					ioctl(fd, IOCTL_PANTILT, 1);
                    sleep(0.1);
			    }
                sleep(1);
                break;

            case 4:
                printf("PANTILT: DIR=LEFT, VALUE=%i\n", PANTILT_MAX);
			    for(i=0; i<PANTILT_MAX; i++) {
					ioctl(fd, IOCTL_PANTILT, 2);
                    sleep(0.1);
			    }
                sleep(1);
                break;

            case 5:
                printf("PANTILT: DIR=RIGHT, VALUE=%i\n", PANTILT_MAX);
			    for(i=0; i<PANTILT_MAX; i++) {
					ioctl(fd, IOCTL_PANTILT, 3);
                    sleep(0.1);
			    }
                sleep(1);
                break;

            case 6:
                printf("PANTILT RESET\n");
				ioctl(fd, IOCTL_PANTILT_RESET);
                sleep(1);
                break;
    	}
    }
    printf("CLOSING USB DEVICE\n");
	close(fd);
	return 0;
}

int menu(void){
    CLEAR_TERM;
    int cmd;
    cmd = 0;
    printf("=======================\n");
    printf("usbcam test application\n");
    printf("=======================\n\n");
    printf("Choose a command to perform\n");
    printf("1. Capture an image\n");
    printf("2. PANTILT UP\n");
    printf("3. PANTILT DOWN\n");
    printf("4. PANTILT LEFT\n");
    printf("5. PANTILT RIGHT\n");
    printf("6. PANTILT RESET\n");
    printf("7. Exit\n");
    scanf("%d", &cmd);
    CLEAR_TERM;
    return cmd;
}

int capture_image(int fd){
	unsigned char *inBuffer;
	unsigned char *finalBuffer;
	unsigned int mySize;
	FILE *foutput;
	long ioctl_return;
	int i = 0;

    // allocate memory to buffers
    inBuffer = (unsigned char *) malloc(USBCAM_BUF_SIZE);
	finalBuffer = (unsigned char *) malloc(USBCAM_BUF_SIZE * 2);

    if((!inBuffer) || (!finalBuffer)) {
		printf("Unable to allocate memory for inBuffer or finalBuffer (%s,%s,%u)\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

    // open jpg file for image
	foutput = fopen(USBCAM_IMAGE, "wb");

    if(foutput != NULL) {

        // streamon
    	ioctl_return = ioctl(fd, IOCTL_STREAMON);

        if(ioctl_return >= 0)
			printf("IOCTL_STREAMON OK!\n");
		else {
			printf("IOCTL_STREAMON ERROR: %ld\n", ioctl_return);
			return -1;
		}

        // grab
		ioctl_return = ioctl(fd, IOCTL_GRAB);

        if(ioctl_return >= 0)
			printf("IOCTL_GRAB OK!\n");
		else {
			printf("IOCTL_GRAB ERROR: %ld\n", ioctl_return);
			return -1;
		}

        // read
        mySize = read(fd, inBuffer, USBCAM_BUF_SIZE);

		if(mySize < 0) {
			printf("READ ERROR: %u\n", mySize);
			return -1;
		}

        // streamoff
        ioctl_return = ioctl(fd, IOCTL_STREAMOFF);

        if(ioctl_return >= 0)
			printf("IOCTL_STREAMOFF OK!\n");
		else {
			printf("IOCTL_STREAMOFF ERROR: %ld\n", ioctl_return);
			return -1;
		}

        if(inBuffer == NULL)
            printf("ERROR: inBuffer is NULL!!\n");

        memcpy(finalBuffer, inBuffer, HEADERFRAME1);
		memcpy(finalBuffer + HEADERFRAME1, dht_data, DHT_SIZE);
		memcpy(finalBuffer + HEADERFRAME1 + DHT_SIZE, inBuffer + HEADERFRAME1, (mySize - HEADERFRAME1));

		fwrite(finalBuffer, mySize + DHT_SIZE, 1, foutput);

        printf("Photo captured!\n");
		fclose(foutput); // #8

        // free buffers
        free(finalBuffer);
        free(inBuffer);
        sleep(3);
    }

    else {
        printf("Unable to img file for writing!\n");
        return -1;
    }

    return 0;
}

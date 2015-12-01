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

int main(int argc, char *argv[]) {
	/*if(!argv[1] || !argv[2]){
		printf("ERROR: Unable to parse arguments!. Please provide arguments as shown below:\n");
		printf("./usbcam_test 'direction' 'value'\n");
		printf("Example: ./usbcam_test down 10\n");
		printf("To call reset provide value of 0\n");
		printf("Example: ./usbcam_test reset 0\n");
		printf("Now exiting\n");
		return -1;
	}*/

	FILE *foutput;
	static int fd;
	unsigned char *inBuffer;
	unsigned char *finalBuffer;
	unsigned int mySize;
	int i = 0;
	int max = atoi(argv[2]);
	long ioctl_return;

	inBuffer = malloc(USBCAM_BUF_SIZE);
	finalBuffer = malloc(USBCAM_BUF_SIZE * 2);

    if((!inBuffer) || (!finalBuffer)) {
		printf("Unable to allocate memory for inBuffer or finalBuffer (%s,%s,%u)\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

/*
    // flush both buffers
    for(i=0; i<USBCAM_BUF_SIZE; i++) {
        inBuffer[i] = '\0';
        finalBuffer[i] = '\0';
    }

    for(i=USBCAM_BUF_SIZE; i<2*USBCAM_BUF_SIZE; i++) {
        finalBuffer[i] = '\0';
    }
*/
	foutput = fopen(USBCAM_IMAGE, "wb"); // #1
	if(foutput != NULL) {

		// open driver in READONLY
		fd = open(USBCAM_DEVICE, O_RDONLY);
		if(fd >= 0)
			printf("File descriptor OPEN\n");
		else
			printf("File descriptor ERROR: %d\n", fd);
		/*
		if(!strcmp(argv[1], "up")){
			printf("PANTILT: DIR=UP, VALUE=%i\n", max);
			for(i=0; i<max; i++){
					ioctl(fd, IOCTL_PANTILT, 0);
			}
			sleep(1);
		}

		else if(!strcmp(argv[1], "down")){
			printf("PANTILT: DIR=DOWN, VALUE=%i\n", max);
			for(i=0; i<max; i++){
					ioctl(fd, IOCTL_PANTILT, 1);
			}
			sleep(1);
		}

		else if(!strcmp(argv[1], "left")){
			printf("PANTILT: DIR=LEFT, VALUE=%i\n", max);
			for(i=0; i<max; i++){
					ioctl(fd, IOCTL_PANTILT, 2);
			}
			sleep(1);
		}

		else if(!strcmp(argv[1], "right")){
			printf("PANTILT: DIR=RIGHT, VALUE=%i\n", max);
			for(i=0; i<max; i++){
					ioctl(fd, IOCTL_PANTILT, 3);
			}
			sleep(1);
		}

		else if(!strcmp(argv[1], "reset")){
			printf("PANTILT_RESET\n");
			ioctl(fd, IOCTL_PANTILT_RESET);
			sleep(1);
		}

		else{
			printf("Command not recognized\n");
			printf("Exiting\n");
			close(fd);
			free(finalBuffer);
			free(inBuffer);
			return -1;
		}*/

		ioctl_return = ioctl(fd, IOCTL_STREAMON); // #2
		if(ioctl_return >= 0)
			printf("IOCTL_STREAMON OK!\n");
		else {
			printf("IOCTL_STREAMON ERROR: %ld\n", ioctl_return);
			return -1;
		}
        //sleep(1);

		ioctl_return = ioctl(fd, IOCTL_GRAB); // #3
		if(ioctl_return >= 0)
			printf("IOCTL_GRAB OK!\n");
		else {
			printf("IOCTL_GRAB ERROR: %ld\n", ioctl_return);
			return -1;
		}

		//sleep(1);

        mySize = read(fd, inBuffer, USBCAM_BUF_SIZE); // #4
        printf("Bytes copied from cam: %u\n", mySize);

		if(mySize < 0) {
			printf("READ ERROR: %u\n", mySize);
			return -1;
		}
		//sleep(1);

		ioctl_return = ioctl(fd, IOCTL_STREAMOFF); // #5
		if(ioctl_return >= 0)
			printf("IOCTL_STREAMOFF OK!\n");
		else {
			printf("IOCTL_STREAMOFF ERROR: %ld\n", ioctl_return);
			return -1;
		}
		//sleep(1);

        if(inBuffer == NULL)
            printf("ERROR: inBuffer is NULL!!");

        printf("Contents of inbuffer:\n");
        for(i=0; i<USBCAM_BUF_SIZE; i++){
            printf("%c", &inBuffer[i]);
        }
        printf("\n");
		// #6
        printf("MEMCPY (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        memcpy(finalBuffer, inBuffer, HEADERFRAME1);
        printf("MEMCPY (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
		memcpy(finalBuffer + HEADERFRAME1, dht_data, DHT_SIZE);
        printf("MEMCPY (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
		memcpy(finalBuffer + HEADERFRAME1 + DHT_SIZE, inBuffer + HEADERFRAME1, (mySize - HEADERFRAME1));
        printf("MEMCPY (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);

		fwrite(finalBuffer, mySize + DHT_SIZE, 1, foutput); // #7*/
		fclose(foutput); // #8

	}

	close(fd);
	free(finalBuffer);
	free(inBuffer);

	return 0;

}

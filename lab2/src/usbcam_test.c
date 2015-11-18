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
#include <fcntl.h>
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

#define USBCAM_IMAGE 		"/tmp/usbcam_image.jpg"
#define USBCAM_DEVICE 		"/dev/usbcam1"
#define USBCAM_BUF_SIZE		42666

int main(void) {

	FILE *foutput;
	static int fd;
	unsigned char * inBuffer;
	unsigned char * finalBuffer;
	unsigned int mySize;

	inBuffer = malloc((USBCAM_BUF_SIZE) * sizeof(unsigned char));
	finalBuffer = malloc((USBCAM_BUF_SIZE * 2) * sizeof(unsigned char));

	if((!inBuffer) || (!finalBuffer)) {
		print("Unable to allocate memory for inBuffer or finalBuffer (%s,%s,%u)\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	foutput = fopen(USBCAM_IMAGE, "wb"); // #1
	if(foutput != NULL) {

		// open driver in READONLY
		fd = open(USBCAM_DEVICE, O_RDONLY);

		ioctl(fd, IOCTL_STREAMON); // #2
		ioctl(fd, IOCTL_GRAB); // #3
		mySize = read(fd, &inBuffer, USBCAM_BUF_SIZE); // #4
		ioctl(fd, IOCTL_STREAMOFF); // #5

		// #6
		memcpy(finalBuffer, inBuffer, HEADERFRAME1);
		memcpy(finalBuffer + HEADERFRAME1, dht_data, DHT_SIZE);
		memcpy(finalBuffer + HEADERFRAME1 + DHT_SIZE, inBuffer + HEADERFRAME1, (mySize - HEADERFRAME1));
		
		fwrite(finalBuffer, mySize + DHT_SIZE, 1, foutput); // #7
		fclose(foutput); // #8

	}

	free(finalBuffer);
	free(inBuffer);

}

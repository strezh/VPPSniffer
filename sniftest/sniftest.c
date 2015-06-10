#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ppdev.h>

int main(int argc, char* argv[])
{
	unsigned char data;
    int pfd;

	pfd = open("/dev/parportsnif0", O_RDWR);
	if (pfd < 0) {
		perror("Failed to open port");
		exit(0);
	}
	if ((ioctl(pfd, PPEXCL) < 0) || (ioctl(pfd, PPCLAIM) < 0)) {
		perror("Failed to lock port");
		close(pfd);
		exit(0);
	}
    // turn all bits off
	data=0x00;
	ioctl(pfd, PPWDATA, &data);
    usleep(100000);
    // turn some bits on and other off
	data=0x55;
	ioctl(pfd, PPWDATA, &data);
    usleep(100000);
    // turn some bits off and other on
	data=0xAA;
	ioctl(pfd, PPWDATA, &data);
    usleep(100000);
    // turn some bits on and other off
	data=0x55;
	ioctl(pfd, PPWDATA, &data);
    usleep(100000);
    // turn some bits off and other on
	data=0xAA;
	ioctl(pfd, PPWDATA, &data);
    usleep(100000);
    // turn all bits off
	data=0x00;
	ioctl(pfd, PPWDATA, &data);
    usleep(100000);

	close(pfd);
	return 0;
}



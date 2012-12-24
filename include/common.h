#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <pthread.h>
#include <getopt.h>
#include <malloc.h>
#include <assert.h>
#include <semaphore.h>
#include <termios.h>

#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <linux/types.h>
#include <linux/fb.h>

#include <asm/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//#include <libv4l1.h>
#//include <libv4l1-videodev.h>
#include <linux/videodev2.h>
#include "../jpeg-8d/jpeglib.h"


#define PACKET 128
#define pixelWidth  320
#define pixelHeight  240
#define THRESHOLD_D 10
#define BLK_THRESHOLD_D 50
#define LENGTH   (pixelHeight*pixelWidth*3*sizeof(byte))
extern int THRESHOLD;
extern int BLK_THRESHOLD;
extern int cFlag;
extern int flag_alarm;

extern int beep_on_flag;
extern int led_on_flag;
//int THRESHOLD = 10;


typedef unsigned char byte;

typedef struct jpeg_file{
	void *start;
	size_t length;
}JPEG;

typedef struct send_buffer{
	void *start;
	size_t length;	
}SEND_BUFFER;

extern int init_bmp_head(void);
extern int ready_for_capture(void);
extern SEND_BUFFER *get_send_buffer(void);
extern int close_video(void);
extern int init_video_device(void);
extern void socket_send(void);
extern void socket_recv(void);

extern void func_lm75a(void);
extern int gprs(void);


#endif


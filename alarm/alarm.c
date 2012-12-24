#include "beep_led.h"
#include "common.h"
#define BUFF_SIZE 5			//发送温度值位数
#define TEMP_PORT 8887		//温度值发送端口号
#define TEMP_VAL 25.00		//报警温度值
static double temp;
int flag_alarm = 0;
static int led_flag = LED_OFF;
static int beep_flag = BEEP_OFF;

void func_lm75a(void) 
{
	int dev_lm75a;
	int data;	
	int temp_int,temp_point1,temp_point2;
	
	/********开启设备*********/
	dev_lm75a= open ("/dev/s5pc100_lm75a",O_RDWR);
	if (dev_lm75a< 0) {
		perror("open");
		exit(0);
	}
	
	int dev_led;
	if((dev_led = open("/dev/s5pc100_led",O_RDWR | O_NONBLOCK))<0){
		perror("open");
		exit(0);
	}
	
	int dev_beep;
	if((dev_beep = open("/dev/s5pc100_beep",O_RDWR | O_NONBLOCK))<0){
		perror("open");
		exit(0);
	}
	/********获取double型温度值********/
	while(1){	
		read (dev_lm75a, (char *)&data, sizeof(data));
		temp_int = data/0x100;
		temp_point1 = (data%0x100-data%0x10)/0x10;
		temp_point2 = data%0x10;
		temp = temp_int + temp_point1*(1.00/16) + temp_point2*(1.00/(16*16));	
		usleep(100000);
		//printf("TEMP:%.2f\n",temp);
		if (led_flag == LED_ON){		
			if (temp >= TEMP_VAL){		//超过温度报警值LED灯亮		
				ioctl(dev_led,LED_ON,0);
				printf("Led on\n");
				usleep(50000);
				ioctl(dev_led,LED_OFF,0);
				printf("Led off\n");
				usleep(50000);		
	
			}else{
				if(led_on_flag == 1)	//LED灯亮
				{
					ioctl(dev_led,LED_ON,0);
				}else{
					ioctl(dev_led,LED_OFF,0);
				}			
			}
									
		}else{
			ioctl(dev_led,LED_OFF,0);
			usleep(100000);
			if(temp < TEMP_VAL){
				led_flag = LED_ON;				
			}
		}	
		if (beep_flag == BEEP_ON){		
			if (temp >= TEMP_VAL){			//超过温度报警值蜂鸣器开启		
				ioctl(dev_beep,BEEP_ON,0);
				printf("Beep on\n");
				usleep(50000);
				ioctl(dev_beep,BEEP_OFF,0);
				printf("Beep off\n");
				usleep(50000);		
			}else{
				if(beep_on_flag == 1)		//蜂鸣器开启	
				{
					ioctl(dev_beep,BEEP_ON,0);
				}else{
					ioctl(dev_beep,BEEP_OFF,0);
				}
			}
		}else{
			ioctl(dev_beep,BEEP_OFF,0);	
			usleep(100000);
			if (temp < TEMP_VAL){
				beep_flag = BEEP_ON;
			}
		}	
		if(flag_alarm == 1)
		{
			break;
		}
	}
	close(dev_lm75a);
	close(dev_led);
	close(dev_beep);
//	printf ("/dev/temp closed :\n");
	pthread_exit(NULL);
}

/*********************发送温度值导致视频不流畅，决定不用***********************/
/*
void func_temp(void)
{
	int newfd,sockfd,portnumber,nZero = 1;
	socklen_t len;
	struct sockaddr_in server_addr,client_addr;
	char buffer[BUFF_SIZE];
	
	if((portnumber = TEMP_PORT)<0)
	{
		perror("Error portnumbser");
		exit(1);
	}

	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket");
		exit(1);
	}

	bzero(&server_addr,sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portnumber);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&nZero,sizeof(nZero)); 
	
	if(bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr_in))==-1)
	{
		perror("bind");
		exit(1);
	}	
	
	if(listen(sockfd,5)==-1)
	{
		perror("Listen");
		exit(1);
	}	
	printf("sockfd_temp Listening ...\n");
	printf("Fun : %s sockfd_temp = %d\n",__FUNCTION__,sockfd);
		
	
	while(1){
		len = sizeof(struct sockaddr_in);
		if((newfd = accept(sockfd,(struct sockaddr *)&client_addr,&len))==-1)
		{
			perror("Accept");
			exit(1);
		}

		printf("server: got connection from %s, port %d, socket %d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port), newfd);
		printf("Fun : %s newfd_temp = %d\n",__FUNCTION__,newfd);
			
		while(1){
//			printf("%f\n",temp);
			if(temp != 0.00)
			{
				usleep(500000);	
				gcvt(temp,BUFF_SIZE,buffer);
				if(strlen(buffer) < 5)
				{
					temp += 0.001;
					gcvt(temp,BUFF_SIZE,buffer);
				}
				printf("Temp:%s Len of buffer:%d\n",buffer,strlen(buffer));
				if(send(newfd,buffer,BUFF_SIZE,0)==-1)
				{
					perror("Send");
					break;
				}
			}else{
				continue;
			}
		}
		close(newfd);
	}

	close(sockfd);
}
*/

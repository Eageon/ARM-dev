#include "common.h"

struct message_info{
	char cnnu[16];
	char phnu[16];
	char message[128];
};
#define serial_name "/dev/s3c2410_serial2"
/************************************************************************************************************************************************
函数名称 : serial_init
实现功能 : 串口初始化 
************************************************************************************************************************************************/
int serial_init(int fd, int baud, int databits, int stopbits, int parity, int vtime, int vmin)
{
	int i,j;

	struct termios new_cfg,old_cfg;

	memset(&new_cfg,0,sizeof(struct termios));
	memset(&old_cfg,0,sizeof(struct termios));
	
	if(tcgetattr(fd,&old_cfg))
	{
		perror("tcgetattr"); return -1;
	}
	new_cfg = old_cfg;
	new_cfg.c_cflag |= (CLOCAL | CREAD);	
	new_cfg.c_oflag |= OPOST;
	new_cfg.c_cflag &= ~(IXON | IXOFF | IXANY);
	new_cfg.c_cflag &= ~CRTSCTS;
	new_cfg.c_cflag |= IGNPAR;	
	new_cfg.c_oflag = 0;
	new_cfg.c_lflag = 0;

	switch(baud)
	{
		default:
		case 9600:
			i = cfsetispeed(&new_cfg,B9600);
			j = cfsetospeed(&new_cfg,B9600);
			break;
		case 115200:
			i = cfsetispeed(&new_cfg,B115200);
			j = cfsetospeed(&new_cfg,B115200);
			break;
	}
	switch(databits)
	{
		case 7:
			new_cfg.c_cflag &= ~CSIZE;
			new_cfg.c_cflag |= CS7;
			break;
		default:
		case 8:
			new_cfg.c_cflag &= ~CSIZE;
			new_cfg.c_cflag |= CS8;
			break;
	}
	switch(parity)
	{
		case 'e':
		case 'E':
			new_cfg.c_iflag |= (INPCK | ISTRIP);
			new_cfg.c_cflag |= PARENB;
			new_cfg.c_cflag &= ~PARODD;
			break;
		case 'o':
		case 'O':
			new_cfg.c_cflag |= PARENB;
			new_cfg.c_cflag |= PARODD;
			new_cfg.c_iflag |= (INPCK | ISTRIP);
			break;
		default:
		case 'n':
		case 'N':
			new_cfg.c_cflag &= ~PARENB;
			new_cfg.c_iflag &= ~INPCK;
			break;
	}	
	switch(stopbits)
	{
		default:
		case 1:
			new_cfg.c_cflag &= ~PARENB;break;
		case 2:
			new_cfg.c_cflag &= PARENB;break;
	}

	new_cfg.c_cc[VTIME] = vtime;
	new_cfg.c_cc[VMIN] = vmin;

	tcflush(fd,TCIFLUSH);

	//activate
	if(tcsetattr(fd,TCSANOW,&new_cfg) != 0)
	{
		perror("tcsetattr");
		return -1;
	}
	
	return 0;
}
/************************************************************************************************************************************************
函数名称 : send
实现功能 : 将指令发送到终端 
************************************************************************************************************************************************/
static void send_gprs(int fd,char *cmgf,char *cmgs,char *message)
{
	int nwrite;
	char buff[128];

	/*AT*/
	memset(buff,0,sizeof(buff));
	strcpy(buff,"AT\r\n");
	nwrite = write(fd,buff,strlen(buff));

	/*AT+CMGF*/
	memset(buff,0,sizeof(buff));
	strcpy(buff,"AT+CMGF=");
	strcat(buff,cmgf);
	strcat(buff,"\r\n");
	nwrite = write(fd,buff,strlen(buff));


	/*AT+CMGS*/
	memset(buff,0,sizeof(buff));
	strcpy(buff,"AT+CMGS=");
	strcat(buff,cmgs);
	strcat(buff,"\r");
	nwrite = write(fd,buff,strlen(buff));

	/*内容*/
	memset(buff,0,sizeof(buff));
	strcpy(buff,message);
	nwrite = write(fd,buff,strlen(buff));
	
}
/************************************************************************************************************************************************
函数名称 : send_en_message
实现功能 : 文本方式发送信息 
************************************************************************************************************************************************/
static void send_en_message(int fd,struct message_info info)
{
	char cmgf[] = "1";
	char cmgs[16] = {'\0'};

	/*对方手机号码*/
	strcpy(info.phnu,"18980065267");

	/*短信内容*/
	strcpy(info.message,"We are farsight CD1110 NO.3 is students .");
	strcat(info.message,"\x1a");
	strcat(cmgs,info.phnu);

	/*开始发送*/
	send_gprs(fd,cmgf,cmgs,info.message);
	printf("message send success !\n\n");
}
/************************************************************************************************************************************************
函数名称 : gprs模块接口函数，用于外部程序调用
实现功能 : 串口初始化 
************************************************************************************************************************************************/
int gprs(void)
{
	int serial_fd;
	struct message_info info;
	
	if((serial_fd = open(serial_name,O_RDWR|O_NOCTTY|O_NDELAY)) == -1)
	{
		perror("Open");
		exit(0);
	}	

	if(serial_init(serial_fd,115200,8,1,'N',0,0)) 
		printf("serial_init failure !!!\n\n");
	
	send_en_message(serial_fd,info);

	close(serial_fd);	
	return 0;
}
/************************************************************************************************************************************************
函数名称 : 主函数，用于测试本程序
实现功能 : 
************************************************************************************************************************************************/
/*
int main()
{
	gprs();
	return 0;
}
*/

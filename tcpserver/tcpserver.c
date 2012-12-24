#include "common.h"

int beep_on_flag,led_on_flag;		//Android端主动控制LED，蜂鸣器
#define BACKLOG 5
#define SERVER_SEND_PORT 8888		//发送图片Socket端口
#define SERVER_RECV_PORT 8889		//接收命令Socket端口
#define BUFFLEN 1024

void socket_recv(void)
{
	char buff[BUFFLEN];
	int n = 0;
	int sockfd_recv,portnumber,nZero = 1;
	socklen_t len_recv;
	ssize_t newfd_recv;
	struct sockaddr_in server_recv_addr,client_addr;
	
	pthread_mutex_t mutex;
	pthread_mutex_init (&mutex, NULL);
	
	memset(buff,0,BUFFLEN);
	
	if((portnumber = SERVER_RECV_PORT)<0)
	{
		perror("Error portnumbser");
		exit(1);
	}

	if((sockfd_recv=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket");
		exit(1);
	}

	bzero(&server_recv_addr,sizeof(struct sockaddr_in));
	server_recv_addr.sin_family = AF_INET;
	server_recv_addr.sin_port = htons(portnumber);
	server_recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	setsockopt(sockfd_recv,SOL_SOCKET,SO_REUSEADDR,(char *)&nZero,sizeof(nZero)); 
	
	if(bind(sockfd_recv,(struct sockaddr *)(&server_recv_addr),sizeof(struct sockaddr_in))==-1)
	{
		perror("bind");
		exit(1);
	}	
	
	if(listen(sockfd_recv,BACKLOG)==-1)
	{
		perror("Listen");
		exit(1);
	}	
	printf("socket_recv Listening...\n");
	printf("Fun : %s sockfd_recv = %d\n",__FUNCTION__,sockfd_recv);
	
	int maxfd = -1;
	fd_set scanfd;
	struct timeval timeout;
	int err;
	
	while(1){
		len_recv = sizeof(struct sockaddr_in);
		if((newfd_recv = accept(sockfd_recv,(struct sockaddr *)&client_addr,&len_recv))==-1)
		{
			perror("Accept");
			exit(1);
		}			

		printf("server: got connection from %s, port %d, socket %d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port), newfd_recv);
		printf("Fun : %s Newfd_recv = %d\n",__FUNCTION__,newfd_recv);
	
		while(1)
		{
			FD_ZERO(&scanfd);
			//FD_SET(0, &scanfd);
			maxfd = 0;
			
			FD_SET(newfd_recv,&scanfd);
			if(maxfd < newfd_recv)
			{
				maxfd = newfd_recv;							
			}
		
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
		
			err = select(maxfd + 1,&scanfd,NULL,NULL,&timeout);
			
			//memset(buff,0,BUFFLEN);
			n = recv(newfd_recv,buff,BUFFLEN,0);
			
			if (0 == n){						//客户端重连设置
				printf("peer shutdown!\n");		
				break;
			}
			
			switch(err)
			{
				//case 0  : break;
				case -1 : break;
				default : 
					if(FD_ISSET(newfd_recv,&scanfd))
					{													
						if(n > 0)
							printf("[%s]\n",buff);
						if(n > 0 && !strncmp(buff,"S",1))
						{
							memset(buff,0,BUFFLEN);
							pthread_mutex_lock(&mutex);
							led_on_flag = 1;		//LED灯亮
							pthread_mutex_unlock (&mutex);
						}else if(n > 0 && !strncmp(buff,"s",1)){
							memset(buff,0,BUFFLEN);
							pthread_mutex_lock(&mutex);
							led_on_flag = 0;		//LED灯灭
							pthread_mutex_unlock (&mutex);
						}else if(n > 0 && !strncmp(buff,"B",1)){
							memset(buff,0,BUFFLEN);
							pthread_mutex_lock(&mutex);
							beep_on_flag = 1;		//蜂鸣器开启
							pthread_mutex_unlock (&mutex);
							gprs();					//发信息到手机
						}else if(n > 0 && !strncmp(buff,"b",1)){
							memset(buff,0,BUFFLEN);
							pthread_mutex_lock(&mutex);
							beep_on_flag = 0;		//蜂鸣器关闭
							pthread_mutex_unlock (&mutex);
						}else{
							break;
						}
						
					}
					break;
			}
		}
		close(newfd_recv);
	}
	
	close(sockfd_recv);
}

void socket_send(void)
{
	int sockfd_send,portnumber,nZero = 1;
	int ret,sended,wait;
	int flag_peer_shutdown = 1;
	socklen_t len_send;
	ssize_t newfd_send;
	struct sockaddr_in server_addr,client_addr;
	init_video_device();		//初始化video
	ready_for_capture();		
	SEND_BUFFER *p_jpeg;
	
	if((portnumber = SERVER_SEND_PORT)<0)
	{
		perror("Error portnumbser");
		exit(1);
	}

	if((sockfd_send = socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket");
		exit(1);
	}

	bzero(&server_addr,sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portnumber);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	setsockopt(sockfd_send,SOL_SOCKET,SO_REUSEADDR,(char *)&nZero,sizeof(nZero)); 
	
	if(bind(sockfd_send,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr_in))==-1)
	{
		perror("bind");
		exit(1);
	}	
	
	if(listen(sockfd_send,5) == -1)
	{
		perror("Listen");
		exit(1);
	}	
	
	
	while(1){
		printf("Listening...\n");
		printf("Fun : %s sockfd_send = %d\n",__FUNCTION__,sockfd_send);	
		len_send = sizeof(struct sockaddr_in);
		
		if((newfd_send = accept(sockfd_send,(struct sockaddr *)&client_addr,&len_send)) == -1)
		{
			perror("Accept");
			exit(1);
		}
		
		flag_peer_shutdown = 0;
		printf("server: got connection from %s, port %d, socket %d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port), newfd_send);
		printf("Fun %s : accept fd = %d\n",__FUNCTION__,newfd_send);
		
		while(1)
		{		
			//if (1 == flag_peer_shutdown);
			//	break;
			//printf("send pic....\n");	
			if(cFlag == 0)			//cFlag置0允许发送数据
			{
				p_jpeg = get_send_buffer();		//获取JPEG格式图片数据
				//printf("after get_send_buffer..\n");			
				if(NULL != p_jpeg)
				{	
					wait = p_jpeg->length;
					sended = 0;				
					while(wait > PACKET) {				//分包发送
						ret = send(newfd_send, p_jpeg->start + sended, PACKET, MSG_NOSIGNAL);
						if(ret == -1) {
							perror("Send");
							flag_peer_shutdown = 1;		//退出发送循环继续允许接受连接
							break;
						}
						sended += ret;
						wait -= ret;					
						//printf("sended %d\n",sended);
						//usleep(2000);
				    }
					if (1 == flag_peer_shutdown)
						break;
						
					if((ret = send(newfd_send, p_jpeg->start + sended, wait, MSG_NOSIGNAL)) == -1){
						perror("Send");
						flag_peer_shutdown = 1;
						break;
					};
					//printf("send complete\n");
					usleep(50000);
				}	
			}
		}
		printf("close socket....\n");		
		close(newfd_send);
	}

	close(sockfd_send);
	close_video();
}

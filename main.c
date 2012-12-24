#include "common.h"

int cFlag = 0;		//发送图片控制位
//pthread_cond_t cont = PTHREAD_COND_INITIALIZER; 
//pthread_mutex_t mutex;

/*********温度发送线程***********/
/*
void *temp_send(void *arg)
{
	func_temp();
	(void *)0;
}
*/

/**********报警器线程***********/
void *lm75a(void *arg)
{
	func_lm75a();
	return (void *)0;
}

/**********发送线程***********/
void *picture_send(void *arg)
{
	socket_send();
	return (void *)0;
}

/*****接收Android端命令线程*****/
void *server_recv(void *arg)
{
	socket_recv();
	return (void *)0;
}

/******服务器控制命令线程*******/
void *cmd(void *arg)
{	
	pthread_mutex_t mutex;
	pthread_mutex_init (&mutex, NULL);
	while(1)
	{
		char ch = getchar();

		switch(ch)
		{		
			case 's':
			case 'S':
				pthread_mutex_lock(&mutex);
				cFlag = 1;	
				pthread_mutex_unlock (&mutex);
				printf("Camera over\n");
				break;

			case 'g':
			case 'G':
				gprs();
				break;
					
			case 'o':
			case 'O': 
				pthread_mutex_lock(&mutex);
				beep_on_flag = 1;
				pthread_mutex_unlock (&mutex);
				printf("Beep on\n");
				break;
				
			case 'x':
			case 'X':
				pthread_mutex_lock(&mutex);
				beep_on_flag = 0;
				pthread_mutex_unlock (&mutex);
				printf("Beep off\n");
				break;
				
			case 'l':
			case 'L':				
				pthread_mutex_lock(&mutex);
				led_on_flag = 1;
				pthread_mutex_unlock (&mutex);
				printf("Led light\n");
				break;
			
			case 'k':
			case 'K': 
				pthread_mutex_lock(&mutex);
				led_on_flag = 0;	
				pthread_mutex_unlock (&mutex);
				printf("Led off\n");
				break;
			
			case 'r':
			case 'R': 
				pthread_mutex_lock(&mutex);
				cFlag = 0;
				pthread_mutex_unlock (&mutex);
				printf("Camera start\n");
				break;

			case 'q':
			case 'Q':
				printf("Program exit......\n");
				pthread_mutex_lock(&mutex);
				flag_alarm = 1;
				pthread_mutex_unlock(&mutex);
				pthread_exit(NULL);				
				exit(1);

			default:
				printf("Catch a command ...\r\n");
		};
	}
	return (void *)0;
} 


/************主函数************/
int main(int argv, char **argc)
{  
	printf("this is a test for v4l2!\n");
		 
	pthread_attr_t threadAttr; 
	pthread_attr_init(&threadAttr); 
	pthread_t thread_recv,thread_cmd,thread_socked_send,pthread_lm75a;
		
	/*****创建接收Android端命令线程*****/
	if(pthread_create(&thread_cmd,&threadAttr,cmd,(void *)0) == -1)
	{
		perror("thread_cmd create");
		exit(1);
	}
	
	/**********创建图片发送线程**********/
	if(pthread_create(&thread_socked_send,&threadAttr, picture_send,(void *)0) == -1)
	{
		perror("thread_temp_send thread_cmd create");
		exit(1);
	}
	
	/*****创建接收Android端命令线程*****/
	if(pthread_create(&thread_recv,&threadAttr,server_recv,(void *)0) == -1)
	{
		perror("thread_recv create");
		exit(1);
	}
	
	/**********创建报警器线程***********/
	if(pthread_create(&pthread_lm75a, &threadAttr, lm75a, (void *)0) == -1)
	{
		perror("pthread_lm75a create");
		exit(1);
	}
	
	/**************线程等待**************/
	pthread_join(pthread_lm75a,NULL);
	pthread_join(thread_cmd,NULL);
	pthread_join(thread_socked_send,NULL);
	pthread_join(thread_recv,NULL);
	

	return 0; 
}

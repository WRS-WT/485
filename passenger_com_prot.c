/* ************************************************************************
 *       Filename:  485_comm.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2016å¹´07æœˆ01æ—¥ 09æ—¶37åˆ†14ç§’
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h> 
#include <time.h>
#include <arpa/inet.h>
#include "passenger_com_prot.h"
#include "tmUart.h"


#define PASS_RECVHEAD  0xf2
#define PASS_RECVTAIL  0xf3

#define PASS_OK 	0x06
#define PASS_FAIL 	0x15

#define PASS_CLEAR_ACK	0x8001
#define PASS_CHECK_ACK	0x8002
#define PASS_DOOR_ACK	0x8003
#define PASS_TIME_ACK	0x8004
#define PASSDOOR_OPEN 	0X01
#define PASSDOOR_CLOSE 	0X00

#ifdef PASS_UART_485
extern BOOL GPIO_set ( unsigned int  bitIndex, BOOL bOn );
extern BOOL  GPIO_setDir ( unsigned int  bitIndex, BOOL bOn );
#endif
#if 0


static int      g_fdGpio = -1;

#define GPIO_SET_DIR            0x1
#define GPIO_GET_DIR            0x2
#define GPIO_READ_BIT   	0x3
#define GPIO_WRITE_BIT  	0x4

typedef struct
{
	unsigned int  grpNum;
	unsigned int  bitNum;
	unsigned int  value;
} gpio_groupbit_info;

static BOOL  GPIO_setDir ( unsigned int  bitIndex, BOOL bOn )
{
	gpio_groupbit_info      tGpioInfo;
	
	if ( 0 >  g_fdGpio )
	{
		g_fdGpio = open ( GPIO_DEVICE , O_RDWR );
		
		if ( 0 >  g_fdGpio )
		{
			printf ( "open gpio fail in %d!\n", __LINE__ );
			return FALSE;
		}
	}
	
	tGpioInfo.grpNum = bitIndex / 8;
	tGpioInfo.bitNum = bitIndex % 8;
	
	if ( bOn )
	{ tGpioInfo.value = 1; }
	
	else
	{ tGpioInfo.value = 0; }
	
	if ( ioctl ( g_fdGpio, GPIO_SET_DIR, &tGpioInfo ) < 0 )
	{
		printf ( "GPIO_SET_DIR fail in %d!\n", __LINE__ );
		close ( g_fdGpio );
		g_fdGpio = -1;
		return FALSE;
	}
	
	else
	{ return TRUE; }
}

static BOOL    GPIO_set ( unsigned int  bitIndex, BOOL bOn )
{
	gpio_groupbit_info      tGpioInfo;
	
	if ( 0 >  g_fdGpio )
	{
		g_fdGpio = open ( GPIO_DEVICE , O_RDWR );
		
		if ( 0 >  g_fdGpio )
		{
			printf ( "open gpio fail in %d!\n", __LINE__ );
			return FALSE;
		}
	}
	
	tGpioInfo.grpNum = bitIndex / 8;
	tGpioInfo.bitNum = bitIndex % 8;
	
	if ( bOn )
	{ tGpioInfo.value = 1; }
	
	else
	{ tGpioInfo.value = 0; }
	
	if ( ioctl ( g_fdGpio, GPIO_WRITE_BIT, &tGpioInfo ) < 0 )
	{
		printf ( "GPIO_WRITE_BIT fail in %d!\n", __LINE__ );
		close ( g_fdGpio );
		g_fdGpio = -1;
		return FALSE;
	}
	
	else
	{ return TRUE; }
}

#endif

static void get_date_time(struct tm *get)
{
	char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	
	time_t timep;
	struct tm *time_date;
	
	time(&timep);
	time_date = gmtime(&timep);

	time_date->tm_year = 1900 + time_date->tm_year;
	time_date->tm_mon = time_date->tm_mon + 1;
	
	if(get != NULL)
		memcpy(get,time_date,sizeof(struct tm));
	
	printf("%d/%d/%d",time_date->tm_year,time_date->tm_mon,time_date->tm_mday);
	printf(" %s %d:%d:%d\n",wday[time_date->tm_wday],time_date->tm_hour,time_date->tm_min,time_date->tm_sec);
}


/************************************************************************
get_data:get the count and time function
	data: recv buf
	get_buf :get msg buf 
************************************************************************/ 

static int get_data(const unsigned char *data,DAT_ANE *get_buf)
{
	if(get_buf == NULL)
		return 0;
	
	memset(get_buf,0,sizeof(DAT_ANE));
	
	get_buf->up_count = ((data[0] & 0xffffffff)<< 4) + ((data[1] & 0xffffffff)<< 2) + (data[2] & 0xffffffff); 
	get_buf->down_count = ((data[3] & 0xffffffff)<< 4) + ((data[4] & 0xffffffff)<< 2) + (data[5] & 0xffffffff); 
	
	get_buf->year = data[6];
	get_buf->mon = data[7];	
	get_buf->day = data[8];
	get_buf->hour = data[9];
	get_buf->min = data[10];
	get_buf->second = data[11];

	return 1;
}

 /************************************************************************
 chk_cal:crc function
	 recv_buf: recv buf
	 len :recv buf len
	 
 return : crc 
 ************************************************************************/ 

static unsigned char chk_cal(const unsigned char *recv_buf,int len)
{
	unsigned char chk = 0;
	int i = 0;
	for(i=0;i<len-3;i++)
	{
		chk += (*(recv_buf+1+i));
	}
	return chk;
}

/************************************************************************
msg_send:send deal function
	info: want to send msg
	len :send buf len
	
return :send buf 
************************************************************************/ 
static int msg_send(MSG_MOBI info,int buf_len,unsigned char *sent_buf)
{
	 //unsigned char *sent_buf;
	 unsigned char chk = 0;
	 int len = 0;
	 struct tm get_time;
	 switch (htons(info.CM))
	 {
		 case CLEAR://clear
		 	 if(buf_len < 9)
			 	return 0;
			 memset(sent_buf,0,9);
			 sent_buf[0] = PASS_RECVHEAD;
			 info.len = htons(0);
			 memcpy(sent_buf+1,(unsigned char *)&info,7);
			 chk = chk_cal(sent_buf,9);
			 sent_buf[7] = chk;
			 sent_buf[8] = PASS_RECVTAIL;
			 len = 9; 
			 
			 break;
		 case CHECK: //cheak
		 	 if(buf_len < 9)
			 	return 0;
			 memset(sent_buf,0,9);
			 sent_buf[0] = PASS_RECVHEAD;
			 info.len = htons(0);
			 memcpy(sent_buf+1,(unsigned char *)&info,7);
			 chk = chk_cal(sent_buf,9);
			 sent_buf[7] = chk;
			 sent_buf[8] = PASS_RECVTAIL;
			 len = 9;
			 break;
		 case DOOR_STATE: //door how  
			 if(buf_len < 9)
			 	return 0;		
			 memset(sent_buf,0,9);
			 sent_buf[0] = PASS_RECVHEAD;
			 info.len = htons(0);
			 memcpy(sent_buf+1,(unsigned char *)&info,7);
			 chk = chk_cal(sent_buf,9);
			 sent_buf[7] = chk;
			 sent_buf[8] = PASS_RECVTAIL;
			 len = 9;
			 break;
		 case SET_TIME://set time
		 	if(buf_len < 15)
			 	return 0;
			 memset(sent_buf,0,15);
			 sent_buf[0] = PASS_RECVHEAD;
			 info.DN = 0xff;
			 info.len = htons(6);
			 memcpy(sent_buf+1,(unsigned char *)&info,7);
			 get_date_time(&get_time);
			 sent_buf[7] = get_time.tm_year - 2000;
			 sent_buf[8] = get_time.tm_mon;
			 sent_buf[9] = get_time.tm_mday;
			 sent_buf[10] = get_time.tm_hour;
			 sent_buf[11] = get_time.tm_min;
			 sent_buf[12] = get_time.tm_sec;
			 chk = chk_cal(sent_buf,15);
			 sent_buf[13] = chk;
			 sent_buf[14] = PASS_RECVTAIL;
			 len = 15;
			 break;
		 default :
		 	return 0;
			 break;
	 }
	 //printf("len = %d\n",len);
	 return len;
}

 
/************************************************************************
msg_deal:recv deal function
	recv_buf:recv buf
	len :recv buf len
	send_msg: want to send msg

return :if nothing want to send is 0,else is 1 
************************************************************************/

 static int msg_deal(const unsigned char *recv_buf,int len,MSG_MOBI *send_msg)
{
	static char sent_time = 0;
	
	printf("%#x||%#x len = %d\n",*recv_buf,*(recv_buf+len-1),len);
	if(*recv_buf != PASS_RECVHEAD || *(recv_buf+len-1) != PASS_RECVTAIL)
		return -1;
	unsigned char chk = chk_cal(recv_buf,len);
	unsigned char real_chk = *(recv_buf+len-2);
	printf("chk=%#x  real_chk=%#x\n",chk,real_chk);

	if(chk != real_chk)
			return -1;
	
	DAT_ANE get_buf;
	MSG_MOBI *msg = (MSG_MOBI *)(recv_buf+1);
	printf("msg->CM == %#x\n",htons(msg->CM));
	int send_flag = 0;	
	
	switch(htons(msg->CM))
	{
		
		case PASS_CLEAR_ACK: //clear
			if(recv_buf[7] == PASS_OK)
			{
				printf("clear OK!\n");
				sent_time = 0;
			}
			else if(recv_buf[7] == PASS_FAIL)
			{
				printf("clear fail!\n");
				sent_time++;
				if(sent_time <= 3)
				{
					msg->CM = CLEAR_SEND;
					memcpy(send_msg,msg,sizeof(MSG_MOBI));
					send_flag = 1;
				}
			}
			break;
		case PASS_CHECK_ACK://check 
			get_data(recv_buf+7,&get_buf);
			printf("up %d down %d time:%d %d %d %d %d %d\n",get_buf.up_count,
				get_buf.down_count,get_buf.year,get_buf.mon,get_buf.day,get_buf.hour,
				get_buf.min,get_buf.second);
			sent_time = 0;
			break;
		case PASS_DOOR_ACK: //door how
			if(recv_buf[7] == PASSDOOR_OPEN)
			{
				printf("door is open!\n");
			}
			else if(recv_buf[7] == PASSDOOR_CLOSE)
			{
				printf("door is close!\n");
			}
			sent_time = 0;
			break;
		case PASS_TIME_ACK: //set time
			if(recv_buf[7] == PASS_OK)
			{
				printf("time set OK!\n");
				sent_time = 0;
			}
			else if(recv_buf[7] == PASS_FAIL)
			{
				printf("time set fail!\n");
				sent_time++;
				if(sent_time <= 3)
				{
					msg->CM = SET_TIME_SEND;
					memcpy(send_msg,msg,sizeof(MSG_MOBI));
					send_flag = 1;
				}
			}
			break;
		default : //??
			break;
	}
	return send_flag;
}



int pass_universal_callBack( const unsigned char *recvBuf, int msgLen, int *uart_Num)
{
	int i = 0;
	static int head_flag = 0;
	static int head_index = 0;
	static int tail_flag = 0;
	static int tail_index = 0;
	static int buf_len = 0;
	static unsigned char pMsgBuf[1024] = {};
	static int  pMsgBuf_len = 0;
	int uart_num = *(int *)uart_Num;


	printf("recv:");
	for(i=0;i<msgLen;i++)
	{
		printf("%x",recvBuf[i]);
	}
	printf("\n");
	if(buf_len+msgLen > sizeof(pMsgBuf))
	{
		bzero(pMsgBuf, sizeof(pMsgBuf));
		buf_len = 0;
		head_index = 0;
		head_flag = 0;
		tail_index = 0;
		tail_flag = 0;
	}
	memcpy(pMsgBuf+buf_len,recvBuf,msgLen);
	buf_len += msgLen;
	i = 0;
	while(*(pMsgBuf+buf_len-1-i) != PASS_RECVHEAD )
	{
		i++;
		if(i > buf_len-1)
			break;
	}
	
	if(*(pMsgBuf+buf_len-1-i) != PASS_RECVHEAD)
	{
		bzero(pMsgBuf, sizeof(pMsgBuf));
		buf_len = 0;
	}
	else
	{
		head_index = buf_len-1-i;
		head_flag = 1;
		printf("head_index == %d\n",head_index);
	}

	if(head_flag == 1)
	{
		i = 0;
		while(*(pMsgBuf+buf_len-1-i) != PASS_RECVTAIL)
		{
			i++;
			if(i > buf_len-1)
				break;
		}
		
		if(*(pMsgBuf+buf_len-1-i) != PASS_RECVTAIL)
		{
			return -1;
		}
		else
		{	
			tail_index = buf_len-1-i;
			tail_flag = 1;
			printf("tail_index == %d\n",tail_index);
		}
	}
	if(tail_index - head_index < 0)
	{
	
		tail_index = 0;
		tail_flag = 0;
		return -1;
	}
	
	if(*(pMsgBuf+head_index) == PASS_RECVHEAD && *(pMsgBuf+tail_index) == PASS_RECVTAIL)//íê??µ?êy?Y
	{	
			MSG_MOBI msg;
			pMsgBuf_len = tail_index - head_index+1;
			int send_flag = msg_deal(pMsgBuf+head_index,pMsgBuf_len,&msg);
			if(send_flag == 1)
			{
				int send_len;
				unsigned char send_buf[PASS_SEND_SIZE];
				send_len = msg_send(msg,sizeof(send_buf),send_buf);
				
				if(send_len != 0)//ÊÕµ½´íÎóÓ¦´ðÊ±ÔÙ´Î·¢ËÍ
				{
					#ifdef PASS_UART_485
						GPIO_setDir(GPIO_INDEX_PSGR, TRUE);
						GPIO_set(GPIO_INDEX_PSGR, TRUE);
					#endif
					
					send_flag = NUart_writeBytes ( uart_num, send_buf, send_len);
					usleep(1000);
					
					#ifdef PASS_UART_485
						GPIO_setDir(GPIO_INDEX_PSGR, TRUE);
						GPIO_set(GPIO_INDEX_PSGR, FALSE);
					#endif
					//printf("flag = %d,send_len = %d\n",send_flag,send_len);
			 		//if(send_flag > 0)
			 		//	flag_send = 1;
					 printf("write_buf:");
					 int wt;
					for(wt = 0; wt < send_len;wt++)
						printf("%02x",*(send_buf+wt));
					printf("\n");
				}
			}
			bzero(pMsgBuf, 1024);
			buf_len = 0;
			head_index = 0;
			head_flag = 0;
			tail_index = 0;
			tail_flag = 0;
	}
	return 0;
}


int pass_msg_send(MSG_MOBI info,int buf_len,unsigned char *sent_buf,unsigned int uart_num)
{
	int flag = 0;
	int len = msg_send(info,buf_len,sent_buf);
	if(len != 0)
	{
		#ifdef PASS_UART_485
			GPIO_setDir(GPIO_INDEX_PSGR, FALSE);
			GPIO_set(GPIO_INDEX_PSGR, FALSE);
		#endif
		flag = NUart_writeBytes ( uart_num, sent_buf, len);
		usleep(1000);
		
		#ifdef PASS_UART_485
			GPIO_setDir(GPIO_INDEX_PSGR, TRUE);
			GPIO_set(GPIO_INDEX_PSGR, FALSE);
		#endif
		
		 
		 printf("write_buf:");
		 int wt;
		for(wt = 0; wt < len;wt++)
			printf("%02x",*(sent_buf+wt));
		printf("\n");
	}
	return flag;
}

int pass_com_send(unsigned char ID,unsigned char DN,unsigned short CM,char *data,int data_len)
{
	key_t IPC_ID =  ftok(PASS_IPCS_PATH, PASS_IPCS_NUM);
	int fd_ipcs = msgget(IPC_ID, IPC_CREAT|0777);
	PASS_MSG pass_msg;
	memset(&pass_msg,0,sizeof(PASS_MSG));
	pass_msg.msg_type = ID;
	pass_msg.msg_type = (pass_msg.msg_type << 8)+DN;
	pass_msg.msg_type = (pass_msg.msg_type << 16)+CM;
	pass_msg.len = data_len;

	memcpy(pass_msg.msg_data,data,data_len);
	int ret_send = msgsnd(fd_ipcs,(void *)&pass_msg, sizeof(PASS_MSG)-4, 0);
	
	return ret_send;
	
}

void pass_com_clear(unsigned char ID,unsigned char DN,unsigned short CM)
{
	int ret = 0;
	key_t IPC_ID =	ftok(PASS_IPCS_PATH, PASS_IPCS_NUM);
	int fd_ipcs = msgget(IPC_ID, IPC_CREAT|0777);
	PASS_MSG pass_msg;
	memset(&pass_msg,0,sizeof(PASS_MSG));
	pass_msg.msg_type = ID;
	pass_msg.msg_type = (pass_msg.msg_type << 8)+DN;
	pass_msg.msg_type = (pass_msg.msg_type << 16)+CM;
	while(ret>= 0)
	{
		ret = msgrcv(fd_ipcs, (void *)&pass_msg,  sizeof(PASS_MSG)-4, pass_msg.msg_type, IPC_NOWAIT);
	}
}
int pass_com_recv(unsigned char ID,unsigned char DN,unsigned short CM,char *data,int time_out)
{
	int ret = 0;
	key_t IPC_ID =  ftok(PASS_IPCS_PATH, PASS_IPCS_NUM);
	int fd_ipcs = msgget(IPC_ID, IPC_CREAT|0777);
	PASS_MSG pass_msg;
	memset(&pass_msg,0,sizeof(PASS_MSG));
	pass_msg.msg_type = ID;
	pass_msg.msg_type = (pass_msg.msg_type << 8)+DN;
	pass_msg.msg_type = (pass_msg.msg_type << 16)+CM;
	while(time_out > 0)
	{
		ret = msgrcv(fd_ipcs, (void *)&pass_msg,  sizeof(PASS_MSG)-4, pass_msg.msg_type, IPC_NOWAIT);
		if(ret > 0)
		{
			memcpy(data,pass_msg.msg_data,pass_msg.len);
			return 1;
		}
		usleep(10*1000);
		time_out --;
	}
	return 0;
}

#ifndef _PASSENGER_COM_PROT_H
#define _PASSENGER_COM_PROT_H
#include <arpa/inet.h>

typedef struct msg_mobi
{
	unsigned char ID;	//�豸ID
	unsigned char DN;	//�ź�
	unsigned short CM;	//�����
	unsigned short len;	//��Ϣ����
}MSG_MOBI;

typedef struct msg_by_pass
{
	long 	msg_type;		//��Ϣ����
	int		len;			//��Ϣ����
	char 	msg_data[64];	//��Ϣ����
}PASS_MSG;

typedef struct data_n_date
{
	int 	up_count;		//�ϳ�����
	int 	down_count;		//�³�����
	char 	year;			//��
	char 	mon;			//��
	char 	day;			//��
	char 	hour;			//ʱ
	char 	min;			//��
	char 	second;			//��
}DAT_ANE;
//#define PASS_UART_485

#ifdef	PASS_UART_485

#define 	GPIO_INDEX_PSGR 28
#define     GPIO_DEVICE     "/dev/hi_gpio"
#endif

#define 	PASS_SEND_SIZE			24
#define 	PASS_UART_NUM 		2
#define		PASS_UART_BAUDRATE	9600

#define 	PASS_IPCS_PATH		"/app/"
#define 	PASS_IPCS_NUM		100

#define CLEAR 		0x1
#define CHECK 		0x2
#define DOOR_STATE 	0x3
#define SET_TIME	0x4
#define CLEAR_SEND 		ntohs(CLEAR)
#define CHECK_SEND 		ntohs(CHECK)
#define DOOR_STATE_SEND ntohs(DOOR_STATE)
#define SET_TIME_SEND	ntohs(SET_TIME)

#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif
/************************************************************************
uart_msg_send:send deal function
	info: want to send msg
	len :send buf len
	
return :send buf 
************************************************************************/ 
int pass_msg_send(MSG_MOBI info,int buf_len,unsigned char *sent_buf,unsigned int uart_num);


/************************************************************************
UART_universal_callBack:recv msg
	recvBuf: recv msg
	msgLen :recv buf len
	pUserParam:avg
return :deal ok is 1,no is else 
************************************************************************/ 
int pass_universal_callBack( const unsigned char *recvBuf, int msgLen, int *uart_Num);
int pass_com_recv(unsigned char ID,unsigned char DN,unsigned short CM,char *data,int time_out);
void pass_com_clear(unsigned char ID,unsigned char DN,unsigned short CM);
int pass_com_send(unsigned char ID,unsigned char DN,unsigned short CM,char *data,int data_len);

#endif


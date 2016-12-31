#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <time.h>
#include "tmUart.h"
#include "passenger_com_prot.h"

#define uartNum     3
#define baudrate 9600
/**/
int main(int argc, char *argv[])
{
	MSG_MOBI info;
	unsigned short c;
	int uart = uartNum;
	NUart_open (uartNum, FALSE, baudrate, UART_TYPE_STD, pass_universal_callBack, &uart );
	c = 1;
	int flag = 0;
	unsigned char sent_buf[PASS_SEND_SIZE];
	/*

	info.CM = SET_TIME_SEND;
	info.ID = 0x01;
	info.DN = 0x01;
	char data[24] = {};
	pass_com_send(info.ID,info.DN,info.CM,data,sizeof(data));
	pass_com_clear(info.ID,info.DN,info.CM);
	printf("out\n");
	int ret = pass_com_recv(info.ID,info.DN,info.CM,data,300);
	printf("ret = %d\n",ret);
	
	flag = pass_msg_send(info,sizeof(sent_buf),sent_buf,uartNum);
	*/
	unsigned char qq1[] = {
		0xf2,0x11,0x80,0x20,0xc0,0x0f,0x60,0x01,0x21,0x0c,0x1e,0xf9,0x35,0x1f,0xf3
	};
	unsigned char qq2[] = {
		0x99,
		0xf3,0xf5,
	};
	
	pass_universal_callBack(qq1,sizeof(qq1),&uart);
	//pass_universal_callBack(qq1,sizeof(qq1),&uart);
	//pass_universal_callBack(qq2,sizeof(qq2),&uart);
	//pass_universal_callBack(qq2,sizeof(qq2),&uart);
	
	return 0;
}


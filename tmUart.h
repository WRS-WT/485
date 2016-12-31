#ifndef         _TM_UART_H
#define _TM_UART_H

//#define       _DEBUG_UART

#define BOOL int
#define TURE 1
#define FALSE 0
#define UART_RECV_BUFF_SIZE             1024
#define MAX_UART_RECV_SIZE              128

//CANBUS MSG INFO
#define CANBUS_MSG_MIN_SIZE             10
#define CANBUS_LENSIGN_POS                      8
#define CANBUS_LENSIGN_SIZE                     2

//HTZT MSG INFO
#define HTZT_UART_MSG_MIN_SIZE                  16
#define HTZT_UART_LENSIGN_POS                   11
#define HTZT_UART_LENSIGN_SIZE                  2

#define MAX_UART_NUM                                    4
#ifndef MAX_UART_NUM
#define MAX_UART_NUM                                    4
#endif

//串口类型应用
typedef enum _uart_type
{
         UART_TYPE_STD = 0, //标准的透传
         UART_TYPE_GPS ,         //GPS数据
         UART_TYPE_CARD, //刷IC或ID卡
         UART_TYPE_STM,          //STM32单片机
         UART_TYPE_KBD,          //串口扩展的键盘
         UART_TYPE_BACANBUS,     //本安串口的canbus数据协议
         UART_TYPE_CANBUS,       // 双can透传
         UART_TYPE_SCREEN,               // 屏幕控制
} UART_TYPE_E;
//===
typedef struct _uart_recv_class
{
	unsigned char *pBuff;
	unsigned int    bufSize;
	
	unsigned int    wtIndex;
	unsigned int    rdIndex;
	int                     dataCount;
} UartRecvInst_t, *UartRecvInst_pt;
//接收串口数据的回调函数类型定义
typedef int ( *UART_GetMsgCallBackFun ) ( const char *pMsgBuf, int msgLen, void *pUserParam );

/*
函数名:NUart_open
入参:
        uartNum:串口编号，从1开始,最大为3
        baudrate:波特率，为4800、9600、19200、38400、57600、115200等等
        fnRecvMsg:调用者提供的串口数据接收回调函数
        maxMsgLen:接收回调函数需要的缓冲最大字节数
返回:
        TRUE为成功，否则失败
作者:徐志强
时间:20120615
*/

BOOL    NUart_open ( unsigned int uartNum, BOOL bNuart, int baudrate, UART_TYPE_E enAppType , UART_GetMsgCallBackFun fnRecvMsg, void *pUserData );
/*
函数名:UART_close
入参:
        uartNum:串口编号，从1开始,最大为3
返回:
        TRUE为成功，否则失败
作者:徐志强
时间:20120615
*/

int     NUart_close ( unsigned int uartNum );

/*
函数名:NUart_writeBytes
入参:
        uartNum:串口编号，从1开始,最大为3
        buff:发送的数据地址
        len:发送的字节数
返回:
        成功返回>0为发送了多少个字节数，否则为失败
作者:徐志强
时间:20120615
*/
int  NUart_writeBytes ( unsigned int uartNum, char *buff, int len );

BOOL    tstUart ( unsigned int uartNum );

#endif

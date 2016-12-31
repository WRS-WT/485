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

//��������Ӧ��
typedef enum _uart_type
{
         UART_TYPE_STD = 0, //��׼��͸��
         UART_TYPE_GPS ,         //GPS����
         UART_TYPE_CARD, //ˢIC��ID��
         UART_TYPE_STM,          //STM32��Ƭ��
         UART_TYPE_KBD,          //������չ�ļ���
         UART_TYPE_BACANBUS,     //�������ڵ�canbus����Э��
         UART_TYPE_CANBUS,       // ˫can͸��
         UART_TYPE_SCREEN,               // ��Ļ����
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
//���մ������ݵĻص��������Ͷ���
typedef int ( *UART_GetMsgCallBackFun ) ( const char *pMsgBuf, int msgLen, void *pUserParam );

/*
������:NUart_open
���:
        uartNum:���ڱ�ţ���1��ʼ,���Ϊ3
        baudrate:�����ʣ�Ϊ4800��9600��19200��38400��57600��115200�ȵ�
        fnRecvMsg:�������ṩ�Ĵ������ݽ��ջص�����
        maxMsgLen:���ջص�������Ҫ�Ļ�������ֽ���
����:
        TRUEΪ�ɹ�������ʧ��
����:��־ǿ
ʱ��:20120615
*/

BOOL    NUart_open ( unsigned int uartNum, BOOL bNuart, int baudrate, UART_TYPE_E enAppType , UART_GetMsgCallBackFun fnRecvMsg, void *pUserData );
/*
������:UART_close
���:
        uartNum:���ڱ�ţ���1��ʼ,���Ϊ3
����:
        TRUEΪ�ɹ�������ʧ��
����:��־ǿ
ʱ��:20120615
*/

int     NUart_close ( unsigned int uartNum );

/*
������:NUart_writeBytes
���:
        uartNum:���ڱ�ţ���1��ʼ,���Ϊ3
        buff:���͵����ݵ�ַ
        len:���͵��ֽ���
����:
        �ɹ�����>0Ϊ�����˶��ٸ��ֽ���������Ϊʧ��
����:��־ǿ
ʱ��:20120615
*/
int  NUart_writeBytes ( unsigned int uartNum, char *buff, int len );

BOOL    tstUart ( unsigned int uartNum );

#endif

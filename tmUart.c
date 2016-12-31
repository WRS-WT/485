
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <linux/if_packet.h>
#include <linux/ioctl.h>
#include <linux/rtc.h>



#include "tmUart.h"


#define TRUE 1
#define INVALID_SOCKET -1

#define TRACE()\
	{             \
		printf(" %s(%d)\n",__FILE__,__LINE__);\
	}


#ifdef  RELEASE_DVR
#define DP()
#else
#define DP()\
	{             \
		printf(" %s(%d) - %d , %s\n",__FILE__,__LINE__,errno,strerror(errno));\
	}
#endif

#ifdef  RELEASE_DVR
#define ERR()
#else
#define ERR()\
	{             \
		printf("*** ERR: %s(%d) - %d , %s\n",__FILE__,__LINE__,errno,strerror(errno));\
	}
#endif

#define RECV_BUFFER_SIZE 1024
#define MAX_NUART_COUNT 3
#define MAGIC_NUART_INST        0x12349876

typedef struct _uart_msg_struct
{
	unsigned char   sign[4];//sign[3] 为串口标识,sign[0]~sign[2] 为标识
	unsigned int    cmd;
	unsigned int    cmdLen;
	unsigned char  data[4];
} UartMst_t, *UartMst_pt;

#define TY_UART_UDP_PORT                20000
typedef enum _net_uart_cmd_enum
{
         CMD_NUART_INVALID = 0,
         CMD_NUART_LOGIN ,
         CMD_NUART_WTUART,
         CMD_NUART_RDUART,
         CMD_NUART_SETCFG,
         CMD_NUART_GETCFG,
         CMD_NUART_LOGOUT,
} NUART_CMD_EN;


typedef struct _nuart_commu_class
{
	unsigned int    magic;
	BOOL            bNUart;
	BOOL            bExit;
	BOOL            bOpen;
	int                     fdHandle;
	
	unsigned int    chanNum;
	int                     baudrate;
	
	pthread_t               hThead;
	
	
	struct sockaddr_in              tUartNAddr;
	UartRecvInst_t          tRecvInst;
	
	//回调函数的参数
	UART_GetMsgCallBackFun  fnRecvMsgHandle;
	void                                           *pUserData;
} NUartInst_t, *NUartInst_pt;

static NUartInst_t      _arNUartInst[MAX_UART_NUM] = {MAGIC_NUART_INST, 0, 0, 0, -1,};


//======== 所有串口的公共部分
const static _NUartMsgHeaderSize = sizeof ( UartMst_t ) - 4;
static UartMst_pt       _pSndNMsg = NULL;
static char     _szSndBuffer[RECV_BUFFER_SIZE];
static BOOL     _bInit = FALSE;
static pthread_mutex_t  _hNsendMutex;//系统配置参数的互斥量
inline static BOOL ENTER_CR_NUart()
{
	return ( 0 == pthread_mutex_lock ( &_hNsendMutex ) );
}
inline static BOOL EXIT_CR_NUart()
{
	return ( 0 == pthread_mutex_unlock ( &_hNsendMutex ) );
}

typedef void * ( *ThreadEntryPtrType ) ( void * );

	BOOL CreateCommonMutex ( pthread_mutex_t *pMutex )
	{
		int resp = 0;
		pthread_mutexattr_t gMutexAttr;
		pthread_mutexattr_init ( &gMutexAttr );
		pthread_mutexattr_settype ( &gMutexAttr, PTHREAD_MUTEX_RECURSIVE_NP );
		resp = pthread_mutex_init ( pMutex, &gMutexAttr );
		
		//assert(0== resp);
		if ( 0 != resp )
		{
			pthread_mutexattr_destroy ( &gMutexAttr );
			return FALSE;
		}
		
		pthread_mutexattr_destroy ( &gMutexAttr );
		return TRUE;
	}
	

	BOOL CloseMutex ( pthread_mutex_t *pMutex )
{
	return ( 0 == pthread_mutex_destroy ( pMutex ) );
}
	
BOOL CreateDetachedThread ( pthread_t *pid, ThreadEntryPtrType entry, void *para )
{
	pthread_t ThreadId = 0;
	pthread_attr_t attr;
	pthread_attr_init ( &attr );
	pthread_attr_setscope ( &attr, PTHREAD_SCOPE_SYSTEM ); //绑定
	pthread_attr_setdetachstate ( &attr, PTHREAD_CREATE_DETACHED ); //分离
	
	if ( pthread_create ( &ThreadId, &attr, entry, para ) == 0 ) //创建线程
	{
		pthread_attr_destroy ( &attr );
		//TRY_EVALUATE_POINTER ( pid, ThreadId );
		return TRUE;
	}
	
	else
	{
		pthread_attr_destroy ( &attr );
		return FALSE;
	}
}

static BOOL     NUart_init()
{
	if ( !CreateCommonMutex ( &_hNsendMutex ) )
	{
		return FALSE;
	}
	
	_pSndNMsg = ( UartMst_pt ) _szSndBuffer;
	_pSndNMsg->sign[0] = 'D';
	_pSndNMsg->sign[1] = 'V';
	_pSndNMsg->sign[2] = 'R';
	_bInit = TRUE;
	return TRUE;
}

static BOOL NUart_deinit()
{
	CloseMutex ( &_hNsendMutex );
	_bInit = FALSE;
	return TRUE;
}

/*static BOOL   UartRecv_init(UartRecvInst_pt pInst,unsigned char *pBuff,unsigned int   bufSize)
{
        if( (NULL == pInst)||(NULL == pBuff) ||(bufSize <512))
        {
                return FALSE;
        }
        pInst->bufSize = bufSize;
        pInst->pBuff = pBuff;
        pInst->wtIndex = 0;
        pInst->rdIndex = 0;
        pInst->dataCount = 0;
        return TRUE;
}

static void     UartRecv_putData(UartRecvInst_pt pInst,unsigned char *pData,int dataLen)
{
        int i =0;

        for(i=0;i<dataLen;i++)
        {
                pInst->pBuff[pInst->wtIndex ++] = pData[i];
                pInst->wtIndex %= pInst->bufSize;
                pInst->dataCount++;
        }
}*/
static int      NUart_nsend ( NUartInst_pt pInst, const void *pData, int data_len )
{
	int     ret = 0;
	ENTER_CR_NUart();
	_pSndNMsg->sign[3] = pInst->chanNum & 0xFF;
	_pSndNMsg->cmdLen = data_len;
	memcpy ( _pSndNMsg->data, pData, data_len );
	_pSndNMsg->cmd = CMD_NUART_WTUART;
	ret =  sendto ( pInst->fdHandle
	                , ( const char * ) _pSndNMsg
	                , _NUartMsgHeaderSize + data_len, 0
	                , ( struct sockaddr_in * ) & ( pInst->tUartNAddr )
	                , sizeof ( struct sockaddr_in ) );
	EXIT_CR_NUart();
	return ret ;
}
void UartPrintf ( unsigned char *pData, int dataLen, BOOL bString );
static int  NUart_sendCMD ( unsigned int uartNum, unsigned int CMD );
static int set_opt ( int fd, int nSpeed, int nBits, char nEvent, int nStop );
//================================
static void *NUart_uartTask ( void *param )
{
	//static unsigned int   tstIndex = 0;
	int     nReadLen = 0;
	unsigned long   rdTimes = 0;
	fd_set rfds;
	struct timeval tv;
	int RetVal;
	int     RECVBUFSIZE = 0;
	char    *pRecvBuffer = NULL;
	NUartInst_pt    pInst = NULL;
	//int   curRecvPos=0;
	//接收消息帧的buffer
	pInst = ( NUartInst_pt ) param;

	RECVBUFSIZE = UART_RECV_BUFF_SIZE;
	pRecvBuffer = ( char * ) malloc ( RECVBUFSIZE );
	memset(pRecvBuffer, 0x0, RECVBUFSIZE);
	if ( NULL == pRecvBuffer )
	{
		DP();
		pInst->bOpen = FALSE;
		return NULL;
	}
	
	//printf("XUZQ TEST--[%u]---------- come in UART_recvTask()---------\n",++tstIndex);
	
	while ( TRUE )
	{
		if ( pInst->bExit )
		{
			printf ( "Uart[%u] :EXIT in UART_recvTask\n", pInst->chanNum );
			break;
		}
		
		FD_ZERO ( &rfds );
		FD_SET ( pInst->fdHandle, &rfds );
		tv.tv_sec  = 30;
		tv.tv_usec = 0;
		rdTimes++;
		RetVal = select ( pInst->fdHandle + 1, &rfds, NULL, NULL, &tv );
		
		if ( -1 == RetVal )
		{
			printf ( "[%ld]:Uart[%u] :read ERROR in UART_recvTask\n", rdTimes, pInst->chanNum );
			break;
		}
		
		else if ( 0 == RetVal )
		{
			continue;
		}
		
		nReadLen = read ( pInst->fdHandle, pRecvBuffer, RECVBUFSIZE );
		
		if ( 0 >= nReadLen )
		{
			printf ( "[%ld]:Uart[%u] :read ERROR in UART_recvTask\n", rdTimes, pInst->chanNum );
			break;
		}
		
#ifdef  _DEBUG_UART
		UartPrintf ( pRecvBuffer, nReadLen, FALSE );
#endif
		
		if ( NULL != pInst->fnRecvMsgHandle )
		{
			//printf(" 2 callback 0x%x \n",pInst->fnRecvMsgHandle);
			RetVal = pInst->fnRecvMsgHandle ( pRecvBuffer, nReadLen, pInst->pUserData );
			
			if ( 0 > RetVal )
			{
				//printf("fnRecvMsgHandle(%d) return %d\n",nReadLen,RetVal);
			}
			
			//else
			//      printf("uart rcv %d\n", RetVal);
		}
	}
	
	if ( NULL != pRecvBuffer )
	{
		free ( pRecvBuffer );
		pRecvBuffer = NULL;
	}
	
	if ( -1 != pInst->fdHandle )
	{
		close ( pInst->fdHandle );
		pInst->fdHandle = -1;
	}
	
	printf ( "[%u]-UART : EXIT in UART_recvTask\n", pInst->chanNum );
	return 0;//maxiaozhi 添加
}

void UartPrintf ( unsigned char *pData, int dataLen, BOOL bDisplayString )
{
	static unsigned long tstTimes = 0;
	int i;
	tstTimes++;
	printf ( "*****  MARK  **************\n[%ld]:recv [%d] data:\n", tstTimes, dataLen );
	
	for ( i = 0; i < dataLen; i++ )
	{
		if ( bDisplayString ) { printf ( "%c", pData[i] ); }
		
		else
		{
			if ( ( i % 16 ) == 0 ) { printf ( "\n" ); }
			
			printf ( "%02x ", pData[i] );
		}
	}
	
	printf ( "\n*****************************\n" );
}
/*
static int NUart_ndpTask ( void *param )
{
	fd_set                          fdRead;
	int                                     nSize;
	int                                     ret;
	struct timeval                  tWait;
	struct sockaddr_in              ClientAddr;
	int     RECVBUFSIZE = 0;
	char    *pRecvBuffer = NULL;
	NUartInst_pt    pInst = NULL;
	UartMst_pt      pMsg = NULL;
	unsigned short  usPort = 0;
	int     hSocket = INVALID_SOCKET;
	char    *pInitBuffer = NULL;
	pInst = ( NUartInst_pt ) param;
	RECVBUFSIZE = UART_RECV_BUFF_SIZE + 64;
	pInitBuffer = ( char * ) malloc ( RECVBUFSIZE );
	
	if ( NULL == pInitBuffer )
	{
		DP();
		pInst->bOpen = FALSE;
		return -1;
	}
	
	pRecvBuffer = pInitBuffer + 32;//为了前面还有多余的buffer可以用
	pMsg = ( UartMst_pt ) pRecvBuffer;
	tWait.tv_sec = 5;
	tWait.tv_usec = 0;
	usPort = TY_UART_UDP_PORT + ( pInst->chanNum - 1 ) * 2 ;
	hSocket  = UdpServerListenCreate ( INADDR_ANY, usPort );
	
	if ( INVALID_SOCKET == hSocket )
	{
		DP();
		pInst->bOpen = FALSE;
		
		//maxiaozhi 修改内存泄露2013-08-07
		if ( NULL != pInitBuffer )
		{
			free ( pInitBuffer );
			pInitBuffer = NULL;
		}
		
		return 1;
	}
	
	pInst->fdHandle = hSocket;
	sleep ( 3 ); //防止NUart还没起来
	NUart_sendCMD ( pInst->chanNum, CMD_NUART_LOGIN );
	
	while ( TRUE )
	{
		if ( pInst->bExit ) { break; }
		
		tWait.tv_sec = 5;
		tWait.tv_usec = 0;
		FD_ZERO ( &fdRead );
		FD_SET ( hSocket, &fdRead );
		ret = select ( hSocket + 1, &fdRead, NULL, NULL, &tWait );
		
		if ( 0 == ret )
		{
			//DP();
			continue;
		}
		
		else if ( -1 == ret )
		{
			DP();
			break;
		}
		
		if ( !FD_ISSET ( hSocket, &fdRead ) )
		{
			DP();
			continue;
		}
		
		memset ( &ClientAddr, 0, sizeof ( ClientAddr ) );
		nSize = sizeof ( ClientAddr );
		ret = recvfrom ( hSocket, pRecvBuffer, RECVBUFSIZE, 0, ( struct sockaddr * ) &ClientAddr, ( socklen_t * ) &nSize );
		
		if ( ret == 0 || ret < 0 )
		{
			DP();
			continue;
		}
		
		//printf("recvfrom(%d) in NUart_ndpTask()\n",ret);
#ifdef  _DEBUG_UART
		
		if ( ( pMsg->cmdLen ) > RECVBUFSIZE )
		{
			UartPrintf ( ( unsigned char * ) pRecvBuffer, ret, FALSE );
		}
		
#endif
		
		if ( CMD_NUART_RDUART != pMsg->cmd )
		{
			printf ( "@@@@@@@@@ pMsg->cmd = %x - %x(myport :%u) at %s_%d\n"
			         , pMsg->cmd
			         , ClientAddr.sin_port
			         , usPort
			         , __FILE__
			         , __LINE__ );
			UartPrintf ( ( unsigned char * ) pRecvBuffer, ret, FALSE );
			continue;
		}
		
		if ( NULL == pInst->fnRecvMsgHandle )
		{
			//DP();
			continue;
		}
		
		if ( pInst->chanNum != pMsg->sign[3] )
		{
			DP();
			continue;
		}
		
		ret = pInst->fnRecvMsgHandle ( ( char * ) pMsg->data, pMsg->cmdLen, pInst->pUserData );
	}
	
	shutdown ( hSocket, 2 );
	close ( hSocket );
	
	if ( NULL != pInitBuffer )
	{
		free ( pInitBuffer );
		pInitBuffer = NULL;
	}
	
	pInst->fdHandle = hSocket = INVALID_SOCKET;
	pInst->bOpen = FALSE;
	return 0;
}
*/

BOOL    NUart_open ( unsigned int uartNum,
                     BOOL bNuart,
                     int baudrate,
                     UART_TYPE_E enAppType,
                     UART_GetMsgCallBackFun fnRecvMsg,
                     void *pUserData )
{
	char    szName[80];
	NUartInst_pt pInst = NULL;
	
	if ( ( uartNum >= MAX_UART_NUM ) || ( uartNum == 0 ) )
	{
		printf ( "ERROR:uartNum(%u) >= MAX_UART_NUM \n", uartNum );
		return FALSE;
	}
	
	//TRACE_DB ( "NUart_open OK:uartNum(%u),(baudrate:%d),(bNuart=%d)\n", uartNum, baudrate, bNuart );
	
	if ( !_bInit )
	{
		NUart_init();
	}
	
	pInst = &_arNUartInst[uartNum];
	
	if ( pInst->bOpen )
	{
		DP();
		return FALSE;
	}
	
	pInst->fnRecvMsgHandle = fnRecvMsg;
	//printf("callback 0x%x \n",pInst->fnRecvMsgHandle);
	pInst->pUserData = pUserData;
	
	if ( bNuart )
	{/*
		unsigned short usNUartPort = 0;
		pInst->bNUart = TRUE;
		pInst->chanNum = uartNum;
		sprintf ( szName, "/app/NUart %u %d %u &", uartNum, baudrate, enAppType );
		system ( szName );
		usNUartPort = TY_UART_UDP_PORT + ( pInst->chanNum - 1 ) * 2 + 1;
		memset ( & ( pInst->tUartNAddr ), 0, sizeof ( struct sockaddr_in ) );
		pInst->tUartNAddr.sin_family = AF_INET;
		pInst->tUartNAddr.sin_port = htons ( usNUartPort );
		pInst->tUartNAddr.sin_addr.s_addr = htonl ( INADDR_ANY );
		pInst->baudrate = baudrate;
		pInst->bOpen = TRUE;
		return  CreateDetachedThread ( & ( pInst->hThead ), NUart_ndpTask, ( void * ) pInst );
	*/
	}
	
	else
	{
		TRACE();
		pInst->bNUart = FALSE;
		sprintf ( szName, "/dev/ttyAMA%d", uartNum );
		pInst->fdHandle = open ( szName, O_RDWR | O_NOCTTY | O_NDELAY );
		
		if ( -1 == pInst->fdHandle )
		{
			DP();
			return FALSE;
		}
		
		TRACE();
		
		if ( 0 == set_opt ( pInst->fdHandle, baudrate, 8, 'N', 1 ) )
		{
			pInst->baudrate = baudrate;
			pInst->bOpen = TRUE;
			TRACE();
			return  CreateDetachedThread ( & ( pInst->hThead ), NUart_uartTask, ( void * ) pInst );
		}
		
		else
		{
			DP();
			close ( pInst->fdHandle );
			pInst->fdHandle = -1;
			return FALSE;
		}
	}
}
int     NUart_close ( unsigned int uartNum )
{
#if 0
	NUartInst_pt pInst = NULL;
	
	if ( _bInit )
	{
		NUart_deinit();
	}
	
	pInst = &_arNUartInst[uartNum];
	
	if ( pInst->bOpen )
	{
		pInst->bOpen = FALSE;
		pInst->fnRecvMsgHandle = NULL;
		pInst->pUserData = NULL;
	}
	
#endif
	return 0;
}
int  NUart_sendCMD ( unsigned int uartNum, unsigned int CMD )
{
	NUartInst_pt pInst = NULL;
	int ret = 0;
	
	if ( ( uartNum >= MAX_UART_NUM ) || ( uartNum == 0 ) )
	{
		printf ( "ERROR:uartNum(%u) >= MAX_UART_NUM at %s_%d\n"
		         , uartNum
		         , __FILE__
		         , __LINE__ );
		return -1;
	}
	
	pInst = &_arNUartInst[uartNum];
	
	if ( pInst->fdHandle == -1 )
	{
		DP();
		return -2;
	}
	
	ENTER_CR_NUart();
	_pSndNMsg->sign[3] = pInst->chanNum & 0xFF;
	_pSndNMsg->cmdLen = 0;
	_pSndNMsg->cmd = CMD;
	ret =  sendto ( pInst->fdHandle
	                , ( const char * ) _pSndNMsg
	                , _NUartMsgHeaderSize, 0
	                , ( struct sockaddr_in * ) & ( pInst->tUartNAddr )
	                , sizeof ( struct sockaddr_in ) );
	EXIT_CR_NUart();
	return ret;
}

int  NUart_writeBytes ( unsigned int uartNum, char *buff, int len )
{
	NUartInst_pt pInst = NULL;
	
	if ( ( uartNum >= MAX_UART_NUM ) || ( uartNum == 0 ) )
	{
		printf ( "ERROR:uartNum(%u) >= MAX_UART_NUM \n", uartNum );
		return -1;
	}
	
	pInst = &_arNUartInst[uartNum];
	
	if ( pInst->fdHandle == -1 )
	{
		DP();
		return -2;
	}
	
	if ( pInst->bNUart )
	{
		return NUart_nsend ( pInst, buff, len );
	}
	
	else
	{
		//TRACE_DB("write uart========>len=%d", len);
		return write ( pInst->fdHandle, buff, len );
	}
}

int set_opt ( int fd, int nSpeed, int nBits, char nEvent, int nStop )
{
	/*
	设置串口属性：
	fd: 文件描述符
	nSpeed: 波特率
	nBits: 数据位
	nEvent: 奇偶校验
	nStop: 停止位
	*/
	struct termios newtio, oldtio;
	
	if ( tcgetattr ( fd, &oldtio ) != 0 )
	{
		perror ( "SetupSerial 1" );
		return -1;
	}
	
	bzero ( &newtio, sizeof ( newtio ) );
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;
	
	switch ( nBits )
	{
		case 7:
			newtio.c_cflag |= CS7;
			break;
			
		default:
		case 8:
			newtio.c_cflag |= CS8;
			break;
	}
	
	switch ( nEvent )
	{
		case 'O':
		{
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= ( INPCK | ISTRIP );
		}
		break;
		
		case 'E':
		{
			//      newtio.c_iflag |= (INPCK |ISTRIP);
			newtio.c_iflag |= ( INPCK ); //不能去掉第8位ISTRIP:表示去掉第8位
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
		}
		break;
		
		default:
		case 'N':
			newtio.c_cflag &= ~PARENB;
			break;
	}
	
	switch ( nSpeed )
	{
		case 2400:
		{
			cfsetispeed ( &newtio, B2400 );
			cfsetospeed ( &newtio, B2400 );
		}
		break;
		
		case 4800:
		{
			cfsetispeed ( &newtio, B4800 );
			cfsetospeed ( &newtio, B4800 );
		}
		break;
		
		case 9600:
		{
			cfsetispeed ( &newtio, B9600 );
			cfsetospeed ( &newtio, B9600 );
		}
		break;
		
		case 38400:
		{
			cfsetispeed ( &newtio, B38400 );
			cfsetospeed ( &newtio, B38400 );
		}
		break;
		
		case 57600:
		{
			cfsetispeed ( &newtio, B57600 );
			cfsetospeed ( &newtio, B57600 );
		}
		break;
		
		case 115200:
		{
			cfsetispeed ( &newtio, B115200 );
			cfsetospeed ( &newtio, B115200 );
		}
		break;
		
		default:
		{
			cfsetispeed ( &newtio, B9600 );
			cfsetospeed ( &newtio, B9600 );
		}
		break;
	}
	
	if ( nStop == 2 )
	{ newtio.c_cflag |= CSTOPB; }
	
	else
	{ newtio.c_cflag &= ~CSTOPB; }
	
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush ( fd, TCIFLUSH );
	
	if ( ( tcsetattr ( fd, TCSANOW, &newtio ) ) != 0 )
	{
		perror ( "com set error" );
		return -1;
	}
	
	return 0;
}


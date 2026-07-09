//我

#include "hal_defs.h"  
#include "hal_cc8051.h"  
#include "hal_int.h"  
#include "hal_mcu.h"  
#include "hal_board.h"  
#include "hal_led.h"  
#include "hal_adc.h"  
#include "hal_rf.h"  
#include "basic_rf.h"  
#include "hal_uart.h"   
#include "TIMER.h"  
#include "get_adc.h"  
#include "sht.h"
#include "UART_PRINT.h"  
#include "util.h"  
#include <stdlib.h>  
#include <string.h>  
#include <stdio.h>  
#include <math.h>  

/*点对点通讯地址设置*/
#define RF_CHANNEL                16         // 频道 11~26  
#define PAN_ID                    0xD0C2     //网络id  
#define MY_ADDR                   0xC2BD     //本机模块地址
#define SEND_ADDR                 0xB4F3     //发送地址
/* 自定义消息格式 */
#define START_HEAD    0xCC//帧头
#define CMD_READ      0x01//读传感器数据
#define SENSOR_TEMP   0x01//温度
#define SENSOR_RH     0x02//湿度
#define SENSOR_FIRE   0x03//火焰
/*  LED n 闪烁 time 毫秒 宏 */
#define FlashLed(n,time) do{\
                   halLedSet(n);\
                   halMcuWaitMs(time);\
                   halLedClear(n);\
                   }while(0)
/*数组大小*/
#define MAX_SEND_BUF_LEN  128 //无线数据最大发送长度
#define MAX_RECV_BUF_LEN  128 //无线数据最大接收长度
/*变量*/
static basicRfCfg_t basicRfConfig; 
static uint8 pTxData[MAX_SEND_BUF_LEN]; //定义无线发送缓冲区的大小
static uint8 pRxData[MAX_RECV_BUF_LEN]; //定义无线接收缓冲区的大小
uint8 APP_SEND_DATA_FLAG;  

/*****************************************************************************************
*函数：uint8 CheckSum(uint8 *buf, uint8 len) 
*功能：计算校验和 
*输入：uint8 *buf-指向输入缓存区, uint8 len输入数据字节个数 
*输出：无 
*返回：返回校验和 
*特殊说明：无 
*****************************************************************************************/ 
uint8 CheckSum(uint8 *buf, uint8 len)  
{  
  uint8 temp = 0;  
  while(len--)  
  {  
    temp += *buf;  
    buf++;  
  }  
  return (uint8)temp;  
}  


void ConfigRf_Init(void)  
{  
  basicRfConfig.panId       =   PAN_ID;        //zigbee的ID号设置  
  basicRfConfig.channel     =   RF_CHANNEL;    //zigbee的频道设置  
  basicRfConfig.myAddr      =   MY_ADDR;   //设置本机地址  
  basicRfConfig.ackRequest  =   TRUE;          //应答信号  
  while(basicRfInit(&basicRfConfig) == FAILED); //检测zigbee的参数是否配置成功  
  basicRfReceiveOn();                // 打开RF  
}  



void main(void)    
{
    halBoardInit();  //模块相关资源的初始化
    ConfigRf_Init(); //无线收发参数的配置初始化 
    Timer4_Init(); //定时器初始化
    Timer4_On();  //打开定时器
    
    SHT_Init();
    
    while(1)
    {   APP_SEND_DATA_FLAG = GetSendDataFlag();           
        if(APP_SEND_DATA_FLAG == 1)   //定时时间到
        {   /*【传感器采集、处理】 开始*/
          /*
          int sensor_val=60,sensor_tem=25;
          call_sht1x((int *)(&sensor_tem),(int *)(&sensor_val));    //取温湿度数据
          */
          uint8 sensor_tem;
          uint8 sensor_val;
          SHT_SmpSnValue((int8 *)(&sensor_tem), (uint8 *)(&sensor_val));
#ifdef CC2530_DEBUG
          //把采集数据传化成字符串，以便于在串口上显示观察
          uart_printf("温湿度传感器，温度：%d℃, 湿度：%d%%\r\n", sensor_tem, sensor_val); 
#endif /*CC2530_DEBUG*/
          memset(pTxData, '\0', MAX_SEND_BUF_LEN);
          pTxData[0]=START_HEAD;//帧头
          pTxData[1]=CMD_READ;//命令
          pTxData[2]=8;//长度
          pTxData[3]=2;//2组传感数据
          pTxData[4]=SENSOR_TEMP;//传感类型
          pTxData[5]=sensor_tem;
          pTxData[6]=SENSOR_RH;//传感类型
          pTxData[7]=sensor_val;
          pTxData[8]=CheckSum((uint8 *)pTxData, pTxData[2]);
          //把数据通过zigbee发送出去
          basicRfSendPacket((unsigned short)SEND_ADDR, (unsigned char *)pTxData, pTxData[2]+1);
          FlashLed(1,100);//无无线发送指示，LED1亮100ms
          Timer4_On();  //打开定时
        }  /*【传感器采集、处理】 结束*/           
    }
}

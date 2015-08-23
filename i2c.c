#include "stm32f10x_lib.h"

//条件编译 1:使用软件模拟I2C
//#define PIN_SCL 	GPIO_Pin_6
//#define PIN_SDA 	GPIO_Pin_7


static __inline void TWI_SCL_0(void)		{ GPIOB->BRR=PIN_SCL; }
static __inline void TWI_SCL_1(void) 		{ GPIOB->BSRR=PIN_SCL;}
static __inline void TWI_SDA_0(void) 		{ GPIOB->BRR=PIN_SDA; }
static __inline void TWI_SDA_1(void) 		{ GPIOB->BSRR=PIN_SDA;}
static __inline u8   TWI_SDA_STATE(void)        { return (GPIOB->IDR & PIN_SDA) != 0; }
//static __inline void TWI_NOP(void) 		{  TWI_Delay(); }

static const u8 TWI_ACK   	=0;
static const u8 TWI_READY 	=0;
static const u8 TWI_NACK 	=1;
static const u8 TWI_BUS_BUSY  	=2;
static const u8 TWI_BUS_ERROR 	=3;

static const u8  TWI_RETRY_COUNT = 3; //重试次数

extern void Delay_mS(u32 n);

static void TWI_Initialize(void);
static u8 TWI_START(void);
static u8 TWI_START_SHT(void);
static void TWI_STOP(void);
static u8  TWI_SendByte(u8 Data);
static u8 TWI_ReceiveByte(void);
static void TWI_SendACK(void);
static void TWI_SendNACK(void);


/*******************************************************************************
 * 函数名称:TWI_Delay                                                                     
 * 描    述:延时函数                                                                     
 *                                                                               
 * 输    入:无                                                                     
 * 输    出:无                                                                     
 * 返    回:无                                                                     
 * 作    者:                                                                     
 * 修改日期:2010年6月8日                                                                    
 *******************************************************************************/
static void TWI_NOP(void)
{
 vu32 i, j;
 vu32 sum = 0;
 i = 20;
 while(i--)
 {
     for (j = 0; j < 10; j++)
	 sum += i; 
 }
 sum = i;
}

/*******************************************************************************
 * 函数名称:TWI_Initialize                                                                     
 * 描    述:I2C初始化函数                                                                     
 *                                                                               
 * 输    入:无                                                                     
 * 输    出:无                                                                     
 * 返    回:无                                                                     
 * 作    者:                                                                     
 * 修改日期:2010年6月8日                                                                    
 *******************************************************************************/
static void TWI_Initialize(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_OD;

  GPIO_InitStructure.GPIO_Pin = PIN_SDA;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Pin = PIN_SCL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  TWI_SDA_1();
  TWI_SCL_0(); 
  
  //////DebugPrint("Software TWI Initializing...\n"); 
}

/*******************************************************************************
 * 函数名称:TWI_START                                                                     
 * 描    述:发送启动                                                                     
 *                                                                               
 * 输    入:无                                                                     
 * 输    出:无                                                                     
 * 返    回:无                                                                     
 * 作    者:                                                                     
 * 修改日期:2010年6月8日                                                                    
 *******************************************************************************/
static u8 TWI_START(void)
{ 
 TWI_SDA_1(); 
 TWI_NOP();
   
 TWI_SCL_1(); 
 TWI_NOP();    

 if(!TWI_SDA_STATE())
 {
  ////DebugPrint("TWI_START:BUSY\n");
  return TWI_BUS_BUSY;
 }
 TWI_SDA_0();
 TWI_NOP();
  
 TWI_SCL_0();  
 TWI_NOP(); 

 if(TWI_SDA_STATE())
 {
  ////DebugPrint("TWI_START:BUS ERROR\n");
  return TWI_BUS_ERROR;
 } 
 
 return TWI_READY;
}

/* --------------------------------------------------------------------------*/
/** 
 * @Brief:  TWI_START_SHT 
 * 
 * @Returns:   
 */
/* --------------------------------------------------------------------------*/
static u8 TWI_START_SHT(void)
{
    TWI_SDA_1();
    TWI_SCL_0();
    TWI_NOP();

    TWI_SDA_1(); 
    TWI_SCL_1(); 
    TWI_NOP();

    if(!TWI_SDA_STATE())
    {
	    return TWI_BUS_BUSY;
    }
    TWI_SDA_0();
    TWI_NOP();

    TWI_SCL_0();  
    TWI_NOP(); 

    TWI_SCL_1();
    TWI_NOP();

    TWI_SDA_1();
    TWI_NOP();

    TWI_SCL_0();
    TWI_NOP();

    return TWI_READY;
}

/* --------------------------------------------------------------------------*/
/** 
 * @Brief:  TWI_STOP 
 */
/* --------------------------------------------------------------------------*/
static void TWI_STOP(void)
{
 TWI_SDA_0(); 
 TWI_NOP();
   
 TWI_SCL_1(); 
 TWI_NOP();    

 TWI_SDA_1();
 TWI_NOP();
  
 //////DebugPrint("TWI_STOP\n");   
}

/* --------------------------------------------------------------------------*/
/** 
 * @Brief:  TWI_SendACK 
 */
/* --------------------------------------------------------------------------*/
static void TWI_SendACK(void)
{
 TWI_SDA_0();
 TWI_NOP();
 TWI_SCL_1();
 TWI_NOP();
 TWI_SCL_0(); 
 TWI_NOP(); 
 TWI_SDA_1();
 //////DebugPrint("TWI_SendACK\n");   
}

/* --------------------------------------------------------------------------*/
/** 
 * @Brief:  TWI_SendNACK 
 */
/* --------------------------------------------------------------------------*/
static void TWI_SendNACK(void)
{
 TWI_SDA_1();
 TWI_NOP();
 TWI_SCL_1();
 TWI_NOP();
 TWI_SCL_0(); 
 TWI_NOP();
 //////DebugPrint("TWI_SendNACK\n");    
}

/* --------------------------------------------------------------------------*/
/** 
 * @Brief:  TWI_SendByte 
 * 
 * @Param: Data
 * 
 * @Returns:   
 */
/* --------------------------------------------------------------------------*/
static u8 TWI_SendByte(u8 Data)
{
 u8 i;
 TWI_SCL_0();
 for(i=0;i<8;i++)
 {  
  //---------数据建立----------
  if(Data&0x80)
  {
   TWI_SDA_1();
  }
  else
  {
   TWI_SDA_0();
  } 
  Data<<=1;
  TWI_NOP();
  //---数据建立保持一定延时----
  
  //----产生一个上升沿[正脉冲] 
  TWI_SCL_1();
  TWI_NOP();
  TWI_SCL_0();
  TWI_NOP();//延时,防止SCL还没变成低时改变SDA,从而产生START/STOP信号
  //---------------------------   
 }
 //接收从机的应答 
 TWI_SDA_1(); 
 TWI_NOP();
 TWI_SCL_1();
 TWI_NOP();   
 if(TWI_SDA_STATE())
 {
  TWI_SCL_0();
  TWI_SDA_1();
  //////DebugPrint("TWI_NACK!\n");
  return TWI_NACK;
 }
 else
 {
  TWI_SCL_0();
  TWI_SDA_1();
  //////DebugPrint("TWI_ACK!\n");
  return TWI_ACK;  
 }    
}

/* --------------------------------------------------------------------------*/
/** 
 * @Brief:  TWI_ReceiveByte 
 * 
 * @Returns:   
 */
/* --------------------------------------------------------------------------*/
static u8 TWI_ReceiveByte(void)
{
 u8 i,Dat;
 TWI_SDA_1();
 TWI_SCL_0(); 
 Dat=0;
 for(i=0;i<8;i++)
 {
  TWI_SCL_1();//产生时钟上升沿[正脉冲],让从机准备好数据 
  TWI_NOP(); 
  Dat<<=1;
  if(TWI_SDA_STATE()) //读引脚状态
  {
   Dat|=0x01; 
  }   
  TWI_SCL_0();//准备好再次接收数据  
  TWI_NOP();//等待数据准备好         
 }
 //////DebugPrint("TWI_Dat:%x\n",Dat);
 return Dat;
}

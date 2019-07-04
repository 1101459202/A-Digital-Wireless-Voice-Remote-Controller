#include "usart.h"
#include "delay.h"
#include "LED.h"
#include "stdarg.h"	
#include "string.h"	
#include <stdio.h>

FILE* COM1 = (FILE*)0x01;
FILE* COM2 = (FILE*)0x02;
FILE* COM3 = (FILE*)0x03;


#if (defined DEBUG)
//static uint32_t PC_RE_NUM = 0;
#endif

extern unsigned int YK_Rev_LOST_TIME;

#define USART1_TX_BUFF_SIZE  32  //���ͻ�����
#define USART1_RX_BUFF_SIZE  128 //���ջ���������һ���ŵ�������  2*5
#define YK_REV_Size 25
#define YK_Tra_Size 5

#define YK_TxBuff_Size USART1_TX_BUFF_SIZE
uint8_t YK_TX[YK_TxBuff_Size]={0xFF,0x00, 0x00, 0x00, 0xEE};//����֡������
#define YK_RxBuff_Size USART1_RX_BUFF_SIZE
uint8_t YK_RX[YK_RxBuff_Size];//�������ݴ�Ż�����
uint8_t YK_REV[YK_REV_Size];//��Ŵ��������͹���������֡���ڽ����ж��н����ɹ�������֡��
uint8_t YK_Rx_Flag=1;//���������еı�־
uint8_t YK_Tx_Flag=1;//���ͳɹ��ı�־
uint8_t YK_REV_Flag=1;//������һ����ȷ������֡�ı�־


void USART1_Config(u32 Baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
//----------------------------------------------------------------------------------------------------------------------------------------
	//��������ģ��M0�˿�
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
   //	
   //	��ӳ��ķ�ʽһ�������֡��ֱ��������£�
   //1.GPIO_Remap_SWJ_JTAGDisable�� /*!< JTAG-DP Disabled and SW-DP Enabled */  ������PB3��PB4��PA15����ͨIO��PA13&14����SWD����
   //2.GPIO_Remap_SWJ_Disable��  /*!< Full SWJ Disabled (JTAG-DP + SW-DP) */  5������ȫΪ��ͨ���ţ�����������JTAG&SWD���������ԣ�ֻ����st-link���� 
   //3.GPIO_Remap_SWJ_NoJTRST�� /*!< Full SWJ Enabled (JTAG-DP + SW-DP) but without JTRST */PB4��Ϊ��ͨIO�ڣ�JTAG&SWD����ʹ�ã���JTAGû�и�λ
   //������õ����е�������ŵ�����ͨIO�ڣ���ô����������е���ӳ������ӦдΪGPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE); 
   //�������PB3��PB4��PA15����ͨIO��PA13&14����SWD���ԣ�����ӳ������ӦдΪGPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); ͬ�������ֻ��PB4��Ϊ��ͨIO�ڵ����
   //	
   //	
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//
	//ѡE62��̨

   //#ifdef 	E62  //MO=0
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC  , ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;	//ѡ���Ӧ������
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;       
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);  //��ʼ��M0�˿�
  GPIO_ResetBits(GPIOC, GPIO_Pin_11);	 // M0=0
	
	//��������ģ��LOCK�˿�
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC  , ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	//ѡ���Ӧ������
  GPIO_InitStructure.GPIO_Mode =GPIO_Mode_IN_FLOATING;       
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);  //��ʼ��LOCK�˿�
	//GPIO_ResetBits(GPIOC, GPIO_Pin_10);	 //LOCK
	
	//��������ģ��AUX�˿�
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA  , ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;	//ѡ���Ӧ������
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;       
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��AUX�˿�
  //GPIO_ResetBits(GPIOA, GPIO_Pin_15);	 // �ر�AUX 

	//��������ģ��WX_EN PA8�˿�
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA  , ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;	//ѡ���Ӧ������
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;       
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��AUX�˿�
   GPIO_SetBits(GPIOA, GPIO_Pin_8);	 // �����ߵ�̨�ĵ�Դ 
	//-------------------------------------------------------------------------------------------------------
	////////////////////////////////////////////////////////////////////////////////////////////
   /* ��1������GPIO��USART������ʱ�� */
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);// ʹ�� USART1 ʱ��
   /* ��2������USART Tx��GPIO����Ϊ���츴��ģʽ */
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
   GPIO_Init(GPIOA, &GPIO_InitStructure);
   /* ��3������USART Rx��GPIO����Ϊ��������ģʽ
   ����CPU��λ��GPIOȱʡ���Ǹ�������ģʽ���������������費�Ǳ����
   ���ǣ��һ��ǽ�����ϱ����Ķ������ҷ�ֹ�����ط��޸���������ߵ����ò���
   */
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
   GPIO_Init(GPIOA, &GPIO_InitStructure);
	
  ////////////////////////////////////////////////////////////////////////////////////////////
    /* ��4��������USART����
    - BaudRate = 115200 baud
    - Word Length = 8 Bits
    - One Stop Bit
    - No parity
    - Hardware flow control disabled (RTS and CTS signals)
    - Receive and transmit enabled
    */
    USART_InitStructure.USART_BaudRate = Baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    //�����ж�
    USART_ITConfig(USART1, USART_IT_IDLE , ENABLE);
	 //�����ж�
    USART_ITConfig(USART1, USART_IT_ERR | USART_IT_ORE | USART_IT_NE | USART_IT_FE ,ENABLE);
   //��5����ʹ�� USART�� �������//
    USART_Cmd(USART1, ENABLE);  
   //CPU��Сȱ�ݣ��������úã����ֱ��Send�����1���ֽڷ��Ͳ���ȥ
    //�����������1���ֽ��޷���ȷ���ͳ�ȥ������ //
   // USART_ClearFlag(USART1, USART_FLAG_TC); // �巢����Ǳ�־��Transmission Complete flag //
		USART_ClearFlag(USART1, USART_FLAG_TC|USART_FLAG_RXNE|USART_FLAG_IDLE|USART_FLAG_ORE|USART_FLAG_NE|USART_FLAG_FE |USART_FLAG_PE ); 

  ////////////////////////////////////////////////////////////////////////////////////////////
  /* DMA clock enable */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//DMA1
  /* DMA1 Channel4 (triggered by USART1 Tx event) Config */
  DMA_DeInit(DMA1_Channel4);  
  DMA_InitStructure.DMA_PeripheralBaseAddr =(uint32_t)(&USART1->DR);// 0x40013804;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)YK_TX;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize = YK_TxBuff_Size;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel4, &DMA_InitStructure);
  DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);//��������ж�
  DMA_ITConfig(DMA1_Channel4, DMA_IT_TE, ENABLE);//��������ж�
	DMA_ClearITPendingBit(DMA1_IT_GL4 | DMA1_IT_TC4| DMA1_IT_TE4 | DMA1_IT_HT4);//
  //DMA_ClearFlag(DMA1_FLAG_GL4 | DMA1_FLAG_TC4 | DMA1_FLAG_TE4 | DMA1_FLAG_HT4);//
  /* Enable USART1 DMA TX request */
  USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
  DMA_Cmd(DMA1_Channel4, DISABLE);
	
	  ////////////////////////////////////////////////////////////////////////////////////////////
 /* DMA1 Channel5 (triggered by USART1 Rx event) Config */
  DMA_DeInit(DMA1_Channel5);  
  DMA_InitStructure.DMA_PeripheralBaseAddr =(uint32_t)(&USART1->DR);// 0x40013804;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)YK_RX;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = YK_RxBuff_Size;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel5, &DMA_InitStructure);
  DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);
  DMA_ITConfig(DMA1_Channel5, DMA_IT_TE, ENABLE);
	DMA_ClearITPendingBit(DMA1_IT_GL5 | DMA1_IT_TC5| DMA1_IT_TE5 | DMA1_IT_HT5);//
  //DMA_ClearFlag(DMA1_FLAG_GL5 | DMA1_FLAG_TC5 | DMA1_FLAG_TE5 | DMA1_FLAG_HT5);//
  /* Enable USART1 DMA RX request */ 
  USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
  DMA_Cmd(DMA1_Channel5, ENABLE);
	
	
	////////////////////////////////////////////////////////////////////////////////////////////	
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  
  /* Enable the USART1 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  
  //Enable DMA Channel4 TX Interrupt 
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
  /*Enable DMA Channel5 RX Interrupt */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);  
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}


//USART1�жϷ�����
void USART1_IRQHandler(void)
{
   u16 DATA_LEN;
   u16 i;
	 YK_Rx_Flag=0;//���췢������ջ���
   if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)//���Ϊ���������ж�
    {
                DATA_LEN=USART1_RX_BUFF_SIZE-DMA_GetCurrDataCounter(DMA1_Channel5); 
              if(DATA_LEN >=YK_REV_Size)//����յ��㹻�����ݲ��Ҳ����˿����ж�
                  {   
                      DMA_Cmd(DMA1_Channel5, DISABLE);		////�ر�DMA,��ֹ�������������						
									   if(YK_RX[0]==0xFF && YK_RX[YK_REV_Size-1]==0xEE)
									   {
											 LEDY(2);
									  	 memcpy(YK_REV,YK_RX,YK_REV_Size);//��������
											  YK_Rev_LOST_TIME=0;
									  	 YK_REV_Flag=1;//�������֡
									   }
									 //����DMA
                   DMA_Cmd(DMA1_Channel5, DISABLE);
                   DMA_ClearFlag(DMA1_FLAG_GL5 | DMA1_FLAG_TC5 | DMA1_FLAG_TE5 | DMA1_FLAG_HT5);//���־
                   DMA1_Channel5->CNDTR = USART1_RX_BUFF_SIZE;//��װ��
                   DMA_Cmd(DMA1_Channel5, ENABLE);//������,�ؿ�DMA
									}					
//									 //����DMA
//                   DMA_Cmd(DMA1_Channel5, DISABLE);
//                   DMA_ClearFlag(DMA1_FLAG_GL5 | DMA1_FLAG_TC5 | DMA1_FLAG_TE5 | DMA1_FLAG_HT5);//���־
//                   DMA1_Channel5->CNDTR = USART1_RX_BUFF_SIZE;//��װ��
//                   DMA_Cmd(DMA1_Channel5, ENABLE);//������,�ؿ�DMA
                   //��SR���DR���Idle----��������ж�
                   i=USART1->SR;
                   i=USART1->DR;
									 i=i;	
     }

		            //��USART���ļ�905��
		            //USART_ClearFlag(USART1, USART_FLAG_IDLE|USART_FLAG_ORE|USART_FLAG_NE|USART_FLAG_FE |USART_FLAG_PE );//������д����־
                USART_ClearITPendingBit(USART1,  USART_IT_IDLE | USART_IT_PE | USART_IT_FE | USART_IT_NE|USART_IT_ERR|USART_IT_ORE);////������д����ж�λ
		            YK_Rx_Flag=1;//�����жϴ�����ִ�����
}

void USART1_TIMEOUT_Handler(void)	
{
   u16 i;
//	 LED1(0);
//	 LED2(0);
   DMA_Cmd(DMA1_Channel5, DISABLE);//�ر�DMA,��ֹ�������������
	 YK_Rx_Flag=0;//���췢������ջ���
   //��SR���DR���Idle----��������ж�
   i=USART1->SR;
   i=USART1->DR;
	 i=i;
   //��USART���ļ�905��
           USART_ClearFlag(USART1, USART_FLAG_TXE|USART_FLAG_TC|USART_FLAG_RXNE|USART_FLAG_IDLE|USART_FLAG_ORE|USART_FLAG_NE|USART_FLAG_FE |USART_FLAG_PE );//������д����־
   USART_ClearITPendingBit(USART1, USART_IT_TXE |USART_IT_TC|USART_IT_RXNE|USART_IT_IDLE | USART_IT_PE | USART_IT_FE | USART_IT_NE|USART_IT_ERR|USART_IT_ORE);////������д����ж�λ
   //����DMA���մ���ͨ�� 
   DMA_ClearFlag(DMA1_FLAG_GL5 | DMA1_FLAG_TC5 | DMA1_FLAG_TE5 | DMA1_FLAG_HT5);//��DMAͨ�����б�־
   DMA1_Channel5->CNDTR = USART1_RX_BUFF_SIZE;//��װ��
   DMA_Cmd(DMA1_Channel5, ENABLE);//������,�ؿ�DMA  

	//����DMA���մ���ͨ�� 
   YK_Tx_Flag=0;
   DMA_Cmd(DMA1_Channel4, DISABLE);//������ϲ�������ɴ����жϺ�ر�DMA
	 DMA_ClearITPendingBit(DMA1_IT_GL4 | DMA1_IT_TC4| DMA1_IT_TE4 | DMA1_IT_HT4);
   YK_Rx_Flag=1;//�����жϴ�����ִ�����
}


//DMA1_Channel5���ڽ����жϷ�����
void DMA1_Channel5_IRQHandler(void)
{
  DMA_ClearITPendingBit(DMA1_IT_GL5|DMA1_FLAG_TC5 | DMA1_IT_TE5 | DMA1_IT_HT5);
  DMA_Cmd(DMA1_Channel5, DISABLE);//�ر�DMA,��ֹ�������������
  DMA1_Channel5->CNDTR = USART1_RX_BUFF_SIZE;//��װ��
  DMA_Cmd(DMA1_Channel5, ENABLE);//������,�ؿ�DMA
}

//DMA1_Channel4���ڷ�������жϷ�����
//USART1ʹ��DMA�������жϷ������
void DMA1_Channel4_IRQHandler(void)
{
	if(DMA_GetFlagStatus(DMA1_FLAG_TC4)){YK_Tx_Flag=1;LEDB(2);}//��DMA������� //��������ж�
  DMA_ClearITPendingBit(DMA1_IT_GL4 | DMA1_IT_TC4| DMA1_IT_TE4 | DMA1_IT_HT4);
  DMA_Cmd(DMA1_Channel4, DISABLE);//������ϲ�������ɴ����жϺ�ر�DMA
  YK_Tx_Flag=1;//��DMA�������
}
//ң��������һ������֡�ķ���
void YK_CMD_TX(void)
	{
    DMA_Cmd(DMA1_Channel4, DISABLE); 
		DMA_ClearITPendingBit(DMA1_IT_GL4 | DMA1_IT_TC4| DMA1_IT_TE4 | DMA1_IT_HT4);
    DMA1_Channel4->CNDTR = (uint16_t)YK_Tra_Size; // ����Ҫ���͵��ֽ���Ŀ��5
    DMA_Cmd(DMA1_Channel4, ENABLE);        //��ʼDMA����
	  YK_Tx_Flag=0;	
  }
	
void USART1_DMA_Tx_Data(unsigned char *t,unsigned  char Lenth)
{
	 DMA_Cmd(DMA1_Channel4, DISABLE);  // �ر�DMAͨ��
	 DMA_ClearITPendingBit(DMA1_IT_GL4 | DMA1_IT_TC4| DMA1_IT_TE4 | DMA1_IT_HT4);//�����ڻ����Ϻ�DMA���ͳ�����ϵͳ���ȶ�����������ˣ�
	 memcpy(YK_TX,t,Lenth);
   DMA1_Channel4->CNDTR = (uint16_t)Lenth; // ����Ҫ���͵��ֽ���Ŀ��25
   DMA_Cmd(DMA1_Channel4, ENABLE);        //��ʼDMA����
	 YK_Tx_Flag=0;
	
}

void WL_EN(unsigned char EN){if(EN)GPIO_SetBits(GPIOA, GPIO_Pin_8);	  else GPIO_ResetBits(GPIOA, GPIO_Pin_8); }
void WL_M0(unsigned char I ){if(I) GPIO_SetBits(GPIOC, GPIO_Pin_11);	else GPIO_ResetBits(GPIOC, GPIO_Pin_11);}
unsigned char WL_LOCK(void ){return  GPIO_ReadInputDataBit(GPIOC ,GPIO_Pin_10  );}
unsigned char WL_AUX(void)  {return  GPIO_ReadInputDataBit(GPIOA ,GPIO_Pin_15  );}
unsigned char WL_Config(unsigned char HEAD,\
	                      unsigned char ID,\
	                      unsigned char FHSS,\
	                      unsigned char BAUD,\
	                      unsigned char W_SPED,\
	                      unsigned char CHAN,\
	                      unsigned char IO_D,\
	                      unsigned char FEC,\
	                      unsigned char P_DB)
	{	
		 WL_EN(1);
		 WL_M0(1);
		 delay_ms(500);
      unsigned char Cmd[6]={0XC0,0X01,0X0A,0X1A,0X0A,0X44};
			//unsigned char Back_Para[3]={0XC1,0XC1,0XC1};
      //[0] HEAD �̶� 0xC0 �� 0xC2����ʾ��֡����Ϊ�������� ����Ϊ 0xC0 �� C2
      //C0�������õĲ�������籣�档
      //C2�������õĲ���������籣�档
      switch(HEAD)
      	{
      	  case 0xc0:{Cmd[0]=0xc0; break;}
      	  case 0xc2:{Cmd[0]=0xc2; break;}
      	  default : {Cmd[0]=0xc0; break;}
         }
      	
      //[1] ID ��Ƶ���� ID
      //��Ĭ��Ϊ 01H��
      //��Ƶ ID ȷ����Ƶ���У�˫������һ����
      	 Cmd[1]=ID;
      	 
      //[2] FHSS nums
      //��Ƶ�ŵ�������Ĭ��Ϊ 0AH��
      //������Ƶ�ŵ���������˫������һ������Ƶ�ŵ�����Խ�࣬ģ��ͨ�ŵĿ���������
      //��Խǿ����ͨ��˫����ͬ��ʱ��Ҳ��������Ƶ�ŵ�����Խ�٣�ģ�鿹��������Խ������ͬ��ʱ�����̡�
      	 Cmd[2]=FHSS;
      	 
      //[3] SPED ���ʲ����������������ʺͿ�������
      //7��6�� ����У��λ
      //00��8N1��Ĭ�ϣ�
      //01��8O1
      //10��8E1
      //11��8N1����ͬ 00��
      //-------------------------------------------------
      //5��4��3 TTL �������ʣ�bps��
      //000�����ڲ�����Ϊ 1200
      //001�����ڲ�����Ϊ 2400
      //010�����ڲ�����Ϊ 4800
      //011�����ڲ�����Ϊ 9600��Ĭ�ϣ�
      //100�����ڲ�����Ϊ 19200
      //101�����ڲ�����Ϊ 38400
      //110�����ڲ�����Ϊ 57600
      //111�����ڲ�����Ϊ 115200
      //-------------------------------------------------
      //2, ������д 0
      //-------------------------------------------------
      //1��0 ���߿������ʣ�bps��
      //00����������Ϊ 16K
      //01����������Ϊ 32K
      //10����������Ϊ 64K��Ĭ�ϣ�
      //11����������Ϊ 128K
      //ͨ��˫������ģʽ���Բ�ͬ
      //-------------------------------------
      //ͨ��˫�������ʿ��Բ�ͬ���ڲ����ʺ���
      //�ߴ�������޹أ���Ӱ�������շ����ԡ�
      //-------------------------------------
      //-------------------------------------
      //��������Խ�ͣ�����ԽԶ������������Խ
      //ǿ������ʱ��Խ����ͨ��˫���������ߴ�
      //�����ʱ�����ͬ�� 
      //������
			Cmd[3]=0;
      switch(BAUD)
      	{
      	  case 96:  {Cmd[3]=(Cmd[3]&0xc7)|0x18;break;}
      	  case 192: {Cmd[3]=(Cmd[3]&0xc7)|0x20;break;}
      		case 115: {Cmd[3]=(Cmd[3]&0xc7)|0x38;break;}
      	  default : {Cmd[3]=(Cmd[3]&0xc7)|0x18; break;}
         }
      	
      //����
      	 switch(W_SPED)
      	{
      	  case 16:  {Cmd[3]=(Cmd[3]&0xfc)|0x00;break;}
      	  case 32:  {Cmd[3]=(Cmd[3]&0xfc)|0x01;break;}
      		case 64:  {Cmd[3]=(Cmd[3]&0xfc)|0x02;break;}
      		case 128: {Cmd[3]=(Cmd[3]&0xfc)|0x03;break;}
      	  default :{Cmd[3]=(Cmd[3]&0xfc)|0x02; break;}
         }
      	Cmd[3]=Cmd[3]&0X3B;//�������λ���ó�8N1
				 
      //4 CHAN ͨ��Ƶ�ʣ�425M + CHAN * 0.5M��
      //��Ĭ��Ϊ 0AH��
      //00H-C8H����Ӧ 425 - 450MHz
      //����Ƶ����Ϊ 1 ʱ��ģ�鲻������Ƶ��
      //��������Ƶ�ʹ̶�Ϊ��λ���õ�Ƶ�ʣ���
      //��Ƶ�ŵ��������� 1�����λ���������幤
      //�� Ƶ �� �� �� �� 425MHz + CHAN *
      //0.5MHz �� �� �� 425MHz + CHAN *
      //0.5MHz +
      //FHSS_nums*0.5MHz��
      Cmd[4]=CHAN;
       
       //				 5 OPTION 7�� ������д 0
       //-------------------------------------------------
       //6 IO ������ʽ��Ĭ�� 1��
       //1��TXD�� AUX ���������RXD ��������
       //0��TXD�� AUX ��·�����RXD ��·����
       //-------------------------------------
       //��λ����ʹ��ģ���ڲ��������衣©����
       //·��ʽ��ƽ��Ӧ������ǿ������ĳЩ���
       //�£�������Ҫ�ⲿ��������E62-T100S2 �û��ֲ� v1.22 �ɶ��ڰ��ص��ӿƼ����޹�˾
       //��Ʒ��ַ�� cdebyte.taobao.com ------------------
       //5��4 , 3 ������д 0
       //-------------------------------------------------
       //2�� FEC ����
       //0���ر� FEC
       //1���� FEC��Ĭ�ϣ�
       //-------------------------------------------------
       //1, 0 ���书�ʣ���Լֵ��
       //00�� 20dBm��Ĭ�ϣ�
       //01�� 17dBm
       //10�� 14dBm
       //11�� 11dBm
       //---------------------------------------
       //����д 0
       //---------------------------------------
       //�ر� FEC ������ʵ�ʴ������������������������������������Խ��������ʵ��Ӧ��ѡ���շ�˫��������ͬ���á�
       //�ⲿ��Դ�����ṩ 200mA ���ϵ����������������֤��Դ�Ʋ�С�� 100mV�����Ƽ�ʹ�ý�С���ʷ��ͣ����Դ����Ч�ʲ��ߡ�
      switch(IO_D)
      	{
      	  case 0:  {Cmd[5]=(Cmd[5]&0xbf)|0X00;break;}
      	  case 1:  {Cmd[5]=(Cmd[5]&0xbf)|0X40;break;}
      	  default :{Cmd[5]=(Cmd[5]&0xbf)|0X40; break;}
         }
				
		switch(FEC)
      	{
      	  case 0:  {Cmd[5]=(Cmd[5]&0xfb)|0X00;break;}
      	  case 1:  {Cmd[5]=(Cmd[5]&0xfb)|0X04;break;}
      	  default :{Cmd[5]=(Cmd[5]&0xfb)|0X04; break;}
         }                     
				                       
		 switch(P_DB)              
      	{                      
      	  case 20:{Cmd[5]=(Cmd[5]&0xfc)|0X00;break;}
      	  case 17:{Cmd[5]=(Cmd[5]&0xfc)|0X01;break;}
      		case 14:{Cmd[5]=(Cmd[5]&0xfc)|0X02;break;}
					case 11:{Cmd[5]=(Cmd[5]&0xfc)|0X03;break;}
      	  default:{Cmd[5]=(Cmd[5]&0xfc)|0X00; break;}
         }
				//���������
				 YK_RX[0]=0;YK_RX[1]=0;YK_RX[2]=0;YK_RX[3]=0;YK_RX[4]=0;YK_RX[5]=0;
				 DMA1_Channel5_IRQHandler();
				 delay_ms(100);
	       USART1_DMA_Tx_Data(Cmd,6);//������������
				 delay_ms(300);
//				 USART1_DMA_Tx_Data(Back_Para,3);
//				 delay_ms(200);
				 if(YK_RX[0]==Cmd[0] && YK_RX[1]==Cmd[1] && YK_RX[2]==Cmd[2] && YK_RX[3]==Cmd[3] && YK_RX[4]==Cmd[4] && YK_RX[5]==Cmd[5])return 1;
					 else return 0;
	}

	unsigned char WL_Get_Config(unsigned char *Par){
    //C1+C1+C1������ģʽ�£�M0=1������ģ�鴮�ڷ������HEX ��ʽ����C1 C1 C1
    //ģ��᷵�ص�ǰ�����ò��������磺C0 01 0A 1A 0A 44
		unsigned char WL_Ckeck_Cmd[3]={0xc1,0xc1,0xc1};
		WL_EN(1);
		WL_M0(1);
		delay_ms(400);
		YK_RX[0]=0;YK_RX[1]=0;YK_RX[2]=0;YK_RX[3]=0;YK_RX[4]=0;YK_RX[5]=0;
		DMA1_Channel5_IRQHandler();
		delay_ms(100);
		USART1_DMA_Tx_Data(WL_Ckeck_Cmd,3);//�������ò�����ѯ
		delay_ms(300);
		if(YK_RX[0]==0xC0){
		Par[0]=YK_RX[0];
		Par[1]=YK_RX[1];
		Par[2]=YK_RX[2];
		Par[3]=YK_RX[3];
		Par[4]=YK_RX[4];
		Par[5]=YK_RX[5];
		return 1;
		}
		else {return 0;}
	}
//------------------------------------------------------------------------------------------------------------------------------
#define USART2_TX_BUFF_SIZE  7//���ͻ�����
#define USART2_RX_BUFF_SIZE 64 //���ջ���������һ���ŵ�������  16*8
#define PC_TxBuff_Size USART2_TX_BUFF_SIZE
uint8_t PC_TX[PC_TxBuff_Size]={0x7E,0x05,0x41,0x00,0x01,0x45,0xef};//����֡������
#define PC_RxBuff_Size 8
uint8_t PC_RX[PC_RxBuff_Size]={0xff,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xee};//����֡���ݻ�����
uint8_t PC_CMD[PC_RxBuff_Size]={0xff,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xee};//��Ŵ�PC���ղ��ҽ���������������֡
uint8_t PC_Rx_Flag=1;
uint8_t PC_Tx_Flag=1;
uint8_t PC_CMD_Flag=1;

void USART2_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
/////////////////////////////////////////////////////////////////////////
   //#VO_CT
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC  , ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	//ѡ���Ӧ������
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;       
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);  
  GPIO_SetBits(GPIOC, GPIO_Pin_9);	//Ĭ�Ͽ���
	//GPIO_ResetBits(GPIOC, GPIO_Pin_9);	
	
	//VO_BUSY
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC  , ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;	//ѡ���Ӧ������
  GPIO_InitStructure.GPIO_Mode =GPIO_Mode_IPU;//GPIO_Mode_IN_FLOATING;       
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);  
	//GPIO_ResetBits(GPIOC, GPIO_Pin_10);	
	/////////////////////////////////////////////////////////////////////////////////////	
	      /*��1������GPIO��USART2������ʱ�� */
        //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        /*��2������USART2 Tx��GPIO����Ϊ���츴��ģʽ */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        /*��3������USART2 Rx��GPIO����Ϊ��������ģʽ
                 ����CPU��λ��GPIOȱʡ���Ǹ�������ģʽ���������������費�Ǳ����
                 ���ǣ��һ��ǽ�����ϱ����Ķ������ҷ�ֹ�����ط��޸���������ߵ����ò���
        */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        //��3���Ѿ����ˣ�����ⲽ���Բ���
        //GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        //
        GPIO_Init(GPIOA, &GPIO_InitStructure);
				
/////////////////////////////////////////////////////////////////////////////////////	
	      USART_InitStructure.USART_BaudRate = 9600;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        USART_Init(USART2, &USART_InitStructure);
				//��������,֡��,����,У����ж� 
        USART_ITConfig(USART2, USART_IT_IDLE , ENABLE);
				//�����ж�
        USART_ITConfig(USART2, USART_IT_ERR | USART_IT_ORE | USART_IT_NE | USART_IT_FE ,ENABLE);
        USART_Cmd(USART2, ENABLE);
        // CPU��Сȱ�ݣ��������úã����ֱ��Send�����1���ֽڷ��Ͳ���ȥ
        //�����������1���ֽ��޷���ȷ���ͳ�ȥ������ //
        //USART_ClearFlag(USART2, USART_FLAG_TC); //�巢����Ǳ�־��Transmission Complete flag //
				USART_ClearFlag(USART2, USART_FLAG_TC|USART_FLAG_RXNE|USART_FLAG_IDLE|USART_FLAG_ORE|USART_FLAG_NE|USART_FLAG_FE |USART_FLAG_PE ); 
					
	/////////////////////////////////////////////////////////////////////////////////////					
       	//DMA1 Channel7 (triggered by USART2 Tx event) Config */
       DMA_DeInit(DMA1_Channel7);  
       DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART2->DR);
       DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)PC_TX;
       DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
       DMA_InitStructure.DMA_BufferSize =USART2_TX_BUFF_SIZE;
       DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
       DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
       DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
       DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
       DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
       DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
       DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
       DMA_Init(DMA1_Channel7, &DMA_InitStructure);
       DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);
       DMA_ITConfig(DMA1_Channel7, DMA_IT_TE, ENABLE);
			 DMA_ClearITPendingBit(DMA1_IT_GL7 | DMA1_IT_TC7| DMA1_IT_TE7 | DMA1_IT_HT7);
      // DMA_ClearFlag(DMA1_FLAG_GL7| DMA1_FLAG_TC7 | DMA1_FLAG_TE7 | DMA1_FLAG_HT7);
       /* Enable USART1 DMA TX request */
       USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
       DMA_Cmd(DMA1_Channel7, DISABLE);			
								
	/////////////////////////////////////////////////////////////////////////////////////
       /* DMA1 Channel6 (triggered by USART2 Rx event) Config */
       DMA_DeInit(DMA1_Channel6);  
       DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART2->DR);
       DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)PC_RX;
       DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
       DMA_InitStructure.DMA_BufferSize = USART2_RX_BUFF_SIZE;
       DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
       DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
       DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
       DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
       DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
       DMA_InitStructure.DMA_Priority = DMA_Priority_High;
       DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
       DMA_Init(DMA1_Channel6, &DMA_InitStructure);
       DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);
       DMA_ITConfig(DMA1_Channel6, DMA_IT_TE, ENABLE);
			 DMA_ClearITPendingBit(DMA1_IT_GL6 | DMA1_IT_TC6| DMA1_IT_TE6 | DMA1_IT_HT6);
       //DMA_ClearFlag(DMA1_FLAG_GL6 | DMA1_FLAG_TC6 | DMA1_FLAG_TE6 | DMA1_FLAG_HT6);
       //Enable USART2 DMA RX request //
       USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
       DMA_Cmd(DMA1_Channel6, ENABLE);
			 					
	/////////////////////////////////////////////////////////////////////////////////////
  //Configure one bit for preemption priority //
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);  
  // Enable the USART2 Interrup--���տ����ж�
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); 
  //Enable DMA Channel7 Interrupt --TX-DMA��������ж�
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  //Enable DMA Channel6 Interrupt --RX--DMA��������ж�
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); 
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
} 
void USART2_IRQHandler(void)
{
   u16 DATA_LEN;
   u16 i;
	 PC_Rx_Flag=0;//������ݽ��ղ������ڴ������췢������ղ�������
	 //DMA_Cmd(DMA1_Channel6, DISABLE);//�ر�DMA,��ֹ�������������
   if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)//���Ϊ���������ж�
    { 
			     // LED4(2);
             DATA_LEN=USART2_RX_BUFF_SIZE-DMA_GetCurrDataCounter(DMA1_Channel6); 
             if(DATA_LEN >=PC_RxBuff_Size)//����յ����ݲ��Ҳ����˿����ж�
             { 
               DMA_Cmd(DMA1_Channel6, DISABLE);//�ر�DMA,��ֹ�������������										
				    if(PC_RX[0]==0xFF && PC_RX[7]==0xEE)
				    {
					    memcpy(PC_CMD,PC_RX,PC_RxBuff_Size);//��������
					    PC_CMD_Flag=1;//�������֡
					   // LED4(2);
				    }
						//DMA_Cmd(DMA1_Channel6, DISABLE);//�ر�DMA,��ֹ�������������
			          DMA_ClearFlag(DMA1_FLAG_GL6 | DMA1_FLAG_TC6 | DMA1_FLAG_TE6 | DMA1_FLAG_HT6);//���־
                 DMA1_Channel6->CNDTR = USART2_RX_BUFF_SIZE;//��װ��
                 DMA_Cmd(DMA1_Channel6, ENABLE);//������,�ؿ�DMA
            }
			     else{ }
			     //��SR���DR���Idle
            i=USART2->SR;
            i=USART2->DR;
		        i=i;
     }
		   //��USART���ļ�905��
		   USART_ClearFlag(USART2, USART_FLAG_IDLE|USART_FLAG_ORE|USART_FLAG_NE|USART_FLAG_FE |USART_FLAG_PE );//������д����־
       USART_ClearITPendingBit(USART2, USART_IT_IDLE | USART_IT_PE | USART_IT_FE | USART_IT_NE|USART_IT_ERR|USART_IT_ORE);////������д����ж�λ
		   
		   //����DMA
//		   DMA_ClearFlag(DMA1_FLAG_GL6 | DMA1_FLAG_TC6 | DMA1_FLAG_TE6 | DMA1_FLAG_HT6);//���־
//       DMA1_Channel6->CNDTR = USART2_RX_BUFF_SIZE;//��װ��
//       DMA_Cmd(DMA1_Channel6, ENABLE);//������,�ؿ�DMA
		   PC_Rx_Flag=1;//�����жϴ�����ִ�����
}     

//DMA1_Channel5���ڽ����жϷ�����
void DMA1_Channel6_IRQHandler(void)
{
  DMA_ClearITPendingBit(DMA1_IT_GL6 | DMA1_IT_TC6| DMA1_IT_TE6 | DMA1_IT_HT6);
 // DMA_ClearFlag(DMA1_FLAG_GL6 | DMA1_FLAG_TC6| DMA1_FLAG_TE6 | DMA1_FLAG_HT6);
  DMA_Cmd(DMA1_Channel6, DISABLE);//�ر�DMA,��ֹ�������������
  DMA1_Channel6->CNDTR = USART2_RX_BUFF_SIZE;//��װ��
  DMA_Cmd(DMA1_Channel6, ENABLE);//������,�ؿ�DMA
}

//DMA1_Channel4���ڷ����жϷ�����
//USART2ʹ��DMA�������жϷ������
void DMA1_Channel7_IRQHandler(void)
{
	if(DMA_GetFlagStatus(DMA1_FLAG_TC7)){PC_Tx_Flag=1;}//��������ж�LED3(2);
  DMA_ClearITPendingBit(DMA1_IT_GL7 | DMA1_IT_TC7| DMA1_IT_TE7 | DMA1_IT_HT7);
  //DMA_ClearFlag(DMA1_FLAG_GL7 | DMA1_FLAG_TC7 | DMA1_FLAG_TE7 | DMA1_FLAG_HT7);
  DMA_Cmd(DMA1_Channel7, DISABLE);//������ϲ�������ɴ����жϺ�ر�DMA
 // PC_Tx_Flag=1;//��DMA�������
}

void USART2_DMA_Tx_Data(u8 COUNT)
{
	 PC_Tx_Flag=0;
	 DMA_ClearITPendingBit(DMA1_IT_GL7 | DMA1_IT_TC7| DMA1_IT_TE7 | DMA1_IT_HT7);//�����ڻ����Ϻ�DMA���ͳ�����ϵͳ���ȶ�����������ˣ�
  // DMA_ClearFlag(DMA1_FLAG_GL7 | DMA1_FLAG_TC7 | DMA1_FLAG_TE7 | DMA1_FLAG_HT7);//��ֹ��һ�η��ͳ����º�߽��ų���
	 DMA_Cmd(DMA1_Channel7, DISABLE);  // �ر�DMAͨ��
   DMA1_Channel7->CNDTR = (uint16_t)COUNT; // ����Ҫ���͵��ֽ���Ŀ��25
	// while(PC_Rx_Flag==0){};//�ȴ������жϴ�����ִ�����//����������ջ���---PC������ȫ˫���շ�������Ҫ�������������ջ��⴦��
   DMA_Cmd(DMA1_Channel7, ENABLE);        //��ʼDMA����
}


u8 VO_BUSY(void){return  (GPIO_ReadInputDataBit(GPIOC ,GPIO_Pin_13  ));}
//===============================USART3_DMA_�˿�==================================================================================================================================
//=======================================================================================================
#define USART3_TX_BUFF_SIZE  64 //���ͻ�����
#define USART3_RX_BUFF_SIZE  128 //��������һ���ŵ�������  16*8
#define EX_TxBuff_Size USART3_TX_BUFF_SIZE
uint8_t EX_TX[EX_TxBuff_Size];//����֡������
#define EX_RxBuff_Size USART3_RX_BUFF_SIZE
uint8_t EX_RX[EX_RxBuff_Size];//����֡���ݻ�����
//uint8_t EX_MSG[512];//��Ŵ�EX���ղ��ҽ���������������֡
int EX_Rx_Count=0;
uint8_t EX_Rx_Flag=0;
uint8_t EX_Tx_Flag=1;

 void  USART3_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
//--------------------------------------------------------------------------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
		// USART3 ʹ��IO�˿�����    
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //�����������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);      
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//��������
  GPIO_Init(GPIOB, &GPIO_InitStructure);   //��ʼ��GPIOB  
	//USART3 ����ģʽ���� 
	USART_InitStructure.USART_BaudRate = 115200;	//���������ã�115200
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//����λ�����ã�8λ
	USART_InitStructure.USART_StopBits = USART_StopBits_1; 	//ֹͣλ���ã�1λ
	USART_InitStructure.USART_Parity = USART_Parity_No ;  //�Ƿ���żУ�飺��
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//Ӳ��������ģʽ���ã�û��ʹ��
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//�����뷢�Ͷ�ʹ��
	USART_Init(USART3, &USART_InitStructure);  //��ʼ��USART3
   
	//��������,֡��,����,У����ж� 
     USART_ITConfig(USART3, USART_IT_IDLE , ENABLE);//���������ж�һ֡���ݽ������
	   USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//ʹ�ý����ж�
	//�����ж�
     USART_ITConfig(USART3, USART_IT_ERR | USART_IT_ORE | USART_IT_NE | USART_IT_FE ,ENABLE);
     USART_Cmd(USART3, ENABLE);
     // CPU��Сȱ�ݣ��������úã����ֱ��Send�����1���ֽڷ��Ͳ���ȥ
     //�����������1���ֽ��޷���ȷ���ͳ�ȥ������ //
     //USART_ClearFlag(USART3, USART_FLAG_TC); //�巢����Ǳ�־��Transmission Complete flag //
			USART_ClearFlag(USART3, USART_FLAG_TC|USART_FLAG_RXNE|USART_FLAG_TXE|USART_FLAG_IDLE|USART_FLAG_ORE|USART_FLAG_NE|USART_FLAG_FE |USART_FLAG_PE ); 
					
	/////////////////////////////////////////////////////////////////////////////////////					
       	//DMA1 Channel2 (triggered by USART3 Tx event) Config */
       DMA_DeInit(DMA1_Channel2);  
       DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART3->DR);
       DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)EX_TX;
       DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
       DMA_InitStructure.DMA_BufferSize =USART3_TX_BUFF_SIZE;
       DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
       DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
       DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
       DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
       DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
       DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
       DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
       DMA_Init(DMA1_Channel2, &DMA_InitStructure);
       DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);
       DMA_ITConfig(DMA1_Channel2, DMA_IT_TE, ENABLE);
			 DMA_ClearITPendingBit(DMA1_IT_GL2 | DMA1_IT_TC2| DMA1_IT_TE2 | DMA1_IT_HT2);
       /* Enable USART1 DMA TX request */
       USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
       DMA_Cmd(DMA1_Channel7, DISABLE);			
								
	/////////////////////////////////////////////////////////////////////////////////////
       /* DMA1 Channel3 (triggered by USART3 Rx event) Config */
       DMA_DeInit(DMA1_Channel3);  
//       DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART3->DR);
//       DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)EX_RX;
//       DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
//       DMA_InitStructure.DMA_BufferSize = USART3_RX_BUFF_SIZE;
//       DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//       DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//       DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//       DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//       DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
//       DMA_InitStructure.DMA_Priority = DMA_Priority_High;
//       DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//       DMA_Init(DMA1_Channel3, &DMA_InitStructure);
//       DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
//       DMA_ITConfig(DMA1_Channel3, DMA_IT_TE, ENABLE);
//			 DMA_ClearITPendingBit(DMA1_IT_GL3 | DMA1_IT_TC3| DMA1_IT_TE3 | DMA1_IT_HT3);
//       // Enable USART2 DMA RX request //
//       USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);
//       DMA_Cmd(DMA1_Channel3, ENABLE);
			 					
	/////////////////////////////////////////////////////////////////////////////////////
  //Configure one bit for preemption priority //
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
  // Enable the USART3 Interrup--���տ����ж�
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
  //Enable DMA1 Channel2 Interrupt --TX-DMA��������ж�
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  //Enable DMA1 Channel3 Interrupt --RX--DMA��������ж�
//  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
}

void USART3_DMA_Tx_Data(void )
{
	 DMA_Cmd(DMA1_Channel2, DISABLE);  // �ر�DMAͨ��
   DMA1_Channel2->CNDTR = USART3_TX_BUFF_SIZE; // ����Ҫ���͵��ֽ���Ŀ
   DMA_Cmd(DMA1_Channel2, ENABLE);        //��ʼDMA����
}

//UART3_DMA TX CH2��������ж�
void DMA1_Channel2_IRQHandler(void)
{
  DMA_ClearITPendingBit(DMA1_IT_GL2 | DMA1_IT_TC2| DMA1_IT_TE2 | DMA1_IT_HT2);
  DMA_Cmd(DMA1_Channel2, DISABLE);//������ϲ�������ɴ����жϺ�ر�DMA
}

void USART3_IRQHandler(void)
{
   u16 i;
	 EX_Rx_Flag=0;//������ݽ��ղ������ڴ������췢������ղ�������
   if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)//���Ϊ���������ж�
    { 
			if(EX_Rx_Count<=126)EX_Rx_Count++;
			else EX_Rx_Count=0;
			EX_RX[EX_Rx_Count]=USART_ReceiveData(USART3);
			USART_ClearFlag(USART3,USART_FLAG_RXNE);
     }	
   if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)//���Ϊ���������ж�
    { 
			 EX_Rx_Flag=1;
			 //��SR���DR���Idle
       i=USART3->SR;
       i=USART3->DR;
		   i=i;
     }
		   //��USART���ļ�905��
		   USART_ClearFlag(USART3, USART_FLAG_IDLE|USART_FLAG_ORE|USART_FLAG_NE|USART_FLAG_FE |USART_FLAG_PE );//������д����־
       USART_ClearITPendingBit(USART3, USART_IT_IDLE | USART_IT_PE | USART_IT_FE | USART_IT_NE|USART_IT_ERR|USART_IT_ORE);////������д����ж�λ
}     


//======================================================================================================
//===============================================================================================================================================
int fputc(int ch, FILE *f)
{
// ��Printf���ݷ������� //
	if (f== COM1 )
		{ 
     USART_SendData(USART1, (unsigned char) ch);
     while (!(USART1->SR & USART_FLAG_TXE));
     return (ch);
		}

	if(f== COM2)
		{ 
     USART_SendData(USART2, (unsigned char) ch);
     while (!(USART2->SR & USART_FLAG_TXE));
     return (ch);
		}
   
	 	if (f== COM3 )
		{ 
     USART_SendData(USART3, (unsigned char) ch);
     while (!(USART3->SR & USART_FLAG_TXE));
     return (ch);
		}
		 return  (ch);
   }

//================================================================================================

/*
////------------------------------------------����2�ش�����----------------------------------------------------------------------
void USART2_Initialise( u32 bound )
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    
    // Enable the USART2 Pins Software Remapping //
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); 
    
    
    // Configure USART2 Rx (PA.03) as input floating //
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure USART2 Tx (PA.02) as alternate function push-pull //
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Enable the USART2 Interrupt //
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);    
    
    USART_InitStructure.USART_BaudRate = bound;                
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;    
    USART_InitStructure.USART_Parity = USART_Parity_No;        
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	
    
    USART_Init(USART2, &USART_InitStructure);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    //USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
    //Enable USART2 //
    USART_Cmd(USART2, ENABLE);
}

void USART2_IRQHandler(void)  
{  
     if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET)  
      {       
             // USART_SendData(USART2, USART_ReceiveData(USART2));             
      }
      
}
*/

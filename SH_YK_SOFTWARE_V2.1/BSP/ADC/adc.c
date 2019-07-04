 #include "adc.h"
 #include "delay.h"
	   
		   
#define ADC1_DR_Address    ((u32)0x4001244C)
__IO uint16_t ADC_ConvertedValue[2]={0,0}; 

/*���ò���ͨ���˿� ʹ��GPIOʱ��	  ����ADC����PA0�˿��ź�*/
 void ADC1_GPIO_Config(void)
{ 
	GPIO_InitTypeDef GPIO_InitStructure;    
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		    //GPIO����Ϊģ������
  GPIO_Init(GPIOB, &GPIO_InitStructure); 	
}


/*����ADC1�Ĺ���ģʽΪMDAģʽ  */
 void ADC1_Mode_Config(void)
{
  DMA_InitTypeDef DMA_InitStructure;
  ADC_InitTypeDef ADC_InitStructure;	
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE); //ʹ��MDA1ʱ��
	/* DMA channel1 configuration */
  DMA_DeInit(DMA1_Channel1);  //ָ��DMAͨ��
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;//����DMA�����ַ
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&ADC_ConvertedValue;	//����DMA�ڴ��ַ��ADCת�����ֱ�ӷ���õ�ַ
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; //����Ϊ����Ϊ���ݴ������Դ
  DMA_InitStructure.DMA_BufferSize =2;	//DMA����������Ϊ2��
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc =DMA_MemoryInc_Enable;  //�ڴ��ַ�̶�
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Low ;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  
  /* Enable DMA channel1 */
  DMA_Cmd(DMA1_Channel1, ENABLE);  //ʹ��DMAͨ��

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //��Ƶ����6ʱ��Ϊ72M/6=12MHz
  //ADC_DeInit(ADC1);  //������ ADC1 ��ȫ���Ĵ�������Ϊȱʡֵ
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	//ʹ��ADC1ʱ��
     
  /* ADC1 configuration */
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; //ʹ�ö���ģʽ��ɨ��ģʽ
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; //������Ӵ�����
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; //ʹ�������Ҷ���
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 2;  // ֻ��2��ת��ͨ��-˳����й���ת����ADCͨ������Ŀ
  ADC_Init(ADC1, &ADC_InitStructure);

  /* ADC1 regular channel9 configuration */ 
	ADC_RegularChannelConfig(ADC1, ADC_Channel_9,   1, ADC_SampleTime_55Cycles5);//ͨ��1��������55.5��ʱ������
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16,  2, ADC_SampleTime_55Cycles5);//ͨ��1��������55.5��ʱ������
  /* Enable ADC1 DMA */
  ADC_DMACmd(ADC1, ENABLE);	 //ʹ��ADC��DMA
  ADC_TempSensorVrefintCmd(ENABLE); //�����ڲ��¶ȴ�����
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE); //ʹ��ADC1

  /* Enable ADC1 reset calibaration register */   
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC1 calibaration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));
     
  /* Start ADC1 Software Conversion */ 
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);  //��ʼת��
}

/*��ʼ��ADC1 */
void ADC1_Init(void)
{
	ADC1_GPIO_Config();
	ADC1_Mode_Config();
}

u16 Get_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=ADC_ConvertedValue[0];
		//delay_ms(5);
	}
	return temp_val/times;
} 	 


int Get_Temp(void)
{				 
	u16 temp_val=0;
	u8 t;
	float temperate;   
	for(t=0;t<20;t++)//��20��,ȡƽ��ֵ
	{
		temp_val+=ADC_ConvertedValue[1];
		//delay_ms(1);
	}
	temp_val/=20;
	temperate=(float)temp_val*(3.3/4096);//�õ��¶ȴ������ĵ�ѹֵ
	temperate=(1.43-temperate)/0.0043+25;//�������ǰ�¶�ֵ	 
	temperate*=10;//����ʮ��,ʹ��С�����һλ
	return (int)temperate;	 

}

























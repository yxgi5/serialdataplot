// TMS320F2808	 Project.
/****************************************************************************/
/*									 										*/
/*	@file 		: *_Main.c									 				*/
/*	@Copyright	: DavisBeans.(c) 2015 MULTIBEANS_ORG All rights reserved.	*/
/*  @Revision	: Ver 1.0.													*/
/*  @Data		: 2016.04.27 Realse.										*/
/*  @Belong		: PROJECT.							 						*/
/*																		    */
/*	**code : (GBK/GB2312) in win_10_x64. CCS v6.1 platform.			    	*/
/****************************************************************************/
/*	@Attention:																*/
/* 	---------------------------------------------------------------------	*/
/*	|    Data    |	Behavior |     Offer	 |    		Content			|	*/
/*  | 2018.11.20 |   creat   | DavisBeans(M) |   creat the document.	|	*/
/*  ---------------------------------------------------------------------	*/
/* 															  MULTIBEANS.	*/
/****************************************************************************/

//###########################################################################
// $TI Release: F281x C/C++ Header Files and Peripheral Examples V130 $
// $Release Date: May  8, 2015 $
// $Copyright: Copyright (C) 2009-2015 Texas Instruments Incorporated -
//             http://www.ti.com/ ALL RIGHTS RESERVED $
//###########################################################################
#include "DSP280x_Device.h"
#include "DSP280x_Examples.h"
#include "IQmathLib.h"
#include "AD9833.h"
#include "string.h"
#include "stdlib.h"

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */


#define POST_SHIFT   0  // Shift results after the entire sample table is full
#define INLINE_SHIFT 1  // Shift results as the data is taken from the results regsiter
#define NO_SHIFT     0  // Do not shift the results

// ADC start parameters
#define ADC_MODCLK 0x4   // HSPCLK = SYSCLKOUT/2*ADC_MODCLK2 = 100/(2*4)      = 12.5 MHz
//    for 60 MHz devices:    HSPCLK =  60/(2*4)      =  7.5 MHz
#define ADC_CKPS   0x0   // ADC module clock = HSPCLK/1      = 12.5MHz/(1)    = 12.5 MHz
//    for 60 MHz devices: ADC module clk = 7.5MHz/(1)=  7.5 MHz
#define ADC_SHCLK  0x1   // S/H width in ADC module periods                   = 2 ADC cycle
#define AVG        1000  // Average sample limit
#define ZOFFSET    0x00  // Average Zero offset
#define BUF_SIZE   1000  // Sample buffer size
#define DELAY_US_ADC	50000

////////////////////////////////////////////////////////////////////////
#define STRINGDATA_SIZE  50      //
////////////////////////////////////////////////////////////////////////
#define			RXA_MAX_COUNT			10

Uint16 rxaInterruptCount = 0;
Uint16 AdcInterruptCount = 0;
char rxaBuffer[RXA_MAX_COUNT]={0};
Uint16 SampleTable[BUF_SIZE];
char charSampleData[];

interrupt void SCIA_RX_ISR( void );
interrupt void ADC_Interrupt( void );

void ADC_Init();
void EVA_init();
void SCIA_INIT();
void SendDatasByScia();
void intTochar();
void GPIO_INIT();

Uint16 array_index = 0;
float adc = 0;
char str[10]="";
Uint16 adc_i;
char stringDataSend[ STRINGDATA_SIZE ]=  "###,";//����һ���ַ��������ڴ洢ƴ������
int breakpoint=0;

void main( void )
{


	InitSysCtrl();

	EALLOW;
	SysCtrlRegs.HISPCP.all = ADC_MODCLK; // HSPCLK = SYSCLKOUT/ADC_MODCLK
	EDIS;

	InitCpuTimers();
	InitPieCtrl();
	IER = 0x0000;
	IFR = 0x0000;
	InitPieVectTable();
	InitScia();
	InitAdc();         // For this example, init the ADC

	ADC_Init();
	DELAY_US(1000);
	SCIA_INIT();
	DELAY_US(1000);

	// Interrupts that are used in this example are re-mapped to
	// ISR functions found within this file.
	PieCtrlRegs.PIEIER9.bit.INTx1 = 1;
	PieCtrlRegs.PIEIER9.bit.INTx3 = 1;

	IER |= M_INT9;
	IER |= M_INT1;
	EINT;
	ERTM;

	// Clear SampleTable
	for (adc_i=0; adc_i<BUF_SIZE; adc_i++)
	{
		SampleTable[adc_i] = 0;
	}
	// Start SEQ1
	AdcRegs.ADCTRL2.all = 0x2000;

	//AD9833��ʼ��
	//��һ��������Ƶ�ʣ���λΪHz || �ڶ�����������λ��Ĭ��Ϊ0 || �����������ǲ���
	API_AD9833_VOID_GenerateWaveforms( 50, 0 , Sin_Wave);
	//ADC��ʼ��

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//������
	//	int data[] = {12345,1234,862};
	//	for (i = 0; i < 3; ++i) {
	//		intTochar( data[i], str );
	//		strcat( strsend, str);
	//		strcat( strsend, ",");
	//
	//		memset(str,0,10);//���str����Ԫ��
	//
	//	}
	//	strcat( strsend, "@@@");
	//	SendDatasByScia( strsend );
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	while(1) {

		breakpoint = 0;

		memset(stringDataSend,0,STRINGDATA_SIZE);//���stringDataSend����Ԫ��
		strcat( stringDataSend, "###,");//Ϊ����������׼��

		for (adc_i=0;adc_i<(BUF_SIZE); adc_i++)
		{
			//#################������ʱ######################
			DELAY_US( DELAY_US_ADC );//�ӳٵ�λΪus��   �ӳ�1ms  ����Ƶ��Ϊ1KHz
			//#################������ʱ######################
			//���� 8000��ֵ�£�ͨ��SCI���Ͳ�����
			// Wait for INT1
			while (AdcRegs.ADCST.bit.INT_SEQ1== 0){}
			AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1;

			SampleTable[ adc_i ]= ( (AdcRegs.ADCRESULT0)>>4);
			intTochar( SampleTable[adc_i], str );//���ɼ�����������ת�����ַ�����
			strcat( stringDataSend, str);//��ת��������������strsend�������һ��Ԫ�غ���
			strcat( stringDataSend, ",");//���Ӷ���(Ӣ������)�ָ���
 			memset(str,0,10);//���str����Ԫ�أ���ֹ�����´�ת��������
			//########################���ͺ���############################
			if ((adc_i+1) % 5 == 0) {
				//ת��Ϊchar����
				strcat( stringDataSend, "@@@\0");
				//���͵�strsend���ݸ�ʽΪ��   ###,data1,data2,data3,...,dataX,@@@    ����data�ĸ���ȡ���ڳ����е� BUF_SIZE
				SendDatasByScia( stringDataSend );
				memset(stringDataSend,0,STRINGDATA_SIZE);//���stringDataSend����Ԫ��
				strcat( stringDataSend, "###,");//Ϊ�ڶ��ν���������׼��
			}//###########ת������������ʾ###########

			//########################���ͺ���############################
			//			SampleTable[array_index++]= ( (AdcRegs.ADCRESULT1)>>4);
			//			SampleTable[array_index++]= ( (AdcRegs.ADCRESULT2)>>4);
			//			SampleTable[array_index++]= ( (AdcRegs.ADCRESULT3)>>4);
			//			SampleTable[array_index++]= ( (AdcRegs.ADCRESULT4)>>4);
			//			SampleTable[array_index++]= ( (AdcRegs.ADCRESULT5)>>4);
			//			SampleTable[array_index++]= ( (AdcRegs.ADCRESULT6)>>4);
			//			SampleTable[array_index++]= ( (AdcRegs.ADCRESULT7)>>4);
			while (AdcRegs.ADCST.bit.INT_SEQ1== 0){}
			AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1;
		}//###########forѭ����������������־###########
	}//###########whlieѭ��������־###########
}//###########main����������־###########


void SendDatasByScia( unsigned char *_ch )
{
	Uint16 i = 0;
//	Uint16 j = 0;
//	j = strlen(_ch);
//	for(i = 0; i <=j; i++) {
	while(*(_ch+i) != '\0'){
		while(SciaRegs.SCICTL2.bit.TXRDY==0);//״̬���ģʽ
		while(SciaRegs.SCICTL2.bit.TXEMPTY==0);//״̬��⣬�ȴ����ͱ�ʶΪ��
		SciaRegs.SCITXBUF = *(_ch+i)&0xFF;
		while(SciaRegs.SCICTL2.bit.TXEMPTY==0);//ȷ�����һ�����ݷ��ͳ�ȥ
		i++;
	}
//	i = 0;
}



interrupt void ADC_Interrupt( void )
{

	adc = ((float)AdcRegs.ADCRESULT0) * 3.0 /65520.0;
	while( AdcInterruptCount ++ == 1024 )
		AdcInterruptCount = 0;

	PieCtrlRegs.PIEACK.all = 0x0001;//��ӦPIEͬ���ж�
	AdcRegs.ADCTRL2.bit.RST_SEQ1 = 1;//��λ���з�����SEQ1
	AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1;//���AD�жϱ�־λ

	EINT;											//ʹ��ȫ���ж�
}


//ADC��ʼ��
void ADC_Init()
{
	AdcRegs.ADCTRL1.bit.ACQ_PS = ADC_SHCLK;  // Sequential mode: Sample rate   = 1/[(2+ACQ_PS)*ADC clock in ns]
	//                     = 1/(3*80ns) =4.17MHz (for 100 MHz devices or
	//                     = 1/(3*133ns) = 2.51 MHz (for 60 MHz devices)
	// If Simultaneous mode enabled: Sample rate = 1/[(3+ACQ_PS)*ADC clock in ns]
	AdcRegs.ADCTRL3.bit.ADCCLKPS = ADC_CKPS;
	AdcRegs.ADCTRL1.bit.SEQ_CASC = 1;        // 1  Cascaded mode
	AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 0x0;
	AdcRegs.ADCTRL1.bit.CONT_RUN = 1;       // Setup continuous run

	AdcRegs.ADCTRL1.bit.SEQ_OVRD = 1;       // Enable Sequencer override feature
	AdcRegs.ADCCHSELSEQ1.all = 0x0;         // Initialize all ADC channel selects to A0
	AdcRegs.ADCCHSELSEQ2.all = 0x0;
	AdcRegs.ADCCHSELSEQ3.all = 0x0;
	AdcRegs.ADCCHSELSEQ4.all = 0x0;
	AdcRegs.ADCMAXCONV.bit.MAX_CONV1 = 0x0;  // convert and store in 8 results registers

}




interrupt void SCIA_RX_ISR( void )
{

	rxaBuffer[ rxaInterruptCount ] = SciaRegs.SCIRXBUF.all;

	rxaInterruptCount ++;

	if( rxaInterruptCount == RXA_MAX_COUNT  ) {
		rxaInterruptCount = 0;						// over the range.
	}

	PieCtrlRegs.PIEACK.all = 0x0101;
	EINT;
}




//=============================================================
// FUNCTION:    SCIA_INIT
// DESCRIPTION: Configures SCIA
//
// PARAMETERS:  SciaRegs
// RETURN:      N/A
//=============================================================
void SCIA_INIT()
{
	SciaRegs.SCICCR.bit.STOPBITS = 0;//1λ����λ
	SciaRegs.SCICCR.bit.PARITYENA = 0;//��ֹ��żУ�鹦��
	SciaRegs.SCICCR.bit.LOOPBKENA = 0;//��ֹ���Ͳ��Թ���
	SciaRegs.SCICCR.bit.ADDRIDLE_MODE = 0;//������ģʽЭ��
	SciaRegs.SCICCR.bit.SCICHAR = 7;//8λ����λ

	SciaRegs.SCICTL1.bit.TXENA = 1;//SCIA����ʹ��
	SciaRegs.SCICTL1.bit.RXENA = 1;//SCIA����ʹ��

	SciaRegs.SCIHBAUD    =0x0001;  // 9600 baud @LSPCLK = 20MHz.
	SciaRegs.SCILBAUD    =0x0044;

	SciaRegs.SCICTL2.bit.RXBKINTENA = 1;//SCIA�����ж�ʹ��
	SciaRegs.SCICTL2.bit.TXINTENA = 1;	 //SCIA�����ж�ʹ��

	SciaRegs.SCICTL1.bit.SWRESET = 1;//��λ�źţ�����SCI
}


//=============================================================
// FUNCTION:    GPIO_INIT
// DESCRIPTION: Configures all GPIO
//
// PARAMETERS:  GpioCtrlRegs
// RETURN:      N/A
//=============================================================
void GPIO_INIT( volatile struct GPIO_MUX_REGS *REG )
{
	EALLOW;

	////////////////////////////////////////////////////////////////////
	/////////////////////     GPIO CONFIGURE  //////////////////////////
	////////////////////////////////////////////////////////////////////
	/* Enable internal pull-up for the selected pins */
	// Pull-ups can be enabled or disabled disabled by the user.
	// This will enable the pullups for the specified pins.

	REG->GPFMUX.bit.SCIRXDA_GPIOF5 = 1;//����Ϊ�������ţ�����SCI���ܣ���ͬ
	REG->GPFMUX.bit.SCITXDA_GPIOF4 = 1;

	//    REG->GPGMUX.bit.SCIRXDB_GPIOG5 = 1;
	//    REG->GPGMUX.bit.SCITXDB_GPIOG4 = 1;


	EDIS;

}

void intTochar( int _data, char *_str)
{
	int i;
	int scaleData=0;
	int intData = _data;
	int intDatatmp = _data;
	int tmpData=0;
	char str[10]=" ";
	char strlong[10]="0";
	//�������ݳ���
	for (i = 0; i < 10; ++i) {
		if ( intDatatmp != 0 ) {
			intDatatmp = intDatatmp / 10;
		} else {
			scaleData = i;break;
		}
	}
	//��ʼ��int��ת��Ϊchar������
	for (i = 0; i < scaleData; ++i) {
		tmpData = intData % 10;
		intData = intData / 10;
		switch (tmpData) {
		case 0:str[ i ] = '0';break;
		case 1:str[ i ] = '1';break;
		case 2:str[ i ] = '2';break;
		case 3:str[ i ] = '3';break;
		case 4:str[ i ] = '4';break;
		case 5:str[ i ] = '5';break;
		case 6:str[ i ] = '6';break;
		case 7:str[ i ] = '7';break;
		case 8:str[ i ] = '8';break;
		case 9:str[ i ] = '9';break;
		default:	break;
		}
	}
	if(scaleData == 0)
	{
		_str[0] = '0';
	}else{
		strcat(strlong,str);
		for (i = 0; i < scaleData; ++i) {
			_str[i] = strlong[ scaleData - i ];
		}
	}
}

#include <stdio.h>
#include <string.h>
#include "NUC1xx.h"
#include "LCD_Driver.h"
#include "DrvGPIO.h"
#include "DrvSYS.h"
#include "DrvADC.h"
#include "Driver_PWM_Servo.h"
#include "Driver\DrvUART.h"

#define HITIME_MIN 1
#define HITIME_MAX 180
#define Servo 0

#define PERIODIC 1	 
#define TOGGLE 2	
#define CONTINUOUS 3

uint32_t seconds = 0;

uint16_t LDR, moisture;
uint8_t Relay;

char Packet_Sent[30];
char LDR_Text[16];
char Moisture_Text[16];
char Text_Received[16];
int f_on=1;

volatile uint8_t comRbuf[9];
volatile uint8_t comRbytes = 0;

void UART_INT_HANDLE(void)
{
	while (UART0->ISR.RDA_IF == 1)
	{
		comRbuf[comRbytes] = UART0->DATA;
		comRbytes++;
		if (comRbytes == 3)
		{
			sprintf(Text_Received, "%s", comRbuf);
			print_lcd(2, Text_Received);
			if (!(strcmp("WTR", Text_Received)))
			{
				DrvGPIO_SetBit(E_GPC, 0);
				f_on=0;
			}
			else if (!(strcmp("OFF", Text_Received)))
			{
				f_on=1;
				DrvGPIO_ClrBit(E_GPC, 0);
			}
			comRbytes = 0;
		}
	}
}

void InitTIMER1(void)
{
	/* Step 1. Enable and Select Timer clock source */
	SYSCLK->CLKSEL1.TMR1_S = 0; // Select 12Mhz for Timer1 clock source
	SYSCLK->APBCLK.TMR1_EN = 1; // Enable Timer1 clock source

	/* Step 2. Select Operation mode */
	TIMER1->TCSR.MODE = 1; // Select periodic mode for operation mode

	/* Step 3. Select Time out period = (Period of timer clock input) * (8-bit Prescale + 1) * (24-bit TCMP)*/
	TIMER1->TCSR.PRESCALE = 255; // Set Prescale [0~255]
	TIMER1->TCMPR = 46875;		 // Set TCMPR [0~16777215]
								 // (1/12000000)*(255+1)*46875 = 1 sec / 1 Hz

	/* Step 4. Enable interrupt */
	TIMER1->TCSR.IE = 1;
	TIMER1->TISR.TIF = 1;	   // Write 1 to clear for safty
	NVIC_EnableIRQ(TMR1_IRQn); // Enable Timer1 Interrupt

	/* Step 5. Enable Timer module */
	TIMER1->TCSR.CRST = 1; // Reset up counter
	TIMER1->TCSR.CEN = 1;  // Enable Timer1
}

void TMR1_IRQHandler(void) // Timer0 interrupt service routine
{
	seconds++;

	if (seconds == 1)
	{
		seconds = 0;
		DrvUART_Write(UART_PORT0, Packet_Sent, strlen(Packet_Sent));
	}
	TIMER1->TISR.TIF = 1;
}

int32_t main(void)
{
	STR_UART_T sParam;
	UNLOCKREG();
	SYSCLK->PWRCON.XTL12M_EN = 1; // enable external clock (12MHz)
	SYSCLK->CLKSEL0.HCLK_S = 0;	  // select external clock (12MHz)
	DrvSYS_Open(48000000);
	LOCKREG();
	InitTIMER1();
	TIMER1->TISR.TIF = 1;
	DrvGPIO_InitFunction(E_FUNC_UART0); // Set UART pins

	sParam.u32BaudRate = 9600;
	sParam.u8cDataBits = DRVUART_DATABITS_8; // one byte
	sParam.u8cStopBits = DRVUART_STOPBITS_1;
	sParam.u8cParity = DRVUART_PARITY_NONE;
	sParam.u8cRxTriggerLevel = DRVUART_FIFO_1BYTES;
	// Set UART Configuration
	if (DrvUART_Open(UART_PORT0, &sParam) != E_SUCCESS)
		;
	DrvUART_EnableInt(UART_PORT0, DRVUART_RDAINT, UART_INT_HANDLE);
	Initial_panel(); // initialize LCD pannel
	clr_all_panel(); // clear LCD panel
	InitPWM(Servo);
	DrvADC_Open(ADC_SINGLE_END, ADC_SINGLE_CYCLE_OP, 0x03, INTERNAL_HCLK, 1); // ADC1 & ADC0
	DrvGPIO_Open(E_GPC, 0, E_IO_OUTPUT);									  // Relay
	DrvGPIO_ClrBit(E_GPC, 0);												  // Relay off

	while (1)
	{
		DrvADC_StartConvert(); // start A/D conversion
		while (DrvADC_IsConversionDone() == FALSE)
			;								  // wait till conversion is done
		LDR = ADC->ADDR[0].RSLT & 0xFFF;	  // between 0-4095
		moisture = ADC->ADDR[1].RSLT & 0xFFF; // between 0-4095
		sprintf(LDR_Text, "LDR: %4d", LDR);
		print_lcd(0, LDR_Text);
		sprintf(Moisture_Text, "MOISTURE %4d", moisture);
		sprintf(Packet_Sent, "%4d %4d", LDR, moisture);
		print_lcd(1, Moisture_Text);
		if (LDR < 1500)
		{
			PWM_Servo(Servo, HITIME_MAX);
		}
		else
		{
			PWM_Servo(Servo, HITIME_MIN);
		}
		if (moisture > 2800 && f_on==1)
		{
								DrvGPIO_SetBit(E_GPC, 0);

		}
		else if (moisture <= 2800 && f_on==1)
		{
							DrvGPIO_ClrBit(E_GPC, 0);
		}

		DrvSYS_Delay(5000);
	}
	DrvUART_Close(UART_PORT0);
}

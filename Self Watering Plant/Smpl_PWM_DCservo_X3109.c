//
// Smpl_PWM_DCservo_X3109 : 2-axis camera stand using two X3109 micro servo
//
// using PWM to generate 50Hz (20ms) pulse to micro Servo signal pin
// 0.55 ~ 1.35ms high time (PWM clock resolution at 10us per count)

// Horizontal Axis
// pin1 : signal to PWM0/GPA12 (NUC140-pin65/NUC120-pin28)
// pin2 : Vcc
// pin3 : Gnd

// Vertical Axis
// pin1 : signal to PWM1/GPA13 (NUC140-pin64/NUC120-pin27)
// pin2 : Vcc
// pin3 : Gnd

#include <stdio.h>																											 
#include "NUC1xx.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvGPIO.h"
#include "NUC1xx-LB_002\LCD_Driver.h"
#include "NUC1xx-LB_002\ScanKey.h"
#include "Driver_PWM_Servo.h"

#define  HITIME_MIN 50
#define  HITIME_MAX 135

#define  CAM_H_AXIS 0
#define  CAM_V_AXIS 1

int32_t main (void)
{
	uint16_t h_hitime, v_hitime;
	int8_t keyin;
	char TEXT1[16],TEXT2[16];
	
	UNLOCKREG();
	SYSCLK->PWRCON.XTL12M_EN=1;
	DrvSYS_Delay(5000);					// Waiting for 12M Xtal stalble
	SYSCLK->CLKSEL0.HCLK_S=0;
	LOCKREG();

	Initial_panel();
	clr_all_panel();
  print_lcd(0,"proj_ServoCam");
	
	OpenKeyPad();
	InitPWM(CAM_H_AXIS);    // initialize PWM for Horizontal Axis
  InitPWM(CAM_V_AXIS);   // initialize PWM for Vertical Axis

	h_hitime=HITIME_MIN;
	v_hitime=HITIME_MIN;
	while(1) {
	  keyin=Scankey();
	  if (keyin!=0) {
		switch(keyin) {
			case 4: h_hitime--; break;
			case 5: h_hitime=HITIME_MIN;break;
			case 6: h_hitime++; break;
			case 2: v_hitime--; break;
			case 7: v_hitime=HITIME_MIN;break;
			case 8: v_hitime++; break;
			default:            break;
		  }
		if      (h_hitime<HITIME_MIN) h_hitime=HITIME_MIN;
		else if (h_hitime>HITIME_MAX) h_hitime=HITIME_MAX;
		if      (v_hitime<HITIME_MIN) v_hitime=HITIME_MIN;
		else if (v_hitime>HITIME_MAX) v_hitime=HITIME_MAX;
		PWM_Servo(CAM_H_AXIS, h_hitime);
		PWM_Servo(CAM_V_AXIS, v_hitime);			
		sprintf(TEXT1,"H HiTime: %d ",h_hitime);
		sprintf(TEXT2,"V HiTime: %d ",v_hitime);
		print_lcd(1,TEXT1);
		print_lcd(2,TEXT2);
		DrvSYS_Delay(100000);
		}
  }
}


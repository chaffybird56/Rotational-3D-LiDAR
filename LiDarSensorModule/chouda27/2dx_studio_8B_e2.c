#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "vl53l1x_api.h"
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include <math.h> 

//Ahmad Choudhry - chouda27
//400312026


#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable
#define MAXRETRIES              5           // number of receive attempts before giving up
void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
//    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
        
}
void PortN0N1_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;                 //activate the clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};
    GPIO_PORTN_DIR_R=0b00000011;
    GPIO_PORTN_DEN_R=0b00000011;
    return;
}
void PortM_Init(void){    
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;                      // activate the clock for Port E
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R11) == 0){};          // allow time for clock to stabilize
  
    GPIO_PORTM_DIR_R = 0b00000000;
    GPIO_PORTM_DEN_R = 0b00001111;                                // Enabled both as digital outputs
    return;
    }
//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

void PortH_Init(void){
    //Use PortM pins for output
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;                  // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){};    // allow time for clock to stabilize
    GPIO_PORTH_DIR_R |= 0xFF;                                          // make PN0 out (PN0 built-in LED1)
  GPIO_PORTH_AFSEL_R &= ~0xFF;                                        // disable alt funct on PN0
  GPIO_PORTH_DEN_R |= 0xFF;                                          // enable digital I/O on PN0
                                                                                                      // configure PN1 as GPIO
  GPIO_PORTH_AMSEL_R &= ~0xFF;                                       // disable analog functionality on PN0
    return;
}

//XSHUT     This pin is an active-low shutdown input; 
//		the board pulls it up to VDD to enable the sensor by default. 
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    //FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}


void spin(){
	//spin function
    for(int i=0; i<512*12/360; i++){
        GPIO_PORTH_DATA_R = 0b00001001;
        SysTick_Wait10ms(5);
        GPIO_PORTH_DATA_R = 0b00000011;
        SysTick_Wait10ms(5);
        GPIO_PORTH_DATA_R = 0b00000110;
        SysTick_Wait10ms(5);
        GPIO_PORTH_DATA_R = 0b00001100;
        SysTick_Wait10ms(5);
        GPIO_PORTH_DATA_R = 0b00000000;
        SysTick_Wait10ms(5);
		if(i%12==0&&i!=0){
				GPIO_PORTN_DATA_R = 0b00000001;
				SysTick_Wait10ms(10);
				GPIO_PORTN_DATA_R = 0b00000000;
        }
		//button detection
if((GPIO_PORTM_DATA_R&0b0000001)==0){
	while(1) {
		if ((GPIO_PORTM_DATA_R&0b00000010)==0){
			break;
		}
	}
}
    }
}
void spin2(){
    for(int i=0; i<512*12/360; i++){
        GPIO_PORTH_DATA_R = 0b00000000;
        SysTick_Wait10ms(5);
        GPIO_PORTH_DATA_R = 0b00001100;
        SysTick_Wait10ms(5);
        GPIO_PORTH_DATA_R = 0b00000110;
        SysTick_Wait10ms(5);
        GPIO_PORTH_DATA_R = 0b00000011;
        SysTick_Wait10ms(5);
        GPIO_PORTH_DATA_R = 0b00001001;
        SysTick_Wait10ms(5);
		if(i%12==0&&i!=0){
				GPIO_PORTN_DATA_R = 0b00000001;
				SysTick_Wait10ms(10);
				GPIO_PORTN_DATA_R = 0b00000000;
        }
		//button detection
    if((GPIO_PORTM_DATA_R&0b0000001)==0){
	while(1) {
		if ((GPIO_PORTM_DATA_R&0b00000010)==0){
			break;
		}
	}
}
    }
}
//*********************************************************************************************************
//*********************************************************************************************************
//***********					MAIN Function				*****************************************************************
//*********************************************************************************************************
//*********************************************************************************************************
uint16_t	dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral
int status=0;

int main(void) {
  uint8_t byteData, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
  uint16_t wordData;
  uint16_t Distance;
  uint16_t SignalRate;
  uint16_t AmbientRate;
  uint16_t SpadNum; 
  uint8_t RangeStatus;
  uint8_t dataReady;

	//initialize
	PLL_Init();	
	SysTick_Init();
	//onboardLEDs_Init();
	I2C_Init();
	UART_Init();
	PortH_Init();
	PortM_Init();
	PortN0N1_Init();
	int input=0;
	
	int x = 0;
	
   status = VL53L1X_GetSensorId(dev, &wordData);


    // Booting ToF chip
   while(sensorState==0){
        status = VL53L1X_BootState(dev, &sensorState);
        SysTick_Wait10ms(10);
  }

  status = VL53L1X_ClearInterrupt(dev); 


  // This function must to be called to initialize the sensor with the default setting  /
  status = VL53L1X_SensorInit(dev);
  status = VL53L1X_SetDistanceMode(dev, 2); //distance mode
  status = VL53L1X_StartRanging(dev);   
			
  while(1) {

		if((GPIO_PORTM_DATA_R&0b00000001)==0){
	

	// Get the Distance Measures 50 times
		int angle = 0;
		for(int j = 0; j < 30; j++) {
		//wait until the ToF sensor's data is ready
	  while (dataReady == 0){
		  status = VL53L1X_CheckForDataReady(dev, &dataReady);
          //FlashLED3(1);
          VL53L1_WaitMs(dev, 5);
	  }
		dataReady = 0;
	if (x%2==0){
	  spin(); 
	}
	else{
		spin2();
	}
		//read the data values from ToF sensor
		status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
	  status = VL53L1X_GetDistance(dev, &Distance);					//The Measured Distance value
		status = VL53L1X_GetSignalRate(dev, &SignalRate);
		status = VL53L1X_GetAmbientRate(dev, &AmbientRate);
		status = VL53L1X_GetSpadNb(dev, &SpadNum);
    
		//FlashLED4(1);

	  status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
		
		int y = Distance*(sin(angle*3.14156/180)); 
    int z = Distance*(cos(angle*3.14156/180)); 
		// print the resulted readings to UART
		sprintf(printf_buffer,"%d %d %d \r\n",x,y,z);
		UART_printf(printf_buffer);
	  SysTick_Wait10ms(1);
		angle+=12;
  }
  
	
	x+=20;


}
			
	}
	VL53L1X_StopRanging(dev);
	while(1){}
}

/**
 * @file main.c
 * @brief 3D LiDAR Scanner - Main Application
 * 
 * This file implements a rotating LiDAR scanner using:
 * - MSP432E401Y microcontroller (ARM Cortex-M4F)
 * - VL53L1X Time-of-Flight sensor
 * - 28BYJ-48 stepper motor with ULN2003 driver
 * 
 * The system rotates the ToF sensor 360 degrees, collects distance measurements
 * at each step, and streams the data via UART to a host computer for point cloud
 * visualization.
 * 
 * @author Ahmad Choudhry (chouda27)
 * @date 2024
 */

#include <stdint.h>
#include <math.h>
#include "tm4c1294ncpdt.h"
#include "vl53l1x_api.h"
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"

// I2C Master Control/Status Register definitions
#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

// VL53L1X I2C device address
#define VL53L1X_I2C_ADDR        0x29

// Stepper motor configuration
#define STEPS_PER_DEGREE        (512 * 12 / 360)  // Microsteps per degree
#define ANGLE_INCREMENT         12                  // Degrees per measurement

// Forward declarations
void I2C_Init(void);
void PortN0N1_Init(void);
void PortM_Init(void);
void PortG_Init(void);
void PortH_Init(void);
void VL53L1X_XSHUT(void);
void stepper_rotate_clockwise(void);
void stepper_rotate_counterclockwise(void);

/**
 * @brief Initialize I2C0 for VL53L1X communication
 * 
 * Configures I2C0 on Port B pins 2 (SCL) and 3 (SDA) at 100 kHz.
 */
void I2C_Init(void) {
    SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           // Activate I2C0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;         // Activate port B
    while((SYSCTL_PRGPIO_R & 0x0002) == 0) {};      // Wait for clock ready

    GPIO_PORTB_AFSEL_R |= 0x0C;                      // Enable alt funct on PB2,3
    GPIO_PORTB_ODR_R |= 0x08;                        // Enable open drain on PB3 (SDA)
    GPIO_PORTB_DEN_R |= 0x0C;                        // Enable digital I/O on PB2,3
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xFFFF00FF) + 0x00002200;  // Configure as I2C
    I2C0_MCR_R = I2C_MCR_MFE;                        // Master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011; // 100 kbps clock
}

/**
 * @brief Initialize Port N pins 0-1 for LED control
 */
void PortN0N1_Init(void) {
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;        // Activate clock for Port N
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R12) == 0) {};
    GPIO_PORTN_DIR_R = 0b00000011;                   // Set as outputs
    GPIO_PORTN_DEN_R = 0b00000011;                   // Enable digital I/O
}

/**
 * @brief Initialize Port M for button inputs
 * 
 * Port M pins 0-3 are configured as active-low button inputs.
 */
void PortM_Init(void) {
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;        // Activate clock for Port M
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R11) == 0) {};
    GPIO_PORTM_DIR_R = 0b00000000;                   // Set as inputs
    GPIO_PORTM_DEN_R = 0b00001111;                   // Enable digital I/O
}

/**
 * @brief Initialize Port G pin 0 for VL53L1X XSHUT (reset) control
 */
void PortG_Init(void) {
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;         // Activate clock for Port G
    while((SYSCTL_PRGPIO_R & SYSCTL_RCGCGPIO_R6) == 0) {};
    GPIO_PORTG_DIR_R &= 0x00;                         // Set as input (HiZ)
    GPIO_PORTG_AFSEL_R &= ~0x01;                     // Disable alt funct on PG0
    GPIO_PORTG_DEN_R |= 0x01;                        // Enable digital I/O on PG0
    GPIO_PORTG_AMSEL_R &= ~0x01;                     // Disable analog functionality
}

/**
 * @brief Initialize Port H for stepper motor control
 * 
 * Port H pins 0-7 control the ULN2003 stepper driver.
 */
void PortH_Init(void) {
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;         // Activate clock for Port H
    while((SYSCTL_PRGPIO_R & SYSCTL_RCGCGPIO_R7) == 0) {};
    GPIO_PORTH_DIR_R |= 0xFF;                        // Set as outputs
    GPIO_PORTH_AFSEL_R &= ~0xFF;                     // Disable alt funct
    GPIO_PORTH_DEN_R |= 0xFF;                        // Enable digital I/O
    GPIO_PORTH_AMSEL_R &= ~0xFF;                     // Disable analog functionality
}

/**
 * @brief Reset VL53L1X sensor via XSHUT pin
 * 
 * XSHUT is an active-low shutdown input. Driving it low puts the sensor
 * into hardware standby, then releasing it enables the sensor.
 */
void VL53L1X_XSHUT(void) {
    GPIO_PORTG_DIR_R |= 0x01;                         // Set PG0 as output
    GPIO_PORTG_DATA_R &= 0b11111110;                 // PG0 = 0 (active low)
    SysTick_Wait10ms(10);                            // Hold reset for 100ms
    GPIO_PORTG_DIR_R &= ~0x01;                       // Set PG0 as input (HiZ, pulled high)
}

/**
 * @brief Rotate stepper motor clockwise by one step
 * 
 * Implements a 4-phase stepping sequence for the 28BYJ-48 stepper motor.
 * Includes button polling for pause/resume functionality.
 */
void stepper_rotate_clockwise(void) {
    for(int i = 0; i < STEPS_PER_DEGREE; i++) {
        // 4-phase stepping sequence (half-step mode)
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
        
        // Flash LED indicator every 12 steps
        if(i % 12 == 0 && i != 0) {
            GPIO_PORTN_DATA_R = 0b00000001;
            SysTick_Wait10ms(10);
            GPIO_PORTN_DATA_R = 0b00000000;
        }
        
        // Check for pause button (active low)
        if((GPIO_PORTM_DATA_R & 0b00000001) == 0) {
            // Wait for resume button
            while(1) {
                if((GPIO_PORTM_DATA_R & 0b00000010) == 0) {
                    break;
                }
            }
        }
    }
}

/**
 * @brief Rotate stepper motor counterclockwise by one step
 * 
 * Reverses the stepping sequence for counterclockwise rotation.
 */
void stepper_rotate_counterclockwise(void) {
    for(int i = 0; i < STEPS_PER_DEGREE; i++) {
        // Reversed 4-phase stepping sequence
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
        
        // Flash LED indicator every 12 steps
        if(i % 12 == 0 && i != 0) {
            GPIO_PORTN_DATA_R = 0b00000001;
            SysTick_Wait10ms(10);
            GPIO_PORTN_DATA_R = 0b00000000;
        }
        
        // Check for pause button (active low)
        if((GPIO_PORTM_DATA_R & 0b00000001) == 0) {
            // Wait for resume button
            while(1) {
                if((GPIO_PORTM_DATA_R & 0b00000010) == 0) {
                    break;
                }
            }
        }
    }
}

/**
 * @brief Main application entry point
 * 
 * Initializes all peripherals, configures the VL53L1X sensor, and enters
 * the main scanning loop. The loop rotates the sensor, takes distance
 * measurements, and streams XYZ coordinates via UART.
 */
int main(void) {
    uint8_t sensorState = 0;
    uint8_t dataReady = 0;
    uint8_t RangeStatus;
    uint16_t Distance;
    uint16_t SignalRate;
    uint16_t AmbientRate;
    uint16_t SpadNum;
    int status = 0;
    int x_position = 0;  // X-axis translation between sweeps
    int angle = 0;
    
    // Initialize system peripherals
    PLL_Init();              // Configure system clock to 120 MHz
    SysTick_Init();          // Initialize system tick timer
    I2C_Init();              // Initialize I2C for VL53L1X
    UART_Init();             // Initialize UART for data streaming
    PortH_Init();            // Initialize stepper motor control
    PortM_Init();            // Initialize button inputs
    PortN0N1_Init();         // Initialize LED outputs
    PortG_Init();            // Initialize XSHUT control
    
    // Reset and initialize VL53L1X sensor
    VL53L1X_XSHUT();
    
    // Verify sensor ID
    uint16_t sensorId;
    status = VL53L1X_GetSensorId(VL53L1X_I2C_ADDR, &sensorId);
    
    // Wait for sensor to boot
    while(sensorState == 0) {
        status = VL53L1X_BootState(VL53L1X_I2C_ADDR, &sensorState);
        SysTick_Wait10ms(10);
    }
    
    // Clear any pending interrupts
    status = VL53L1X_ClearInterrupt(VL53L1X_I2C_ADDR);
    
    // Initialize sensor with default settings
    status = VL53L1X_SensorInit(VL53L1X_I2C_ADDR);
    status = VL53L1X_SetDistanceMode(VL53L1X_I2C_ADDR, 2);  // Long distance mode
    status = VL53L1X_StartRanging(VL53L1X_I2C_ADDR);
    
    // Main scanning loop
    while(1) {
        // Wait for start button press (active low)
        if((GPIO_PORTM_DATA_R & 0b00000001) == 0) {
            
            // Perform 30 measurements per sweep (360 degrees / 12 degrees per measurement)
            for(int j = 0; j < 30; j++) {
                // Wait for ToF sensor data ready
                dataReady = 0;
                while(dataReady == 0) {
                    status = VL53L1X_CheckForDataReady(VL53L1X_I2C_ADDR, &dataReady);
                    VL53L1_WaitMs(VL53L1X_I2C_ADDR, 5);
                }
                
                // Rotate stepper motor (alternate direction for each sweep)
                if(x_position % 2 == 0) {
                    stepper_rotate_clockwise();
                } else {
                    stepper_rotate_counterclockwise();
                }
                
                // Read distance measurement from sensor
                status = VL53L1X_GetRangeStatus(VL53L1X_I2C_ADDR, &RangeStatus);
                status = VL53L1X_GetDistance(VL53L1X_I2C_ADDR, &Distance);
                status = VL53L1X_GetSignalRate(VL53L1X_I2C_ADDR, &SignalRate);
                status = VL53L1X_GetAmbientRate(VL53L1X_I2C_ADDR, &AmbientRate);
                status = VL53L1X_GetSpadNb(VL53L1X_I2C_ADDR, &SpadNum);
                
                // Clear interrupt to enable next measurement
                status = VL53L1X_ClearInterrupt(VL53L1X_I2C_ADDR);
                
                // Convert polar coordinates (angle, distance) to Cartesian (x, y, z)
                int y = Distance * (sin(angle * 3.14159 / 180));
                int z = Distance * (cos(angle * 3.14159 / 180));
                
                // Stream XYZ coordinates via UART
                sprintf(printf_buffer, "%d %d %d \r\n", x_position, y, z);
                UART_printf(printf_buffer);
                
                SysTick_Wait10ms(1);
                angle += ANGLE_INCREMENT;  // Increment angle by 12 degrees
            }
            
            // Increment X position for next sweep
            x_position += 20;
            angle = 0;  // Reset angle for next sweep
        }
    }
    
    // Stop ranging (unreachable in normal operation)
    VL53L1X_StopRanging(VL53L1X_I2C_ADDR);
    while(1) {}  // Infinite loop
}


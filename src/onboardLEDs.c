/**
 * @file onboardLEDs.c
 * @brief Onboard LED control functions for MSP432E401Y
 * 
 * Provides functions to control the four onboard LEDs for status indication.
 * LEDs are located on Port N (LED1, LED2) and Port F (LED3, LED4).
 */

#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "SysTick.h"
#include "onboardLEDs.h"

#define LED_DELAY_MS 1  // Delay in 10ms units

/**
 * @brief Flash LED1 (Port N pin 1) a specified number of times
 * @param count Number of times to flash
 */
void FlashLED1(int count) {
    while(count--) {
        GPIO_PORTN_DATA_R ^= 0b00000010;  // Toggle LED1
        SysTick_Wait10ms(LED_DELAY_MS);
        GPIO_PORTN_DATA_R ^= 0b00000010;  // Toggle back
        SysTick_Wait10ms(LED_DELAY_MS);
    }
}

/**
 * @brief Flash LED2 (Port N pin 0) a specified number of times
 * @param count Number of times to flash
 */
void FlashLED2(int count) {
    while(count--) {
        GPIO_PORTN_DATA_R ^= 0b00000001;  // Toggle LED2
        SysTick_Wait10ms(LED_DELAY_MS);
        GPIO_PORTN_DATA_R ^= 0b00000001;  // Toggle back
        SysTick_Wait10ms(LED_DELAY_MS);
    }
}

/**
 * @brief Flash LED3 (Port F pin 4) a specified number of times
 * @param count Number of times to flash
 */
void FlashLED3(int count) {
    while(count--) {
        GPIO_PORTF_DATA_R ^= 0b00010000;  // Toggle LED3
        SysTick_Wait10ms(LED_DELAY_MS);
        GPIO_PORTF_DATA_R ^= 0b00010000;  // Toggle back
        SysTick_Wait10ms(LED_DELAY_MS);
    }
}

/**
 * @brief Flash LED4 (Port F pin 0) a specified number of times
 * @param count Number of times to flash
 */
void FlashLED4(int count) {
    while(count--) {
        GPIO_PORTF_DATA_R ^= 0b00000001;  // Toggle LED4
        SysTick_Wait10ms(LED_DELAY_MS);
        GPIO_PORTF_DATA_R ^= 0b00000001;  // Toggle back
        SysTick_Wait10ms(LED_DELAY_MS);
    }
}

/**
 * @brief Flash all LEDs simultaneously
 */
void FlashAllLEDs(void) {
    GPIO_PORTN_DATA_R ^= 0b00000011;  // Toggle LEDs 1 and 2
    GPIO_PORTF_DATA_R ^= 0b00010001;  // Toggle LEDs 3 and 4
    SysTick_Wait10ms(25);             // 250ms delay
    GPIO_PORTN_DATA_R ^= 0b00000011;  // Toggle back
    GPIO_PORTF_DATA_R ^= 0b00010001;  // Toggle back
    SysTick_Wait10ms(25);             // 250ms delay
}

/**
 * @brief Flash LED1 to indicate I2C transmit operation
 */
void FlashI2CTx(void) {
    FlashLED1(1);
}

/**
 * @brief Flash LED2 to indicate I2C receive operation
 */
void FlashI2CRx(void) {
    FlashLED2(1);
}

/**
 * @brief Flash all LEDs to indicate I2C error
 * @param count Number of times to flash all LEDs
 */
void FlashI2CError(int count) {
    while(count--) {
        FlashAllLEDs();
    }
}

/**
 * @brief Initialize onboard LEDs
 * 
 * Configures Port N pins 0-1 (LED1, LED2) and Port F pins 0,4 (LED3, LED4)
 * as GPIO outputs. Flashes all LEDs once to indicate successful initialization.
 */
void onboardLEDs_Init(void) {
    // Configure Port N LEDs (LED1 on PN1, LED2 on PN0)
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;        // Activate clock for Port N
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R12) == 0) {};  // Wait for clock ready
    GPIO_PORTN_DIR_R |= 0x03;                         // Set PN0,PN1 as outputs
    GPIO_PORTN_AFSEL_R &= ~0x03;                      // Disable alt funct on PN0,PN1
    GPIO_PORTN_DEN_R |= 0x03;                         // Enable digital I/O on PN0,PN1
    GPIO_PORTN_AMSEL_R &= ~0x03;                      // Disable analog functionality

    // Configure Port F LEDs (LED3 on PF4, LED4 on PF0)
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;         // Activate clock for Port F
    while((SYSCTL_PRGPIO_R & SYSCTL_RCGCGPIO_R5) == 0) {};  // Wait for clock ready
    GPIO_PORTF_DIR_R |= 0x11;                         // Set PF0,PF4 as outputs
    GPIO_PORTF_AFSEL_R &= ~0x11;                      // Disable alt funct on PF0,PF4
    GPIO_PORTF_DEN_R |= 0x11;                         // Enable digital I/O on PF0,PF4
    GPIO_PORTF_AMSEL_R &= ~0x11;                      // Disable analog functionality
    
    // Flash all LEDs to indicate successful initialization
    FlashAllLEDs();
}

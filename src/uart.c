/**
 * @file uart.c
 * @brief UART communication implementation
 */

#include "uart.h"
#include "tm4c1294ncpdt.h"
#include <stdint.h>
#include <stdio.h>

// Printf buffer for formatted string output
char printf_buffer[1023];

/**
 * @brief Initialize UART0 for serial communication
 * 
 * Configures UART0 at 115200 baud, 8N1 format.
 * Uses PIOSC (16 MHz) as clock source.
 */
void UART_Init(void) {
    SYSCTL_RCGCUART_R |= 0x0001;        // Activate UART0
    SYSCTL_RCGCGPIO_R |= 0x0001;        // Activate port A
    while((SYSCTL_PRUART_R & SYSCTL_PRUART_R0) == 0) {};  // Wait for ready
        
    UART0_CTL_R &= ~UART_CTL_UARTEN;    // Disable UART during configuration
    UART0_IBRD_R = 8;                    // Integer baud rate divisor
    UART0_FBRD_R = 44;                   // Fractional baud rate divisor
    UART0_LCRH_R = (UART_LCRH_WLEN_8 | UART_LCRH_FEN);  // 8-bit word, FIFO enabled
    
    // Configure UART clock source
    UART0_CC_R = (UART0_CC_R & ~UART_CC_CS_M) + UART_CC_CS_PIOSC;
    SYSCTL_ALTCLKCFG_R = (SYSCTL_ALTCLKCFG_R & ~SYSCTL_ALTCLKCFG_ALTCLK_M) + 
                         SYSCTL_ALTCLKCFG_ALTCLK_PIOSC;
    UART0_CTL_R &= ~UART_CTL_HSE;       // High-speed disable (divide by 16)

    UART0_LCRH_R = 0x0070;              // 8-bit word length, enable FIFO
    UART0_CTL_R = 0x0301;               // Enable RXE, TXE and UART
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) + 0x00000011;  // UART function
    GPIO_PORTA_AMSEL_R &= ~0x03;        // Disable analog function on PA1-0
    GPIO_PORTA_AFSEL_R |= 0x03;        // Enable alt funct on PA1-0
    GPIO_PORTA_DEN_R |= 0x03;          // Enable digital I/O on PA1-0
}

/**
 * @brief Wait for and read a character from UART
 * @return Received ASCII character
 */
char UART_InChar(void) {
    while((UART0_FR_R & 0x0010) != 0) {};  // Wait until RXFE is 0 (receive buffer not empty)
    return((char)(UART0_DR_R & 0xFF));
}

/**
 * @brief Send a character via UART
 * @param data Character to send
 */
void UART_OutChar(char data) {
    while((UART0_FR_R & 0x0020) != 0) {};  // Wait until TXFF is 0 (transmit buffer not full)
    UART0_DR_R = data;
}

/**
 * @brief Send a null-terminated string via UART
 * @param array Null-terminated string to send
 */
void UART_printf(const char* array) {
    int ptr = 0;
    while(array[ptr]) {
        UART_OutChar(array[ptr]);
        ptr++;
    }
}

/**
 * @brief Check and report status via UART
 * @param array Status message string
 * @param status Status code (0 = success, non-zero = error)
 */
void Status_Check(char* array, int status) {
    if (status != 0) {
        UART_printf(array);
        sprintf(printf_buffer, " failed with (%d)\r\n", status);
        UART_printf(printf_buffer);
    } else {
        UART_printf(array);
        UART_printf(" Successful.\r\n");
    }
}

	
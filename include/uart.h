/**
 * @file uart.h
 * @brief UART communication interface for MSP432E401Y
 * 
 * Provides functions for UART0 initialization and communication.
 * Used for streaming point cloud data to the host computer.
 */

#ifndef UART_H
#define UART_H

// Printf buffer for formatted string output
extern char printf_buffer[1023];

/**
 * @brief Initialize UART0 for serial communication
 * 
 * Configures UART0 on Port A pins 0 (RX) and 1 (TX) at 115200 baud.
 * Uses PIOSC (16 MHz) as clock source.
 */
void UART_Init(void);

/**
 * @brief Wait for and read a character from UART
 * @return Received ASCII character
 */
char UART_InChar(void);

/**
 * @brief Send a character via UART
 * @param data Character to send
 */
void UART_OutChar(char data);

/**
 * @brief Send a null-terminated string via UART
 * @param array Null-terminated string to send
 */
void UART_printf(const char* array);

/**
 * @brief Check and report status via UART
 * @param array Status message string
 * @param status Status code (0 = success, non-zero = error)
 */
void Status_Check(char* array, int status);

#endif // UART_H

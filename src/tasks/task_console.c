/**
 * @file task_console_rx.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "task_console.h"

#ifdef ECE353_FREERTOS  
/**
 * @brief 
 * This function is the event handler for the console UART.
 *
 * The ISR will receive characters from the UART and store them in a console buffer
 * until the user presses the ENTER key.  At that point, the ISR will send a task
 * notification to the console task to process the received string.
 *
 * The ISR will also echo the received character back to the console.
 *
 * @param handler_arg 
 * @param event 
 */
void console_event_handler(void *handler_arg, cyhal_uart_event_t event)
{
    (void)handler_arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t c;

    if ((event & CYHAL_UART_IRQ_RX_NOT_EMPTY) == CYHAL_UART_IRQ_RX_NOT_EMPTY)
    {
        // Read in the character
        cyhal_uart_getc(&cy_retarget_io_uart_obj, &c, 0);

        // Echo the character to the hardware FIFO
        cyhal_uart_putc(&cy_retarget_io_uart_obj, c);

        // If character is equal to backspace or the delete key
        // remove the last character the array
        if (c == '\b' || c == 127)
        {
            // ADD CODE
            if (produce_console_buffer->index > 0)
            {
                produce_console_buffer->index--;
            }
        }

        // else if the current character is the \n or \r
        // Null terminate the string
        // and send a task notification to the bottom half task
        else if (c == '\n' || c == '\r')
        {
            produce_console_buffer->data[produce_console_buffer->index] = '\0';

            // Swap the roles of the produce and consume buffer
            console_buffer_t *temp = produce_console_buffer;
            produce_console_buffer = consume_console_buffer;
            consume_console_buffer = temp;
            produce_console_buffer->index = 0;
            
            vTaskNotifyGiveFromISR(TaskHandle_Console_Rx, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

        // else add the character to the buffer and increment the index
        else {
            if (produce_console_buffer->index < (CONSOLE_MAX_MESSAGE_LENGTH - 1))
            {
                produce_console_buffer->data[produce_console_buffer->index] = c;
                produce_console_buffer->index++;
            }
        }

    }
    if ((event & CYHAL_UART_IRQ_TX_EMPTY) == CYHAL_UART_IRQ_TX_EMPTY)
    {
        /* ADD CODE */

        // If the CB is empty, disable TX Empty Interrupts
        if (circular_buffer_empty(circular_buffer_tx))
        {
            cyhal_uart_enable_event(&cy_retarget_io_uart_obj, CYHAL_UART_IRQ_TX_EMPTY, 0, 0);
        }

        // If the CB is not empty, get next char/byte from the
        // CB and begin transmitting it
        else
        {
            char next_byte;
            circular_buffer_remove(circular_buffer_tx, &next_byte);
            cyhal_uart_putc(&cy_retarget_io_uart_obj, next_byte);
        }
    }
    else
    {
    }
}

/**
 * @brief
 * This function initializes the console tasks and resources.
 * @return true
 * @return false
 */
bool task_console_init(void)
{
    /* Register a function for the UART ISR*/
    cyhal_uart_register_callback(
        &cy_retarget_io_uart_obj,           // UART object
        console_event_handler,         // Event handler
        NULL                       // Handler argument
    );

    /* Initialize UART Rx Resources */
    if (!task_console_resources_init_rx())
    {
        return false; // Initialization failed
    }

    /* Initialize UART Tx Resources */
    if (!task_console_resources_init_tx())
    {
        return false; // Initialization failed
    }
    else
    {
        // Enable UART Rx Interrupts
        cyhal_uart_enable_event(
            &cy_retarget_io_uart_obj, 
            CYHAL_UART_IRQ_RX_NOT_EMPTY, 
            7, 
            true);
    }
    
    return true; // Initialization successful
}
#endif  
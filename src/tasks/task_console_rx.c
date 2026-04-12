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
#include "main.h"

#ifdef ECE353_FREERTOS
#include "drivers.h"
#include "task_console.h"
#include "devices.h"
#include "task_eeprom.h"
#include "task_cap_touch.h"
#include "cyhal_uart.h"
#include <string.h>
/**
 * @brief
 * This file contains the implementation of the console receive (Rx) task.
 * The task is responsible for processing incoming console commands and
 * controlling the state of the LEDs accordingly.
 * 
 * The task uses a double buffer to process the incoming console commands.
 * The supported commands will be "RED ON" and "RED OFF" to control the red LED.
 */

/* Global Variables */
console_buffer_t console_buffer1; // Double buffer for console input
console_buffer_t console_buffer2; // Double buffer for console input

// Allocate pointers for the double buffer
console_buffer_t *produce_console_buffer;
console_buffer_t *consume_console_buffer;

// Allocate a task handler
TaskHandle_t TaskHandle_Console_Rx;

// Queue used by Console Rx to receive responses from gatekeeper tasks
static QueueHandle_t Queue_Console_Responses;
#define TASK_CONSOLE_RX_DEBUG    (0)

/**
 * @brief
 * This function is the bottom half task for receiving console input.
 *
 * It waits for a task notification from the ISR indicating that a new 
 * command has been received. The task then processes the command and 
 * controls the state of the LEDs accordingly.
 *
 * @param param Unused parameter
 */
void task_console_rx(void *param)
{
    (void)param; // Unused parameter
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;
    bool valid_command;
    uint32_t notifications = 0;
    char *cmd_ptr = NULL;
    bool cmd_ready = false;
    uint8_t rx_char = 0;
    char poll_buffer[CONSOLE_MAX_MESSAGE_LENGTH];
    uint32_t poll_index = 0;
    cy_rslt_t poll_rslt;

    while (1)
    {
        // Check for ISR notifications, then always poll UART as a fallback path.
        notifications = ulTaskNotifyTake(pdTRUE, 0);
        cmd_ready = false;
        cmd_ptr = NULL;

        if(notifications > 0)
        {
            cmd_ptr = consume_console_buffer->data;
            cmd_ready = true;
        }
        else
        {
            // Poll UART so CLI still works even if RX interrupts are not firing.
            while(1)
            {
                poll_rslt = cyhal_uart_getc(&cy_retarget_io_uart_obj, &rx_char, 1);
                if(poll_rslt != CY_RSLT_SUCCESS)
                {
                    break;
                }

                if((rx_char == '\n') || (rx_char == '\r'))
                {
                    if(poll_index == 0)
                    {
                        continue;
                    }

                    poll_buffer[poll_index] = '\0';
                    cmd_ptr = poll_buffer;
                    cmd_ready = true;
                    poll_index = 0;
                    break;
                }
                else if((rx_char == '\b') || (rx_char == 127))
                {
                    if(poll_index > 0)
                    {
                        poll_index--;
                    }
                }
                else
                {
                    if(poll_index < (CONSOLE_MAX_MESSAGE_LENGTH - 1))
                    {
                        poll_buffer[poll_index] = (char)rx_char;
                        poll_index++;
                    }
                }
            }
        }

        if(!cmd_ready)
        {
            // Yield CPU when no complete command is available.
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

#if TASK_CONSOLE_RX_DEBUG
        // Trace the raw command string captured by the ISR path.
        task_console_printf("CLI RAW: '%s'\r\n", cmd_ptr);
#endif

        // Parse the CLI command to determine if this is a device request
        valid_command = parse_cli_data(cmd_ptr, &request_packet);

#if TASK_CONSOLE_RX_DEBUG
        if(valid_command)
        {
            task_console_printf(
                "CLI PARSE OK: device=%d op=%d addr=0x%04X value=0x%02X\r\n",
                request_packet.device,
                request_packet.operation,
                request_packet.address,
                request_packet.value
            );
        }
        else
        {
            task_console_printf("CLI PARSE FAILED\r\n");
        }
#endif

        if(valid_command)
        {
            // All gatekeeper responses for CLI commands return to this queue.
            request_packet.response_queue = Queue_Console_Responses;

            if((request_packet.device == DEVICE_EEPROM) && (request_packet.operation == DEVICE_OP_WRITE))
            {
                // Send write request and block until the EEPROM task responds.
                if(xQueueSend(Queue_EEPROM_Requests, &request_packet, portMAX_DELAY) != pdPASS)
                {
                    task_console_printf("EEPROM WRITE dispatch failed\r\n");
                    continue;
                }

                if(xQueueReceive(Queue_Console_Responses, &response_packet, portMAX_DELAY) != pdPASS)
                {
                    task_console_printf("EEPROM WRITE response receive failed\r\n");
                    continue;
                }

                // Print only after the response is received for write commands.
                task_console_printf(
                    "EEPROM WRITE: Addr=0x%04X, Value=0x%02X\r\n",
                    request_packet.address,
                    request_packet.value
                );
            }
            else if((request_packet.device == DEVICE_EEPROM) && (request_packet.operation == DEVICE_OP_READ))
            {
                // Send read request and wait for the value from the EEPROM task.
                if(xQueueSend(Queue_EEPROM_Requests, &request_packet, portMAX_DELAY) != pdPASS)
                {
                    task_console_printf("EEPROM READ dispatch failed\r\n");
                    continue;
                }

                if(xQueueReceive(Queue_Console_Responses, &response_packet, portMAX_DELAY) != pdPASS)
                {
                    task_console_printf("EEPROM READ response receive failed\r\n");
                    continue;
                }

                // Print the returned EEPROM value after a blocking response wait.
                task_console_printf(
                    "EEPROM READ: Addr=0x%04X, Value=0x%02X\r\n",
                    request_packet.address,
                    response_packet.payload.eeprom
                );
            }
            else if((request_packet.device == DEVICE_CAP_TOUCH) && (request_packet.operation == DEVICE_OP_READ))
            {
                // Send CAP_TOUCH request and wait for the X/Y response
                if(xQueueSend(Queue_Request_Cap_Touch, &request_packet, portMAX_DELAY) != pdPASS)
                {
                    task_console_printf("CAP_TOUCH dispatch failed\r\n");
                    continue;
                }

                if(xQueueReceive(Queue_Console_Responses, &response_packet, portMAX_DELAY) != pdPASS)
                {
                    task_console_printf("CAP_TOUCH response receive failed\r\n");
                    continue;
                }

                if(response_packet.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
                {
                    task_console_printf(
                        "Cap Touch: Sensor 0=%d, Sensor 1=%d\r\n",
                        response_packet.payload.cap_touch[0],
                        response_packet.payload.cap_touch[1]
                    );
                }
                else
                {
                    task_console_printf("Failed to read Capacitive Touch data\r\n");
                }
            }

            // Continue to the next command after handling CLI device requests
            continue;
        }

        // Process the data pointed to by the console buffer pointer
        if (strcmp(cmd_ptr, "RED ON") == 0)
        {
            // Turn on the red LED
            cyhal_gpio_write(PIN_LED_RED, LED_STATE_ON);
        }
        else if (strcmp(cmd_ptr, "RED OFF") == 0)
        {
            // Turn off the red LED
            cyhal_gpio_write(PIN_LED_RED, LED_STATE_OFF);
        }

        // All the other commands are ignored
        else
        {
            // Do nothing
        }
        
    }
}

/**
 * @brief
 * This function initializes the resources for the console Rx task.
 * @return true if resources were initialized successfully
 * @return false if resource initialization failed
 */
bool task_console_resources_init_rx(void)
{
    BaseType_t rslt;

    // Allocate an array of data from the heap for the console buffers
    console_buffer1.data = (char *)pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);
    console_buffer2.data = (char *)pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);

    if ((console_buffer1.data == NULL) || (console_buffer2.data == NULL))
    {
        return false;
    }

    // Create a shared response queue for CLI command/response transactions
    Queue_Console_Responses = xQueueCreate(1, sizeof(device_response_msg_t));
    if(Queue_Console_Responses == NULL)
    {
        return false;
    }

    // Initialize the produce and consume buffers
    produce_console_buffer = &console_buffer1;
    consume_console_buffer = &console_buffer2;

    // Set the initial lengths of the console buffers to 0
    produce_console_buffer->index = 0;
    consume_console_buffer->index = 0;

    // Create the Rx task
    rslt = xTaskCreate(
        task_console_rx,
        // Match expected console prefix width/content for CLI response logs.
        "Task Console Rx",
        // Allocate extra stack for parsing and command/response formatting.
        (configMINIMAL_STACK_SIZE * 4),
        NULL,
        tskIDLE_PRIORITY + 1,
        &TaskHandle_Console_Rx
    );
    
    return (rslt == pdPASS); // Resources initialized successfully
}
#endif

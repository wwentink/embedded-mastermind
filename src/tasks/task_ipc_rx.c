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
#include <stdbool.h>

#if defined(ECE353_FREERTOS)
#include "task_ipc.h"

/* Globals */
TaskHandle_t TaskHandle_IPC_Rx = NULL;

/* Use a double buffering strategy for IPC packets */
static volatile ipc_packet_t IPC_Rx_Buffer0;
static volatile ipc_packet_t IPC_Rx_Buffer1;

volatile ipc_packet_t* volatile IPC_Rx_Produce_Buffer = &IPC_Rx_Buffer0;
volatile ipc_packet_t* volatile IPC_Rx_Consume_Buffer = &IPC_Rx_Buffer1;

/**
 * @brief
 *
 * This task is used to process received IPC packets.  The task will block
 * on a FreeRTOS Task Notification.  When a notification is received,
 * the task will process the IPC packet stored in the consume buffer.
 *
 * For validation purposes, the task will print out the contents of the
 * received IPC packet to the console.
 * 
 * @param arg
 * Unused parameter
 */
void task_ipc_rx(void *param)
{
    (void)param;

    while(1)
    {
        // Wait for a FreeRTOS Task Notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if(validate_packet((ipc_packet_t *)IPC_Rx_Consume_Buffer) == true) 
        {
            /* ADD CODE */
            // Process the received IPC packet
            switch(IPC_Rx_Consume_Buffer->cmd)
            {
                case IPC_CMD_DISCOVERY:
                    printf("Received IPC Discovery Packet with sequence number: %d\n\r", IPC_Rx_Consume_Buffer->sequence_num);
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX);
                    ipc_send_ack(IPC_Rx_Consume_Buffer->sequence_num);
                    break;
                case IPC_CMD_ACTIVE_PLAYER:
                    printf("Received IPC Active Player Packet with sequence number: %d\n\r", IPC_Rx_Consume_Buffer->sequence_num);
                    ipc_send_ack(IPC_Rx_Consume_Buffer->sequence_num);
                    break;
                case IPC_CMD_INACTIVE_PLAYER:
                    printf("Received IPC Inactive Player Packet with sequence number: %d\n\r", IPC_Rx_Consume_Buffer->sequence_num);
                    ipc_send_ack(IPC_Rx_Consume_Buffer->sequence_num);
                    break;
                case IPC_CMD_STATUS:
                    printf(
                        "Received IPC Status Packet with sequence number: %d, status: 0x%02X\n\r",
                        IPC_Rx_Consume_Buffer->sequence_num,
                        IPC_Rx_Consume_Buffer->payload.status
                    );
                    ipc_send_ack(IPC_Rx_Consume_Buffer->sequence_num);
                    break;
                case IPC_CMD_ACK:
                    printf("Received IPC ACK Packet with sequence number: %d\n\r", IPC_Rx_Consume_Buffer->sequence_num);
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED);
                    break;
                default:
                    printf("Received IPC Packet with unknown command: %d\n\r", IPC_Rx_Consume_Buffer->cmd);
                    break;
            }
        }
        else {
            printf("Invalid IPC packet received!\n\r");
        }
    }
}

bool task_ipc_resources_init_rx(void)
{
    // Create the IPC Rx Task
    BaseType_t task_ipc_rx_status = xTaskCreate(
        task_ipc_rx,                 // Function that implements the task.
        "IPC Rx Task",               // Text name for the task.
        IPC_STACK_SIZE,             // Stack size in words, not bytes.
        NULL,                       // Parameter passed into the task.
        IPC_PRIORITY,               // Priority at which the task is created.
        &TaskHandle_IPC_Rx          // Used to pass out the created task's handle.
    );

    if(task_ipc_rx_status != pdPASS)
    {
        return false;
    }

    return true;    
}

#endif
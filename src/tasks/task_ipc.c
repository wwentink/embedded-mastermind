/**
 * @file task_ipc.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "task_ipc.h"
#include "cy_result.h"
#include "cyhal_hw_types.h"
#include "cyhal_uart.h"
#include "main.h"
#include "task_console.h"

#if defined(ECE353_FREERTOS)

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_uart_t IPC_Uart_Obj;
cyhal_uart_cfg_t IPC_Uart_Config =
{
    .data_bits = 8,
    .stop_bits = 1,
    .parity = CYHAL_UART_PARITY_NONE,
    .rx_buffer = NULL,
    .rx_buffer_size = 0
};

uint32_t IPC_Actual_Baud;

volatile uint16_t IPC_Last_Tx_Sequence = 0;
volatile uint16_t IPC_Last_Ack_Sequence = 0;
volatile bool IPC_Ack_Sequence_Valid = false;


/**
 * @brief 
 * Simple checksum calculation function.  Take the XOR of all bytes
 * except the start and checksum bytes.
 * @param packet 
 * @return __inline 
 */
static __inline uint8_t calculate_checksum(ipc_packet_t *packet)
{
    uint8_t checksum = 0;
    for(int i = 1; i < sizeof(ipc_packet_t) - 1; i++)
    {
        checksum ^= ((uint8_t*)packet)[i];
    }
    return checksum;
}   

/**
 * @brief 
 * Validates the given IPC packet by checking the start byte and checksum
 * @param packet 
 * @return __inline 
 */
bool validate_packet(ipc_packet_t *packet)
{
    uint8_t checksum = 0;

    // Check that the packet pointer is valid
    if(packet == NULL)
    {
        return false;
    }


    // Check for the start byte
    if(packet->start_byte != IPC_PACKET_START)
    {
        return false;
    }

    // Calculate the checksum
    checksum = calculate_checksum(packet);

    // Validate the checksum
    return (checksum == packet->checksum);
}

/********************************************************************/
/* Helper Functions for sending IPC packets                         */
/********************************************************************/
/* ADD CODE */
/* Look at task_ipc.h to find the list of helper functions */
bool ipc_send_discovery(uint16_t sequence_num) {
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = IPC_CMD_DISCOVERY,
        .sequence_num = sequence_num,
        .payload.status = IPC_STATUS_OK,
        .checksum = 0,
    };

    packet.checksum = calculate_checksum(&packet);
    IPC_Last_Tx_Sequence = sequence_num;

    if (xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) != pdPASS) {
        return false; // Failed to send packet to queue
    } 
    
    return true; // Packet successfully sent to queue
}

bool ipc_send_active_player(uint16_t sequence_num) {
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = IPC_CMD_ACTIVE_PLAYER,
        .sequence_num = sequence_num,
        .payload.status = IPC_STATUS_OK,
        .checksum = 0,
    };

    packet.checksum = calculate_checksum(&packet);
    IPC_Last_Tx_Sequence = sequence_num;

    if (xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) != pdPASS) {
        return false; // Failed to send packet to queue
    } 
    
    return true; // Packet successfully sent to queue
}

bool ipc_send_inactive_player(uint16_t sequence_num) {
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = IPC_CMD_INACTIVE_PLAYER,
        .sequence_num = sequence_num,
        .payload.status = IPC_STATUS_OK,
        .checksum = 0,
    };

    packet.checksum = calculate_checksum(&packet);
    IPC_Last_Tx_Sequence = sequence_num;

    if (xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) != pdPASS) {
        return false; // Failed to send packet to queue
    } 
    
    return true; // Packet successfully sent to queue
}
bool ipc_send_status(uint16_t sequence_num, ipc_status_t status) {
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = IPC_CMD_STATUS,
        .sequence_num = sequence_num,
        .payload.status = status,
        .checksum = 0,
    };

    packet.checksum = calculate_checksum(&packet);
    IPC_Last_Tx_Sequence = sequence_num;

    if (xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) != pdPASS) {
        return false; // Failed to send packet to queue
    } 
    
    return true; // Packet successfully sent to queue
}

bool ipc_send_ack(uint16_t sequence_num) {
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = IPC_CMD_ACK,
        .sequence_num = sequence_num,
        .payload.status = IPC_STATUS_OK,
        .checksum = 0,
    };

    packet.checksum = calculate_checksum(&packet);

    if (xQueueSend(Queue_IPC_Tx, &packet, pdMS_TO_TICKS(100)) != pdPASS) {
        return false; // Failed to send packet to queue
    } 
    
    return true; // Packet successfully sent to queue
}

bool ipc_wait_for_ack(uint32_t timeout_ms) {
    TickType_t start_ticks = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while(1)
    {
        TickType_t elapsed_ticks = xTaskGetTickCount() - start_ticks;
        TickType_t remaining_ticks;
        EventBits_t events;
        bool ack_matches = false;

        if(elapsed_ticks >= timeout_ticks)
        {
            return false;
        }

        remaining_ticks = timeout_ticks - elapsed_ticks;

        events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED,
            pdTRUE,
            pdFALSE,
            remaining_ticks
        );

        if((events & ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED) == 0)
        {
            return false;
        }

        taskENTER_CRITICAL();
        if(IPC_Ack_Sequence_Valid)
        {
            ack_matches = (IPC_Last_Ack_Sequence == IPC_Last_Tx_Sequence);
            IPC_Ack_Sequence_Valid = false;
        }
        taskEXIT_CRITICAL();

        if(ack_matches)
        {
            return true;
        }
    }
}


/**
 * @brief
 * Interrupt handler for the IPC UART. This function handles both RX and TX interrupts.
 *
 * @param handler_arg Pointer to handler arguments (not used).
 * @param event The UART event that triggered the interrupt.
 */
void ipc_event_handler(void *handler_arg, cyhal_uart_event_t event)
{
    (void)handler_arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t c;
    static uint8_t raw_data_index = 0;

    if ((event & CYHAL_UART_IRQ_RX_NOT_EMPTY) == CYHAL_UART_IRQ_RX_NOT_EMPTY)
    {
        // Read the received character
        cyhal_uart_getc(&IPC_Uart_Obj, &c, 0);

        // If we are waiting for the first byte, only accept IPC_PACKET_START.
        if ((raw_data_index == 0) && (c != IPC_PACKET_START))
        {
            // Ignore bytes until the start byte arrives.
        }

        // Else store the byte in the current produce buffer.
        else
        {
            ((uint8_t*)IPC_Rx_Produce_Buffer)[raw_data_index] = c;
            raw_data_index++;

            // If an entire IPC packet has been received, swap buffers and notify Rx task.
            if (raw_data_index >= sizeof(ipc_packet_t))
            {
                volatile ipc_packet_t* completed_packet = IPC_Rx_Produce_Buffer;

                IPC_Rx_Produce_Buffer = IPC_Rx_Consume_Buffer;
                IPC_Rx_Consume_Buffer = completed_packet;

                raw_data_index = 0;

                vTaskNotifyGiveFromISR(TaskHandle_IPC_Rx, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }

    if ((event & CYHAL_UART_IRQ_TX_EMPTY) == CYHAL_UART_IRQ_TX_EMPTY)
    {
    }
    else
    {
    }
}

bool task_ipc_init(void)
{
    cy_rslt_t rslt;

    // Initialize the IPC UART
   rslt =  cyhal_uart_init(
        &IPC_Uart_Obj, 
        PIN_IPC_TX, 
        PIN_IPC_RX, 
        NC, 
        NC, 
        NULL, 
        &IPC_Uart_Config
    );
    if (rslt != CY_RSLT_SUCCESS)
    {
        return false; // Initialization failed
    }

    rslt = cyhal_uart_set_baud(&IPC_Uart_Obj, 115200, &IPC_Actual_Baud);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return false; // Initialization failed
    }

    cyhal_uart_clear(&IPC_Uart_Obj);

    // Register the UART handler
    cyhal_uart_register_callback(&IPC_Uart_Obj, ipc_event_handler, NULL);

    // Enable Rx Interrupts
    cyhal_uart_enable_event(
        &IPC_Uart_Obj,
        CYHAL_UART_IRQ_RX_NOT_EMPTY,
        3,
        true
    );


    if(task_ipc_resources_init_rx() == false)
    {
        return false; // Initialization failed
    }

    if(task_ipc_resources_init_tx() == false)
    {
        return false; // Initialization failed
    }

    return true; // Initialization successful
}
#endif  
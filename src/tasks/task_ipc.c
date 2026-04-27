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
volatile ipc_packet_t IPC_Last_Rx_Packet;
volatile bool IPC_Last_Rx_Packet_Valid = false;
volatile uint8_t IPC_Rx_Raw_Data_Index = 0;

void ipc_reset_link_state(void)
{
    taskENTER_CRITICAL();
    IPC_Last_Ack_Sequence = 0U;
    IPC_Ack_Sequence_Valid = false;
    IPC_Last_Rx_Packet_Valid = false;
    IPC_Rx_Raw_Data_Index = 0U;
    taskEXIT_CRITICAL();

    /* Flush any bytes currently buffered by the UART peripheral. */
    (void)cyhal_uart_clear(&IPC_Uart_Obj);
}

static bool ipc_cmd_is_valid(ipc_cmd_t cmd)
{
    switch(cmd)
    {
        case IPC_CMD_DISCOVERY:
        case IPC_CMD_ACTIVE_PLAYER:
        case IPC_CMD_INACTIVE_PLAYER:
        case IPC_CMD_STATUS:
        case IPC_CMD_ACK:
        case IPC_CMD_GAME_READY:
        case IPC_CMD_GAME_GUESS:
        case IPC_CMD_GAME_FEEDBACK:
        case IPC_CMD_GAME_TURN_END_ACK:
        case IPC_CMD_GAME_OVER:
        case IPC_CMD_GAME_RESTART:
            return true;
        default:
            return false;
    }
}

static bool ipc_game_digits_valid(const uint8_t digits[4])
{
    for(int i = 0; i < 4; i++)
    {
        if(digits[i] > 7U)
        {
            return false;
        }
    }

    return true;
}

static bool ipc_game_payload_valid(const ipc_packet_t *packet)
{
    if(packet == NULL)
    {
        return false;
    }

    switch(packet->cmd)
    {
        case IPC_CMD_GAME_READY:
        case IPC_CMD_GAME_GUESS:
            return ipc_game_digits_valid(packet->payload.game.digits);

        case IPC_CMD_GAME_FEEDBACK:
            return
                (packet->payload.game.exact <= 4U) &&
                (packet->payload.game.misplaced <= 4U) &&
                ((uint16_t)packet->payload.game.exact + (uint16_t)packet->payload.game.misplaced <= 4U);

        case IPC_CMD_GAME_TURN_END_ACK:
        case IPC_CMD_GAME_OVER:
        case IPC_CMD_GAME_RESTART:
            return true;

        default:
            return true;
    }
}


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
    if(checksum != packet->checksum)
    {
        return false;
    }

    // Reject packets with unknown command values.
    if(!ipc_cmd_is_valid(packet->cmd))
    {
        return false;
    }

    // Legacy commands still use IPC_STATUS_OK in the payload marker field.
    if((packet->cmd == IPC_CMD_DISCOVERY) ||
       (packet->cmd == IPC_CMD_ACTIVE_PLAYER) ||
       (packet->cmd == IPC_CMD_INACTIVE_PLAYER) ||
       (packet->cmd == IPC_CMD_ACK))
    {
        if(packet->payload.status != IPC_STATUS_OK)
        {
            return false;
        }
    }

    if(packet->cmd == IPC_CMD_STATUS)
    {
        if(
            (packet->payload.status != IPC_STATUS_OK) &&
            (packet->payload.status != IPC_STATUS_CRC_FAIL) &&
            (packet->payload.status != IPC_STATUS_INVALID_MSG_TYPE)
        )
        {
            return false;
        }
    }

    if(!ipc_game_payload_valid(packet))
    {
        return false;
    }

    return true;
}

static bool ipc_queue_packet(const ipc_packet_t *packet)
{
    if(packet == NULL)
    {
        return false;
    }

    return (xQueueSend(Queue_IPC_Tx, packet, pdMS_TO_TICKS(100)) == pdPASS);
}

/********************************************************************/
/* Helper Functions for sending IPC packets                         */
/********************************************************************/
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

    return ipc_queue_packet(&packet);
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

    return ipc_queue_packet(&packet);
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

    return ipc_queue_packet(&packet);
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

    return ipc_queue_packet(&packet);
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

    return ipc_queue_packet(&packet);
}

static bool ipc_send_game_packet(ipc_cmd_t cmd, uint16_t sequence_num, const uint8_t digits[4], uint8_t exact, uint8_t misplaced, uint8_t guess_count, uint8_t flags)
{
    ipc_packet_t packet = {
        .start_byte = IPC_PACKET_START,
        .cmd = cmd,
        .sequence_num = sequence_num,
        .payload.game = {
            .digits = {0, 0, 0, 0},
            .exact = exact,
            .misplaced = misplaced,
            .guess_count = guess_count,
            .flags = flags,
        },
        .checksum = 0,
    };

    if(digits != NULL)
    {
        for(int i = 0; i < 4; i++)
        {
            packet.payload.game.digits[i] = digits[i];
        }
    }

    packet.checksum = calculate_checksum(&packet);
    IPC_Last_Tx_Sequence = sequence_num;

    return ipc_queue_packet(&packet);
}

bool ipc_send_game_ready(uint16_t sequence_num, const uint8_t digits[4])
{
    return ipc_send_game_packet(IPC_CMD_GAME_READY, sequence_num, digits, 0U, 0U, 0U, 0U);
}

bool ipc_send_game_guess(uint16_t sequence_num, const uint8_t digits[4], uint8_t guess_count)
{
    return ipc_send_game_packet(IPC_CMD_GAME_GUESS, sequence_num, digits, 0U, 0U, guess_count, 0U);
}

bool ipc_send_game_feedback(uint16_t sequence_num, uint8_t exact, uint8_t misplaced, uint8_t guess_count, bool win)
{
    return ipc_send_game_packet(IPC_CMD_GAME_FEEDBACK, sequence_num, NULL, exact, misplaced, guess_count, win ? 1U : 0U);
}

bool ipc_send_game_turn_end_ack(uint16_t sequence_num)
{
    return ipc_send_game_packet(IPC_CMD_GAME_TURN_END_ACK, sequence_num, NULL, 0U, 0U, 0U, 0U);
}

bool ipc_send_game_over(uint16_t sequence_num, uint8_t local_guesses, uint8_t peer_guesses, bool local_won)
{
    return ipc_send_game_packet(IPC_CMD_GAME_OVER, sequence_num, NULL, local_guesses, peer_guesses, 0U, local_won ? 1U : 0U);
}

bool ipc_send_game_restart(uint16_t sequence_num, uint8_t reason)
{
    return ipc_send_game_packet(IPC_CMD_GAME_RESTART, sequence_num, NULL, 0U, 0U, 0U, reason);
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

    if ((event & CYHAL_UART_IRQ_RX_NOT_EMPTY) == CYHAL_UART_IRQ_RX_NOT_EMPTY)
    {
        // Read the received character
        cyhal_uart_getc(&IPC_Uart_Obj, &c, 0);

        // If we are waiting for the first byte, only accept IPC_PACKET_START.
        if ((IPC_Rx_Raw_Data_Index == 0) && (c != IPC_PACKET_START))
        {
            // Ignore bytes until the start byte arrives.
        }

        // Else store the byte in the current produce buffer.
        else
        {
            ((uint8_t*)IPC_Rx_Produce_Buffer)[IPC_Rx_Raw_Data_Index] = c;
            IPC_Rx_Raw_Data_Index++;

            // If an entire IPC packet has been received, swap buffers and notify Rx task.
            if (IPC_Rx_Raw_Data_Index >= sizeof(ipc_packet_t))
            {
                volatile ipc_packet_t* completed_packet = IPC_Rx_Produce_Buffer;

                IPC_Rx_Produce_Buffer = IPC_Rx_Consume_Buffer;
                IPC_Rx_Consume_Buffer = completed_packet;

                IPC_Rx_Raw_Data_Index = 0;

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

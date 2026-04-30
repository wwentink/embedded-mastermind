/**
 * @file ex03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE11)
#include "drivers.h"
#include "task_buttons.h"
#include "task_ipc.h"
#include "rtos_events.h"

char APP_DESCRIPTION[] = "ECE353: ICE 11 - FreeRTOS IPC Rx/Tx";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
EventGroupHandle_t ECE353_RTOS_Events = NULL;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/* This function will be used to discover other board. This function should not
 * return until the discovery is complete.  The discovery is complete when we receive
 * a discovery message from the other board OR we send a discovery message that is 
 * Acked by the other board */
void discover_board(uint16_t *sequence_num)
{
    bool discovery_complete = false;

    /* Clear any stale IPC events/ACK state before we begin discovery retries. */
    xEventGroupClearBits(
        ECE353_RTOS_Events,
        ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED | ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX
    );

    taskENTER_CRITICAL();
    IPC_Ack_Sequence_Valid = false;
    taskEXIT_CRITICAL();
    
    while(discovery_complete == false)
    {
        EventBits_t events;

        ipc_send_discovery((*sequence_num)++);

        events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED | ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(250)
        );

        if(events & (ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED | ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX))
        {
            discovery_complete = true;

            if(events & ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX)
            {
                printf("Discovery message received from other board!\n\r");
            }

            if(events & ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED)
            {
                printf("ACK received for discovery message!\n\r");
            }
        }
        else
        {
            printf("No response to discovery message. Retrying...\n\r");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief
 * This task will be used to verify the functionality of the IPC UART specification
 * by ....
 *
 * @param arg 
 * Unused parameter
 */
void task_system_control(void *arg)
{
    (void)arg; // Unused parameter
    EventBits_t events;

    uint16_t sequence_num = 0;

    /* Begin the discovery process. */
    discover_board(&sequence_num);

    while(1)
    {
        // Wait for SW1, SW2,or SW3 to be pressed.  If you have not gotten task_buttons.c working yet, 
        // you will need to do so before you can proceed with this task. 
        events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                    ECE353_EVENT_SW1_PRESSED | ECE353_EVENT_SW2_PRESSED | ECE353_EVENT_SW3_PRESSED,
                                    pdTRUE,
                                    pdFALSE,
                                    portMAX_DELAY);

        if(events & ECE353_EVENT_SW1_PRESSED)
        {
            /* Send the active player message  */
            ipc_send_active_player(sequence_num++);

            /* Wait for the ack */
            bool ack_received = ipc_wait_for_ack(100);

            /* Print out a message indicating if the ACK was received */
            if(ack_received)
            {
                printf("ACK received for active player message!\n\r");
            }
            else
            {
                printf("ACK NOT received for active player message!\n\r");
            }

        }
        else if(events & ECE353_EVENT_SW2_PRESSED)
        {
            /* Send the inactive player message  */
            ipc_send_inactive_player(sequence_num++);
    
            /* Wait for the ack */
            bool ack_received = ipc_wait_for_ack(100);

            /* Print out a message indicating if the ACK was received */
            if(ack_received)
            {
                printf("ACK received for inactive player message!\n\r");
            }
            else
            {
                printf("ACK NOT received for inactive player message!\n\r");
            }

        }
        else if(events & ECE353_EVENT_SW3_PRESSED)
        {
            /* Send the status message with an error code  */
            ipc_send_status(sequence_num++, IPC_STATUS_CRC_FAIL);

            /* Wait for the ack */
            bool ack_received = ipc_wait_for_ack(100);

            /* Print out a message indicating if the ACK was received */
            if(ack_received)
            {
                printf("ACK received for status message!\n\r");
            }
            else
            {
                printf("ACK NOT received for status message!\n\r");
            }
        }
        else
        {
             printf("Unknown Event!\n\r\n\r");
        }
    }
}

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    // Initialize the LEDs
    rslt = leds_init_gpio();
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("LED initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    rslt = buttons_init_gpio();

    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("Button GPIO initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{
    // Initialize the EventGroup
    ECE353_RTOS_Events = xEventGroupCreate();

    if(!task_button_init())
    {
        printf("Button task initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_ipc_init())
    {
        printf("IPC initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Create the System Control Task
    xTaskCreate(
        task_system_control, 
        "System Control", 
        configMINIMAL_STACK_SIZE*5,      
        NULL, 
        2, 
        NULL
    );

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif
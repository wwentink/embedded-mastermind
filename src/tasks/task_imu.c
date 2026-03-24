/**
 * @file task_imu.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-16
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_imu.h"

 #if defined(ECE353_FREERTOS)
#include "imu.h"
#include "task_console.h"

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
QueueHandle_t Queue_IMU_Request;

/*****************************************************************************/
/* Static Variables                                                          */
/*****************************************************************************/
static SemaphoreHandle_t *SPI_Semaphore = NULL;
static cyhal_spi_t *imu_spi_obj = NULL;
static cyhal_gpio_t imu_cs_pin = NC;

/**
 * @brief 
 * This function acts as the published interface to get data from the IMU.
 * X, Y, and Z data is returned via the imu_data parameter.
 * @param return_queue 
 * @param imu_data 
 * @return true 
 * @return false 
 */
bool system_sensors_get_imu(QueueHandle_t return_queue, int16_t imu_data[3])
{
    if(return_queue == NULL || imu_data == NULL)
    {
        return false;
    }

    /* ADD CODE */

    return true;
}

 void task_imu(void *arg)
 {
    (void) arg;
    int16_t accel_data[3];

    xSemaphoreTake(*SPI_Semaphore, portMAX_DELAY);

    if(!imu_init(imu_spi_obj, imu_cs_pin))
    {
        task_console_printf("IMU initialization failed!");
        vTaskSuspend(NULL);
    } else {
        task_console_printf("IMU initialization successful!");
    }

    xSemaphoreGive(*SPI_Semaphore);

    while(1)
    {
        /* Wait for a request to be available */
        vTaskDelay(pdMS_TO_TICKS(250));
        
        xSemaphoreTake(*SPI_Semaphore, portMAX_DELAY);

        // read acceleration data from the IMU
        imu_read_registers(imu_spi_obj, imu_cs_pin, IMU_REG_OUTX_L_XL, (uint8_t *)accel_data, 6);

        xSemaphoreGive(*SPI_Semaphore);

        // print out the raw acceleration data to the console
        task_console_printf("Accel: %d, %d, %d\n", accel_data[0], accel_data[1], accel_data[2]);
    }
}

/**
  * @brief 
  * This function will create the IMU task for reading data from the IMU sensor.
  * It assumes that you have already created a semaphore for SPI access and initialized
  * the SPI peripheral.  This function does NOT initialize the SPI peripheral OR CS Pin 
  * because the SPI peripheral is shared between multiple tasks (e.g. IMU, EEPROM, etc.). 
  * @param spi_semaphore 
  * @return true 
  * @return false 
  */
 bool task_imu_resources_init(void *spi_semaphore, cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
 {
    if(spi_semaphore == NULL || spi_obj == NULL || cs_pin == NC)
    {
        return false;
    }

    /* Save the SPI resources */
    SPI_Semaphore = (SemaphoreHandle_t *)spi_semaphore;
    imu_spi_obj = spi_obj;
    imu_cs_pin = cs_pin;

    /* Create the IMU Request Queue */
    Queue_IMU_Request = xQueueCreate(1, sizeof(device_request_msg_t));
    if(Queue_IMU_Request == NULL)
    {
        return false;
    }

    /* Create the IMU Task */
   if (xTaskCreate(
       task_imu, 
       "IMU", 
       10*configMINIMAL_STACK_SIZE, 
       spi_semaphore, 
       tskIDLE_PRIORITY + 1, 
       NULL) != pdPASS)
   {
       return false;
   }

   return true;
 }

#endif /* ECE353_FREERTOS */
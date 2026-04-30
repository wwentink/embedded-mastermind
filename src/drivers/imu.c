/**
 * @file imu.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "imu.h"
#include "cy_result.h"
#include "cyhal_hw_types.h"

/**
 * @brief 
 * Read a register from the IMU
 * @param reg 
 * @param value 
 */
void imu_write_reg(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg, 
    uint8_t value
)
{
    // Write 2 bytes: Register address and value
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2]; // Not used for write, but required by the API
    tx_buffer[0] = reg & 0x7F; // Clear the MSB for write operation
    tx_buffer[1] = value;

    // Pull CS low to select the IMU
    cyhal_gpio_write(cs_pin, 0);

    // Send data over SPI
    cyhal_spi_transfer(spi_obj, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer), 100);

    // Pull CS high to deselect the IMU
    cyhal_gpio_write(cs_pin, 1);
}

/**
 * @brief 
 * Read a register from the IMU
 * @param reg 
 * @return uint8_t 
 */
uint8_t imu_read_reg(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg
)
{
    // Write the register address with the MSB set to indicate a read operation
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2];
    tx_buffer[0] = reg | 0x80; // Set the MSB for read operation
    tx_buffer[1] = 0xFF; // Dummy byte for clocking out the data

    // Pull CS low to select the IMU
    cyhal_gpio_write(cs_pin, 0);

    // Send the register address and read the response
    cyhal_spi_transfer(spi_obj, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer), 100);

    // Pull CS high to deselect the IMU
    cyhal_gpio_write(cs_pin, 1);

    return rx_buffer[1]; // The second byte contains the register value
}

/**
 * @brief Read multiple registers from the IMU
 * 
 * @param reg 
 * @param buffer 
 * @param length 
 */
void imu_read_registers(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg, 
    uint8_t *buffer, 
    uint8_t length
)
{
    // Write the starting register address with the MSB set to indicate a read operation
    uint8_t tx_buffer[1 + length];
    uint8_t rx_buffer[length + 1]; // First byte is dummy for the register address
    tx_buffer[0] = reg | 0x80; // Set the MSB for read operation

    // Pull CS low to select the IMU
    cyhal_gpio_write(cs_pin, 0);

    // Send the register address and read the response
    cyhal_spi_transfer(spi_obj, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer), 100);

    // Pull CS high to deselect the IMU
    cyhal_gpio_write(cs_pin, 1);

    // Copy the received data to the output buffer (skip the first byte which is dummy)
    memcpy(buffer, &rx_buffer[1], length);
}

/**
 * @brief 
 * This function verifys the WHO_AM_I register and configures the IMU
 * to 104 Hz, ±2g, ±250 dps.
 * @param spi_obj 
 * @param cs_pin 
 * @return true 
 * @return false 
 */
bool imu_init(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
    // Wait for 15mS after power-up
    cyhal_system_delay_ms(15);

    // Verify the WHO_AM_I register
    uint8_t who_am_i = imu_read_reg(spi_obj, cs_pin, IMU_REG_WHO_AM_I);
    if (who_am_i != 0x6A)
    {
        return false;
    }

    // Reset the IMU
    imu_write_reg(spi_obj, cs_pin, IMU_REG_CTRL3_C, 0x01); // CTRL3_C register

    // Wait for the reset to complete
    do
    {
        cyhal_system_delay_ms(1);
    } while (imu_read_reg(spi_obj, cs_pin, IMU_REG_CTRL3_C) & 0x01);

    // Configure the IMU:  104 Hz, ±2g, ±250 dps
    imu_write_reg(spi_obj, cs_pin, IMU_REG_CTRL1_XL, ODR_104HZ | FS_XL_2G);

    return true;
}
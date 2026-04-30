/**
 * @file eeprom.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2023-10-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "eeprom.h"
#include "cyhal_hw_types.h"
#include <sys/types.h>

static uint8_t eeprom_read_status_register(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint8_t tx_buffer[2] = {EEPROM_CMD_RDSR, 0x00};
	uint8_t rx_buffer[2] = {0x00, 0x00};

	cyhal_gpio_write(cs_pin, 0);
	cyhal_spi_transfer(spi_obj, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer), 0xFF);
	cyhal_gpio_write(cs_pin, 1);

	return rx_buffer[1];
}


/** Determine if the EEPROM is busy writing the last
 *  transaction to non-volatile storage
 *
 * @param spi_obj
 * @param cs_pin
 *
 */
void eeprom_wait_for_write(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint32_t timeout = 100000U;

	while(1)
	{
		/* Check if the Write In Progress (WIP) bit is clear. */
		if((eeprom_read_status_register(spi_obj, cs_pin) & EEPROM_STATUS_WIP) == 0)
		{
			break; // WIP bit is clear, write is complete
		}

		if(timeout-- == 0U)
		{
			break;
		}
	}
}

/** Enables Writes to the EEPROM
 *
 * @param spi_obj
 * @param cs_pin
 */
void eeprom_write_enable(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint8_t tx_buffer[1] = {EEPROM_CMD_WREN}; // Write Enable command
	uint8_t rx_buffer[1] = {0x00}; // Buffer to hold the response (not used for this command)

	// Pull CS low to select the EEPROM
	cyhal_gpio_write(cs_pin, 0);

	// Send the Write Enable command
	cyhal_spi_transfer(spi_obj, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer), 0xFF);

	// Pull CS high to deselect the EEPROM
	cyhal_gpio_write(cs_pin, 1);

	/* Poll until the WEL bit is set before attempting a write command. */
	for(uint32_t timeout = 100000U; timeout > 0U; timeout--)
	{
		if((eeprom_read_status_register(spi_obj, cs_pin) & EEPROM_STATUS_WEL) != 0)
		{
			break;
		}
	}
}

/** Disable Writes to the EEPROM
 *
 * @param spi_obj
 * @param cs_pin
 */
void eeprom_write_disable(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint8_t tx_buffer[1] = {EEPROM_CMD_WRDI}; // Write Disable command
	uint8_t rx_buffer[1] = {0x00}; // Buffer to hold the response (not used for this command)

	// Pull CS low to select the EEPROM
	cyhal_gpio_write(cs_pin, 0);

	// Send the Write Disable command
	cyhal_spi_transfer(spi_obj, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer), 0xFF);

	// Pull CS high to deselect the EEPROM
	cyhal_gpio_write(cs_pin, 1);
}

/** Writes a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 * @param data    -- value to write into memory
 *
 */
void eeprom_write_byte(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin, uint16_t address, uint8_t data)
{
	eeprom_wait_for_write(spi_obj, cs_pin);

	eeprom_write_enable(spi_obj, cs_pin);

	uint8_t tx_buffer[4] = {EEPROM_CMD_WRITE, (address >> 8) & 0xFF, address & 0xFF, data}; // Write command followed by 2-byte address and data

	// Pull CS low to select the EEPROM
	cyhal_gpio_write(cs_pin, 0);

	// Send the command, address, and data
	cyhal_spi_transfer(spi_obj, tx_buffer, sizeof(tx_buffer), NULL, 0, 0xFF);

	// Pull CS high to deselect the EEPROM
	cyhal_gpio_write(cs_pin, 1);
}

/** Reads a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 *
 */
uint8_t eeprom_read_byte(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin, uint16_t address)
{
    eeprom_wait_for_write(spi_obj, cs_pin);

    uint8_t tx_buffer[4] = {EEPROM_CMD_READ, (address >> 8) & 0xFF, address & 0xFF, 0x00};
    uint8_t rx_buffer[4] = {0x00, 0x00, 0x00, 0x00};

    cyhal_gpio_write(cs_pin, 0);
    cyhal_spi_transfer(spi_obj, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer), 0xFF);
    cyhal_gpio_write(cs_pin, 1);

    return rx_buffer[3];
}
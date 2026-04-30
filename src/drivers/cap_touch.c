/**
 * @file cap_touch.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-03-24
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "cap_touch.h"
#include "i2c.h"

/**
 * @brief
 * Read one byte from a FT6236 register.
 *
 * @param i2c_obj
 * I2C object used to access the capacitive touch controller.
 * @param reg_addr
 * FT6236 register address.
 * @param reg_value
 * Pointer where the register value is returned.
 * @return true
 * Register read succeeded.
 * @return false
 * Register read failed.
 */
bool cap_touch_read_register(cyhal_i2c_t *i2c_obj, uint8_t reg_addr, uint8_t *reg_value)
{
	if((i2c_obj == NULL) || (reg_value == NULL))
	{
		return false;
	}

	if(i2c_read_u8(i2c_obj, FT6X06_I2C_ADDR, reg_addr, reg_value) != CY_RSLT_SUCCESS)
	{
		return false;
	}

	return true;
}

/**
 * @brief
 * Write one byte to a FT6236 register.
 *
 * @param i2c_obj
 * I2C object used to access the capacitive touch controller.
 * @param reg_addr
 * FT6236 register address.
 * @param reg_value
 * Value to write.
 * @return true
 * Register write succeeded.
 * @return false
 * Register write failed.
 */
bool cap_touch_write_register(cyhal_i2c_t *i2c_obj, uint8_t reg_addr, uint8_t reg_value)
{
	if(i2c_obj == NULL)
	{
		return false;
	}

	if(i2c_write_u8(i2c_obj, FT6X06_I2C_ADDR, reg_addr, reg_value) != CY_RSLT_SUCCESS)
	{
		return false;
	}

	return true;
}

/**
 * @brief
 * Read the number of active touch points from TD_STATUS[3:0].
 *
 * @param i2c_obj
 * I2C object used to access the capacitive touch controller.
 * @param num_points
 * Pointer where the number of touch points is returned.
 * @return true
 * Read succeeded.
 * @return false
 * Read failed.
 */
bool cap_touch_get_num_points(cyhal_i2c_t *i2c_obj, uint8_t *num_points)
{
	uint8_t td_status = 0;

	if((i2c_obj == NULL) || (num_points == NULL))
	{
		return false;
	}

	if(!cap_touch_read_register(i2c_obj, FT6236_REG_TD_STATUS, &td_status))
	{
		return false;
	}

	/* TD_STATUS[3:0] reports the active number of touch points. */
	*num_points = td_status & FT6236_MASK_TOUCH_POINTS;

	/* Reject invalid point counts so idle bus values do not look like touches. */
	if(*num_points > FT6236_MAX_TOUCH_POINTS)
	{
		*num_points = 0;
		return false;
	}

	return true;
}

/**
 * @brief
 * Read the X/Y location of touch point 1 from registers P1_XH/P1_XL/P1_YH/P1_YL.
 *
 * @param i2c_obj
 * I2C object used to access the capacitive touch controller.
 * @param x_pos
 * Pointer where X position is returned.
 * @param y_pos
 * Pointer where Y position is returned.
 * @return true
 * Read succeeded.
 * @return false
 * Read failed.
 */
bool cap_touch_get_xy_position(cyhal_i2c_t *i2c_obj, uint16_t *x_pos, uint16_t *y_pos)
{
	uint8_t p1_xh = 0;
	uint8_t p1_xl = 0;
	uint8_t p1_yh = 0;
	uint8_t p1_yl = 0;
	uint8_t event_flag = 0;

	if((i2c_obj == NULL) || (x_pos == NULL) || (y_pos == NULL))
	{
		return false;
	}

	if(!cap_touch_read_register(i2c_obj, FT6236_REG_P1_XH, &p1_xh))
	{
		return false;
	}

	if(!cap_touch_read_register(i2c_obj, FT6236_REG_P1_XL, &p1_xl))
	{
		return false;
	}

	if(!cap_touch_read_register(i2c_obj, FT6236_REG_P1_YH, &p1_yh))
	{
		return false;
	}

	if(!cap_touch_read_register(i2c_obj, FT6236_REG_P1_YL, &p1_yl))
	{
		return false;
	}

	/* P1_XH[7:6] encodes the touch event; reserved value means invalid sample. */
	event_flag = (p1_xh & FT6236_MASK_TOUCH_EVENT) >> FT6236_SHIFT_TOUCH_EVENT;
	if(event_flag == FT6236_EVENT_RESERVED)
	{
		return false;
	}

	*x_pos = ((uint16_t)(p1_xh & FT6236_MASK_COORD_MSB_NIBBLE) << 8) | p1_xl;
	*y_pos = ((uint16_t)(p1_yh & FT6236_MASK_COORD_MSB_NIBBLE) << 8) | p1_yl;

	return true;
}
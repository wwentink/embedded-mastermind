#ifndef __CAP_TOUCH_H__
#define __CAP_TOUCH_H__

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "stdio.h"

#define FT6X06_I2C_ADDR                 0x38

//*****************************************************************************
// Fill out the #defines below to configure which pins are connected to
// the I2C Bus
//*****************************************************************************
#define FT6X06_FOCALTECH_ID_R         0xA8

/* FT6236 Working Mode Register Map */
#define FT6236_REG_DEV_MODE           0x00
#define FT6236_REG_GEST_ID            0x01
#define FT6236_REG_TD_STATUS          0x02
#define FT6236_REG_P1_XH              0x03
#define FT6236_REG_P1_XL              0x04
#define FT6236_REG_P1_YH              0x05
#define FT6236_REG_P1_YL              0x06

/* Bit masks used to decode FT6236 register values */
#define FT6236_MASK_TOUCH_POINTS      0x0F
#define FT6236_MASK_COORD_MSB_NIBBLE  0x0F
#define FT6236_MAX_TOUCH_POINTS       2
#define FT6236_MASK_TOUCH_EVENT       0xC0
#define FT6236_SHIFT_TOUCH_EVENT      6
#define FT6236_EVENT_RESERVED         0x03

bool cap_touch_read_register(cyhal_i2c_t *i2c_obj, uint8_t reg_addr, uint8_t *reg_value);
bool cap_touch_write_register(cyhal_i2c_t *i2c_obj, uint8_t reg_addr, uint8_t reg_value);
bool cap_touch_get_num_points(cyhal_i2c_t *i2c_obj, uint8_t *num_points);
bool cap_touch_get_xy_position(cyhal_i2c_t *i2c_obj, uint16_t *x_pos, uint16_t *y_pos);

#endif


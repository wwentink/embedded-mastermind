/**
 * @file master_mind.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-01-06
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "main.h"
#include "drivers.h"
#include "master_mind_lib.h"

/**
 * @brief 
 * This function will parse an LCD message and perform the appropriate action
 * @param msg 
 * @return true 
 * @return false 
 */
bool master_mind_handle_msg(lcd_msg_t* msg)
{
    static bool tile_size_logged = false;   // Used for tile size verification
    static uint16_t current_screen_bg = LCD_COLOR_BLACK;

    if (msg == NULL)
    {
        return false;
    }

    // Tile size verification
    if (!tile_size_logged)
    {
        printf("* Tile size: W=%d H=%d (pixels)\r\n", TILE_W, TILE_H);
        tile_size_logged = true;
    }

    // Handle the message based on its command
    switch (msg->command)
    {
        // Print a message to the text area
        case LCD_CMD_PRINT_MESSAGE:
        {
            // Starting position for upper left
            const char* str = msg->payload.message;
            int cx = 5;  // Left margin
            int cy = Consolas_20ptFontInfo.height / 2 + 10;  // Top margin
            
            // Draw each character
            for (int i = 0; str[i] != '\0'; i++)
            {
                char c = str[i];
                
                // Handle newline
                if (c == '\n')
                {
                    cx = 5;
                    cy += Consolas_20ptFontInfo.height;
                }
                // Handle space and printable characters
                else if (c == ' ')
                {
                    cx += Consolas_20ptFontInfo.space_width; // Advance by space width
                }
                else if (c >= Consolas_20ptFontInfo.start_char && c <= Consolas_20ptFontInfo.end_char)
                {
                    // Get character index and width
                    int char_index = c - Consolas_20ptFontInfo.start_char;
                    int char_width = Consolas_20ptFontInfo.char_info[char_index].width;
                    
                    // Draw the character
                    lcd_draw_image(
                        cx + char_width / 2,
                        cy,
                        char_width,
                        Consolas_20ptFontInfo.height,
                        Consolas_20ptBitmaps + Consolas_20ptFontInfo.char_info[char_index].offset,
                        (current_screen_bg == LCD_COLOR_BLACK) ? LCD_COLOR_WHITE : LCD_COLOR_BLACK,
                        current_screen_bg,
                        true
                    );
                    
                    // Advance x position
                    cx += char_width;
                }
            }
            
            return true;
        }
        
        // Draw a tile at the specified row and column
        case LCD_CMD_DRAW_TILE:
        {
            lcd_rect_t rect;
            lcd_tile_t* tile = &msg->payload.tile;
            
            // Get the rectangle dimensions for this tile
            if (!lcd_tile_rect(&rect, tile->row, tile->col))
            {
                return false;
            }
            
            // Draw the background rectangle
            lcd_draw_rectangle(
                rect.cx,
                rect.cy,
                rect.w,
                rect.h,
                tile->color_bg,
                true  // centered
            );
            
            // Draw the number image (0-7)
            if (tile->number <= 7)
            {
                lcd_draw_image(
                    rect.cx,
                    rect.cy,
                    FONT_CHAR_INFO_LARGE_NUMBERS[tile->number].width,
                    FONT_CHAR_INFO_LARGE_NUMBERS[tile->number].height,
                    FONT_NUM_LARGE_BITMAPS + FONT_CHAR_INFO_LARGE_NUMBERS[tile->number].offset,
                    tile->color_fg,
                    tile->color_bg,
                    true  // centered
                );
            }
            
            return true;
        }

        // Draw a tile with inverted colors at the specified row and column
        case LCD_CMD_DRAW_TILE_INVERTED:
        {
            lcd_rect_t rect;
            lcd_tile_t* tile = &msg->payload.tile;
            
            // Get the rectangle dimensions for this tile
            if (!lcd_tile_rect(&rect, tile->row, tile->col))
            {
                return false;
            }
            
            // Use a light gray for the selected tile background if the foreground is white
            uint16_t highlight_bg = tile->color_fg;
            if (highlight_bg == LCD_COLOR_WHITE) {
                highlight_bg = 0xC618; // Light gray RGB565
            }
            
            // Draw the background rectangle (inverted colors)
            lcd_draw_rectangle(
                rect.cx,
                rect.cy,
                rect.w,
                rect.h,
                highlight_bg,  // Swapped: foreground as background
                true  // centered
            );
            
            // Draw the number image with inverted colors (0-7)
            if (tile->number <= 7)
            {
                lcd_draw_image(
                    rect.cx,
                    rect.cy,
                    FONT_CHAR_INFO_LARGE_NUMBERS[tile->number].width,
                    FONT_CHAR_INFO_LARGE_NUMBERS[tile->number].height,
                    FONT_NUM_LARGE_BITMAPS + FONT_CHAR_INFO_LARGE_NUMBERS[tile->number].offset,
                    tile->color_bg,  // Swapped: background as foreground
                    highlight_bg,  // Swapped: foreground as background
                    true  // centered
                );
            }
            
            return true;
        }
        
        // Clear the entire screen
        case LCD_CMD_CLEAR_SCREEN:
            current_screen_bg = msg->payload.tile.color_bg;
            lcd_clear_screen(current_screen_bg);
            return true;
        
        // Print SW1 count at position (10, 50)
        case LCD_CMD_PRINT_SW1_COUNT:
        {
            const char* str = msg->payload.message;
            int cx = 10;  // Starting X position
            int cy = 50;  // Starting Y position
            
            // Draw each character
            for (int i = 0; str[i] != '\0'; i++)
            {
                char c = str[i];
                
                // Handle space and printable characters
                if (c == ' ')
                {
                    cx += Consolas_20ptFontInfo.space_width;
                }
                else if (c >= Consolas_20ptFontInfo.start_char && c <= Consolas_20ptFontInfo.end_char)
                {
                    // Get character index and width
                    int char_index = c - Consolas_20ptFontInfo.start_char;
                    int char_width = Consolas_20ptFontInfo.char_info[char_index].width;
                    
                    // Draw the character
                    lcd_draw_image(
                        cx + char_width / 2,
                        cy,
                        char_width,
                        Consolas_20ptFontInfo.height,
                        Consolas_20ptBitmaps + Consolas_20ptFontInfo.char_info[char_index].offset,
                        LCD_COLOR_WHITE,
                        LCD_COLOR_BLACK,
                        true
                    );
                    
                    // Advance x position
                    cx += char_width;
                }
            }
            
            return true;
        }
        
        // Print SW2 count at position (10, 100)
        case LCD_CMD_PRINT_SW2_COUNT:
        {
            const char* str = msg->payload.message;
            int cx = 10;  // Starting X position
            int cy = 100; // Starting Y position
            
            // Draw each character
            for (int i = 0; str[i] != '\0'; i++)
            {
                char c = str[i];
                
                // Handle space and printable characters
                if (c == ' ')
                {
                    cx += Consolas_20ptFontInfo.space_width;
                }
                else if (c >= Consolas_20ptFontInfo.start_char && c <= Consolas_20ptFontInfo.end_char)
                {
                    // Get character index and width
                    int char_index = c - Consolas_20ptFontInfo.start_char;
                    int char_width = Consolas_20ptFontInfo.char_info[char_index].width;
                    
                    // Draw the character
                    lcd_draw_image(
                        cx + char_width / 2,
                        cy,
                        char_width,
                        Consolas_20ptFontInfo.height,
                        Consolas_20ptBitmaps + Consolas_20ptFontInfo.char_info[char_index].offset,
                        LCD_COLOR_WHITE,
                        LCD_COLOR_BLACK,
                        true
                    );
                    
                    // Advance x position
                    cx += char_width;
                }
            }
            
            return true;
        }
            
        default:
            return false;
    }
}
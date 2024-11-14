#pragma once

#define LCD_PIN_NUM_MISO GPIO_NUM_19
#define LCD_PIN_NUM_MOSI GPIO_NUM_23
#define LCD_PIN_NUM_CLK  GPIO_NUM_18
#define LCD_PIN_NUM_CS   GPIO_NUM_5
#define LCD_PIN_NUM_DC   GPIO_NUM_2
#define LCD_PIN_NUM_BCKL GPIO_NUM_12

#define SCREEN_OFFSET_TOP 0
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define LCD_SCREEN_MARGIN_LEFT  20
#define LCD_SCREEN_MARGIN_RIGHT 20

void ili9341_init(void);
void ili9341_deinit(void);
void ili9341_writeLE(const uint16_t *buffer);
void ili9341_writeBE(const uint16_t *buffer);

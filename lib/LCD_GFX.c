/*
 * LCD_GFX.c
 *
 * Created: 9/20/2021 6:54:25 PM
 *  Author: You
 */ 

#include "LCD_GFX.h"
#include "ST7735.h"
/******************************************************************************
* Local Functions
******************************************************************************/
static void drawPixelSafe(int16_t x, int16_t y, uint16_t color)
{
    if (x >= 0 && x < LCD_WIDTH &&
        y >= 0 && y < LCD_HEIGHT)
    {
        LCD_drawPixel((uint8_t)x, (uint8_t)y, color);
    }
}


/******************************************************************************
* Global Functions
******************************************************************************/

/**************************************************************************//**
* @fn			uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
* @brief		Convert RGB888 value to RGB565 16-bit color data
* @note
*****************************************************************************/
uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
	return ((((31*(red+4))/255)<<11) | (((63*(green+2))/255)<<5) | ((31*(blue+4))/255));
}

/**************************************************************************//**
* @fn			void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color)
* @brief		Draw a single pixel of 16-bit rgb565 color to the x & y coordinate
* @note
*****************************************************************************/
void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color) {
	LCD_setAddr(x,y,x,y);
	SPI_ControllerTx_16bit(color);
}

/**************************************************************************//**
* @fn			void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor)
* @brief		Draw a character starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor){
	uint16_t row = character - 0x20;		//Determine row of ASCII table starting at space
	int i, j;
	if ((uint16_t)x + 5 <= LCD_WIDTH &&
    (uint16_t)y + 8 <= LCD_HEIGHT){
		for(i=0;i<5;i++){
			uint8_t pixels = ASCII[row][i]; //Go through the list of pixels
			for(j=0;j<8;j++){
				if ((pixels>>j)&1==1){
					LCD_drawPixel(x+i,y+j,fColor);
				}
				else {
					LCD_drawPixel(x+i,y+j,bColor);
				}
			}
		}
	}
}


/******************************************************************************
* LAB 4 TO DO. COMPLETE THE FUNCTIONS BELOW.
* You are free to create and add any additional files, libraries, and/or
*  helper function. All code must be authentically yours.
******************************************************************************/

/**************************************************************************//**
* @fn			void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
* @brief		Draw a colored circle of set radius at coordinates
* @note
*****************************************************************************/
void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
{
	// Fill this out

    int16_t x = 0;
    int16_t y = radius;
    int16_t d = 1 - radius;

    while (x <= y)
    {
        drawPixelSafe(x0 + x, y0 + y, color);
        drawPixelSafe(x0 - x, y0 + y, color);
        drawPixelSafe(x0 + x, y0 - y, color);
        drawPixelSafe(x0 - x, y0 - y, color);

        drawPixelSafe(x0 + y, y0 + x, color);
        drawPixelSafe(x0 - y, y0 + x, color);
        drawPixelSafe(x0 + y, y0 - x, color);
        drawPixelSafe(x0 - y, y0 - x, color);

        x++;

        if (d < 0)
        {
            d += 2 * x + 1;
        }
        else
        {
            y--;
            d += 2 * (x - y) + 1;
        }
    }
}



/**************************************************************************//**
* @fn			void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
* @brief		Draw a line from and to a point with a color
* @note
*****************************************************************************/
void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
{
	// Fill this out
 
    int32_t dx = (x1 > x0) ? (int32_t)x1 - x0 : (int32_t)x0 - x1;
    int32_t dy = (y1 > y0) ? (int32_t)y1 - y0 : (int32_t)y0 - y1;

    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;

    int32_t err = dx - dy;

    while (1)
    {
        drawPixelSafe(x0, y0, c);

        if (x0 == x1 && y0 == y1)
            break;

        int32_t e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}




/**************************************************************************//**
* @fn			void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
* @brief		Draw a colored block at coordinates
* @note
*****************************************************************************/
void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
{
	// Fill this out
    if (x0 > x1 || y0 > y1)
        return;

    if (x0 >= LCD_WIDTH || y0 >= LCD_HEIGHT)
        return;

    if (x1 >= LCD_WIDTH)
        x1 = LCD_WIDTH - 1;

    if (y1 >= LCD_HEIGHT)
        y1 = LCD_HEIGHT - 1;

    LCD_setAddr(x0, y0, x1, y1);

    uint32_t numPixels =
        (uint32_t)(x1 - x0 + 1) *
        (uint32_t)(y1 - y0 + 1);

    clear(LCD_PORT, LCD_TFT_CS);

    for (uint32_t i = 0; i < numPixels; i++)
    {
        SPI_ControllerTx_16bit_stream(color);
    }

    set(LCD_PORT, LCD_TFT_CS);
}


/**************************************************************************//**
* @fn			void LCD_setScreen(uint16_t color)
* @brief		Draw the entire screen to a color
* @note
*****************************************************************************/
void LCD_setScreen(uint16_t color) 
{
	// Fill this out

    LCD_drawBlock(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, color);
}


/**************************************************************************//**
* @fn			void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
* @brief		Draw a string starting at the point with foreground and background colors
* @note
*****************************************************************************/

void LCD_drawString(uint8_t x, uint8_t y,
                    char* str, uint16_t fg, uint16_t bg)
{
    uint16_t cursorX = x;

    if (str == 0)
        return;

    // Character size: 5x8, plus 1 pixel spacing
    if ((uint16_t)y + 8 > LCD_HEIGHT)
        return;

    while (*str != '\0')
    {
        if (cursorX + 7 >= LCD_WIDTH)
            break;

        LCD_drawChar((uint8_t)cursorX, y,
                     (uint8_t)*str, fg, bg);

        cursorX += 6;
        str++;
    }
}


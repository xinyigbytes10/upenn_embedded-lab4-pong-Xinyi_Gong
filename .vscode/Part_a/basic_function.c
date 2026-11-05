
#include <avr/io.h>
#include "ST7735.h"
#include "LCD_GFX.h"


int main(void)
{
    lcd_init();

    // 1. Clear screen first
    LCD_setScreen(BLACK);

    // 2. Draw rectangle
    LCD_drawBlock(10, 15, 50, 35, RED);

    // 3. Draw line
    LCD_drawLine(10, 45, 100, 45, GREEN);

    // 4. Draw circle
    LCD_drawCircle(65, 85, 20, BLUE);

    // 5. Draw string LAST
    LCD_drawString(10, 115, "HELLO", WHITE, BLACK);

    while (1)
    {
    }

    return 0;
}

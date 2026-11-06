#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "ST7735.h"
#include "LCD_GFX.h"

/*
 * ESE 5190 Lab 4 Part E - Level III - Blynk leaderboard
 * MCU: ATmega328PB Xplained Mini
 *
 * Joystick Y: PC0 / ADC0 (retained for optional joystick mode)
 * ESP32 UP via level shifter: PD2 (input)
 * ESP32 DOWN via level shifter: PD3 (input)
 * Match result TX: PD1 / USART0 -> level shifter HV3/LV3 -> ESP32 GPIO10 (D10)
 * Player LED: PC1
 * Computer LED: PC2
 * Buzzer: PD4
 * Motor PWM: PD5 / OC0B / Timer0
 * LCD Backlight: PD6
 *
 * LCD uses existing ST7735 SPI wiring.
 * Motor power and MCU must share ground.
 */

#define W LCD_WIDTH
#define H LCD_HEIGHT
#define TOP 16

#define PADDLE_W 4
#define PADDLE_H 22

#define BALL_RADIUS 3
#define BALL_SIZE (2 * BALL_RADIUS + 1)

#define WIN_SCORE 5

#define PLAYER_X 3
#define CPU_X (W - PADDLE_W - 3)

#define PLAYER_LED PC1
#define CPU_LED PC2
#define BUZZER_PIN PD4
#define MOTOR_PIN PD5
#define LCD_BACKLIGHT_PIN PD6

// 1 = Blynk ESP32 wireless control; 0 = original Part B joystick control.
#define USE_WIRELESS_CONTROL 1

// ATmega328PB pins for ESP32 signals after the 3.3V-to-5V level shifter.
// IMPORTANT: Verify PD2/PD3 are not already used by your LCD wiring!
#define REMOTE_UP_PIN PD2
#define REMOTE_DOWN_PIN PD3
#define REMOTE_MOVE_STEP 3

// 0 = passive buzzer; 1 = active buzzer
#define BUZZER_ACTIVE 1

// 1 = two startup test beeps
#define BUZZER_STARTUP_TEST 1

#define MOTOR_START_DUTY 160
#define MOTOR_MAX_DUTY 240
#define MOTOR_STEP 5

static int16_t playerY, cpuY;
static int16_t ballX, ballY;

static int8_t ballDX, ballDY;

static uint8_t playerScore, cpuScore;

// Level II: select difficulty with the existing joystick at boot.
// 0=EASY (slow CPU), 1=NORMAL (original), 2=HARD (fast CPU).
static uint8_t difficulty = 1;
static uint8_t aiTick = 0;

static uint16_t randomState = 0xACE1;

// ====================================
// LEVEL III: Send completed match to ESP32 using USART0 (9600 baud).
// PD1 (TXD0) = ATmega 5V signal; use level shifter HV3 -> LV3
// -> ESP32-S3 GPIO10 / D10 (3.3V input). Never wire 5V directly.
// Frame: R,playerScore,cpuScore,difficulty (newline terminated)
// Difficulty: 0=EASY, 1=NORMAL, 2=HARD.
// ====================================

static void result_uart_init(void)
{
    // ATmega328PB USART0, asynchronous 8N1, 9600 baud @ 16MHz.
    UBRR0H = 0;
    UBRR0L = 103;
    UCSR0A = 0;
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    UCSR0B = (1 << TXEN0);
}

static void result_uart_char(char c)
{
    while (!(UCSR0A & (1 << UDRE0))) { }
    UDR0 = (uint8_t)c;
}

static void result_uart_digit(uint8_t value)
{
    result_uart_char((char)('0' + value));
}

static void sendMatchResult(void)
{
    // WIN_SCORE=5, so both scores are one digit (0..5).
    result_uart_char('R');
    result_uart_char(',');
    result_uart_digit(playerScore);
    result_uart_char(',');
    result_uart_digit(cpuScore);
    result_uart_char(',');
    result_uart_digit(difficulty);
    result_uart_char('\n');
    // Wait for the final frame to completely leave the transmitter.
    while (!(UCSR0A & (1 << TXC0))) { }
    UCSR0A |= (1 << TXC0);
}

// ====================================
// Random number generator
// ====================================

static uint16_t random16(void)
{
    randomState ^= (uint16_t)(randomState << 7);
    randomState ^= (randomState >> 9);
    randomState ^= (uint16_t)(randomState << 8);

    return randomState;
}

// ====================================
// Joystick ADC0 / PC0
// ====================================

static void adc_init(void)
{
    DDRC &= ~(1 << PC0);
    PORTC &= ~(1 << PC0);

    DIDR0 |= (1 << ADC0D);

    // AVCC reference, ADC0
    ADMUX = (1 << REFS0);

    // ADC enabled, prescaler 128
    ADCSRA = (1 << ADEN) |
             (1 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);
}

// Time-limited ADC read: never allow joystick conversion to freeze the game.
// If ADC does not complete, use center (NORMAL) as a safe fallback.
static uint16_t readJoystick(void)
{
    ADMUX = (1 << REFS0);
    ADCSRA |= (1 << ADSC);

    uint16_t timeout = 60000U;
    while ((ADCSRA & (1 << ADSC)) && timeout > 0U)
    {
        --timeout;
    }

    if (ADCSRA & (1 << ADSC))
    {
        // Reinitialize ADC to allow later attempts, and default to center.
        adc_init();
        return 512U;
    }
    return ADC;
}

// ====================================
// Wireless GPIO inputs from ESP32 through the level shifter
// ====================================

static void wireless_init(void)
{
    // PD2 and PD3 are externally driven digital inputs.
    DDRD &= ~((1 << REMOTE_UP_PIN) | (1 << REMOTE_DOWN_PIN));
    // No internal pull-ups; low means not pressed.
    PORTD &= ~((1 << REMOTE_UP_PIN) | (1 << REMOTE_DOWN_PIN));
}

// ====================================
// Motor PWM - Timer0 / PD5
// ====================================

static void motor_setDuty(uint8_t duty)
{
    if (duty == 0)
    {
        // Disconnect PWM and force OFF
        TCCR0A &= ~(1 << COM0B1);

        OCR0B = 0;

        PORTD &= ~(1 << MOTOR_PIN);
    }
    else
    {
        OCR0B = duty;

        // Enable non-inverting OC0B PWM
        TCCR0A |= (1 << COM0B1);
    }
}

static void motor_init(void)
{
    DDRD |= (1 << MOTOR_PIN);
    PORTD &= ~(1 << MOTOR_PIN);

    TCCR0A = 0;
    TCCR0B = 0;
    TCNT0 = 0;
    OCR0B = 0;

    // Fast PWM, TOP = 255
    TCCR0A = (1 << WGM01) |
             (1 << WGM00);

    // Prescaler = 64
    // PWM frequency ~977 Hz at 16MHz
    TCCR0B = (1 << CS01) |
             (1 << CS00);

    motor_setDuty(0);
}

static void motorAnimation(void)
{
    motor_setDuty(0);
    _delay_ms(300);

    // Startup kick
    motor_setDuty(MOTOR_MAX_DUTY);
    _delay_ms(350);

    motor_setDuty(MOTOR_START_DUTY);
    _delay_ms(200);

    // Gradual acceleration
    for (uint16_t d = MOTOR_START_DUTY;
         d <= MOTOR_MAX_DUTY;
         d += MOTOR_STEP)
    {
        motor_setDuty((uint8_t)d);
        _delay_ms(80);
    }

    // Hold high speed
    motor_setDuty(MOTOR_MAX_DUTY);
    _delay_ms(800);

    // Gradual deceleration to zero
    for (int16_t d = MOTOR_MAX_DUTY;
         d >= 0;
         d -= MOTOR_STEP)
    {
        motor_setDuty((uint8_t)d);
        _delay_ms(60);
    }

    // Completely stop motor
    motor_setDuty(0);
    _delay_ms(600);
}

// ====================================
// Buzzer - PD4
// ====================================

static void beep(uint16_t durationMs)
{
#if BUZZER_ACTIVE

    // Active buzzer
    PORTD |= (1 << BUZZER_PIN);

    while (durationMs--)
    {
        _delay_ms(1);
    }

    PORTD &= ~(1 << BUZZER_PIN);

#else

    // Passive buzzer
    // ~2 kHz, 50% duty cycle
    uint32_t cycles =
        (uint32_t)durationMs * 2UL;

    for (uint32_t i = 0; i < cycles; i++)
    {
        PORTD |= (1 << BUZZER_PIN);
        _delay_us(250);

        PORTD &= ~(1 << BUZZER_PIN);
        _delay_us(250);
    }

    PORTD &= ~(1 << BUZZER_PIN);

#endif
}

// ====================================
// Hardware initialization
// ====================================

static void hardware_init(void)
{
    // Score LEDs
    DDRC |= (1 << PLAYER_LED) |
            (1 << CPU_LED);

    PORTC &= ~((1 << PLAYER_LED) |
               (1 << CPU_LED));

    // Buzzer output
    DDRD |= (1 << BUZZER_PIN);
    PORTD &= ~(1 << BUZZER_PIN);

    adc_init();
    motor_init();

#if USE_WIRELESS_CONTROL
    wireless_init();
#endif

    // LCD Backlight
    DDRD |= (1 << LCD_BACKLIGHT_PIN);
    PORTD |= (1 << LCD_BACKLIGHT_PIN);
}

// ====================================
// LCD Graphics
// ====================================

static void drawPaddle(
    uint8_t x,
    int16_t y,
    uint16_t color)
{
    LCD_drawBlock(
        x,
        (uint8_t)y,
        (uint8_t)(x + PADDLE_W - 1),
        (uint8_t)(y + PADDLE_H - 1),
        color
    );
}

static void drawBall(uint16_t color)
{
    // Rounded 7x7 ball
    if (ballX < 0 || ballY < TOP ||
        ballX + BALL_SIZE > W ||
        ballY + BALL_SIZE > H)
    {
        return;
    }

    uint8_t x = (uint8_t)ballX;
    uint8_t y = (uint8_t)ballY;

    // Top row
    LCD_drawBlock(
        x + 2, y,
        x + BALL_SIZE - 3, y,
        color
    );

    // Center region
    LCD_drawBlock(
        x, y + 1,
        x + BALL_SIZE - 1,
        y + BALL_SIZE - 2,
        color
    );

    // Bottom row
    LCD_drawBlock(
        x + 2, y + BALL_SIZE - 1,
        x + BALL_SIZE - 3,
        y + BALL_SIZE - 1,
        color
    );
}

static void drawScore(void)
{
    char scoreText[] = "P:0 C:0";

    scoreText[2] =
        (char)('0' + playerScore);

    scoreText[6] =
        (char)('0' + cpuScore);

    LCD_drawBlock(
        0, 0, W - 1, TOP - 1, BLACK
    );

    LCD_drawString(
        5, 3,
        scoreText,
        WHITE, BLACK
    );

    // Display the selected difficulty on the score bar.
    if (difficulty == 0)
        LCD_drawString(73, 3, "EASY", GREEN, BLACK);
    else if (difficulty == 1)
        LCD_drawString(73, 3, "NORM", WHITE, BLACK);
    else
        LCD_drawString(73, 3, "HARD", RED, BLACK);
}

// ====================================
// Ball initialization
// ====================================

static void resetBall(void)
{
    // Start at screen center
    ballX = (W - BALL_SIZE) / 2;
    ballY = (TOP + H - BALL_SIZE) / 2;

    // Random speeds: 1 or 2 pixels/frame
    int8_t speedX =
        (int8_t)(1 + (random16() & 1U));

    int8_t speedY =
        (int8_t)(1 + (random16() & 1U));

    // Random X and Y directions
    ballDX = (random16() & 1U)
             ? speedX : -speedX;

    ballDY = (random16() & 1U)
             ? speedY : -speedY;
}

// ====================================
// Reset game
// ====================================

static void redrawAll(void)
{
    LCD_setScreen(BLACK);

    drawScore();

    drawPaddle(
        PLAYER_X, playerY, GREEN
    );

    drawPaddle(
        CPU_X, cpuY, RED
    );

    drawBall(WHITE);
}

static void resetGame(void)
{
    playerScore = 0;
    cpuScore = 0;

    playerY =
        (TOP + H - PADDLE_H) / 2;

    cpuY = playerY;

    resetBall();
    redrawAll();
}

// ====================================
// Player paddle - Blynk wireless GPIO (or joystick fallback)
// ====================================

static void updatePlayer(void)
{
    int16_t oldY = playerY;

#if USE_WIRELESS_CONTROL
    // ESP32 GPIO5 -> LV1/HV1 -> PD2: UP
    // ESP32 GPIO6 -> LV2/HV2 -> PD3: DOWN
    uint8_t inputs = PIND;
    uint8_t up = (inputs & (1 << REMOTE_UP_PIN)) != 0;
    uint8_t down = (inputs & (1 << REMOTE_DOWN_PIN)) != 0;

    if (up && !down)
    {
        playerY -= REMOTE_MOVE_STEP;
    }
    else if (down && !up)
    {
        playerY += REMOTE_MOVE_STEP;
    }
    // If neither or both buttons are active, hold position.
#else
    // Original Part B joystick control (PC0 / ADC0)
    uint16_t adc = readJoystick();

    if (adc < 400)
    {
        playerY -= 3;
    }
    else if (adc > 624)
    {
        playerY += 3;
    }
#endif

    // Keep paddle inside the playing field.
    if (playerY < TOP)
    {
        playerY = TOP;
    }

    if (playerY > H - PADDLE_H)
    {
        playerY = H - PADDLE_H;
    }

    if (oldY != playerY)
    {
        drawPaddle(
            PLAYER_X, oldY, BLACK
        );

        drawPaddle(
            PLAYER_X, playerY, GREEN
        );
    }
}

// ====================================
// PART E / LEVEL II: select CPU difficulty at startup
// Uses the existing joystick PC0; ESP32 connections are unchanged.
// ====================================

// Four-second joystick selection with live feedback and a countdown.
// Release to center for NORMAL; hold UP/DOWN through the last second
// to confirm EASY/HARD. No extra button or wiring is required.
static void chooseDifficulty(void)
{
    LCD_setScreen(BLACK);
    LCD_drawString(4, 20, "SELECT DIFFICULTY", WHITE, BLACK);
    LCD_drawString(4, 36, "JOY UP: EASY", GREEN, BLACK);
    LCD_drawString(4, 46, "CENTER: NORMAL", WHITE, BLACK);
    LCD_drawString(4, 56, "JOY DOWN: HARD", RED, BLACK);

    difficulty = 1;
    uint8_t previous = 255U; // Force first label redraw.

    // 40 samples x 100ms = ~4 seconds, plus short LCD drawing time.
    for (uint8_t i = 0; i < 40U; ++i)
    {
        uint16_t adc = readJoystick();
        if (adc < 400U)
            difficulty = 0; // EASY
        else if (adc > 624U)
            difficulty = 2; // HARD
        else
            difficulty = 1; // NORMAL

        // Only redraw the difficulty line when the selection changes.
        if (difficulty != previous)
        {
            LCD_drawBlock(0, 70, W - 1, 84, BLACK);
            if (difficulty == 0)
                LCD_drawString(4, 72, "CHOSEN: EASY", GREEN, BLACK);
            else if (difficulty == 2)
                LCD_drawString(4, 72, "CHOSEN: HARD", RED, BLACK);
            else
                LCD_drawString(4, 72, "CHOSEN: NORM", WHITE, BLACK);
            previous = difficulty;
        }

        // Draw 4, 3, 2, 1 seconds remaining; redraw only once/second.
        if ((i % 10U) == 0U)
        {
            char countdown[] = "START IN: 4s";
            countdown[10] = (char)('4' - (i / 10U));
            LCD_drawBlock(0, 90, W - 1, 106, BLACK);
            LCD_drawString(4, 92, countdown, WHITE, BLACK);
        }
        _delay_ms(100);
    }

    LCD_setScreen(BLACK);
    LCD_drawString(8, 45, "STARTING PONG", WHITE, BLACK);
    _delay_ms(250);
    aiTick = 0;
}

// ====================================
// ====================================
// Computer paddle
// ====================================

static void updateComputer(void)
{
    int16_t oldY = cpuY;

    int16_t target =
        ballY + BALL_SIZE / 2;

    // EASY: move 1 pixel every 3 frames (much slower AI)
    // NORMAL: original Part B, 1 pixel every frame
    // HARD: up to 3 pixels per frame
    uint8_t step = 1;
    if (difficulty == 0)
    {
        aiTick++;
        if (aiTick < 3)
            step = 0;
        else
            aiTick = 0;
    }
    else if (difficulty == 2)
    {
        step = 3;
    }

    int16_t error = target - (cpuY + PADDLE_H / 2);
    if (error > 0)
    {
        if (error < step)
            step = (uint8_t)error;
        cpuY += step;
    }
    else if (error < 0)
    {
        if (-error < step)
            step = (uint8_t)(-error);
        cpuY -= step;
    }

    if (cpuY < TOP)
    {
        cpuY = TOP;
    }

    if (cpuY > H - PADDLE_H)
    {
        cpuY = H - PADDLE_H;
    }

    if (oldY != cpuY)
    {
        drawPaddle(
            CPU_X, oldY, BLACK
        );

        drawPaddle(
            CPU_X, cpuY, RED
        );
    }
}

// ====================================
// PART E / LEVEL I: score & match LED animations
// PC1 = player LED; PC2 = computer LED
// ====================================

static void scoreLEDAnimation(uint8_t playerWon)
{
    // Blink the scoring player's LED three times.
    uint8_t pin = playerWon ? PLAYER_LED : CPU_LED;
    PORTC &= ~((1 << PLAYER_LED) | (1 << CPU_LED));
    for (uint8_t i = 0; i < 3; ++i)
    {
        PORTC |= (1 << pin);
        _delay_ms(100);
        PORTC &= ~(1 << pin);
        _delay_ms(70);
    }
}

static void matchLEDAnimation(void)
{
    // Winner celebration: chase between the two LEDs,
    // then blink both together. The game is paused while animating.
    for (uint8_t i = 0; i < 8; ++i)
    {
        if (i & 1U)
        {
            PORTC |= (1 << PLAYER_LED);
            PORTC &= ~(1 << CPU_LED);
        }
        else
        {
            PORTC |= (1 << CPU_LED);
            PORTC &= ~(1 << PLAYER_LED);
        }
        _delay_ms(100);
    }

    PORTC &= ~((1 << PLAYER_LED) | (1 << CPU_LED));
    for (uint8_t i = 0; i < 3; ++i)
    {
        PORTC |= (1 << PLAYER_LED) | (1 << CPU_LED);
        _delay_ms(140);
        PORTC &= ~((1 << PLAYER_LED) | (1 << CPU_LED));
        _delay_ms(100);
    }
}

static void showWinner(uint8_t playerWon)
{
    LCD_setScreen(BLACK);
    if (playerWon)
    {
        LCD_drawString(14, H / 2 - 10, "PLAYER WINS!", GREEN, BLACK);
    }
    else
    {
        LCD_drawString(14, H / 2 - 10, "CPU WINS!", RED, BLACK);
    }
    LCD_drawString(14, H / 2 + 8, "GAME OVER", WHITE, BLACK);
}

// ====================================
// Scoring
// ====================================

static void scorePoint(uint8_t playerWon)
{
    if (playerWon)
    {
        playerScore++;
    }
    else
    {
        cpuScore++;
    }

    // Brief sound and a visually distinct three-pulse point animation.
    beep(80);
    scoreLEDAnimation(playerWon);
    drawScore();

    if (playerScore >= WIN_SCORE ||
        cpuScore >= WIN_SCORE)
    {
        // Upload the result before the score gets reset.
        sendMatchResult();
        showWinner(playerWon);
        matchLEDAnimation();
        _delay_ms(350);

        // Keep the original Part B win motor animation.
        motorAnimation();
        resetGame();
    }
    else
    {
        resetBall();
        redrawAll();
    }
}

// ====================================
// Ball movement and collisions
// ====================================

static void updateBall(void)
{
    // Erase previous ball
    drawBall(BLACK);

    // Move ball
    ballX += ballDX;
    ballY += ballDY;

    // Top wall collision
    if (ballY <= TOP)
    {
        ballY = TOP;

        // Reverse Y, preserve speed
        ballDY = -ballDY;

        beep(15);
    }

    // Bottom wall collision
    else if (ballY >= H - BALL_SIZE)
    {
        ballY = H - BALL_SIZE;

        ballDY = -ballDY;

        beep(15);
    }

    // Player paddle collision
    if (ballDX < 0 &&
        ballX <= PLAYER_X + PADDLE_W &&
        ballX + BALL_SIZE >= PLAYER_X &&
        ballY + BALL_SIZE > playerY &&
        ballY < playerY + PADDLE_H)
    {
        ballX = PLAYER_X + PADDLE_W;

        // Reverse X, preserve speed
        ballDX = -ballDX;

        beep(25);
    }

    // Computer paddle collision
    if (ballDX > 0 &&
        ballX + BALL_SIZE >= CPU_X &&
        ballX <= CPU_X + PADDLE_W &&
        ballY + BALL_SIZE > cpuY &&
        ballY < cpuY + PADDLE_H)
    {
        ballX = CPU_X - BALL_SIZE;

        ballDX = -ballDX;

        beep(25);
    }

    // Player misses ball
    if (ballX < 0)
    {
        scorePoint(0);
        return;
    }

    // Computer misses ball
    if (ballX > W - BALL_SIZE)
    {
        scorePoint(1);
        return;
    }

    // Restore paddle graphics
    if (ballX <= PLAYER_X +
                 PADDLE_W + BALL_SIZE)
    {
        drawPaddle(
            PLAYER_X, playerY, GREEN
        );
    }

    if (ballX >= CPU_X - BALL_SIZE)
    {
        drawPaddle(
            CPU_X, cpuY, RED
        );
    }

    drawBall(WHITE);
}

// ====================================
// Main
// ====================================

int main(void)
{
    // Motor OFF at startup
    DDRD |= (1 << MOTOR_PIN);
    PORTD &= ~(1 << MOTOR_PIN);

    // Initialize LCD and SPI
    lcd_init();

    // Initialize all other peripherals
    hardware_init();
    result_uart_init();

    // LCD Backlight on PD6
    DDRD |= (1 << LCD_BACKLIGHT_PIN);
    PORTD |= (1 << LCD_BACKLIGHT_PIN);

#if BUZZER_STARTUP_TEST
    beep(200);
    _delay_ms(120);
    beep(200);
#endif

    // Seed pseudo-random generator
    randomState ^=
        (uint16_t)(readJoystick() << 4);

    randomState ^= (uint16_t)TCNT0;

    if (randomState == 0)
    {
        randomState = 0xACE1;
    }

    // LEVEL II: joystick selects EASY/NORMAL/HARD on startup.
    chooseDifficulty();

    // Start Pong: resetGame() clears the selection screen.
    resetGame();

    while (1)
    {
        updatePlayer();
        updateComputer();
        updateBall();

        _delay_ms(30);
    }
}

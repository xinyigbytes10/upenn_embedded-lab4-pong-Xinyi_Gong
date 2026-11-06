# Lab 4 Pong

**GitHub Repository URL:** https://github.com/xinyigbytes10/upenn_embedded-lab4-pong-Xinyi_Gong

| Team Member Name | Email Address       | GitHub Handle       |
|------------------|---------------------|---------------------|
| [Xinyi Gong]         | [xinyig10@engineering.upenn.edu]           | [xinyigbytes10]          |

## PART A:

PIN configuration: 

Green is CS,
Blue is SCK,
Yellow is DC,
Purple is data/MOSI

Thee image shows the code for make one white pixel. The screenshot of Saleae Logic Analyzer shows the capatured result. 
![](image\PART_A_TESTCODE.png)
![](image\part_a.png)

The logic analyzer captures the SPI communication between the ATmega328PB and the ST7735 LCD while attempting to draw a white pixel at (0, 0). Channel 1 is Chip Select (CS), which remains LOW during transmission. Channel 0 is Data/Command (D/C), which is LOW for commands and HIGH for data. Channel 2 provides the SPI clock, while Channel 3 carries data through MOSI.

The first command, 0x2A (CASET), sets both the starting and ending column addresses to zero. The second command, 0x2B (RASET), sets both row addresses to zero, selecting a single pixel at (0, 0). Finally, 0x2C (RAMWR) initiates the pixel memory write.

The capture confirms that the LCD receives the pixel address commands correctly. However, the white pixel data (0xFF 0xFF in RGB565 format) is not visible in this capture.

![](image\Part_a_annotated.png)
## Part E: Take It Further

For Part E, we implemented **one feature from each required level** while preserving the original Pong game and the wireless paddle control from Part D. The three C files are cumulative: Level II includes Level I, and Level III includes both Levels I and II.

| Level | Selected requirement | Implementation | Source file |
| --- | --- | --- | --- |
| **Level I** | Interesting LED animations when a point or game is won | Different scoring and match-winning LED patterns | `PartE_LevelI_LED_Celebrations.c` |
| **Level II** | Different levels of difficulty | Joystick-selectable Easy, Normal, and Hard computer opponents | `PartE_LevelII_Difficulty.c` |
|

### Level I — Score and Win LED Animations

**Requirement:** Add interesting LED animations when a player wins a point and when the game is won.

We extended the existing score-indicator LEDs (Player: **PC1**, Computer: **PC2**) with two distinct animations:

- **Point scored:** The LED belonging to the player who scored flashes **three times** (100 ms on, 70 ms off per flash in the provided source).
- **Game won:** When either player reaches **5 points**, the two LEDs alternate for **8 steps**, then flash **together 3 times**.
- At the end of a match, the LCD also displays a **PLAYER WINS!** or **CPU WINS!** message, followed by the original motor animation before the game resets.

The animations are implemented in `scoreLEDAnimation()` and `matchLEDAnimation()`, called from `scorePoint()`. They use the existing LEDs and require **no additional hardware**.

**Demo:** Play until a point is scored to see the three flashes. Continue until a player reaches five points to see the longer celebration pattern.

### Level II — Selectable Difficulty

**Requirement:** Add different levels of difficulty.

We added **three difficulty settings** for the computer-controlled paddle. At startup, the LCD displays a selection screen and the **existing joystick (PC0/ADC0)** sets the difficulty:

| Joystick position at selection | Difficulty | Computer paddle behavior |
| --- | --- | --- |
| Up (ADC < 400) | **Easy** | Moves 1 pixel every 3 game frames |
| Center (ADC 400–624) | **Normal** | Moves 1 pixel every game frame (original Part B behavior) |
| Down (ADC > 624) | **Hard** | Moves up to 3 pixels every game frame |

The selection screen stays active for approximately **4 seconds** and then the game starts automatically. The most recently read joystick position determines the difficulty; hold the joystick in the desired position until gameplay begins. The selected difficulty remains in effect until the board is reset.

The new logic is implemented in `chooseDifficulty()` and the modified `updateComputer()`. This version **also includes the Level I LED animations**.

**Demo:** Restart the ATmega328PB, hold the joystick up, centered, or down during the selection screen, and compare how quickly the computer paddle follows the ball.

### Level III — Blynk Digital Leaderboard

**Requirement:** Add username input and send game result data back through Blynk to create a digital leaderboard.

We extended the wireless Pong system by implementing a **username-based digital leaderboard** using the Blynk IoT platform.

The feature allows players to enter their usernames through the Blynk mobile application and automatically records match results when a game ends.


## Part B: 
Pong game displayed on the ST7735 LCD, with a player paddle, computer paddle, ball collisions, score display, buzzer feedback, score LEDs, and a motor animation after a match is won.

## Part D:
 Wireless player-paddle control using the **Blynk mobile app → ESP32-S3 → logic level shifter → ATmega328PB**. The **UP** button uses Blynk virtual pin **V0** and ESP32 **GPIO 5**, which is read at ATmega **PD2**. The **DOWN** button uses **V1**, ESP32 **GPIO 6**, and ATmega **PD3**.
- The default setting `#define USE_WIRELESS_CONTROL 1` enables the wireless buttons. Changing it to `0` and rebuilding enables the original joystick paddle control instead. In Levels II and III, the joystick is still used for the startup difficulty selection, even when wireless paddle control is enabled.





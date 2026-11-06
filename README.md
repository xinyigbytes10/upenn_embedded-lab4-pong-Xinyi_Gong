# Lab 4 Pong

**GitHub Repository URL:**

| Team Member Name | Email Address       | GitHub Handle       |
|------------------|---------------------|---------------------|
| [Xinyi Gong]         | [xinyig10@engineering.upenn.edu]           | [xinyigbytes10]          |




## Part E: Take It Further

For Part E, we implemented **one feature from each required level** while preserving the original Pong game and the wireless paddle control from Part D. The three C files are cumulative: Level II includes Level I, and Level III includes both Levels I and II.

| Level | Selected requirement | Implementation | Source file |
| --- | --- | --- | --- |
| **Level I** | Interesting LED animations when a point or game is won | Different scoring and match-winning LED patterns | `PartE_LevelI_LED_Celebrations.c` |
| **Level II** | Different levels of difficulty | Joystick-selectable Easy, Normal, and Hard computer opponents | `PartE_LevelII_Difficulty.c` |
| **Level III** | Random distractions during gameplay | Random LED flashes and buzzer sounds during a match | `PartE_LevelIII_AllFeatures.c` |

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

### Level III — Random Player Distractions

**Requirement:** Add distractions that turn on randomly during gameplay, such as flashing lights or other effects.

We used the **two existing LEDs and the active buzzer** to create occasional distractions during the Pong match:

- At randomized intervals of roughly **7–13 seconds of gameplay**, a distraction event begins.
- During each event (approximately **0.9 seconds**), both LEDs flash and the buzzer pulses in an on/off pattern.
- The effect is updated through `updateDistraction()` once per game frame, allowing the Pong game loop to continue updating instead of pausing for the full event.
- `stopDistraction()` turns the outputs off before scoring and victory animations so that those events take priority.

No additional sensors or external hardware are needed for this option. **The Level III file is the final combined version**, containing the Level I animations, Level II difficulty selection, and Level III distractions.

**Demo:** Start the game, select a difficulty using the joystick, and continue playing. The LEDs and buzzer should activate intermittently while the ball and paddles continue moving.

## Original Game and Wireless Control Retained

All three versions build on the earlier lab work:

- **Part B:** Pong game displayed on the ST7735 LCD, with a player paddle, computer paddle, ball collisions, score display, buzzer feedback, score LEDs, and a motor animation after a match is won.
- **Part D:** Wireless player-paddle control using the **Blynk mobile app → ESP32-S3 → logic level shifter → ATmega328PB**. The **UP** button uses Blynk virtual pin **V0** and ESP32 **GPIO 5**, which is read at ATmega **PD2**. The **DOWN** button uses **V1**, ESP32 **GPIO 6**, and ATmega **PD3**.
- The default setting `#define USE_WIRELESS_CONTROL 1` enables the wireless buttons. Changing it to `0` and rebuilding enables the original joystick paddle control instead. In Levels II and III, the joystick is still used for the **startup difficulty selection**, even when wireless paddle control is enabled.



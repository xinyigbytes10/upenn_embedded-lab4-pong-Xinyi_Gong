/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************

  You’ll need:
   - Blynk IoT app (download from App Store or Google Play)
   - ESP32 board
   - Decide how to connect to Blynk
     (USB, Ethernet, Wi-Fi, Bluetooth, ...)

  There is a bunch of great example sketches included to show you how to get
  started. Think of them as LEGO bricks  and combine them as you wish.
  For example, take the Ethernet Shield sketch and combine it with the
  Servo example, or choose a USB sketch and add a code from SendData
  example.
 *************************************************************/

/**
 * FILL OUT THE FOLLOWING BASED ON YOUR TEMPLATE INFORMATION
 * 1. BLYNK_TEMPLATE_ID (line 47)
 * 2. BLYNK_TEMPLATE_NAME (line 48)
 * 3. BLYNK_AUTH_TOKEN (line 49)
 * 4. MY_SSID (line 53)
 * 5. MY_PASSWORD (line 54)
 */

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

/* Fill-in your Template ID (only if using Blynk.Cloud) */
#define BLYNK_TEMPLATE_ID           ".................." // FILL THIS OUT
#define BLYNK_TEMPLATE_NAME         ".................." // FILL THIS OUT
#define BLYNK_AUTH_TOKEN            ".................." // FILL THIS OUT

// Your WiFi credentials.
// Set password to "" for open networks.
#define MY_SSID                     ".................." // FILL THIS OUT
#define MY_PASSWORD                 ".................." // FILL THIS OUT

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = BLYNK_AUTH_TOKEN;

void setup()
{
  // Debug console
  Serial.begin(9600);
  Blynk.begin(auth, MY_SSID, MY_PASSWORD);
}

void loop()
{
  Blynk.run();
}

    For the ESP-WROOM-32 select the following board
    ESP32 > LOLIN D32

    Press and HOLD the BOOT button untill connection is OK
*/


// ----------------------------------------
//  WiFi for the ESP32
// ----------------------------------------
// Add library: see above
#include <WiFi.h>


// ----------------------------------------
//  NTP
// ----------------------------------------
#include <time.h>
struct tm timeinfo;


// ----------------------------------------
//  OLED display (NodeMCU SD1306 128x32)
// ----------------------------------------
// Add library "ESP8266 and ESP32 OLED driver for SSD1306 displays" by ThingPulse
// https://github.com/ThingPulse/esp8266-oled-ssd1306
//
// New
#include <SSD1306Wire.h>
SSD1306Wire  display(0x3c, 5, 4);  // I2C at address 0x3C , SDA = pin 5 , SCL = pin 4
// Legacy
/*
#include <SSD1306.h>
SSD1306  display(0x3c, 5, 4); // I2C at address 0x3C , SDA = pin 5 , SCL = pin 4
*/


// ----------------------------------------
//  Serial print (debug)
// ----------------------------------------
void debug(String text, bool newline = true)
{
  #if USE_SERIAL
    if (newline) { Serial.println(text); }
    else  { Serial.print(text); }
  #endif
}


// ----------------------------------------
//  Global configuration
// ----------------------------------------

#include "config.h"

// Global variables
long every1s;
long every10s;


/******************************************************************************/
/*          Setup                                                             */
/******************************************************************************/
void setup()
{
  // Serial monito
  #if USE_SERIAL
    delay(1000);
    Serial.begin(9600);
    while (!Serial) {
      ; // Wait for serial
    }
    delay(500);
    debug("--- Serial monitor started ---");
  #endif
  
  
  // Start the OLED Display
  display.init();
  display.flipScreenVertically();                 // this is to flip the screen 180 degrees
  display.setColor(WHITE);

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Arashi Projects");

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 20, "NTP Clock");

  display.display();
  updateProgress(0);

  if (WiFiConnect())  // If you can connect to WiFi
  {
    NTPConnect();  // also try syncing time
  }
  else
  {
    delay(5000);
    ESP.restart();  // prefer above ESP.reset();
  }

  every1s = millis();
  every10s = millis();
}


/******************************************************************************/
/*          Loop                                                             */
/******************************************************************************/
void loop()
{
  // Every 1s
  if (millis() > every1s + 1000)
  {
    // Print the local time
    printLocalTime();
    every1s = millis();
  }

  // Every 10s
  if (millis() > every10s + 10000)
  {
    // Check WiFi
    if (WiFi.status() != WL_CONNECTED)
    {
      // Connection to WiFi lost, try reconnect (should be automatically anyway)
      debug("WiFi down, trying to re-connect");
      WiFi.reconnect();
    }
    every10s = millis();
  }
}




/******************************************************************************/
/*          Subs                                                             */
/******************************************************************************/

// Progressbar
void updateProgress(int progress)
{
  display.drawProgressBar(10, 53, 110, 10, progress);  // start x, start y, width, height, progress
  display.display();
  delay(100);
}


// Connect to AP
bool WiFiConnect()
{
  int NAcounts = 0;
  WiFi.begin(wifi_ssid, wifi_pwd); // SSID, password
    
  #if USE_SERIAL
    WiFi.printDiag(Serial);
  #endif

  debug("Connecting to WiFi ", false);
  while (WiFi.status() != WL_CONNECTED && (NAcounts < 10))
  {
    debug(".", false);
    updateProgress(5*NAcounts);
    delay(500);
    NAcounts++;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    debug(" could not connect");
    return false;
  }
  else
  {
    debug("Connected with IP", false);
    debug(WiFi.localIP().toString().c_str());
    updateProgress(50);
    /*
      true: module will try to reestablish lost connection to the AP
      false: module will stay disconnected
      Note: running setAutoReconnect(true) when module is already disconnected will not make it reconnect to the access point. Instead reconnect() should be used.
    */
    WiFi.setAutoReconnect(true);
    // WiFi.persistent(true);
    return true;
  }
}


// Connect to NTP server
bool NTPConnect()
{
  int NAcounts = 0;
  updateProgress(60);

  debug("Connecting to NTP server ", false);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  while ( (!getLocalTime(&timeinfo)) && (NAcounts < 3) )
  {
    debug(".", false);
    updateProgress(60+(5*NAcounts));
    delay(500);
    NAcounts++;
  }

  if (getLocalTime(&timeinfo))
  {
    debug(" connected");
    updateProgress(100);
    return true;
  }
  else
  {
    debug(" could not connect");
    return false;
  }
}

// Print local time on the display
void printLocalTime() {
  // time.h: https://www.tutorialspoint.com/c_standard_library/time_h.htm
  // Time formats: http://www.cplusplus.com/reference/ctime/strftime/
  // https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/


  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  char timeMinutes[3];
  strftime(timeMinutes, 3, "%M", &timeinfo);
  char timeSeconds[3];
  strftime(timeSeconds, 3, "%S", &timeinfo);

  char dateYear[5];
  strftime(dateYear, 5, "%Y", &timeinfo);
  char dateMonth[3];
  strftime(dateMonth, 3, "%m", &timeinfo);
  char dateDay[3];
  strftime(dateDay, 3, "%d", &timeinfo);

  String timeString = timeHour;
  timeString = timeString + ":";
  timeString = timeString + timeMinutes;
  timeString = timeString + ":";
  timeString = timeString + timeSeconds;

  String dateString = dateDay;
  dateString = dateString + "-";
  dateString = dateString + dateMonth;
  dateString = dateString + "-";
  dateString = dateString + dateYear;

  debug(timeString, false);
  debug(" - ", false);
  debug(dateString);

  // strftime(timePrint,3, "%A, %B %d %Y %H:%M:%S", &timeinfo);

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  display.drawString(64, 0, timeString);

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 30, dateString);

  if (WiFi.status() == WL_CONNECTED)
  {
    display.drawCircle(-1, 65, 4);
    display.drawCircle(-1, 65, 8);
    display.drawCircle(-1, 65, 12);
  }
  
  display.display();
}

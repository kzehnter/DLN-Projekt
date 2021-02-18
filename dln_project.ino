#include <SPI.h>
#include <WiFi101.h>
#include "epd4in2.h"
#include "epdpaint.h"

#define COLORED     0
#define UNCOLORED   1

// WLAN
char ssid[] = "WLAN-10.";
char pass[] = "}&Xt34DmMq/<*d-M";

// Telegram
WiFiSSLClient client;
const char url[] = "https://api.telegram.org/";
String botEndPoint = "botAAEpr0T2dvKmEYfsniMDOI8iQmFUi0rGvK0/getUpdates/getUpdates";
const char sha1[] = "F2:AD:29:9C:34:48:DD:8D:F4:CF:52:32:F6:57:33:68:2E:81:C1:90";

// Display
Epd display;
const char testText[] = "Hallo Welt und DLN-Kurs! Das hier ist ein Test";

void setup() {
  //if (!connectWiFi()) return;
  //const char text[] = getText()
  //if (getText() == null) return;   
  if (!writeOnDisplay(testText)) return; 
}

void loop() {
//  delay(10000);
};

/** Connects to wifi
 *  returns true if successful, false if too many connection attempts
 */
bool connectWiFi(){
  int status = WL_IDLE_STATUS;
  
  int tries = 0;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid,pass);
    tries++;
    delay(5000);
    if (tries > 5) return false;
  }
  return true;
}

/** 
 * 
 */
bool writeOnDisplay(const char text[]){
  if (display.Init() != 0) return false;              //init display with reset
  display.ClearFrame();                               //clear SRAM of display
  unsigned char image[1500];                          //create small image buffer  
  int height = 24;                                    //different for font sizes
  Paint paint(image, 400, height);                    //create paint
  paint.Clear(UNCOLORED);                             //clear paint
  paint.DrawStringAt(0, 3, text, &Font20, COLORED);   //put text on paint
                                                      //define area for paint on display 
  display.SetPartialWindow(paint.GetImage(), 0, 0 /*adjust to go down a row*/, paint.GetWidth(), paint.GetHeight());
  display.DisplayFrame();                             //refresh display -> shows image
  display.Sleep();                                    //display back in deep sleep
  return true;
}

char* getText(){
  short responseCode = 0;
  int updateID = 0;
  char* answer;
  
  if (!client.connect(url, 443)) return NULL; 
  client.println("GET " + botEndPoint + "?offset=211608644" /*+ (updateID + 1)*/);
  client.println("Content-Type: application/json");
  client.println();

  int i = 0;
  while(client.available()) {
    answer[i] = client.read();
    i++;
  }
  client.println("Connection: close");
  client.stop();
  return
}

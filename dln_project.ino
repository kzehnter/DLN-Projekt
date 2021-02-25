#include <SPI.h>
#include <WiFi101.h>
#include "epd4in2.h"
#include "epdpaint.h"
#include "arduino_secrets.h"

#define COLORED     0
#define UNCOLORED   1
#define BUFSIZE     800

static char buf[BUFSIZE+1];

// Telegram
WiFiSSLClient client;
const char url[] = "api.telegram.org";

// Display
Epd display;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  if (!connectWiFi()) return;
  char *rawMessage = getText(); 
  if (rawMessage == NULL) return;
  Serial.println(rawMessage);
  char *message = convertText(rawMessage);
  //if (writeOnDisplay(const_cast<char*>(rawMessage)) == 1) return;
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
int writeOnDisplay(const char text[]){
  if (display.Init() != 0) return 1;              //init display with reset
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
  return 0;
}

/**
 * 
 */
char * getText(){
  if (!client.connect(url, 443)) return NULL; 
  Serial.println("Connected");
  client.print("GET ");
  client.println(botEndPoint);
  client.println("Content-Type: application/json");
  client.println();

  
  int j = 0;
  while (j++ != 50) {
    if (client.available()) {
      int i = 0;
      char c;
      while (client.available() && (i < BUFSIZE)) {
        c = client.read();
        buf[i++] = c;
      }
      buf[i] = '\0';
    } else {
      delay(100);
    }
  }
  client.println("Connection: close");
  client.stop();
  return buf;
}

/**
 * 
 */
char * convertText(char text[]) {
  return text;
}

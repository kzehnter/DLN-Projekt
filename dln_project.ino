#include <SPI.h>
#include <WiFi101.h>
#include <string.h>
#include "epd4in2.h"
#include "epdpaint.h"
#include "arduino_secrets.h"

#define COLORED     0
#define UNCOLORED   1
#define BUFSIZE     800

char buf[BUFSIZE+1];

// Telegram
WiFiSSLClient client;
const char url[] = "api.telegram.org";

// Display
Epd display;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  if (!connectWiFi()) return;
  char *bufptr = buf;
  getText(); 
  if (buf[0] == 0) return;
  if (writeOnDisplay(const_cast<char*>(bufptr)) == 1) return;
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

/** Gets text from telegram bot and filters out the message text.
 *  Writes to buf
 */
void getText(){
  if (!client.connect(url, 443)) return; 
  Serial.println("Connected");
  client.print("GET ");
  client.println(botEndPoint);
  client.println("Content-Type: application/json");
  client.println();

  
  int j = 0;
  while (j++ != 50) {                                 // try reading at most 50 times 
    if (client.available()) {
      int i = 0;                                      // buf index
      char c;                                         
      while (client.available() && (i < BUFSIZE)) {   // read until end of answer or end of buf
        c = client.read();
        buf[i++] = c;
      }
      buf[i] = '\0';                                  // null terminate buf
    } else {
      delay(100);                                     // wait after every reading try
    }
  }
  client.println("Connection: close");
  client.stop();  

  char * temp = strstr(buf, "text");                  //  Takes everything from first "text" to end and
  strncpy(buf, temp+7, strlen(temp)-12);              //  cuts off 7 chars at the beginning and 5 chars at the end
}

/** 
 * 
 */
int writeOnDisplay(const char text[]){
  if (display.Init() != 0) return 1;                  //init display with reset
  display.ClearFrame();                               //clear SRAM of display
  unsigned char image[1500];                          //create small image buffer 
  
  int length = strlen(buf);
  if (buf[0] = '\\') {                                //if has commands at begin 
    char * commands = (char*)malloc(8);
    strncpy(commands, buf, 3);                        // maybe change to until second \
    strncpy(buf, buf+3, length);
    length -= 3; 
    switch (commands[1]) {
      case '1':
        break;
      case '2':
        break;
      case '3':
        break;
    }
  }
  int i = 0;
  while(i<length) {
    char * sentence = strtok(buf, "\n");
    
    int height = 24;                                  //different for font sizes
    Paint paint(image, 400, height);                  //create paint
    paint.Clear(UNCOLORED);                           //clear paint
    paint.DrawStringAt(0, 3, text, &Font8, COLORED);  //put text on paint
                                                      //define area for paint on display 
    display.SetPartialWindow(paint.GetImage(), 0, 0 /*adjust to go down a row*/, paint.GetWidth(), paint.GetHeight());
  }
  display.DisplayFrame();                             //refresh display -> shows image
  display.Sleep();                                    //display back in deep sleep
  return 0;
}

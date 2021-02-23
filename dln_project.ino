#include <SPI.h>
#include <WiFi101.h>
#include "epd4in2.h"
#include "epdpaint.h"

#define COLORED     0
#define UNCOLORED   1

// WLAN
const char ssid[] = "WLAN-10.";
const char pass[] = "}&Xt34DmMq/<*d-M";

// Telegram
WiFiSSLClient client;
const char url[] = "api.telegram.org";
String botEndPoint = "/bot1411748234:AAEpr0T2dvKmEYfsniMDOI8iQmFUi0rGvK0/getUpdates?offset=-1";
const char sha1[] = "F2:AD:29:9C:34:48:DD:8D:F4:CF:52:32:F6:57:33:68:2E:81:C1:90";

// Display
Epd display;
const char testText[] = "Hallo Welt und DLN-Kurs! Das hier ist ein Test";

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  if (!connectWiFi()) return;
  //const char text[] = getText()
  //if (getText() == NULL) return;
  String temp = getText();
  char rawMessage[temp.length() + 1];
  temp.toCharArray(rawMessage, temp.length() + 1); 
  Serial.println("Got message"); 
  Serial.println(temp); 
  //if (!writeOnDisplay(rawMessage)) return;
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

String getText(){
  if (!client.connect(url, 443)) return ""; 
  Serial.println("443");
  client.println("GET " + botEndPoint);
  client.println("Content-Type: application/json");
  client.println();

  while (!client.available()) ;

  String answer = "";
  while (client.available()) {
    char c = client.read();
    answer += c;
  }
  client.println("Connection: close");
  client.stop();
  return answer;
}

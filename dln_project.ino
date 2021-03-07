#include <SPI.h>
#include <WiFi101.h>
#include "epd4in2.h"
#include "epdpaint.h"
#include "arduino_secrets.h"

#define BUFSIZE     1600
#define COLORED     0
#define UNCOLORED   1
#define OPTIONSNR   1
#define FONTNR      5

char buf[BUFSIZE+1];                          // doppelt
char *bufptr = buf;

// Telegram
WiFiSSLClient client;
const char url[] = "api.telegram.org";

// Display
Epd display;
sFONT fonts[FONTNR] = {Font8,Font12,Font16,Font20,Font24};

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  if (!connectWiFi()) return;
  getText(); 
  if (buf[0] == 0) return;
  convertText();
  Serial.println(buf);
  if (writeOnDisplay()) return;
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
  bool backslash = false;
  while (j++ != 50) {                                       // try reading at most 50 times 
    if (client.available()) {
      int i = 0;                                            // buf index
      char c;                                         
      while (client.available() && (i < BUFSIZE)) {         // read until end of answer or end of buf
        c = client.read();
        if (backslash) {                                    //                     
          switch (c) {
            case 'n':
              buf[i++] = '\n';
              break; 
            case '\\':
              buf[i++] = '\\';
              break;
            case '"':
              buf[i++] = '\"';
              break;
            case '\'':
              buf[i++] = '\'';
              break;
          }
          backslash = false;
        } else if (c == '\\') {                             // check for 
          backslash = true; 
        } else buf[i++] = c;
      }
      buf[i] = '\0';                                        // null terminate buf
    } else {
      delay(100);                                           // wait after every reading try
    }
  }
  client.println("Connection: close");
  client.stop();  
}

/** Filters message and fits text to screen.
 *  Puts \n when end of screen is reached
 *  Checks for options and calculates last possible \n position
 *  Scans for \n and puts it in last position if there has been no \n
 */
void convertText() {
  char * temp = strstr(bufptr, "text");                        //  Takes everything from first "text" to end and
  strncpy(bufptr, temp+7, strlen(temp)-12);                    //  cuts off 7 chars at the beginning and 5 chars at the end
  buf[strlen(temp)-12] = '\0';
  
  int width = 14;
  int counter = 28;
  if (optionCheck(bufptr)) {
    width = fonts[bufptr[1]-'0'].Width;
    counter = 400/width + OPTIONSNR + 1;
  }
  for (int i = 1; i < strlen(bufptr)-1; i++) {
    counter--;
    if (counter == 0){
      if (strlen(bufptr)<BUFSIZE) {           // at least 1 Byte left in memory for buffer
        char * p = bufptr+i;
        if(p) {
          rightShift(p);
          *p = '\n';
        }
        i++;
      } else break;
    }
    if (bufptr[i] == '\\' && bufptr[i+1] == 'n') {
      if (optionCheck(bufptr+i+2)) {
        width = fonts[bufptr[i+3]-'0'].Width;
        i += OPTIONSNR + 3; // \n und %,%
      } else {
        i += 1; 
      } 
      counter = 400/width;
    }
  }
}

/** 
 * 
 */
int writeOnDisplay(){
  if (display.Init() != 0) return 1;                        //init display with reset
  display.ClearFrame();                                     //clear SRAM of display
  unsigned char image[1500];                                //create small image buffer 

  int height = 20;
  int y = 0;
  char * sentence = strtok(bufptr, "\n");
  bool commandsExist = false;
  char commands[OPTIONSNR];

  while (sentence != NULL) {                                             //value of sentence if at \0 char while will stop
    if (optionCheck(sentence)) {  
        commandsExist = true;   
        strncpy(commands, sentence+1, OPTIONSNR);                 //commands now contains all Option-chars
        height = fonts[commands[0]-'0'].Height;                   //command converted to int for fontsize and its height
        sentence += 2 + OPTIONSNR;                                // 2 "%" and number of Options
    }
    Paint paint(image, 400, height);
    Serial.println(sentence);

    // checks command and chooses right fontsize
    if (commandsExist) {      
      paint.Clear(UNCOLORED);
      paint.DrawStringAt(0, 2, sentence, &fonts[commands[0]-'0'], COLORED); 
    } else {
      paint.Clear(UNCOLORED);
      paint.DrawStringAt(0, 2, sentence, &Font20, COLORED);  //put text on paint
    }
                                                      //define area for paint on display 
    display.SetPartialWindow(paint.GetImage(), 0, y, paint.GetWidth(), paint.GetHeight());
    y += height;
    sentence = strtok(NULL, "\n");
  }
  display.DisplayFrame();                             //refresh display -> shows image
  display.Sleep();                                    //display back in deep sleep
  return 0;
}

/** Checks validity of options.
 *  Adjust for more options or different checks
 */
bool optionCheck(char * ptr) {
  return ((ptr[0] == '%' && ptr[OPTIONSNR+1] == '%') && (0 <= ptr[1]-'0' && ptr[1]-'0' < FONTNR));
}

/** Moves every char 2 to the right.
 *  https://stackoverflow.com/questions/34908360/insert-a-character-at-a-specific-position-in-a-char-array
 */
void rightShift(char * s){
  int n = strlen(s);
  s[n+1] = 0;
  while (n) {
    s[n] = s[n-1];
    n--; 
  }
}

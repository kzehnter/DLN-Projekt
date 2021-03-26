#include <SPI.h>
#include <WiFi101.h>
#include <RTCZero.h>
#include "src/epd4in2/epd4in2.h"
#include "src/epd4in2/epdpaint.h"
#include "arduino_secrets.h"

/* arduino_secrets.h should be created in the project folder
 * Nessecary fields:
 *    const char ssid[] = "[NETWORK-SSID]";
 *    const char pass[] = "[PASSWORD]";
 *    const char botEndpoint[] = "/bot[BOT-ACCESS-TOKEN]/getUpdates?offset=-1";
 */

#define BUFSIZE     1600
#define COLORED     0
#define UNCOLORED   1
#define OPTIONSNR   1
#define FONTNR      5
#define ALARMNR     2

// ---- RealTimeClock Module
// Change these values to set the current initial time
const byte seconds = 0;
const byte minutes = 0;
const byte hours = 12;
// Contains times of alarms, hour, min, sec format
const byte alarmTimes[ALARMNR][3] = {{12, 0, 15}
                                    ,{12, 2, 0}};
int alarmCounter = -1;
RTCZero rtc;
bool matched = false;

// ---- Text storage
char buf[BUFSIZE+1];
char * bufptr = buf;

// ---- Telegram
WiFiSSLClient client;
const char url[] = "api.telegram.org";

// ---- Display
Epd display;
sFONT fonts[FONTNR] = {Font8,Font12,Font16,Font20,Font24};
sFONT stdfont = Font20;

void setup() {
  Serial.begin(9600);
  while(!Serial);

  WiFi.maxLowPowerMode();
  
  rtc.begin();
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(15, 3, 21);
  createRTC();
}

void loop() {  
  if (matched) {
    matched = false;
    if (connectWiFi()) {
      Serial.println("Connected");
      if (getText()) {
        Serial.println("Read message from telegram");
        if (convertText()) { 
          Serial.println(bufptr);
          writeOnDisplay();
          Serial.println("All done");
        }
      }
    }
    createRTC();
  }
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

/** Gets text from telegram bot and checks for escapes at readtime.
 *  Writes to buf, uses HTTPS
 */
bool getText(){
  // ---- connect
  if (!client.connect(url, 443)) return false;
  client.print("GET ");
  client.println(botEndPoint);                              
  client.println("Content-Type: application/json");
  client.println();

  // ---- read
  int j = 0;
  bool backslash = false;                               // for escape check
  while (j++ != 50) {                                   // try reading at most 50 times 
    if (client.available()) {
      int i = 0;                                        // buf index
      char c;                                         
      while (client.available() && (i < BUFSIZE+1)) {   // read until end of answer or end of buf
        c = client.read();
        if (backslash) {                                // if last char was '\\'                    
          switch (c) {                                  // create matching escape sequence and put it at bufptr[i]
            case 'n':                                   // prevents later strtok issues
              bufptr[i++] = '\n';
              break; 
            case '\\':
              bufptr[i++] = '\\';
              break;
            case '\"':
              bufptr[i++] = '\"';
              break;
            case '\'':
              bufptr[i++] = '\'';
              break;
          }
          backslash = false;
        } else if (c == '\\') {                         // check for '\\' to form escape
          backslash = true; 
        } else bufptr[i++] = c;
      }
      bufptr[i] = '\0';                                 // null terminate buf
    } else {
      delay(100);                                       // wait after every reading try
    }
  }

  // ---- disconnect
  client.println("Connection: close");
  client.stop(); 
  return (bufptr[0] != 0);                              // if bufptr is empty then something definitely went wrong                               
}

/** Filters message and fits text to screen.
 *  Puts \n when end of screen is reached
 *  Checks for options and calculates last possible \n position
 *  Scans for \n and puts it in last position if there has been no \n
 *  
 *  Little bug: if the text reaches the end of the line and there is
 *              a command-like structure afterwards (%3%) it will see
 *              interpret it as a command because of '\n' before it
 */
bool convertText() {
  // ---- cut out text from server response
  char * temp = strstr(bufptr, "text");                 // Takes everything from first "text" to end and
  if (!temp) return false;
  strncpy(bufptr, temp+7, strlen(temp)-12);             // cuts off 7 chars at the beginning and 5 chars at the end
  buf[strlen(temp)-12] = '\0';                          // null terminate at the right point

  // ---- scan text and fits it to screen size
  unsigned int i = 1;                                   // unsigned for comparison with strlen
  int width = stdfont.Width;                            // take width of stdfont
  int counter = 400/width;                              // amount of chars per line for stdfont
  if (optionCheck(bufptr)) {                            // if buf begins with options
    width = fonts[bufptr[1]-'0'].Width;                 // set current char width
    counter = 400/width;
    i += OPTIONSNR + 2;                                 // increment i to right loop starting point
  }
  for (; i < strlen(bufptr)-1; i++) {
    counter--;
    if (counter == 0){
      if (strlen(bufptr)<BUFSIZE) {                     // at least 1 Byte left in memory for buffer
        char * p = bufptr+i;                            // create pointer to shift position
        if(p) {                                         // if p doesnt point to 0 char
          rightShift(p);
          *p = '\n';
        }
      } else break;
    }
    if (bufptr[i] == '\n') {                                
      if (optionCheck(bufptr+i+1)) {                    // check for options after '\n'
        width = fonts[bufptr[i+2]-'0'].Width;           // set width
        i += OPTIONSNR + 3; // \n und %,%               // adjust i to not confuse counter
      } else {
        i++;                                            // jump forward to not count '\n' with counter
      } 
      counter = 400/width;
    }
  }
  return true;
}

/** Checks for commands and writes to screen one sentence at a time.
 */
bool writeOnDisplay(){
  // ---- prepare display
  if (display.Init() != 0) return false;                // init display with reset
  display.ClearFrame();                                 // clear SRAM of display
  unsigned char image[1500];                            // create small image buffer 

  // ---- read commands and write to paint object
  int height = stdfont.Height;                          // height of stdfont
  int y = 0;
  char * sentence = strtok(bufptr, "\n");               // tokenized char pointer -> write 1 sentence at a time to paint
  bool commandsExist = false;
  char commands[OPTIONSNR];

  while (sentence != NULL) {                            // value of sentence if at \0 char while will stop
    // ---- check for commands
    if (optionCheck(sentence)) {  
        commandsExist = true;   
        strncpy(commands, sentence+1, OPTIONSNR);       // commands now contains all Option-chars
        height = fonts[commands[0]-'0'].Height;         // command converted to int for fontsize and its height
        sentence += 2 + OPTIONSNR;                      // 2 "%" and number of Options
    }
    Paint paint(image, 400, height);
    paint.Clear(UNCOLORED);
    
    // ---- check command and choose right fontsize
    if (commandsExist) {      
      paint.DrawStringAt(0, 2, sentence, &fonts[commands[0]-'0'], COLORED); 
    } else {  
      paint.DrawStringAt(0, 2, sentence, &stdfont, COLORED);
    }
    // ---- define area for paint on display 
    display.SetPartialWindow(paint.GetImage(), 0, y, paint.GetWidth(), paint.GetHeight());
    y += height;
    sentence = strtok(NULL, "\n");                      // move to next token
  }
  display.DisplayFrame();                               // refresh display -> shows image
  display.Sleep();                                      // display back in deep sleep
  return true;
}

/** Checks validity of options.
 *  Adjust for more options or different checks
 */
bool optionCheck(char * ptr) {
  return ((ptr[0] == '%' && ptr[OPTIONSNR+1] == '%') && (0 <= ptr[1]-'0' && ptr[1]-'0' < FONTNR));
}

/** Moves every char to the right.
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

/** Sets up alarm with possible multiple alarms.
 */
void createRTC(){
  if (alarmCounter < ALARMNR-1) {                       // increment alarmCounter or set to 0
    alarmCounter++;
  } else alarmCounter = 0;
  rtc.setAlarmTime(alarmTimes[alarmCounter][0],         // take right alarm and its times from array
                   alarmTimes[alarmCounter][1], 
                   alarmTimes[alarmCounter][2]);
  rtc.enableAlarm(rtc.MATCH_HHMMSS);                    // hour, min, sec has to be equal to alarm
  rtc.attachInterrupt(alarmMatch);
  rtc.standbyMode();
}

/** Executed every time an alarm goes off.
 */
void alarmMatch(){
  matched = true;
}

#include <SPI.h>
#include "epd4in2.h"
#include "epdpaint.h"

#define COLORED     0
#define UNCOLORED   1

// object for display
Epd epd;

void setup() {
  Serial.begin(9600);
  while (!Serial);
}

void loop() {
  String text = "Hallo Welt und DLN-Kurs!";  

  //epd.Reset();
  if (epd.Init() != 0) {
    return;
  }
  //epd.ClearFrame();

  // read 'text', cast 'const char*' to 'char*'
  char *m = const_cast<char*>(text.c_str() /*returns const char* */);
  // break m into pieces with delimiter \n
  char* stuff = strtok(m, "\n");

  // tricky, library is unclear
  unsigned char image[1500];
  int x = 0;
  int y = 0;

  while (stuff != NULL) {
    int height = 24;

    int nothing = UNCOLORED;
    int color = COLORED;
    
    Paint paint(image, 400, height);
    
    paint.Clear(nothing);
    paint.DrawStringAt(0, 3, stuff, &Font20, color);
    epd.SetPartialWindow(paint.GetImage(), 0, y, paint.GetWidth(), paint.GetHeight());

    y += height;
    stuff = strtok(NULL, "\n");
    x++;
  }

  epd.DisplayFrame();
  epd.Sleep();
  delay(10000);
}

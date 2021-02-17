#include <SPI.h>
#include "epd4in2.h"
#include "epdpaint.h"

#define COLORED     0
#define UNCOLORED   1

// object for display
Epd display;
const char text[] = "Hallo Welt und DLN-Kurs! Das hier ist ein Test";

void setup() {    
  if (display.Init() != 0) {
    return;
  }
  display.ClearFrame();

  // read 'text', cast 'const char*' to 'char*'
  char *m = const_cast<char*>(text /*returns const char* */);
  // break m into pieces with delimiter \n
  char* stuff = strtok(m, "\n");

  // tricky, library is unclear
  unsigned char image[1500];
  int x = 0;
  int y = 0;

  while (stuff != NULL) {
    int height = 24;

    Paint paint(image, 400, height);
    
    paint.Clear(UNCOLORED);
    paint.DrawStringAt(0, 3, stuff, &Font20, COLORED);
    display.SetPartialWindow(paint.GetImage(), 0, y, paint.GetWidth(), paint.GetHeight());

    y += height;
    stuff = strtok(NULL, "\n");
    x++;
  }
  display.DisplayFrame();
  display.Sleep();
}

void loop() {
//  delay(10000);
};

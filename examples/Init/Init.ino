#include "VGA_esp32.h"
#include "GFX.h"

VGA_esp32 vga;
GFX gfx(vga);

// _8BIT            r0  r1  r2          g0  g1  g2               b0 b1
// _16BIT   r0  r1  r2  r3  r4  g0  g1  g2  g3  g4  g5  b0 b1 b2 b3 b4  HSync  VSync
Pins pin = {11, 12, 13, 14, 15, 21, 20 ,19, 18, 17, 16, 4, 5, 6, 9, 10, 22,    23};

void setup() {
  Serial.begin(115200); delay(1000);
  vga.setPins(pin);

//                     Mode   Color(_8BIT, _16BIT)  Scale(0..2) Double buffer
  vga.init(MODE640x480_60Hz,               _16BIT,           0,        false);

  gfx.testRGBPanel();
} 

void loop() {
}

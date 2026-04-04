#include "VGA_esp32.h"
#include "GFX.h"

VGA_esp32 vga;
GFX gfx(vga);

void setup() {
  Serial.begin(115200);
  vga.init(MODE640x480_60Hz, _16BIT, 1, false);

  gfx.testRGBPanel();
}

void loop() {

}

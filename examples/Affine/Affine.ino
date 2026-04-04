#include "VGA_esp32.h"
#include "Sprite.h"
#include "GFX.h"
#include "1.h"

VGA_esp32 vga;
GFX gfx(vga);
Sprite sp(vga);

#define objects 5
#define maxObj 200

typedef struct{
  int x, y;
  int dx, dy;
  uint8_t img;
  int angStep, ang;
  int zX, zY;
  int zDX, zDY;
} Object;
Object obj[maxObj];
uint16_t maskCol;
int cont;
bool stop = false;

void initObj(){
  for (int i = 0; i < cont; i++){
    obj[i].x = random(16, vga.Width() - 16);
    obj[i].y = random(16, vga.Height() - 16);
    obj[i].dx = random(1, 3) * (random(2) ? 1 : -1);
    obj[i].dy = random(1, 3) * (random(2) ? 1 : -1);


    obj[i].ang = random(360);
    obj[i].angStep = random(3, 5);
    int a = random(2);
    obj[i].angStep *= (a == 0 ? -1 : 1);
    obj[i].img = random(16);

    obj[i].zX = random(100, 200); obj[i].zDX = random(3, 10);
    obj[i].zY = random(100, 200); obj[i].zDY = random(3, 5);
  }
}

void checkObj(){
  for (int i = 0; i < cont; i++){
    if (obj[i].x <= 0 || obj[i].x >= vga.MaxX() - 16) obj[i].dx *= (-1);
    if (obj[i].y <= 0 || obj[i].y >= vga.MaxY() - 16) obj[i].dy *= (-1);

    obj[i].x += obj[i].dx;
    obj[i].y += obj[i].dy;

    obj[i].ang += obj[i].angStep;

    if (obj[i].zX < 50 || obj[i].zX > 300) obj[i].zDX *= -1; obj[i].zX += obj[i].zDX;
    if (obj[i].zY < 50 || obj[i].zY > 300) obj[i].zDY *= -1; obj[i].zY += obj[i].zDY;

  }  
}

void addObj(){
  if (cont >= maxObj || vga.FPS() < 59) return;

  obj[cont].x = random(16, vga.Width() - 16);
  obj[cont].y = random(16, vga.Height() - 16);
  obj[cont].dx = random(1, 3);
  obj[cont].dy = random(1, 3);
  obj[cont].img = random(16);
  obj[cont].ang = random(360);
  obj[cont].angStep = random(3, 5);
  int a = random(2);
  obj[cont].angStep *= (a == 0 ? -1 : 1);
  obj[cont].zX = random(100, 200); obj[cont].zDX = random(3, 10);
  obj[cont].zY = random(100, 200); obj[cont].zDY = random(3, 5);  
  cont++; 
}

void setup() {
  Serial.begin(115200);
  vga.init(MODE640x480_60Hz, 8, 1, 1);
  vga.initBG();
  gfx.testRGBPanel();
  vga.scrToBg();
  //vga.setViewport(50, 50, 320 - 50, 240 - 50);

  cont = objects;
  maskCol = 0b11100;//gfx.getCol(0, 255, 0);
  sp.loadImages(_1);
  initObj();
}

int a = 0;
int p = 100;
int dx = 5;
void loop() {
  Serial.printf("FPS: %.2f, Count: %d\n", vga.FPS(), cont);
  vga.bgToScr();
  //gfx.cls();

  for (int i = 0; i < cont; i++)
    //sp.putImage(obj[i].x, obj[i].y, obj[i].img);
    sp.putAffineSprite(obj[i].x, obj[i].y, obj[i].ang, obj[i].zX, obj[i].zX, 0b11100, obj[i].img);
    checkObj();
  if (vga.Timer() % 25 == 0) addObj();

  //gfx.putText(2, 2, LEFT, cont, 0xFFFF);
  //gfx.putText(2, 10, LEFT, vga.FPS(), 0xFFFF);
  vga.swap();
  //Serial.println(vga.FPS());
  //vga.updateFPS();
  p += dx;
  if (p < 50 || p > 300) dx *= -1;
}

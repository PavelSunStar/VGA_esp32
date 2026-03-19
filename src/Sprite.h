#pragma once

#include "VGA_esp32.h"

struct Image{
    int width, height;
    int maxX, maxY;
    int cx, cy;    
    int lineSize, size;
    int fullSize;
    int offset, offsetLine;

    // viewport
    int x1, x2;
    int y1, y2;    
};

class Sprite{
    public:
        Sprite(VGA_esp32 &vga);
        ~Sprite();

        uint8_t MaxImg() { return _images; };
        int Width(uint8_t num = 0)  { return ((!_created || num >= _images) ? 0 : _img[num].width); }
        int Height(uint8_t num = 0) { return ((!_created || num >= _images) ? 0 : _img[num].height); }
        int CX(uint8_t num = 0)     { return ((!_created || num >= _images) ? 0 : _img[num].cx); }
        int CY(uint8_t num = 0)     { return ((!_created || num >= _images) ? 0 : _img[num].cy); }

        uint8_t* Buffer8(uint8_t num = 0) {
            return (!_created || !_buf || num >= _images) ? nullptr : _buf + _img[num].offset;
        }

        uint16_t* Buffer16(uint8_t num = 0) {
            return (!_created || !_buf || num >= _images) ? nullptr : (uint16_t*)(_buf + _img[num].offset);
        }

        bool createImages(int xx, int yy, uint8_t num);
        bool loadImages(const uint8_t* data);
        void putImage(int x, int y, uint8_t num = 0);
        void putSprite(int x, int y, uint16_t maskColor, uint8_t num = 0);

        void putAffineSprite(int dstX, int dstY, float ang = 0, uint16_t zoomX = 100, uint16_t zoomY = 100, uint16_t maskColor = 0, uint8_t num = 0);
        //void putRotate(float x, float y, float angle, uint8_t num = 0, float zoom = 1.0f);
        //void putSpriteRotate(int x, int y, float angle, uint16_t maskColor, uint8_t num = 0, float zoom = 1.0f); 
        
        uint16_t getPixel(int x, int y, uint8_t num = 0);

    private:
        Image*      _img;
        uint8_t*    _buf = nullptr;
        uint8_t**   _line8 = nullptr;
        uint16_t**  _line16 = nullptr;        

        uint8_t     _bpp;
        uint8_t     _shift;
        uint8_t     _images;
        bool        _created = false;

        void resetParam(uint8_t num);
        bool setBufferAddr();

        VGA_esp32 &_vga;
};


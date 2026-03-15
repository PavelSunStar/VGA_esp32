#include "Sprite.h"
#include "Matrix.h"

Sprite::Sprite(VGA_esp32 &vga) : _vga(vga){

}

Sprite::~Sprite(){
    
}

void Sprite::resetParam(uint8_t num){
    if (num == 0) return;

    _images = num;

    if (_img){
        delete[] _img;
        _img = nullptr;
    }

    _img = new Image[_images];
}

bool Sprite::setBufferAddr(){
    if (!_buf) {
        Serial.println("initBuffer: invalid screen buffer");
        return false;
    }

    if (_line8) {
        delete[] _line8;
        _line8 = nullptr;
    }

    if (_line16) {
        delete[] _line16;
        _line16 = nullptr;
    } 

    int lines = 0;
    for (int i = 0; i < _images; i++) lines += _img[i].height;

    if (_bpp == _16BIT) {
        uint16_t* p = (uint16_t*)_buf;
        _line16 = new uint16_t*[lines]();

        if (!_line16) {
            Serial.println("initBuffer: failed to alloc line16 table");
            return false;
        }

        for (int i = 0; i < lines; i++) {
            _line16[i] = p;
            p += _img[i].width;
        }
    } else {
        uint8_t* p = _buf;
        _line8 = new uint8_t*[lines]();

        if (!_line8) {
            Serial.println("initBuffer: failed to alloc line8 table");
            return false;
        }

        for (int i = 0; i < lines; i++) {
            _line8[i] = p;
            p += _img[i].width;
        }
    }

    return true;
}

bool Sprite::loadImages(const uint8_t* data){
    if (!data) return false;

    uint8_t bpp = *data++;
    if (bpp != _8BIT && bpp != _16BIT) return false;
    _bpp = _vga._scr.bpp; 
    resetParam(*data++);

    data += _images << 2;
    const uint8_t* ptr = data;    
    Serial.printf("bit: %d, img: %d\n", bpp, _images);

    int fullSize = 0;
    uint32_t offsetLine = 0;
    _shift = (_bpp == _16BIT ? 1 : 0); 
    uint8_t shift = (bpp == _16BIT ? 1 : 0);   
    for (int i = 0; i < _images; i++) {
        _img[i].width      = (*data++) | (*data++ << 8);
        _img[i].height     = (*data++) | (*data++ << 8);
        Serial.printf("w: %d, h: %d\n", _img[i].width, _img[i].height);

        _img[i].maxX         = _img[i].width - 1;
        _img[i].maxY         = _img[i].height - 1;
        _img[i].cx         = _img[i].width >> 1;
        _img[i].cy         = _img[i].height >> 1;
        _img[i].lineSize   = _img[i].width << _shift;
        _img[i].size       = _img[i].width * _img[i].height;
        _img[i].fullSize   = _img[i].lineSize * _img[i].height;
        _img[i].offset     = fullSize;
        _img[i].offsetLine = offsetLine;

        // viewport
        _img[i].x1 = 0; _img[i].x2 = _img[i].maxX; 
        _img[i].y1 = 0; _img[i].y2 = _img[i].maxY;

        data += (_img[i].width << shift) * _img[i].height;
        offsetLine += _img[i].height;
        fullSize += _img[i].fullSize;
    }
    
    if (!_vga.allocateMemory(_buf, fullSize, false)) return false;
    if (!setBufferAddr()) return false;

    uint32_t index = 0;
    uint32_t addr = 0;
    for (int i = 0; i < _images; i++){
        for (int y = 0; y < _img[i].height; y++){
            if (_bpp == _16BIT){
                _line16[index++] = (uint16_t*)(_buf + addr);
            } else {
                _line8[index++] = _buf + addr;
            }

            addr += _img[i].lineSize;
        }
    }

    for (int i = 0; i < _images; i++){
        ptr += 4;

        if (_bpp == bpp){
            memcpy(_buf + _img[i].offset, ptr, _img[i].fullSize);
            ptr += _img[i].fullSize; 
        } else if ((bpp == _16BIT) && (_bpp == _8BIT)){
            const uint16_t* sour = (uint16_t*)ptr;
            uint8_t* dest = _buf + _img[i].offset;
            int size = _img[i].size;

            while (size-- > 0){
                uint16_t col16 = *sour++;
                uint8_t r3 = (col16 >> 13) & 0x07;
                uint8_t g3 = (col16 >> 8)  & 0x07;
                uint8_t b2 = (col16 >> 3)  & 0x03;
                
                *dest++ = (r3 << 5) | (g3 << 2) | b2;
            }

            ptr += _img[i].size << 1;
        } else if ((bpp == _8BIT) && (_bpp == _16BIT)){
            const uint8_t* sour = ptr;
            uint16_t* dest = (uint16_t*)(_buf + _img[i].offset);                       
            int size = _img[i].size;

            while (size-- > 0){
                uint8_t col8 = *sour++;

                uint8_t r3 = (col8 >> 5) & 0x07;
                uint8_t g3 = (col8 >> 2) & 0x07;
                uint8_t b2 =  col8       & 0x03;

                uint16_t r5 = (r3 << 2) | (r3 >> 1);
                uint16_t g6 = (g3 << 3) |  g3;
                uint16_t b5 = (b2 << 3) | (b2 << 1) | (b2 >> 1);

                *dest++ = (r5 << 11) | (g6 << 5) | b5;
            }

            ptr += _img[i].size;
        }               
    }

    return (_created = true);
}

void Sprite::putImage(int x, int y, uint8_t num){
    if (!_created || num >= _images) return;
    
    auto& s = _vga._scr;
    Image &im = _img[num];
    if (x > s.x2 || y > s.y2) return;    
    int xx = x + im.maxX;
    int yy = y + im.maxY;
    if (xx < s.x1 || yy < s.y1) return;

    int imageLineSize   = im.lineSize; 
    int scrLineSize     = s.lineSize;

    if (x >= s.x1 && y >= s.y1 && xx <= s.x2 && yy <= s.y2){
        uint8_t* img = _buf + im.offset;
        uint8_t* scr = (_bpp == _16BIT) ? (uint8_t*)&s.line16[_vga. _backBufLine + y][x] : (uint8_t*)&s.line8[_vga. _backBufLine + y][x];     

        int lines = im.height;  
        while (lines-- > 0){
            memcpy(scr, img, imageLineSize);
            scr += scrLineSize;
            img += imageLineSize;
        }        
    } else {
        int sxl = (x  < s.x1 ? (s.x1 - x)  : 0);
        int sxr = (xx > s.x2 ? (xx - s.x2) : 0);
        int syu = (y  < s.y1 ? (s.y1 - y)  : 0);
        int syd = (yy > s.y2 ? (yy - s.y2) : 0);

        int copyX = (im.width  - sxl - sxr) << _shift;
        int copyY =  im.height - syu - syd; 
        if (copyX <= 0 || copyY <= 0) return;   
        
        uint8_t* img = (_bpp == _16BIT)
            ? (uint8_t*)&_line16[im.offsetLine + syu][sxl]
            : (uint8_t*)&_line8[im.offsetLine + syu][sxl];
        uint8_t* scr = (_bpp == _16BIT)
            ? (uint8_t*)&s.line16[_vga. _backBufLine + y + syu][x + sxl]
            : (uint8_t*)&s.line8 [_vga. _backBufLine + y + syu][x + sxl];  
            
        while (copyY-- > 0){
            memcpy(scr, img, copyX);
            scr += scrLineSize;
            img += imageLineSize;
        }            
    }
}    

void Sprite::putSprite(int x, int y, uint16_t maskColor, uint8_t num){
    if (!_created || num >= _images) return;
    
    auto& s = _vga._scr;
    Image &im = _img[num];
    if (x > s.x2 || y > s.y2) return;    
    int xx = x + im.maxX;
    int yy = y + im.maxY;
    if (xx < s.x1 || yy < s.y1) return;

    if (x >= s.x1 && y >= s.y1 && xx <= s.x2 && yy <= s.y2){
        int lines = im.height;    
        int scrLineSize = s.lineSize - im.lineSize;

        if (_bpp == _16BIT){
            uint16_t* img = &_line16[im.offsetLine][0];
            uint16_t* scr = &s.line16[_vga._backBufLine + y][x];

            while (lines-- > 0){
                for (int xx = 0; xx < im.width; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }
                scr += scrLineSize;
            }            
        } else {
            uint8_t* img = &_line8[im.offsetLine][0];
            uint8_t* scr = &s.line8[_vga._backBufLine + y][x]; 

            while (lines-- > 0){
                for (int xx = 0; xx < im.width; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }
                scr += scrLineSize;
            }  
        }
    } else { 
        int sxl = (x  < s.x1 ? (s.x1 - x)  : 0);
        int sxr = (xx > s.x2 ? (xx - s.x2) : 0);
        int syu = (y  < s.y1 ? (s.y1 - y)  : 0);
        int syd = (yy > s.y2 ? (yy - s.y2) : 0);

        int copyX = im.width  - sxl - sxr;
        int copyY =  im.height - syu - syd; 
        if (copyX <= 0 || copyY <= 0) return;   
        int skipSour = im.width - copyX;
        int skipScr = s.width - copyX;

        if (_bpp == _16BIT){
            uint16_t* img = &_line16[im.offsetLine + syu][sxl];
            uint16_t* scr = &s.line16[_vga._backBufLine + y + syu][x + sxl];

            while (copyY-- > 0){
                for (int xx = 0; xx < copyX; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }

                scr += skipScr;
                img += skipSour;
            } 
        } else {
            uint8_t* img = &_line8[im.offsetLine + syu][sxl];
            uint8_t* scr = &s.line8[_vga._backBufLine + y + syu][x + sxl];

            while (copyY-- > 0){
                for (int xx = 0; xx < copyX; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }

                scr += skipScr;
                img += skipSour;
            }             
        }
    }
}

void Sprite::putAffineSprite(int dstX, int dstY, float ang, uint16_t zoomX, uint16_t zoomY, uint16_t maskColor, uint8_t num){
    if (!_created || num >= _images) return;

    auto& s = _vga._scr;    
    Image& im = _img[num];

    const int srcW = im.width;
    const int srcH = im.height;
    Affine2D m = Matrix::make(  (float)dstX, (float)dstY, 
                                (float)srcW * 0.5f, (float)srcH * 0.5f, 
                                angle(ang), 
                                percentTo(zoomX), percentTo(zoomY)
    );

    RectBounds rb = Matrix::bounds(m, (float)srcW, (float)srcH);
    int x1 = (int)floorf(rb.sx); // округляет число вниз до ближайшего целого
    int y1 = (int)floorf(rb.sy);
    int x2 = (int)ceilf(rb.ex);  // округляет число вверх до ближайшего целого.
    int y2 = (int)ceilf(rb.ey);

    x1 = std::max(x1, s.x1);
    y1 = std::max(y1, s.y1);
    x2 = std::min(x2, s.x2 + 1);
    y2 = std::min(y2, s.y2 + 1);
    if (x1 >= x2 || y1 >= y2) return;

    Affine2DInv inv;
    if (!Matrix::invert(inv, m)) return;

    // стартовые координаты для первой точки (x1, y1)
    int32_t row_u = inv.a * x1 + inv.b * y1 + inv.c + HALF;
    int32_t row_v = inv.d * x1 + inv.e * y1 + inv.f + HALF;
      
    uint8_t** srcLines = _line8 + im.offsetLine;
    uint8_t* dstBase   = &s.line8[_vga._backBufLine + y1][x1];
    const uint8_t mask = (uint8_t)(maskColor & 0xFF);
    //const int dstSkip  = s.width - x2 + x1;

    for (int y = y1; y < y2; y++){
        uint8_t* dst = dstBase;
        int32_t u = row_u;
        int32_t v = row_v;

        for (int x = x1; x < x2; x++){
            int sx = u >> FP_SHIFT;
            int sy = v >> FP_SHIFT;

            if ((unsigned)sx < (unsigned)srcW &&
                (unsigned)sy < (unsigned)srcH)
            {
                uint8_t* srcRow = srcLines[sy];
                uint8_t col = srcRow[sx];
                if (col != mask) *dst = col;
            }

            ++dst;
            u += inv.a;
            v += inv.d;
        }

        dstBase += s.width;
        row_u   += inv.b;
        row_v   += inv.e;
    }
}

/*
void Sprite::putMatImage(int32_t dstX, int32_t dstY, float ang, uint8_t num){
    if (!_created || num >= _images) return;

    auto& s = _vga._scr;    
    Image& im = _img[num];

    const int srcW = im.width;
    const int srcH = im.height;
    if (srcW <= 0 || srcH <= 0) return;

    Affine2D m = Matrix::make(
        (float)dstX,
        (float)dstY,
        (float)srcW * 0.5f,
        (float)srcH * 0.5f,
        ang,
        1.0f,
        1.0f
    );

    RectBounds rb = Matrix::bounds(m, (float)srcW, (float)srcH);

    int x1 = (int)floorf(rb.sx);
    int y1 = (int)floorf(rb.sy);
    int x2 = (int)ceilf(rb.ex);
    int y2 = (int)ceilf(rb.ey);

    x1 = std::max(x1, s.x1);
    y1 = std::max(y1, s.y1);
    x2 = std::min(x2, s.x2 + 1);
    y2 = std::min(y2, s.y2 + 1);
    if (x1 >= x2 || y1 >= y2) return;

    Affine2DInv inv;
    if (!Matrix::invert(inv, m)) return;

    const int32_t A = inv.a;
    const int32_t B = inv.b;
    const int32_t C = inv.c;
    const int32_t D = inv.d;
    const int32_t E = inv.e;
    const int32_t F = inv.f;

    const int32_t uu = A * x1 + C + HALF;
    const int32_t vv = D * x1 + F + HALF;

    for (int y = y1; y < y2; y++){
        uint8_t* dst = &s.line8[y + _vga._backBufLine][x1];
        int32_t u = uu + B * y;
        int32_t v = vv + E * y;

        for (int x = x1; x < x2; x++){
            const int sx = u >> FP_SHIFT;
            const int sy = v >> FP_SHIFT;

            if ((unsigned)sx < (unsigned)srcW && (unsigned)sy < (unsigned)srcH){
                const uint8_t col = _line8[sy + im.offsetLine][sx];
                if (col != 0b11100) *dst = col;
            }

            ++dst;
            u += A;
            v += D;
        }
    }
}

void Sprite::putAffineSprite(int32_t dstX, int32_t dstY, float ang, uint16_t maskColor, uint8_t num){
    if (!_created || num >= _images) return;

    auto& s  = _vga._scr;
    Image& im = _img[num];

    const int srcW = im.width;
    const int srcH = im.height;

    // ВАЖНО:
    // Matrix::make() уже сам переводит angle -> rad,
    // поэтому сюда передаём просто ang, без angle(ang).
    Affine2D m = Matrix::make(
        (float)dstX,
        (float)dstY,
        (float)srcW * 0.5f,
        (float)srcH * 0.5f,
        ang,
        2.0f,
        2.0f
    );

    RectBounds rb = Matrix::bounds(m, (float)srcW, (float)srcH);

    int x1 = (int)floorf(rb.sx);
    int y1 = (int)floorf(rb.sy);
    int x2 = (int)ceilf(rb.ex);
    int y2 = (int)ceilf(rb.ey);

    // clip to screen
    x1 = std::max(x1, s.x1);
    y1 = std::max(y1, s.y1);
    x2 = std::min(x2, s.x2 + 1);
    y2 = std::min(y2, s.y2 + 1);

    if (x1 >= x2 || y1 >= y2) return;

    Affine2DInv inv;
    if (!Matrix::invert(inv, m)) return;

    // стартовые координаты для первой точки (x1, y1)
    int32_t row_u = inv.a * x1 + inv.b * y1 + inv.c + HALF;
    int32_t row_v = inv.d * x1 + inv.e * y1 + inv.f + HALF;

    if (_bpp == _16BIT) {
        uint16_t** srcLines = _line16 + im.offsetLine;
        uint16_t* dstBase   = &s.line16[_vga._backBufLine + y1][x1];
        const uint16_t mask = maskColor;
        const int dstSkip   = s.width - (x2 - x1);

        for (int y = y1; y < y2; y++) {
            uint16_t* dst = dstBase;

            int32_t u = row_u;
            int32_t v = row_v;

            for (int x = x1; x < x2; x++) {
                int sx = u >> FP_SHIFT;
                int sy = v >> FP_SHIFT;

                if ((unsigned)sx < (unsigned)srcW &&
                    (unsigned)sy < (unsigned)srcH)
                {
                    uint16_t* srcRow = srcLines[sy];
                    uint16_t col = srcRow[sx];
                    if (col != mask) *dst = col;
                }

                ++dst;
                u += inv.a;
                v += inv.d;
            }

            dstBase += s.width;
            row_u   += inv.b;
            row_v   += inv.e;
        }
    } else {
        uint8_t** srcLines = _line8 + im.offsetLine;
        uint8_t* dstBase   = &s.line8[_vga._backBufLine + y1][x1];
        const uint8_t mask = (uint8_t)(maskColor & 0xFF);
        const int dstSkip  = s.width - (x2 - x1);

        for (int y = y1; y < y2; y++) {
            uint8_t* dst = dstBase;

            int32_t u = row_u;
            int32_t v = row_v;

            for (int x = x1; x < x2; x++) {
                int sx = u >> FP_SHIFT;
                int sy = v >> FP_SHIFT;

                if ((unsigned)sx < (unsigned)srcW &&
                    (unsigned)sy < (unsigned)srcH)
                {
                    uint8_t* srcRow = srcLines[sy];
                    uint8_t col = srcRow[sx];
                    if (col != mask) *dst = col;
                }

                ++dst;
                u += inv.a;
                v += inv.d;
            }

            dstBase += s.width;
            row_u   += inv.b;
            row_v   += inv.e;
        }
    }
}
*/

uint16_t Sprite::getPixel(int x, int y, uint8_t num){
    if (!_created || num >= _images) return 0; 
    
    Image& im = _img[num];
    if (x < im.x1 || y < im.y1 || x > im.x2 ||  y > im.y2) return 0;

    return ((_bpp == _16BIT) ? _line16[im.offsetLine + y][x] : (uint16_t)_line8[im.offsetLine + y][x]);
}


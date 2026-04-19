#include "Tiles.h"

Tiles::Tiles(VGA_esp32& vga) : _vga(vga){

}

Tiles::~Tiles(){
    destroy();
}  

void Tiles::destroy(){
    if (_t.buf){ 
        heap_caps_free(_t.buf);
        _t.buf = nullptr;
    }   
    
    if (_t.offset){
        delete[] _t.offset;
        _t.offset = nullptr;
    }
}

bool Tiles::loadTiles(const uint8_t* data, uint8_t blockWidth, uint8_t blockHeight, uint8_t offsetX, uint8_t offsetY){
    if (!data) return (_t.loaded = false);
    if (_t.buf || _t.offset) destroy();

    uint8_t imgBPP = *data++;
    if (imgBPP != _8BIT && imgBPP != _16BIT) return (_t.loaded = false);
    if (*data++ != 1) return (_t.loaded = false);   // only one image
    data += 4; // skip image offset table

    int imgWidth  = *data++ | (*data++ << 8);
    int imgHeight = *data++ | (*data++ << 8);
    Serial.printf("Width: %d, Height: %d\n", imgWidth, imgHeight);

    int stepX = blockWidth + offsetX;
    int stepY = blockHeight + offsetY;

    int sizeX = (imgWidth  - offsetX) / stepX;
    int sizeY = (imgHeight - offsetY) / stepY;
    if (sizeX <= 0 || sizeY <= 0) return (_t.loaded = false);

    _t.bpp = imgBPP;   // лучше пока так
    _t.width = blockWidth;
    _t.height = blockHeight;
    _t.maxX = _t.width - 1;
    _t.maxY = _t.height - 1;
    _t.lineSize = _t.width * (_t.bpp == _16BIT ? 2 : 1);
    _t.size = _t.width * _t.height;
    _t.fullSize = _t.lineSize * _t.height;
    _t.tilesCount = sizeX * sizeY;
    _t.maxTile = _t.tilesCount - 1;
    Serial.println(_t.tilesCount);

    size_t size = _t.fullSize * _t.tilesCount;
    _t.buf = (uint8_t*)_vga.allocateMemory(size, true);
    if (!_t.buf) return (_t.loaded = false);

    _t.offset = new uint32_t[_t.tilesCount];
    uint32_t ofs = 0;
    for (int i = 0; i < _t.tilesCount; i++){
        _t.offset[i] = ofs;
        ofs += _t.fullSize;
    }

    if (_t.bpp == _16BIT){
        const uint16_t* src = (const uint16_t*)data;
        uint16_t* dst = (uint16_t*)_t.buf;

        for (int ty = 0; ty < sizeY; ty++){
            for (int tx = 0; tx < sizeX; tx++){
                int srcX = tx * stepX + offsetX;
                int srcY = ty * stepY + offsetY;

                for (int y = 0; y < blockHeight; y++){
                    const uint16_t* srcLine = src + (srcY + y) * imgWidth + srcX;
                    memcpy(dst, srcLine, blockWidth * 2);
                    dst += blockWidth;
                }
            }
        }
    } else {
        const uint8_t* src = data;
        uint8_t* dst = _t.buf;

        for (int ty = 0; ty < sizeY; ty++){
            for (int tx = 0; tx < sizeX; tx++){
                int srcX = tx * stepX + offsetX;
                int srcY = ty * stepY + offsetY;

                for (int y = 0; y < blockHeight; y++){
                    const uint8_t* srcLine = src + (srcY + y) * imgWidth + srcX;
                    memcpy(dst, srcLine, blockWidth);
                    dst += blockWidth;
                }
            }
        }
    }

    return (_t.loaded = true);
}

void Tiles::putTile(int x, int y, uint8_t num){
    if (!_t.loaded || num >= _t.tilesCount) return;

    int xx = x + _t.maxX;    
    int yy = y + _t.maxY;
    
    auto& s = _vga._scr;
    if (x > s.x1 || y > s.y1) return;
    if (xx < s.x0 || yy < s.y0) return;

    if (x >= s.x0 && y >= s.y0 && xx <= s.x1 && yy <= s.y1){
        // === FAST PATH: sprite fully inside viewport ===
        int lines = _t.height;

        if (s.bpp == _16BIT){
            uint8_t* img = _t.buf + _t.offset[num];
            uint8_t* scr = (uint8_t*)s.line16[_vga._backBufLine + y] + (x << 1);
            const int copyBytes = _t.lineSize;
            const int dstSkip = s.lineSize;

            while (lines--){
                memcpy(scr, img, copyBytes);
                img += copyBytes;
                scr += dstSkip;
            }
        } else {
            uint8_t* img = _t.buf + _t.offset[num];
            uint8_t* scr = (uint8_t*)s.line8[_vga._backBufLine + y] + x;
            const int copyBytes = _t.lineSize;
            const int dstSkip = s.lineSize;

            while (lines--){
                memcpy(scr, img, copyBytes);
                img += copyBytes;
                scr += dstSkip;
            }
        }
    } else {
        // === CLIPPED PATH ===
        int sxl = (x  < s.x0 ? (s.x0 - x)  : 0);
        int sxr = (xx > s.x1 ? (xx - s.x1) : 0);
        int syu = (y  < s.y0 ? (s.y0 - y)  : 0);
        int syd = (yy > s.y1 ? (yy - s.y1) : 0);

        int copyY = _t.height - syu - syd;
        if (copyY <= 0) return;

        if (s.bpp == _16BIT){
            int copyBytes = (_t.width - sxl - sxr) << 1;
            if (copyBytes <= 0) return;

            uint8_t* img = _t.buf + _t.offset[num] + syu * _t.lineSize + (sxl << 1);
            uint8_t* scr = (uint8_t*)s.line16[_vga._backBufLine + y + syu] + ((x + sxl) << 1);

            const int imgStep = _t.lineSize;
            const int scrStep = s.lineSize;

            while (copyY--){
                memcpy(scr, img, copyBytes);
                img += imgStep;
                scr += scrStep;
            }
        } else {
            int copyBytes = _t.width - sxl - sxr;
            if (copyBytes <= 0) return;

            uint8_t* img = _t.buf + _t.offset[num] + syu * _t.lineSize + sxl;
            uint8_t* scr = (uint8_t*)s.line8[_vga._backBufLine + y + syu] + (x + sxl);

            const int imgStep = _t.lineSize;
            const int scrStep = s.lineSize;

            while (copyY--){
                memcpy(scr, img, copyBytes);
                img += imgStep;
                scr += scrStep;
            }
        }
    }
}

bool Tiles::loadMap(const uint8_t* data){
    if (!data || !_t.loaded) return (_m.loaded = false);

    if (_m.buf){
        heap_caps_free(_m.buf);
        _m.buf = nullptr;        
        
    }

    _m.width  = *data++ | (*data++ << 8);
    _m.height = *data++ | (*data++ << 8);

    int size = _m.width * _m.height;
    _m.buf = (uint8_t*)_vga.allocateMemory(size, true);
    memcpy(_m.buf, data, size);

    _m.x0 = 0; _m.y0 = 0;
    _m.x1 = _vga._scr.width / _t.width;     _m.visX = _m.x1;
    _m.y1 = _vga._scr.height / _t.height;   _m.visY = _m.y1;
    Serial.printf("Width: %d, Height: %d\n", _m.x1, _m.y1);
    
    return (_m.loaded = true);
}


void Tiles::drawMap(int camX, int camY){
    if (!_t.loaded || !_m.loaded) return;

    int tw = _t.width;
    int th = _t.height;

    int firstTileX = camX / tw;
    int firstTileY = camY / th;

    int offsetX = -(camX % tw);
    int offsetY = -(camY % th);

    int visX = (offsetX == 0 ? _m.visX : _m.visX + 1);
    int visY = (offsetY == 0 ? _m.visY : _m.visY + 1);

    uint8_t* map = _m.buf + firstTileY * _m.width + firstTileX;

    for (int y = 0; y < visY; y++){
        uint8_t* saveMap = map;

        for (int x = 0; x < visX; x++){
            putTile(offsetX + x * tw, offsetY + y * th, *saveMap++);
        }

        map += _m.width;
    }
}

/*
void Tiles::drawMap(int camX, int camY){
    if (!_t.loaded || !_m.loaded) return;

    auto& s = _vga._scr;

    int tileW = _t.width;
    int tileH = _t.height;

    int firstTileX = camX / tileW;
    int firstTileY = camY / tileH;

    int offsetX = -(camX % tileW);
    int offsetY = -(camY % tileH);

    int visX = s.width  / tileW + 2;
    int visY = s.height / tileH + 2;

    for (int my = 0; my < visY; my++){
        int ty = firstTileY + my;
        if (ty < 0 || ty >= _m.height) continue;

        for (int mx = 0; mx < visX; mx++){
            int tx = firstTileX + mx;
            if (tx < 0 || tx >= _m.width) continue;

            uint8_t tile = _m.buf[ty * _m.width + tx];
            //if (tile == 0) continue;

            putTile(offsetX + mx * tileW, offsetY + my * tileH, tile);
        }
    }
}
    */
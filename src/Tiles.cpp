#include "Tiles.h"

Tiles::Tiles(VGA_esp32& vga) : _vga(vga){

}

Tiles::~Tiles(){
    if (_tiles) heap_caps_free(_tiles);
}

bool Tiles::loadTiles(const uint8_t* data,
                      uint8_t blockSizeX, uint8_t blockSizeY,
                      uint8_t offsetX, uint8_t offsetY)
{
    if (!data) return false;

    uint8_t imgBPP = *data++;
    if (imgBPP != _8BIT) return false;   // пока только 8-bit
    if (*data++ != 1) return false;      // only one image

    data += 4; // skip image offset table

    int imgWidth  = data[0] | (data[1] << 8); data += 2;
    int imgHeight = data[0] | (data[1] << 8); data += 2;

    if (blockSizeX == 0 || blockSizeY == 0) return false;
    if (imgWidth <= offsetX || imgHeight <= offsetY) return false;

    int stepX = blockSizeX + offsetX;
    int stepY = blockSizeY + offsetY;

    int sizeX = (imgWidth  - offsetX) / stepX;
    int sizeY = (imgHeight - offsetY) / stepY;

    if (sizeX <= 0 || sizeY <= 0) return false;

    _tileSizeX = blockSizeX;
    _tileSizeY = blockSizeY;
    _maxTiles  = sizeX * sizeY;

    int size = blockSizeX * blockSizeY * _maxTiles;
    if (!_vga.allocateMemory(_tiles, size, false)) return false;

    const uint8_t* srcImage = data;
    uint8_t* dst = _tiles;

    auto getDot = [&](int x, int y) -> uint8_t {
        if ((unsigned)x >= (unsigned)imgWidth ||
            (unsigned)y >= (unsigned)imgHeight) return 0;
        return srcImage[y * imgWidth + x];
    };

    for (int iy = 0; iy < sizeY; iy++) {
        int oy = offsetY + iy * (blockSizeY + offsetY);

        for (int ix = 0; ix < sizeX; ix++) {
            int ox = offsetX + ix * (blockSizeX + offsetX);

            for (int yy = 0; yy < blockSizeY; yy++) {
                for (int xx = 0; xx < blockSizeX; xx++) {
                    *dst++ = getDot(ox + xx, oy + yy);
                }
            }
        }
    }

    Serial.printf("Tiles loaded: %d (%d x %d)\n", _maxTiles, sizeX, sizeY);
    return true;
}

void Tiles::draw(int px, int py, uint16_t num){
    if (!_tiles || num >= _maxTiles) return;

    const int tw = 16;
    const int th = 16;
    const int sw = _vga._scr.width;
    const int sh = _vga._scr.height;

    uint8_t* img = _tiles + (tw * th * num);
    uint8_t* dst = _vga._scr.buf;

    for (int y = 0; y < th; y++){
        int dy = py + y;
        if ((unsigned)dy >= (unsigned)sh) continue;

        for (int x = 0; x < tw; x++){
            int dx = px + x;
            if ((unsigned)dx >= (unsigned)sw) continue;

            dst[dy * sw + dx] = img[y * tw + x];
        }
    }
}
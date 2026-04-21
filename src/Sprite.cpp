#include "Sprite.h"

Sprite::Sprite(VGA_esp32 &vga) : _vga(vga){

}

Sprite::~Sprite(){
    #if IS_P4
        deinitPPA();
    #endif   
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

bool Sprite::setBufferAddr() {
    if (!_buf || !_img || _images == 0) {
        Serial.println("setBufferAddr: invalid sprite buffer");
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

    int totalLines = 0;
    for (int i = 0; i < _images; i++) totalLines += _img[i].height;

    int line = 0;
    if (_bpp == _16BIT){
        uint16_t* base = (uint16_t*)_buf;
        _line16 = new uint16_t*[totalLines];

        for (int i = 0; i < _images; i++){
            Image &im = _img[i];
            for (int y = 0; y < im.height; y++){
                _line16[line++] = base;
                base += im.width;
            }
        }
    } else {
        uint8_t* base = _buf;
        _line8 = new uint8_t*[totalLines];

        for (int i = 0; i < _images; i++){
            Image &im = _img[i];
            for (int y = 0; y < im.height; y++){
                _line8[line++] = base;
                base += im.width;
            }
        }
    }

    return true;
}

bool Sprite::loadImages(const uint8_t* data){
    if (!data) return false;

    uint8_t imgBPP = *data++;
    if (imgBPP != _8BIT && imgBPP != _16BIT) return false;
    resetParam(*data++);

    data += _images << 2;
    const uint8_t* ptr = data;    

    int fullSize = 0;
    uint32_t offsetLine = 0;
    _bpp = _vga._scr.bpp; 
    _shift = (_bpp == _16BIT ? 1 : 0);
    int imgShift = (imgBPP == 16 ? 1 : 0); 

    for (int i = 0; i < _images; i++){
        _img[i].width       = (*data++) | (*data++ << 8);
        _img[i].height      = (*data++) | (*data++ << 8);
        _img[i].maxX        = _img[i].width - 1;
        _img[i].maxY        = _img[i].height - 1;
        _img[i].cx          = _img[i].width >> 1;
        _img[i].cy          = _img[i].height >> 1;
        _img[i].lineSize    = _img[i].width << _shift;
        _img[i].size        = _img[i].width * _img[i].height;
        _img[i].fullSize    = _img[i].lineSize * _img[i].height;
        _img[i].offset      = fullSize;
        _img[i].offsetLine  = offsetLine;

        // viewport
        _img[i].x0 = 0; _img[i].x1 = _img[i].maxX; 
        _img[i].y0 = 0; _img[i].y1 = _img[i].maxY;

        offsetLine  += _img[i].height;
        fullSize    += _img[i].fullSize;
        data        += (_img[i].width << imgShift) * _img[i].height;
    }
    _buf = (uint8_t*) _vga.allocateMemory(fullSize, true);
    if (!_buf) return false;

    //if (!_vga.allocateMemory(_buf, fullSize, false)) return false;  // DMA = false;
    if (!setBufferAddr()) return false;

    for (int i = 0; i < _images; i++){
        ptr += 4;
        int size = _img[i].size;

        if (imgBPP == _bpp){
            memcpy(_buf + _img[i].offset, ptr, _img[i].fullSize);   // Convert not needed
            ptr += _img[i].fullSize; 
        } else if (imgBPP == 8 && _bpp == 16){                      // Convert 8 -> 16
            uint16_t* dest = (uint16_t*)(_buf + _img[i].offset);

            while (size-- > 0){
                uint8_t col8 = *ptr++;

                uint8_t r3 = (col8 >> 5) & 0x07;
                uint8_t g3 = (col8 >> 2) & 0x07;
                uint8_t b2 =  col8       & 0x03;

                uint16_t r5 = (r3 << 2) | (r3 >> 1);
                uint16_t g6 = (g3 << 3) |  g3;
                uint16_t b5 = (b2 << 3) | (b2 << 1) | (b2 >> 1);

                *dest++ = (r5 << 11) | (g6 << 5) | b5;
            }
        } else if (imgBPP == 16 && _bpp == 8){
            uint16_t* sour = (uint16_t*)ptr;
            uint8_t* dest = _buf + _img[i].offset;

            while (size-- > 0){
                uint16_t col16 = *sour++;

                uint8_t r3 = (col16 >> 13) & 0x07;
                uint8_t g3 = (col16 >> 8)  & 0x07;
                uint8_t b2 = (col16 >> 3)  & 0x03;
                
                *dest++ = (r3 << 5) | (g3 << 2) | b2;
            }
            
            ptr += _img[i].fullSize;
        }
    }

    return (_created = true);
}

void Sprite::putImage(int x, int y, uint8_t num){
    if (!_created || num >= _images) return;

    auto& s = _vga._scr;
    Image& im = _img[num];

    if ((unsigned)x > (unsigned)s.x1 || (unsigned)y > (unsigned)s.y1) return;

    int xx = x + im.maxX;
    int yy = y + im.maxY;
    if (xx < s.x0 || yy < s.y0) return;

    // === FAST PATH: sprite fully inside viewport ===
    if (x >= s.x0 && y >= s.y0 && xx <= s.x1 && yy <= s.y1){
        int lines = im.height;

        if (_bpp == _16BIT){
            uint8_t* img = _buf + im.offset;
            uint8_t* scr = (uint8_t*)s.line16[_vga._backBufLine + y] + (x << 1);
            const int copyBytes = im.width << 1;
            const int dstSkip = s.lineSize;

            while (lines--){
                memcpy(scr, img, copyBytes);
                img += copyBytes;
                scr += dstSkip;
            }
        } else {
            uint8_t* img = _buf + im.offset;
            uint8_t* scr = (uint8_t*)s.line8[_vga._backBufLine + y] + x;
            const int copyBytes = im.width;
            const int dstSkip = s.lineSize;

            while (lines--){
                memcpy(scr, img, copyBytes);
                img += copyBytes;
                scr += dstSkip;
            }
        }
        return;
    }

    // === CLIPPED PATH ===
    int sxl = (x  < s.x0 ? (s.x0 - x)  : 0);
    int sxr = (xx > s.x1 ? (xx - s.x1) : 0);
    int syu = (y  < s.y0 ? (s.y0 - y)  : 0);
    int syd = (yy > s.y1 ? (yy - s.y1) : 0);

    int copyX = (im.width - sxl - sxr) << _shift;
    int copyY =  im.height - syu - syd;
    if (copyX <= 0 || copyY <= 0) return;

    uint8_t* img = (_bpp == _16BIT)
        ? (uint8_t*)_line16[im.offsetLine + syu] + (sxl << 1)
        : (uint8_t*)_line8 [im.offsetLine + syu] + sxl;

    uint8_t* scr = (_bpp == _16BIT)
        ? (uint8_t*)s.line16[_vga._backBufLine + y + syu] + ((x + sxl) << 1)
        : (uint8_t*)s.line8 [_vga._backBufLine + y + syu] + (x + sxl);

    const int imgStep = im.lineSize;
    const int scrStep = s.lineSize;

    while (copyY--){
        memcpy(scr, img, copyX);
        img += imgStep;
        scr += scrStep;
    }
}

void Sprite::putSprite(int x, int y, uint16_t maskColor, uint8_t num){
    if (!_created || num >= _images) return;
    
    auto& s = _vga._scr;
    Image &im = _img[num];
    if (x > s.x1 || y > s.y1) return;    
    int xx = x + im.maxX;
    int yy = y + im.maxY;
    if (xx < s.x0 || yy < s.y0) return;

    if (x >= s.x0 && y >= s.y0 && xx <= s.x1 && yy <= s.y1){
        int lines = im.height;  
        int skip = s.width - im.width;  

        if (_bpp == _16BIT){
            uint16_t* img = &_line16[im.offsetLine][0];
            uint16_t* scr = &s.line16[_vga._backBufLine + y][x];

            while (lines-- > 0){
                for (int xx = 0; xx < im.width; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }
                scr += skip;
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
                scr += skip;
            }  
        }
    } else { 
        int sxl = (x  < s.x0 ? (s.x0 - x)  : 0);
        int sxr = (xx > s.x1 ? (xx - s.x1) : 0);
        int syu = (y  < s.y0 ? (s.y0 - y)  : 0);
        int syd = (yy > s.y1 ? (yy - s.y1) : 0);

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

void Sprite::putAffineSprite(int dstX, int dstY, float ang, int zoomX, int zoomY, uint16_t maskColor, uint8_t num){
    if (!_vga._inited || !_created || num >= _images) return;

    auto& s = _vga._scr; 
    Image& im = _img[num];
    const int sourW = im.width;
    const int sourH = im.height;    

    mat = Matrix::make((float)dstX, (float)dstY, 
                    (sourW - 1) * 0.5f, (sourH - 1) * 0.5f, 
                    angle(ang), 
                    percentTo(zoomX), percentTo(zoomY)
    );
    
    rectMat = Matrix::bounds(mat, (float)sourW, (float)sourH);
    int x0 = (int)floorf(rectMat.sx); // округляет число вниз до ближайшего целого
    int y0 = (int)floorf(rectMat.sy);
    int x1 = (int)ceilf(rectMat.ex);  // округляет число вверх до ближайшего целого.
    int y1 = (int)ceilf(rectMat.ey);  

    x0 = std::max(x0, s.x0);
    y0 = std::max(y0, s.y0);
    x1 = std::min(x1, s.x1 + 1);
    y1 = std::min(y1, s.y1 + 1);
    if (x0 >= x1 || y0 >= y1) return;

    if (!Matrix::invert(invMat, mat)) return; 
    int row_u = invMat.a * x0 + invMat.b * y0 + invMat.c;
    int row_v = invMat.d * x0 + invMat.e * y0 + invMat.f;    

    int u, v, sx, sy;
    if (_bpp == _16BIT){
        uint16_t** sour = _line16 + im.offsetLine;
        uint16_t* dest = &s.line16[_vga._backBufLine + y0][x0];

        while (y0++ < y1){
            uint16_t* dst = dest;
            u = row_u;
            v = row_v;

            for (int x = x0; x < x1; x++){
                sx = u >> FP_SHIFT;
                sy = v >> FP_SHIFT;

                if ((unsigned)sx < (unsigned)sourW && (unsigned)sy < (unsigned)sourH){
                    uint16_t col = sour[sy][sx];
                    if (col != maskColor) *dst = col;
                }

                dst++;
                u += invMat.a;
                v += invMat.d;                
            }

            dest += s.width;
            row_u   += invMat.b;
            row_v   += invMat.e;            
        }
    } else { 
        uint8_t** srcLines = _line8 + im.offsetLine;
        uint8_t* dstBase   = &s.line8[_vga._backBufLine + y0][x0];
        const uint8_t mask = (uint8_t)(maskColor & 0xFF);

        for (int y = y0; y < y1; y++){
            uint8_t* dst = dstBase;
            int32_t u = row_u;
            int32_t v = row_v;

            for (int x = x0; x < x1; x++){
                int sx = u >> FP_SHIFT;
                int sy = v >> FP_SHIFT;

                if ((unsigned)sx < (unsigned)sourW &&
                    (unsigned)sy < (unsigned)sourH)
                {
                    uint8_t* srcRow = srcLines[sy];
                    uint8_t col = srcRow[sx];
                    if (col != mask) *dst = col;
                }

                ++dst;
                u += invMat.a;
                v += invMat.d;
            }

            dstBase += s.width;
            row_u   += invMat.b;
            row_v   += invMat.e;
        }  
    }  
}    

uint16_t Sprite::getPixel(int x, int y, uint8_t num){
    if (!_created || num >= _images) return 0; 
    
    Image& im = _img[num];
    if (x < im.x0 || y < im.y0 || x > im.x1 ||  y > im.y1) return 0;

    return ((_bpp == _16BIT) ? _line16[im.offsetLine + y][x] : (uint16_t)_line8[im.offsetLine + y][x]);
}

// PPA for P4 ==================================================================
#if IS_P4
bool Sprite::initPPA(){
    if (_ppa_srm) {
        Serial.println("PPA already initialized");
        return true;
    }

    ppa_client_config_t cfg = {};
    cfg.oper_type = PPA_OPERATION_SRM;
    cfg.max_pending_trans_num = 1;

    esp_err_t err = ppa_register_client(&cfg, &_ppa_srm);
    Serial.printf("ppa_register_client: %d (%s)\n", err, esp_err_to_name(err));

    return (err == ESP_OK);
}

void Sprite::deinitPPA(){
    if (_ppa_srm){
        esp_err_t err = ppa_unregister_client(_ppa_srm);
        Serial.printf("ppa_unregister_client: %d (%s)\n", err, esp_err_to_name(err));
        _ppa_srm = nullptr;
    }
}
/*
bool Sprite::initPPA() {
    if (_ppa_ready) return true;

    ppa_client_config_t cfg = {};
    cfg.oper_type = PPA_OPERATION_SRM;
    cfg.max_pending_trans_num = 1;

    esp_err_t err = ppa_register_client(&cfg, &_ppa_srm);
    if (err != ESP_OK) {
        Serial.printf("ppa_register_client failed: %s\n", esp_err_to_name(err));
        return false;
    }

    _ppa_ready = true;
    return true;
}

void Sprite::deinitPPA() {
    if (_ppa_srm) {
        ppa_unregister_client(_ppa_srm);
        _ppa_srm = nullptr;
    }
    _ppa_ready = false;
}
*/
bool Sprite::tryPPA(int dstX, int dstY, float zoomX, float zoomY, uint8_t num){
    if (_bpp != _16BIT) return false;

    if (_ppa_srm == nullptr) {
        Serial.println("PPA: client is null");
        return false;
    }

    auto& s = _vga._scr;
    Image& im = _img[num];

    ppa_srm_oper_config_t cfg = {};

    // INPUT
    cfg.in.buffer = (uint16_t*)(_buf + im.offset);//
    cfg.in.pic_w  = im.width;
    cfg.in.pic_h  = im.height;
    cfg.in.block_w = im.width;
    cfg.in.block_h = im.height;
    cfg.in.block_offset_x = 0;
    cfg.in.block_offset_y = 0;//
    cfg.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

    // OUTPUT
    cfg.out.buffer = (uint16_t*)&_vga._scr.line16[_vga._backBufLine][0];//
    cfg.out.buffer_size = s.fullSize;
    cfg.out.pic_w  = s.width;
    cfg.out.pic_h  = s.height;
    cfg.out.block_offset_x = dstX;
    cfg.out.block_offset_y = dstY;
    cfg.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

    // TRANSFORM
    cfg.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
    cfg.scale_x = zoomX;
    cfg.scale_y = zoomY;
    cfg.mirror_x = false;
    cfg.mirror_y = false;

    cfg.rgb_swap = false;
    cfg.byte_swap = false;
    cfg.alpha_update_mode = PPA_ALPHA_NO_CHANGE;
    cfg.alpha_fix_val = 0;

    cfg.mode = PPA_TRANS_MODE_BLOCKING;

    esp_err_t err = ppa_do_scale_rotate_mirror(_ppa_srm, &cfg);
    Serial.printf("PPA do SRM: %d (%s)\n", err, esp_err_to_name(err));

    return (err == ESP_OK);
}

#endif


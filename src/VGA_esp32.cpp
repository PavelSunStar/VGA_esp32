#include "VGA_esp32.h"
#include <esp_LCD_panel_ops.h>

VGA_esp32::VGA_esp32(){
    #if defined(psramFound)
        _psram_ok = psramFound();
    #else
        _psram_ok = esp_psram_is_initialized();
    #endif

    //FPS
    _frameCount = 0;
    _fpsStartTime = millis(); 
   // _cLine = 0;    
}

VGA_esp32::~VGA_esp32(){
    if (_scr.line8) delete[] _scr.line8;
    if (_scr.line16) delete[] _scr.line16;
    if (_scr.buf) heap_caps_free(_scr.buf);
}

bool VGA_esp32::allocateMemory(uint8_t* &buffer, size_t size, bool dma){
    if (size == 0) return false;

    size_t aligned = (size + 31) & ~size_t(31);

    uint32_t caps;
    if (dma) {
        caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT;
    } else if (_psram_ok) {
        caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    } else {
        caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    }

    buffer = (uint8_t*)heap_caps_aligned_alloc(32, aligned, caps);

    if (!buffer) {
        Serial.println(F("Error: failed to allocate image buffer"));
        return false;
    } else {
        memset(buffer, 0, aligned);
    }

    const char* ramType = "UNKNOWN";
    if (esp_ptr_external_ram(buffer)) {
        ramType = "PSRAM";
    } else if (esp_ptr_internal(buffer)) {
        ramType = "INTERNAL RAM";
    }

    Serial.printf("Allocated %u bytes in %s, ptr=%p\n",
                  (unsigned)aligned,
                  ramType,
                  buffer);

    return true;
}

bool VGA_esp32::setBufferAddr(){
    if (!_scr.buf || _scr.width <= 0 || _scr.height <= 0) {
        Serial.println("initBuffer: invalid screen buffer");
        return false;
    }

    if (_scr.line8) {
        delete[] _scr.line8;
        _scr.line8 = nullptr;
    }

    if (_scr.line16) {
        delete[] _scr.line16;
        _scr.line16 = nullptr;
    }   

    int lines = _scr.height * (_dBuff ? 2 : 1);
    if (_scr.bpp == 16) {
        uint16_t* p = (uint16_t*)_scr.buf;
        _scr.line16 = new uint16_t*[lines]();

        if (!_scr.line16) {
            Serial.println("initBuffer: failed to alloc line16 table");
            return false;
        }

        for (int i = 0; i < lines; i++) {
            _scr.line16[i] = p;
            p += _scr.width;
        }
    } else {
        uint8_t* p = _scr.buf;
        _scr.line8 = new uint8_t*[lines]();

        if (!_scr.line8) {
            Serial.println("initBuffer: failed to alloc line8 table");
            return false;
        }

        for (int i = 0; i < lines; i++) {
            _scr.line8[i] = p;
            p += _scr.width;
        }
    }

    Serial.println("Set buffer address...Ok");
    return true;
}

bool VGA_esp32::init(Mode &m, int bpp, int scale, int dBuff){
    if (m.pclk_hz == 0)                 return (_inited = false);
    if (bpp != _8BIT && bpp != _16BIT)  return (_inited = false);

    _m = m;
    _dBuff = dBuff;
    _scale = std::clamp(scale, 0, 2);
    _scr.bpp        = bpp;
    _scr.width      = m.hRes >> _scale;
    _scr.height     = m.vRes >> _scale;
    _scr.maxX       = _scr.width - 1;
    _scr.maxY       = _scr.height - 1;
    _scr.cx         = _scr.width >> 1;
    _scr.cy         = _scr.height >> 1;
    _scr.lineSize   = _scr.width << (_scr.bpp == _16BIT ? 1 : 0);
    _scr.size       = _scr.width * _scr.height;
    _scr.fullSize   = _scr.lineSize * _scr.height;

    _frontBuf = 0;      _backBuf = (_dBuff ? _scr.fullSize : 0);
    _frontBufLine = 0;  _backBufLine = (_dBuff ? _scr.height : 0);

    Serial.println("\n[=== Init VGA ===]");
    memoryInfo();
    setViewport(0, 0, _scr.maxX, _scr.maxY);

    if (!allocateMemory(_scr.buf, _scr.fullSize << (_dBuff ? 1 : 0))) return false;
    if (!setBufferAddr()) return false;
    if (!setRGBPanel()) return false;
    regSemaphore();
    regCallBack();
    if (!initPanel()) return false;

    Serial.println("VGA init...done\n");
    return (_inited = true);
}

void VGA_esp32::setViewport(int x1, int y1, int x2, int y2){
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    _scr.x1 = std::clamp(x1, 0, _scr.maxX);
    _scr.y1 = std::clamp(y1, 0, _scr.maxY);
    _scr.x2 = std::clamp(x2, 0, _scr.maxX);
    _scr.y2 = std::clamp(y2, 0, _scr.maxY);
}

void VGA_esp32::memoryInfo(){
    Serial.printf(
        "RAM free: %u | RAM largest: %u\nPSRAM free: %u | PSRAM largest: %u\n",
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)
    ); 
}

bool VGA_esp32::setRGBPanel(Pins pins){
    panel_config = {};

    //Timing
    panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
    panel_config.timings.pclk_hz                = _m.pclk_hz;
    panel_config.timings.h_res                  = _m.hRes;
    panel_config.timings.v_res                  = _m.vRes;

    panel_config.timings.hsync_pulse_width      = _m.hSync;
    panel_config.timings.hsync_back_porch       = _m.hBack;
    panel_config.timings.hsync_front_porch      = _m.hFront;
    panel_config.timings.flags.hsync_idle_low   = (_m.hPol == 1) ^ 1;

    panel_config.timings.vsync_pulse_width      = _m.vSync;
    panel_config.timings.vsync_back_porch       = _m.vBack;
    panel_config.timings.vsync_front_porch      = _m.vFront;
    panel_config.timings.flags.vsync_idle_low   = (_m.vPol == 1) ^ 1;
    
    panel_config.timings.flags.de_idle_high     = true;
    panel_config.timings.flags.pclk_active_neg  = true;
    panel_config.timings.flags.pclk_idle_high   = true;

    //Panel config
    panel_config.data_width             = (_scr.bpp == 16 ? 16 : 8);        
    panel_config.bits_per_pixel         = (_scr.bpp == 16 ? 16 : 8);   
    panel_config.num_fbs                = 0;
    panel_config.bounce_buffer_size_px  = optimal_bounce_buffer_px();
    panel_config.sram_trans_align       = 32;
    panel_config.psram_trans_align      = 32;
    //panel_config.dma_burst_size = 64;

    //Pins config
    if (_scr.bpp == 16){
        // B0..B4
        for (int i = 0; i < 5; i++)
            panel_config.data_gpio_nums[i] = pins.b[i];

        // G0..G5
        for (int i = 0; i < 6; i++)
            panel_config.data_gpio_nums[5 + i] = pins.g[i];

        // R0..R4
        for (int i = 0; i < 5; i++)
            panel_config.data_gpio_nums[11 + i] = pins.r[i];
    } else {
        panel_config.data_gpio_nums[0] = pins.b[3];
        panel_config.data_gpio_nums[1] = pins.b[4];
        panel_config.data_gpio_nums[2] = pins.g[3];
        panel_config.data_gpio_nums[3] = pins.g[4];
        panel_config.data_gpio_nums[4] = pins.g[5];
        panel_config.data_gpio_nums[5] = pins.r[2];
        panel_config.data_gpio_nums[6] = pins.r[3];
        panel_config.data_gpio_nums[7] = pins.r[4];
    }
    panel_config.hsync_gpio_num = pins.h;
    panel_config.vsync_gpio_num = pins.v;
    panel_config.de_gpio_num = -1;
    panel_config.pclk_gpio_num = pins.pClk;
    panel_config.disp_gpio_num = -1;

    //Flags
    panel_config.flags.disp_active_low = true;
    panel_config.flags.refresh_on_demand = false;
    panel_config.flags.fb_in_psram = false;
    panel_config.flags.double_fb = false;
    panel_config.flags.no_fb = true;
    panel_config.flags.bb_invalidate_cache = false;
    
    esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &panel_handle);
    if (err != ESP_OK) {
        Serial.printf("esp_lcd_new_rgb_panel error: %d\n", err);
        return false;
    }

    Serial.println("RGB Panel set...Ok");
    return true;
}

bool VGA_esp32::initPanel(){
    err = esp_lcd_panel_reset(panel_handle);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to reset panel: %s\n", esp_err_to_name(err));
        return false;
    }

    err = esp_lcd_panel_init(panel_handle);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to init panel: %s\n", esp_err_to_name(err));
        return false;
    }

    Serial.println("Init panel complete...Ok");
    return true;
}

void VGA_esp32::setPins(Pins p){
    _pins = p;
}

void VGA_esp32::setPins(
    int r0, int r1, int r2, int r3, int r4,
    int g0, int g1, int g2, int g3, int g4, int g5,
    int b0, int b1, int b2, int b3, int b4,
    int hsync, int vsync,
    int pClkPin)
{
    // Заполняем структуру
    _pins.r[0] = r0; _pins.r[1] = r1; _pins.r[2] = r2; _pins.r[3] = r3; _pins.r[4] = r4;
    _pins.g[0] = g0; _pins.g[1] = g1; _pins.g[2] = g2; _pins.g[3] = g3; _pins.g[4] = g4; _pins.g[5] = g5;
    _pins.b[0] = b0; _pins.b[1] = b1; _pins.b[2] = b2; _pins.b[3] = b3; _pins.b[4] = b4;
    _pins.h = hsync;
    _pins.v = vsync;
    _pins.pClk = pClkPin;
}

/*
uint16_t VGA_esp32::optimal_bounce_buffer_px() {
    int res = 0;
    if (_m.hRes == 640 && _m.vRes == 480) {
        res = (_scr.bpp == 16) ? 15360 : 30720;
    }

    const int bppBytes = (_scr.bpp == 16) ? 2 : 1;
    const int outLineBytes = _m.hRes * bppBytes;

    _lines       = (res / outLineBytes) >> (_scr.bpp == 16 ? 0 :_scale);
    _lastPos     = (_m.hRes * _m.vRes * bppBytes) - res;
    _copyBytes   = outLineBytes;
    _copyBytes2x = _copyBytes << 1;

    if (_scr.bpp == 8) {
        _pixels = _m.hRes >> 3;   
        _skip   = _m.hRes >> 2;   
    } else {
        _pixels = _m.hRes >> 3;   
        _skip   = _m.hRes >> 1;   
    }

    return res;
}
    */

uint16_t VGA_esp32::optimal_bounce_buffer_px(){
    int res = 0;
    _shift = (_scr.bpp == 16 ? 1 : 0);
    if (_m.hRes == 640 && _m.vRes == 480) res = 30720 >> _shift;    
    _lastPos     = (_m.hRes * (_m.vRes << _shift)) - res;


    _lines = (res / _m.hRes) >> _scale;
    _pixels = _m.hRes >> 3;
    _copyBytes = _m.hRes << _shift;
    _copyBytes2x = _copyBytes << 1;
    _skip = (_m.hRes >> 2) << _shift;

    return res;
}

bool VGA_esp32::regSemaphore(){
    if (!_dBuff){ 
        Serial.println("Double buffer not selected.");
        return false;
    } else {
        sem_vsync_end = xSemaphoreCreateBinary();
        sem_gui_ready = xSemaphoreCreateBinary();

        if (!sem_vsync_end || !sem_gui_ready){
            Serial.println("Error: Semaphores init fail.");
            return false;
        }    
    }

    Serial.println("Registered semaphores...Ok");
    return true;
}

void VGA_esp32::regCallBack(){
    esp_lcd_rgb_panel_event_callbacks_t cb = {
        .on_color_trans_done = nullptr,
        .on_vsync            = on_vsync,
        .on_bounce_empty     = on_bounce_empty,                
        .on_frame_buf_complete = nullptr
    };

    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cb, this)); 
    Serial.println("Registered callBacks...Ok"); 
}

bool IRAM_ATTR VGA_esp32::on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx){
    VGA_esp32* vga = (VGA_esp32*)user_ctx;

    vga->_timer++;

    return false;
}

bool IRAM_ATTR VGA_esp32::on_bounce_empty(
        esp_lcd_panel_handle_t panel,
        void *bounce_buf,
        int pos_px,
        int len_bytes,
        void *user_ctx)
{
    VGA_esp32* vga = (VGA_esp32*) user_ctx;

    if (vga->_scale == 0){
        uint8_t *dest = (uint8_t*)(bounce_buf);
        uint8_t *sour = vga->_scr.buf + vga->_frontBuf + (pos_px << (vga->_shift));
        memcpy(dest, sour, len_bytes);
    } else {
        uint32_t *dest      = (uint32_t*)(bounce_buf);        
        uint32_t col;        
        int lines           = vga->_lines;
        int pixels          = vga->_pixels;
        int copyBytes       = vga->_copyBytes;
        int copyBytes2x     = vga->_copyBytes2x;
        int skip            = vga->_skip;
        uint32_t* savePos   = nullptr; 
        
        if (vga->_scale == 1){
            if (vga->_scr.bpp == 16){
                uint16_t *sour = (uint16_t*)(vga->_scr.buf + vga->_frontBuf) + (pos_px >> 2);

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                    }    
                    memcpy(dest, savePos, copyBytes);
                    dest += skip;
                } 
            } else {
                uint8_t *sour = vga->_scr.buf + vga->_frontBuf + (pos_px >> 2);

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                        col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);
                    dest += skip;
                } 
            }        
        } else {
            int skip2 = skip << 1;

            if (vga->_scr.bpp == 16){
                uint16_t *sour = (uint16_t*)(vga->_scr.buf + vga->_frontBuf) + (pos_px >> 4);

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);   dest += skip;
                    memcpy(dest, savePos, copyBytes2x); dest += skip2;
                }
            } else {
                uint8_t *sour = vga->_scr.buf + vga->_frontBuf + (pos_px >> 4);

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                        col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);   dest += skip;
                    memcpy(dest, savePos, copyBytes2x); dest += skip2;
                }
            }       
        }
    }

    BaseType_t hp_task_woken = pdFALSE;
    if (pos_px >= vga->_lastPos && vga->_dBuff) {
        if (xSemaphoreTakeFromISR(vga->sem_gui_ready, &hp_task_woken) == pdTRUE) {
            std::swap(vga->_frontBuf, vga->_backBuf);
            std::swap(vga->_frontBufLine, vga->_backBufLine);
            xSemaphoreGiveFromISR(vga->sem_vsync_end, &hp_task_woken);
        }
    }   
      
    return (hp_task_woken == pdTRUE);
}

void VGA_esp32::swap(){
    if (_dBuff){
        xSemaphoreGive(sem_gui_ready);
        xSemaphoreTake(sem_vsync_end, portMAX_DELAY);
    }

    updateFPS();
}

void VGA_esp32::updateFPS(){
    uint64_t now = millis();
    _frameCount++;

    if (now - _fpsStartTime >= 1000) {
        _fps = _frameCount * 1000.0f / (now - _fpsStartTime);
        _frameCount = 0;
        _fpsStartTime = now;
    }
}

/*
void VGA_esp32::make_rotation_matrix(float* r, float dst_x, float dst_y, float src_x, float src_y, float angle_deg, float zoom_x, float zoom_y){
    float rad = fmodf(angle_deg, 360.0f) * deg_to_rad;
    float s = sinf(rad);
    float c = cosf(rad);
    r[0] =  c * zoom_x;
    r[1] = -s * zoom_y;
    r[2] =  dst_x - src_x * r[0] - src_y * r[1];
    r[3] =  s * zoom_x;
    r[4] =  c * zoom_y;
    r[5] =  dst_y - src_x * r[3] - src_y * r[4];
}

bool VGA_esp32::make_invert_affine32(int32_t* out, const float* m){
    float det = m[0] * m[4] - m[1] * m[3];
    if (det == 0.0f) return false;
    det = (float)FP_ONE / det;

    out[0] = (int32_t)lroundf(det *  m[4]);                         // A
    out[1] = (int32_t)lroundf(det * -m[1]);                         // B
    out[2] = (int32_t)lroundf(det * (m[1] * m[5] - m[2] * m[4]));   // C
    out[3] = (int32_t)lroundf(det * -m[3]);                         // D
    out[4] = (int32_t)lroundf(det *  m[0]);                         // E
    out[5] = (int32_t)lroundf(det * (m[2] * m[3] - m[0] * m[5]));   // F

    return true;
}

/*
bool IRAM_ATTR VGA_esp32::on_bounce_empty(
        esp_lcd_panel_handle_t panel,
        void *bounce_buf,
        int pos_px,
        int len_bytes,
        void *user_ctx)
{
    VGA_esp32* vga = (VGA_esp32*)user_ctx;

    if (vga->_scr.bpp == 16) {
        uint16_t* dest = (uint16_t*)bounce_buf;
        uint16_t* sour = (uint16_t*)(vga->_scr.buf + vga->_frontBuf);

        if (vga->_scale == 0) {
            memcpy(dest, sour + pos_px, len_bytes);

        } else if (vga->_scale == 1) {
            sour += (pos_px >> 2);
            int lines = vga->_lines;
            uint16_t* savePos = nullptr;

            while (lines-- > 0) {
                savePos = dest;

                for (int i = 0; i < vga->_tik; i++) {
                    dup2_565(dest, *sour++);
                    dup2_565(dest, *sour++);
                    dup2_565(dest, *sour++);
                    dup2_565(dest, *sour++);
                    dup2_565(dest, *sour++);
                    dup2_565(dest, *sour++);
                    dup2_565(dest, *sour++);
                    dup2_565(dest, *sour++);
                }

                memcpy(dest, savePos, vga->_copyBytes);
                dest += vga->_m.hRes;
            }

        } else {
            sour += (pos_px >> 4);
            int lines = vga->_lines;
            uint16_t* savePos = nullptr;

            while (lines-- > 0) {
                savePos = dest;

                for (int i = 0; i < vga->_tik; i++) {
                    dup4_565(dest, *sour++);
                    dup4_565(dest, *sour++);
                    dup4_565(dest, *sour++);
                    dup4_565(dest, *sour++);
                }

                memcpy(dest, savePos, vga->_copyBytes);
                dest += vga->_m.hRes;

                memcpy(dest, savePos, vga->_copyBytes2x);
                dest += vga->_copyBytes;
            }
        }

    } else {
        uint8_t* dest = (uint8_t*)bounce_buf;
        uint8_t* sour = vga->_scr.buf + vga->_frontBuf;

        if (vga->_scale == 0) {
            memcpy(dest, sour + pos_px, len_bytes);

        } else if (vga->_scale == 1) {
            sour += (pos_px >> 2);
            int lines = vga->_lines;
            uint8_t* savePos = nullptr;

            while (lines-- > 0) {
                savePos = dest;

                for (int i = 0; i < vga->_tik; i++) {
                    dup2_8(dest, *sour++);
                    dup2_8(dest, *sour++);
                    dup2_8(dest, *sour++);
                    dup2_8(dest, *sour++);
                    dup2_8(dest, *sour++);
                    dup2_8(dest, *sour++);
                    dup2_8(dest, *sour++);
                    dup2_8(dest, *sour++);
                }

                memcpy(dest, savePos, vga->_copyBytes);
                dest += vga->_m.hRes;
            }

        } else {
            sour += (pos_px >> 4);
            int lines = vga->_lines;
            uint8_t* savePos = nullptr;

            while (lines-- > 0) {
                savePos = dest;

                for (int i = 0; i < vga->_tik; i++) {
                    dup4_8(dest, *sour++);
                    dup4_8(dest, *sour++);
                    dup4_8(dest, *sour++);
                    dup4_8(dest, *sour++);
                }

                memcpy(dest, savePos, vga->_copyBytes);
                dest += vga->_m.hRes;

                memcpy(dest, savePos, vga->_copyBytes2x);
                dest += vga->_copyBytes2x;
            }
        }
    }

    BaseType_t hp_task_woken = pdFALSE;
    if (pos_px >= vga->_lastPos && vga->_dBuff) {
        if (xSemaphoreTakeFromISR(vga->sem_gui_ready, &hp_task_woken) == pdTRUE) {
            std::swap(vga->_frontBuf, vga->_backBuf);
            std::swap(vga->_frontBufLine, vga->_backBufLine);
            xSemaphoreGiveFromISR(vga->sem_vsync_end, &hp_task_woken);
        }
    }

    return (hp_task_woken == pdTRUE);
}
*/
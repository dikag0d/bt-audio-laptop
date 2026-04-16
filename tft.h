#ifndef TFT_H
#define TFT_H

#include <stdint.h>
#include <stdbool.h>

#define TFT_WIDTH 240
#define TFT_HEIGHT 240

// Inisialisasi simulator TFT dengan SDL2
bool tft_init(void);

// Render/Update layar fisik dengan data yang sudah di-draw
void tft_update(void);

// Menutup dan membersihkan SDL2
void tft_close(void);

// Menggambar satu pixel berwarna
void tft_draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);

// Mewarnai layar dengan warna solid (Fill Screen)
void tft_fill_screen(uint8_t r, uint8_t g, uint8_t b);

// Menggambar bujursangkar / segi empat
void tft_draw_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);

// --- Tambahan Baru: Render Teks Sederhana ---
// Menggambar satu karakter (5x7 font)
void tft_draw_char(int x, int y, char c, uint8_t r, uint8_t g, uint8_t b, int size);

// Menggambar string/teks
void tft_draw_string(int x, int y, const char* str, uint8_t r, uint8_t g, uint8_t b, int size);

// SDL event loop handler
bool tft_process_events(void);

#endif // TFT_H

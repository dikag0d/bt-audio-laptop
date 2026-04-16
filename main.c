#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>
#include "tft.h"

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Menjalankan Animasi Efek Plasma (Menyerupai GIF) 240x240...\n");

    if (!tft_init()) {
        printf("Gagal menyalakan layar!\n");
        return 1;
    }

    bool isRunning = true;
    
    // Looping rendering / game-loop
    while (isRunning) {
        if (!tft_process_events()) {
            isRunning = false;
            break;
        }

        // Dapatkan waktu berjalan (dalam detik)
        float time = SDL_GetTicks() / 1000.0f;

        // Render efek plasma ke setiap titik piksel di 240x240
        // Ini adalah algoritma populer demo-scene retro
        for (int x = 0; x < TFT_WIDTH; x++) {
            for (int y = 0; y < TFT_HEIGHT; y++) {
                
                // Normalisasi koordinat agar perhitungan sinus lebih halus
                float cx = (float)x / 50.0f;
                float cy = (float)y / 50.0f;

                // Perhitungan gelombang plasma
                float v = 0.0f;
                v += sinf(cx + time);
                v += sinf(cy + time / 2.0f);
                v += sinf((cx + cy + time) / 2.0f);
                v += sinf(sqrtf(cx * cx + cy * cy) + time);

                // Konversi hasil gelombang (v) menjadi RGB warna-warni yang mengalir
                uint8_t r = (uint8_t)((sinf(v * 3.14159f) * 0.5f + 0.5f) * 255.0f);
                uint8_t g = (uint8_t)((sinf(v * 3.14159f + 2.0f * 3.14159f / 3.0f) * 0.5f + 0.5f) * 255.0f);
                uint8_t b = (uint8_t)((sinf(v * 3.14159f + 4.0f * 3.14159f / 3.0f) * 0.5f + 0.5f) * 255.0f);

                tft_draw_pixel(x, y, r, g, b);
            }
        }

        // Karena kita menggambar seluruh 57000 piksel, kita tidak butuh fill_screen
        tft_update();

        // 16ms untuk kecepatan kurang lebih 60 Frame Per Second
        SDL_Delay(16);
    }

    tft_close();
    printf("TFT telah dimatikan.\n");

    return 0;
}

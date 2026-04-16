#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "tft.h"

#define BT_DEVICE_PATH "/org/bluez/hci0/dev_48_87_59_00_AC_6E"

void trim(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == ' ')) {
        str[--len] = '\0';
    }
}

// Ambil nilai track metadata dari Bluetooth AVRCP via dbus
void get_bt_track_field(const char *field, char *out, int max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "dbus-send --system --dest=org.bluez --print-reply "
        BT_DEVICE_PATH "/player0 "
        "org.freedesktop.DBus.Properties.Get "
        "string:\"org.bluez.MediaPlayer1\" string:\"Track\" 2>/dev/null "
        "| grep -A1 '\"%s\"' | tail -1 "
        "| sed 's/.*string \"//;s/.*uint32 //;s/\"$//'",
        field);
    
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fgets(out, max_len, fp) == NULL) {
            strcpy(out, "");
        }
        pclose(fp);
        trim(out);
    }
}

// Ambil status player Bluetooth (playing/paused/stopped)
void get_bt_status(char *out, int max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "dbus-send --system --dest=org.bluez --print-reply "
        BT_DEVICE_PATH "/player0 "
        "org.freedesktop.DBus.Properties.Get "
        "string:\"org.bluez.MediaPlayer1\" string:\"Status\" 2>/dev/null "
        "| grep 'string' | head -1 | sed 's/.*string \"//;s/\"$//'");
    
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fgets(out, max_len, fp) == NULL) {
            strcpy(out, "stopped");
        }
        pclose(fp);
        trim(out);
    }
}

// Helper untuk menggambar garis (Bresenham)
void tft_draw_line(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        tft_draw_pixel(x1, y1, r, g, b);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

// Ambil nilai uint32 dari properti dbus
uint32_t get_bt_uint32(const char *property) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "dbus-send --system --dest=org.bluez --print-reply "
        BT_DEVICE_PATH "/player0 "
        "org.freedesktop.DBus.Properties.Get "
        "string:\"org.bluez.MediaPlayer1\" string:\"%s\" 2>/dev/null "
        "| grep uint32 | awk '{print $NF}'",
        property);
    FILE *fp = popen(cmd, "r");
    uint32_t val = 0;
    if (fp) {
        if (fscanf(fp, "%u", &val) != 1) val = 0;
        pclose(fp);
    }
    return val;
}

// Ambil nilai uint32 dari dictionary Track
uint32_t get_bt_track_uint32(const char *field) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "dbus-send --system --dest=org.bluez --print-reply "
        BT_DEVICE_PATH "/player0 "
        "org.freedesktop.DBus.Properties.Get "
        "string:\"org.bluez.MediaPlayer1\" string:\"Track\" 2>/dev/null "
        "| grep -A1 '\"%s\"' | tail -1 | grep uint32 | awk '{print $NF}'",
        field);
    FILE *fp = popen(cmd, "r");
    uint32_t val = 0;
    if (fp) {
        if (fscanf(fp, "%u", &val) != 1) val = 0;
        pclose(fp);
    }
    return val;
}

// Menggambar ikon Bluetooth yang lebih akurat
void draw_bt_icon(int cx, int cy, uint8_t r, uint8_t g, uint8_t b) {
    // Bluetooth logo points scale approx +/- 12
    tft_draw_line(cx, cy - 12, cx, cy + 12, r, g, b);
    tft_draw_line(cx + 1, cy - 12, cx + 1, cy + 12, r, g, b); // Tebalkan

    tft_draw_line(cx, cy - 12, cx + 8, cy - 6, r, g, b);
    tft_draw_line(cx + 8, cy - 6, cx - 8, cy + 6, r, g, b);
    tft_draw_line(cx - 8, cy - 6, cx + 8, cy + 6, r, g, b);
    tft_draw_line(cx + 8, cy + 6, cx, cy + 12, r, g, b);
}

int main() {
    if (!tft_init()) return 1;

    char device_name[64] = "Mencari...";
    char song_title[128] = "";
    char artist[128] = "";
    char bt_status[32] = "";
    int scroll_offset = 0;
    int artist_scroll = 0;

    printf("Smart Dashboard TFT - Bluetooth HP Mode\n");

    while (tft_process_events()) {
        // === 1. Ambil Nama Device Bluetooth ===
        FILE *fp_bt = popen("bluetoothctl info 2>/dev/null | grep 'Name:' | cut -d' ' -f2-", "r");
        if (fp_bt) {
            char tmp[64] = "";
            if (fgets(tmp, sizeof(tmp), fp_bt) != NULL) {
                trim(tmp);
                if (strlen(tmp) > 0) strncpy(device_name, tmp, sizeof(device_name) - 1);
                else strcpy(device_name, "Tidak ada");
            }
            pclose(fp_bt);
        }

        // === 2. Ambil metadata LANGSUNG dari Bluetooth HP ===
        get_bt_track_field("Title", song_title, sizeof(song_title));
        get_bt_track_field("Artist", artist, sizeof(artist));
        get_bt_status(bt_status, sizeof(bt_status));
        
        uint32_t position = get_bt_uint32("Position");
        uint32_t duration = get_bt_track_uint32("Duration");

        // Bersihkan artis dari " . Recommended for you" jika ada
        char *rec = strstr(artist, " \xe2\x80\xa2");
        if (rec) *rec = '\0';
        rec = strstr(artist, " -");
        if (rec) *rec = '\0';

        if (strlen(song_title) == 0) strcpy(song_title, "Tidak ada lagu");
        if (strlen(artist) == 0) strcpy(artist, "Unknown");

        // ========== RENDERING ==========
        tft_fill_screen(15, 15, 35);

        // --- Header Bar ---
        tft_draw_rect(0, 0, 240, 50, 30, 30, 70);
        tft_draw_rect(0, 48, 240, 2, 0, 120, 255);
        draw_bt_icon(25, 25, 0, 180, 255);
        tft_draw_string(45, 10, "SMART", 255, 255, 255, 3);
        tft_draw_string(45, 30, "DASHBOARD", 100, 180, 255, 2);

        // --- Nama HP ---
        tft_draw_string(10, 60, "DEVICE:", 120, 120, 120, 2);
        char disp_name[14];
        strncpy(disp_name, device_name, 13);
        disp_name[13] = '\0';
        tft_draw_string(10, 80, disp_name, 0, 220, 255, 3);

        // --- Garis Pemisah ---
        tft_draw_rect(10, 108, 220, 1, 50, 50, 90);

        // --- Status (dari HP) ---
        if (strcmp(bt_status, "playing") == 0) {
            for (int i = 0; i < 12; i++)
                tft_draw_rect(12, 118 + i, 12 - i, 1, 0, 220, 100);
            tft_draw_string(30, 118, "PLAYING", 0, 220, 100, 2);
        } else if (strcmp(bt_status, "paused") == 0) {
            tft_draw_rect(12, 118, 4, 14, 255, 200, 0);
            tft_draw_rect(20, 118, 4, 14, 255, 200, 0);
            tft_draw_string(30, 118, "PAUSED", 255, 200, 0, 2);
        } else {
            tft_draw_string(10, 118, "STOPPED", 150, 150, 150, 2);
        }

        // --- Judul Lagu (size 3, marquee jika panjang) ---
        int title_len = strlen(song_title);
        if (title_len <= 13) {
            tft_draw_string(10, 142, song_title, 255, 255, 255, 3);
        } else {
            char scroll_buf[14];
            for (int i = 0; i < 13; i++) {
                int idx = (scroll_offset + i) % (title_len + 4);
                scroll_buf[i] = (idx < title_len) ? song_title[idx] : ' ';
            }
            scroll_buf[13] = '\0';
            tft_draw_string(10, 142, scroll_buf, 255, 255, 255, 3);
            scroll_offset = (scroll_offset + 1) % (title_len + 4);
        }

        // --- Artis (size 2, marquee jika panjang) ---
        int artist_len = strlen(artist);
        if (artist_len <= 19) {
            tft_draw_string(10, 172, artist, 180, 180, 200, 2);
        } else {
            char art_buf[20];
            for (int i = 0; i < 19; i++) {
                int idx = (artist_scroll + i) % (artist_len + 4);
                art_buf[i] = (idx < artist_len) ? artist[idx] : ' ';
            }
            art_buf[19] = '\0';
            tft_draw_string(10, 172, art_buf, 180, 180, 200, 2);
            artist_scroll = (artist_scroll + 1) % (artist_len + 4);
        }

        // --- Progress Bar ---
        tft_draw_rect(10, 205, 220, 3, 40, 40, 60);
        int bar_width = 0;
        if (duration > 0) {
            bar_width = (int)((float)position / duration * 220);
        }
        if (bar_width > 220) bar_width = 220;
        
        tft_draw_rect(10, 205, bar_width, 3, 0, 200, 255);
        tft_draw_rect(10 + bar_width - 3, 202, 6, 9, 0, 220, 255);

        // --- Timer (mm:ss / mm:ss) ---
        char time_buf[32];
        uint32_t cur_s = position / 1000;
        uint32_t tot_s = duration / 1000;
        snprintf(time_buf, sizeof(time_buf), "%02u:%02u / %02u:%02u", 
                 cur_s / 60, cur_s % 60, tot_s / 60, tot_s % 60);
        tft_draw_string(10, 215, time_buf, 150, 150, 180, 1);

        // --- Footer ---
        tft_draw_string(40, 225, "FROM PHONE BT", 60, 60, 80, 1);

        tft_update();
        SDL_Delay(500);
    }

    tft_close();
    return 0;
}

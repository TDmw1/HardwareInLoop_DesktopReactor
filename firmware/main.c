#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/watchdog.h" //verification framework

//AUTHOR: TD Medde-Witage

// 1. PIN DEFINITIONS
#define SPI_PORT spi0
#define PIN_SCK  18
#define PIN_SDA  19
#define PIN_RES  12
#define PIN_DC   13
#define PIN_CS   17
#define PIN_BLK  16

// AD3 HARDWARE PROFILING PINS 
#define PIN_DBG_RX     14  // Probes USB read latency
#define PIN_DBG_RENDER 15  // Probes Graphics execution time

// 2. COLOR DEFINITIONS (16-bit RGB565 format)
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED   0xF800
#define COLOR_YELLOW 0xFFE0

// ==========================================
// GC9A01 HARDWARE DRIVER FUNCTIONS
// ==========================================

void write_cmd(uint8_t cmd) {
    gpio_put(PIN_DC, 0); // Command mode
    gpio_put(PIN_CS, 0); // Activate chip
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1); // Deactivate chip
}

void write_data(uint8_t data) {
    gpio_put(PIN_DC, 1); // Data mode
    gpio_put(PIN_CS, 0); // Activate chip
    spi_write_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1); // Deactivate chip
}

void lcd_init() {
    // 1. Hardware Reset
    gpio_put(PIN_RES, 1); sleep_ms(10);
    gpio_put(PIN_RES, 0); sleep_ms(50);
    gpio_put(PIN_RES, 1); sleep_ms(120);

    // 2. Inner Register Enable
    write_cmd(0xFE);
    write_cmd(0xEF);

    // 3. Power Control & Gamma Voltages
    write_cmd(0xEB); write_data(0x14); 
    write_cmd(0x84); write_data(0x40); 
    write_cmd(0x85); write_data(0xFF); 
    write_cmd(0x86); write_data(0xFF); 
    write_cmd(0x87); write_data(0xFF);
    write_cmd(0x88); write_data(0x0A);
    write_cmd(0x89); write_data(0x21); 
    write_cmd(0x8A); write_data(0x00); 
    write_cmd(0x8B); write_data(0x80); 
    write_cmd(0x8C); write_data(0x01); 
    write_cmd(0x8D); write_data(0x01); 
    write_cmd(0x8E); write_data(0xFF); 
    write_cmd(0x8F); write_data(0xFF); 

    // 4. Memory Access & Color Format
    write_cmd(0xB6); write_data(0x00); write_data(0x00); 
    write_cmd(0x3A); write_data(0x05); 
    write_cmd(0x36); write_data(0x08); //BGR Mode

    // 5. Display Tuning
    write_cmd(0xC3); write_data(0x13); 
    write_cmd(0xC4); write_data(0x13); 
    write_cmd(0xC9); write_data(0x22); 

    // 6. Frame Rate & Panel Settings
    write_cmd(0xF0); write_data(0x45); write_data(0x09); write_data(0x08); write_data(0x08); write_data(0x26); write_data(0x2A); 
    write_cmd(0xF1); write_data(0x43); write_data(0x70); write_data(0x72); write_data(0x36); write_data(0x37); write_data(0x6F); 
    write_cmd(0xF2); write_data(0x45); write_data(0x09); write_data(0x08); write_data(0x08); write_data(0x26); write_data(0x2A); 
    write_cmd(0xF3); write_data(0x43); write_data(0x70); write_data(0x72); write_data(0x36); write_data(0x37); write_data(0x6F); 

    write_cmd(0x60); write_data(0x38); write_data(0x0B); write_data(0x6D); write_data(0x6D); write_data(0x39); write_data(0xF0); write_data(0x6D); write_data(0x6D); 
    write_cmd(0x61); write_data(0x38); write_data(0xF4); write_data(0x1F); write_data(0x1F); write_data(0x38); write_data(0xF0); write_data(0x1F); write_data(0x1F); 
    write_cmd(0x64); write_data(0x28); write_data(0x29); write_data(0xF1); write_data(0x01); write_data(0xF1); write_data(0x00); write_data(0x07); 
    write_cmd(0x66); write_data(0x3C); write_data(0x00); write_data(0xCD); write_data(0x67); write_data(0x45); write_data(0x45); write_data(0x10); write_data(0x00); write_data(0x00); write_data(0x00); 
    write_cmd(0x67); write_data(0x00); write_data(0x3C); write_data(0x00); write_data(0x00); write_data(0x00); write_data(0x01); write_data(0x54); write_data(0x10); write_data(0x32); write_data(0x98); 
    write_cmd(0x74); write_data(0x10); write_data(0x85); write_data(0x80); write_data(0x00); write_data(0x00); write_data(0x4E); write_data(0x00); 
    write_cmd(0x98); write_data(0x3e); write_data(0x07); 

    // 7. Final Activation
    write_cmd(0x35); // Tearing effect line ON
    write_cmd(0x21); // Inversion ON

    write_cmd(0x11); // Sleep Out
    sleep_ms(120);   // Required delay for charge pumps to stabilize
    write_cmd(0x29); // Display On
}

// Defines a specific rectangular area on the screen to draw into
void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    write_cmd(0x2A); // Column Address Set (X)
    write_data(x0 >> 8); write_data(x0 & 0xFF);
    write_data(x1 >> 8); write_data(x1 & 0xFF);

    write_cmd(0x2B); // Row Address Set (Y)
    write_data(y0 >> 8); write_data(y0 & 0xFF);
    write_data(y1 >> 8); write_data(y1 & 0xFF);

    write_cmd(0x2C); // Memory Write Command (Ready to receive pixels)
}

// Sweeps the entire 240x240 display and sets it to a single color
void fill_screen(uint16_t color) {
    set_window(0, 0, 239, 239); // The GC9A01 is 240x240 pixels
    
    gpio_put(PIN_DC, 1); // Data mode
    gpio_put(PIN_CS, 0); // Activate chip
    
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    
    // Push the color to all 57,600 pixels (240 * 240)
    for (int i = 0; i < 57600; i++) {
        spi_write_blocking(SPI_PORT, &hi, 1);
        spi_write_blocking(SPI_PORT, &lo, 1);
    }
    
    gpio_put(PIN_CS, 1); // Deactivate chip
}
// Draws a filled rectangle at a specific X,Y coordinate
void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (w == 0 || h == 0) return; // Don't draw if width or height is 0
    
    set_window(x, y, x + w - 1, y + h - 1);
    
    gpio_put(PIN_DC, 1); // Data mode
    gpio_put(PIN_CS, 0); // Activate chip
    
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint32_t pixels = w * h;
    
    for (uint32_t i = 0; i < pixels; i++) {
        spi_write_blocking(SPI_PORT, &hi, 1);
        spi_write_blocking(SPI_PORT, &lo, 1);
    }
    
    gpio_put(PIN_CS, 1); // Deactivate chip
}

// Blazing fast circle rendering using integer math
void fill_circle(int16_t xc, int16_t yc, int16_t r, uint16_t color) {
    if (r <= 0) return;
    int16_t x = r, y = 0;
    int16_t P = 1 - r;
    
    while (x >= y) {
        // Draw horizontal slices to fill the circle instantly
        fill_rect(xc - x, yc + y, 2 * x + 1, 1, color);
        fill_rect(xc - x, yc - y, 2 * x + 1, 1, color);
        fill_rect(xc - y, yc + x, 2 * y + 1, 1, color);
        fill_rect(xc - y, yc - x, 2 * y + 1, 1, color);
        y++;
        if (P <= 0) {
            P = P + 2 * y + 1;
        } else {
            x--;
            P = P + 2 * y - 2 * x + 1;
        }
    }
}

// Draws a 1-pixel thick hollow outline
void draw_hollow_circle(int16_t xc, int16_t yc, int16_t r, uint16_t color) {
    if (r <= 0) return;
    int16_t x = r, y = 0;
    int16_t P = 1 - r;
    
    while (x >= y) {
        // Draw the 8 symmetric points of a circle outline
        fill_rect(xc + x, yc + y, 1, 1, color);
        fill_rect(xc - x, yc + y, 1, 1, color);
        fill_rect(xc + x, yc - y, 1, 1, color);
        fill_rect(xc - x, yc - y, 1, 1, color);
        fill_rect(xc + y, yc + x, 1, 1, color);
        fill_rect(xc - y, yc + x, 1, 1, color);
        fill_rect(xc + y, yc - x, 1, 1, color);
        fill_rect(xc - y, yc - x, 1, 1, color);
        y++;
        if (P <= 0) {
            P = P + 2 * y + 1;
        } else {
            x--;
            P = P + 2 * y - 2 * x + 1;
        }
    }
}

// Precision "Donut" Eraser to prevent flickering
// Erases only the outer crust between the old circle and the new circle
void erase_crust(int16_t xc, int16_t yc, int16_t old_r, int16_t new_r) {
    // Safely bite 2 pixels into the new circle and 2 pixels outside the old one
    // This mathematically guarantees 0 stray orphan pixels can survive.
    int32_t r_in2 = (new_r - 2) * (new_r - 2); 
    int32_t r_out2 = (old_r + 2) * (old_r + 2); 
    
    for (int16_t y = -old_r - 2; y <= old_r + 2; y++) {
        int32_t y2 = y * y;
        
        if (y2 > r_in2) {
            // This entire row is outside the inner safe zone, clear the full width
            int32_t w = 0;
            while ((w * w) + y2 <= r_out2) w++;
            if (w > 0) {
                fill_rect(xc - w + 1, yc + y, (w - 1) * 2 + 1, 1, COLOR_BLACK);
            }
        } else {
            // This row intersects the inner safe zone
            // Skip the middle, and ONLY clear the left and right outer edges
            int32_t w_out = 0;
            while ((w_out * w_out) + y2 <= r_out2) w_out++;
            
            int32_t w_in = 0;
            while ((w_in * w_in) + y2 <= r_in2) w_in++;
            
            if (w_out > w_in) {
                fill_rect(xc - w_out + 1, yc + y, w_out - w_in, 1, COLOR_BLACK); // Left Edge
                fill_rect(xc + w_in, yc + y, w_out - w_in, 1, COLOR_BLACK);      // Right Edge
            }
        }
    }
}



// ==========================================
// MAIN SYSTEM LOOP
// ==========================================

int main() {
    // 1. Initialize USB-CDC Communication
    stdio_init_all();
    
    spi_init(SPI_PORT, 2 * 1000 * 1000);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SDA, GPIO_FUNC_SPI);
    
    // 2. Initialize Control Pins
    gpio_init(PIN_RES); gpio_set_dir(PIN_RES, GPIO_OUT);
    gpio_init(PIN_DC);  gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_CS);  gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_init(PIN_BLK); gpio_set_dir(PIN_BLK, GPIO_OUT);

    // Initialize AD3 Debug Pins
    gpio_init(PIN_DBG_RX);     gpio_set_dir(PIN_DBG_RX, GPIO_OUT);
    gpio_init(PIN_DBG_RENDER); gpio_set_dir(PIN_DBG_RENDER, GPIO_OUT);
    
    // Turn on the backlight
    gpio_put(PIN_BLK, 1); 
    
    // 3. Boot up the display controller
    lcd_init();

    // 4. CLEAR ARTIFACTS
    fill_screen(COLOR_BLACK);
    
    // Draw the static "Containment Ring" once at boot
    fill_circle(120, 120, 118, COLOR_WHITE);
    fill_circle(120, 120, 116, COLOR_BLACK);

   //watchdog after booting
    watchdog_enable(500, 1);

    #define COLOR_GREEN 0x07E0 

    uint32_t total_packets = 0;
    uint32_t dropped_packets = 0;
    uint8_t last_seq_id = 0;
    
    uint8_t target_cpu = 0;
    float current_radius = 20.0f; 
    
    uint16_t old_radius = 20;
    uint16_t old_color = COLOR_GREEN;

    // 6. The Main Telemetry Loop
    while (true) {
        // 1. Pet the Watchdog 
        watchdog_update(); 

        // 2. NON-BLOCKING STATE MACHINE
        int c = getchar_timeout_us(0); 
        
        // 3. FRAME SYNCHRONIZATION
        if (c == 0xAA) { 
            gpio_put(PIN_DBG_RX, 1); // AD3 TRIGGER HIGH

            uint8_t payload[3];
            int read_count = 0;
            
            for (int i = 0; i < 3; i++) {
                int b = getchar_timeout_us(2000); 
                if (b != PICO_ERROR_TIMEOUT) {
                    payload[i] = (uint8_t)b;
                    read_count++;
                } else {
                    break; 
                }
            }
            
            gpio_put(PIN_DBG_RX, 0); // AD3 TRIGGER LOW
            
            // 4. DATA VALIDATION (Checksum)
            if (read_count == 3) {
                uint8_t seq = payload[0];
                uint8_t cpu = payload[1];
                uint8_t checksum = payload[2];
                
                uint8_t expected_checksum = 0xAA ^ seq ^ cpu;
                
                if (checksum == expected_checksum) {
                    total_packets++;
                    if (total_packets > 1 && seq != (uint8_t)(last_seq_id + 1)) {
                        dropped_packets++; 
                    }
                    last_seq_id = seq;
                    target_cpu = cpu; 
                    
                    printf("VALIDATE | PKT: %lu | DROP: %lu | CPU: %d%%\n", 
                            total_packets, dropped_packets, target_cpu);
                } else {
                    printf("ERROR | CHECKSUM MISMATCH\n");
                }
            } else {
                printf("ERROR | PACKET FRAGMENTED\n");
            }
        }

        // --- GRAPHICS ENGINE (Restored Donut Logic) ---
        float target_radius = 20.0f + ((target_cpu * 90.0f) / 100.0f);
        current_radius += (target_radius - current_radius) * 0.08f;
        uint16_t new_radius = (uint16_t)current_radius;
        
        uint16_t new_color = COLOR_GREEN;
        if (target_cpu >= 60) new_color = COLOR_YELLOW;
        if (target_cpu >= 85) new_color = COLOR_RED;
        
        if (new_radius != old_radius || new_color != old_color) {
            
            gpio_put(PIN_DBG_RENDER, 1); // AD3 TRIGGER HIGH
            
            // If shrinking, cleanly erase only the outer crust.
            if (new_radius < old_radius) {
                erase_crust(120, 120, old_radius, new_radius);
            }
            
            // Repaint the core
            fill_circle(120, 120, new_radius, new_color);
            
            gpio_put(PIN_DBG_RENDER, 0); // AD3 TRIGGER LOW
            
            old_radius = new_radius;
            old_color = new_color;
        }
        
        sleep_ms(20); 
    }
}

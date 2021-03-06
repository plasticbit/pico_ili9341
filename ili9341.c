#include "pico/stdlib.h"
#include "pico/types.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include <math.h>

#define MISO 4
#define SCK  6
#define MOSI 7

const uint_fast16_t WIDTH = 240;
const uint_fast16_t HEIGHT = 320;
const uint RESET_PIN = 19;
const uint DC_RS_PIN = 20;
const uint CS_PIN = 5;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb16;


void setup() {
    gpio_init(RESET_PIN);
    gpio_init(DC_RS_PIN);
    gpio_init(CS_PIN);
    gpio_set_dir(RESET_PIN, GPIO_OUT);
    gpio_set_dir(DC_RS_PIN, GPIO_OUT);
    gpio_set_dir(CS_PIN, GPIO_OUT);
    gpio_set_function(MISO, GPIO_FUNC_SPI);
    gpio_set_function(SCK, GPIO_FUNC_SPI);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);

    spi_init(spi0, 120 * MHZ); // Where is the maximum???
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_put(RESET_PIN, 0);
    gpio_put(DC_RS_PIN, 0);
    gpio_put(CS_PIN, 0);
}

void hardware_reset() {
    setup();

    gpio_put(RESET_PIN, 1);
    sleep_us(10);
    gpio_put(RESET_PIN, 0);
    sleep_us(10);
    gpio_put(RESET_PIN, 1);
    sleep_us(10);
}

void write_register(uint8_t r) {
    gpio_put(CS_PIN, 0);
    gpio_put(DC_RS_PIN, 0);
    spi_write_blocking(spi0, &r, sizeof(r));
    gpio_put(CS_PIN, 1);
}

void write_data(uint8_t d) {
    gpio_put(CS_PIN, 0);
    gpio_put(DC_RS_PIN, 1);
    spi_write_blocking(spi0, &d, sizeof(d));
    gpio_put(CS_PIN, 1);

}

void software_reset() {
    write_register(0x01);
    sleep_ms(10); // Lower to 5ms?
}

void write_memory() {
    write_register(0x2c);
}

void displayOff() {
    write_register(0x28);
}

void displayOn() {
    write_register(0x29);
}

void set_columnAddr(uint_fast16_t x0, uint_fast16_t x1) {
    write_register(0x2a);

    write_data(x0 >> 8);
    write_data(x0 & 0xff);
    write_data(x1 >> 8);
    write_data(x1 & 0xff);
}

void set_pageAddr(uint_fast16_t y0, uint_fast16_t y1) {
    write_register(0x2b);

    write_data(y0 >> 8);
    write_data(y0 & 0xff);
    write_data(y1 >> 8);
    write_data(y1 & 0xff);
}

void set_pixel(uint_fast16_t x, uint_fast16_t y) {
    set_columnAddr(x, x + 1);
    set_pageAddr(y, y + 1);
    write_memory();
}

void write_pixel16(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b) {
    write_data((r & 0b11111000) | (g >> 5));
    write_data(((g << 3) & 0b11100000) | (b >> 3));
}

void drawCircle(int cx, int cy, int r, rgb16 c) {
    for (int i = 0; i < 360; i++) {
        int x = cx + (r * cos(i * M_PI / 180.0));
        int y = cy + (r * sin(i * M_PI / 180.0));

        set_pixel(x, y);
        write_pixel16(c.r, c.g, c.b);
    }
}

int main() {
    // initial process
    hardware_reset();
    software_reset();

    // sleep out
    write_register(0x11);
    sleep_ms(100);

    // display on
    write_register(0x29);

    write_register(0x36);
    write_data(0x48);

    write_register(0x3a);
    write_data(0x55);

    write_register(0xb1);
    write_data(0x00);
    write_data(0x18);

    // fill black
    write_memory();

    uint_fast16_t i = 0;
    uint_fast16_t p = WIDTH * HEIGHT;
    while (i++ < p) {
        write_data(0x0);
        write_data(0x0);
    }

    for (size_t i = 0; i < 120; i += 2) {
        rgb16 c = { (i << 1) | 255, i << 1, (i << 1) & 255 };
        drawCircle((WIDTH - 1) / 2, (HEIGHT - 1) / 2, i, c);
    }

    for (uint16_t x = 0; x < WIDTH; x += 4) {
        for (uint16_t i = 0; i < HEIGHT; i++) {
            set_pixel(x, i);
            write_pixel16(128, x, i >> 1);
        }
    }

    return 0;
}

/**
 * "cmake -G "MinGW Makefiles" .."
 * "make -j 12"
 * "cp ./ips_lcd.uf2 F:\\"
**/

/**
 * x:0, y:0
 * _____________
 * [           ]
 * [           ]
 * [           ]
 * [  Display  ]
 * [           ]
 * [           ]
 * [           ] x:240, y:320
 * -------------
 * [ Pinheader ]
 * -------------
*/

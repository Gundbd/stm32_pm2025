// SSD1306 SPI дисплей для PICSimLab
// Распиновка (STM32F103C8 → модуль SSD1306):
//   PA5 → CLK
//   PA7 → DIN (MOSI)
//   PA0 → RST
//   PA1 → DC
//   PA4 → /CE (CS)
//


#include <stdint.h>
#include <stm32f10x.h>

#define PIN_RST   (1U << 0)  // PA0
#define PIN_DC    (1U << 1)  // PA1
#define PIN_CS    (1U << 4)  // PA4

static void delay(uint32_t ticks) {
    for (volatile uint32_t i = 0; i < ticks; i++) {
        __NOP();
    }
}

static void spi1_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    SPI1->CR1 = SPI_CR1_MSTR |
                SPI_CR1_BR_1 |        // делитель /8
                SPI_CR1_CPOL |
                SPI_CR1_CPHA |
                SPI_CR1_SSM |
                SPI_CR1_SSI;

    SPI1->CR1 |= SPI_CR1_SPE;
}

static void spi1_write(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE)) {
    }
    SPI1->DR = data;
}

static void ssd_cmd(uint8_t c) {
    GPIOA->BSRR = (PIN_CS << 16);
    GPIOA->BSRR = (PIN_DC << 16);
    spi1_write(c);
    GPIOA->BSRR = PIN_CS;
}

static void ssd_data(uint8_t d) {
    GPIOA->BSRR = (PIN_CS << 16);
    GPIOA->BSRR = PIN_DC;
    spi1_write(d);
    GPIOA->BSRR = PIN_CS;
}

static void ssd_init(void) {
    // аппаратный сброс
    GPIOA->BSRR = (PIN_RST << 16);
    delay(50000);
    GPIOA->BSRR = PIN_RST;
    delay(50000);

    ssd_cmd(0xAE); // OFF
    ssd_cmd(0x20); ssd_cmd(0x00); // горизонтальный режим
    ssd_cmd(0x21); ssd_cmd(0x00); ssd_cmd(0x7F);
    ssd_cmd(0x22); ssd_cmd(0x00); ssd_cmd(0x07);
    ssd_cmd(0x8D); ssd_cmd(0x14);
    ssd_cmd(0xA1);
    ssd_cmd(0xC8);
    ssd_cmd(0xDA); ssd_cmd(0x12);
    ssd_cmd(0x81); ssd_cmd(0xCF);
    ssd_cmd(0xD9); ssd_cmd(0xF1);
    ssd_cmd(0xDB); ssd_cmd(0x40);
    ssd_cmd(0xA4);
    ssd_cmd(0xA6);
    ssd_cmd(0xAF); // ON
}

static void ssd_clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        ssd_cmd(0xB0 | page);
        ssd_cmd(0x00);
        ssd_cmd(0x10);
        for (uint8_t x = 0; x < 128; x++) {
            ssd_data(0x00);
        }
    }
}

static void ssd_demo_pattern(void) {
    for (uint8_t page = 0; page < 8; page++) {
        ssd_cmd(0xB0 | page);
        ssd_cmd(0x00);
        ssd_cmd(0x10);
        for (uint8_t x = 0; x < 128; x++) {
            uint8_t v = ((page + (x >> 3)) & 1) ? 0xAA : 0x55;
            ssd_data(v);
        }
    }
}

int main(void) {
    // тактирование портов A, C и AFIO
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;

    // PA0,1,4 — обычный выход
    GPIOA->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0 |
                    GPIO_CRL_CNF1 | GPIO_CRL_MODE1 |
                    GPIO_CRL_CNF4 | GPIO_CRL_MODE4);
    GPIOA->CRL |= (GPIO_CRL_MODE0_0 |
                   GPIO_CRL_MODE1_0 |
                   GPIO_CRL_MODE4_0);

    // PA5,7 — AF push‑pull
    GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5 |
                    GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    GPIOA->CRL |= (GPIO_CRL_CNF5_1 | GPIO_CRL_MODE5_0 |
                   GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7_0);

    // PC13 — LED
    GPIOC->CRH &= ~GPIO_CRH_CNF13;
    GPIOC->CRH |= GPIO_CRH_MODE13_0;

    // начальные уровни
    GPIOA->BSRR = PIN_CS | PIN_RST;
    GPIOA->BSRR = (PIN_DC << 16);

    spi1_init();
    ssd_init();
    ssd_clear();
    ssd_demo_pattern();

    while (1) {
        GPIOC->BSRR = (1U << 13);
        delay(3000000);
        GPIOC->BSRR = (1U << (13 + 16));
        delay(3000000);
    }
}




#include <stdint.h>
#include <stm32f10x.h>

// Массив периодов таймера (в тиках после предделителя)
// Подбирается экспериментально; важны только относительные частоты.
static const uint16_t tim_periods[] = {
    250,   // самая быстрая частота мигания
    500,   // средняя
    1000,  // медленнее
    2000   // самая медленная
};

static volatile uint8_t current_period_idx = 0;

static void tim2_update_period(void) {
    TIM2->ARR = tim_periods[current_period_idx] - 1;
    TIM2->EGR = TIM_EGR_UG; // обновить регистры
}

static void delay(volatile uint32_t t) {
    while (t--) {
        __NOP();
    }
}

void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {   // флаг обновления
        TIM2->SR &= ~TIM_SR_UIF;   // сбросить флаг
        // инвертируем светодиод на PC13
        GPIOC->ODR ^= (1U << 13);
    }
}

int main(void) {
    // Включаем тактирование портов A, C, AFIO и таймера TIM2
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |
                    RCC_APB2ENR_IOPCEN |
                    RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // PC13 — выход push‑pull (LED)
    GPIOC->CRH &= ~GPIO_CRH_CNF13;
    GPIOC->CRH |= GPIO_CRH_MODE13_0; // 10 МГц, general purpose push‑pull

    // PA0 — вход с подтяжкой вверх (кнопка к GND)
    GPIOA->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
    GPIOA->CRL |= GPIO_CRL_CNF0_1;   // вход с pull‑up/pull‑down
    GPIOA->ODR |= (1U << 0);         // включаем pull‑up (лог.1)

    // Настройка TIM2:
    // Предделитель: делим тактирование до ~1 кГц (если APB1 ≈ 64–72 МГц).
    TIM2->PSC = 64000 - 1;          // 64 МГц / 64000 ≈ 1 кГц
    current_period_idx = 0;
    tim2_update_period();

    TIM2->CR1 |= TIM_CR1_ARPE;      // буфер ARR
    TIM2->DIER |= TIM_DIER_UIE;     // разрешить прерывание по обновлению
    TIM2->CR1 |= TIM_CR1_CEN;       // запустить таймер

    // Разрешаем прерывание TIM2 в NVIC
    NVIC_EnableIRQ(TIM2_IRQn);

    uint8_t prev_button = 1; // 1 — не нажата (pull‑up)

    while (1) {
        // Чтение кнопки PA0 (активный уровень — 0)
        uint8_t button = (GPIOA->IDR & (1U << 0)) ? 1 : 0;

        // Обработка фронта нажатия (1 -> 0)
        if (!button && prev_button) {
            // переключаем частоту
            current_period_idx++;
            if (current_period_idx >= (sizeof(tim_periods) / sizeof(tim_periods[0]))) {
                current_period_idx = 0;
            }
            tim2_update_period();

            // простая антидребезговая задержка
            delay(50000);
        }

        prev_button = button;
    }
}



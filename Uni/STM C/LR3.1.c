#include <stm32f0xx.h>

/* Функция инициализации светодиодов D1-D16 и линий управления цветом */
void leds_init(void)
{
    /* Включение тактирования порта A */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* Включение тактирования порта B */
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    /* Включение тактирования порта C */
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    /* Настройка на вывод линий PA6, PA7, PA8 (RED, GREEN, BLUE) */
    GPIOA->MODER |= (GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0);

    /* Настройка на вывод линий PC0 - PC15 (D1 - D16) */
    GPIOC->MODER |= GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 |
                    GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 |
                    GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0 |
                    GPIO_MODER_MODER9_0 | GPIO_MODER_MODER10_0 | GPIO_MODER_MODER11_0 |
                    GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 |
                    GPIO_MODER_MODER15_0;

    /*Включение подтягивающих резисторов PB4 (SB1), PB5 (SB2)*/
    GPIOB->PUPDR = GPIOB->PUPDR | (GPIO_PUPDR_PUPDR4_0 | GPIO_PUPDR_PUPDR5_0);

    /*Включение подтягивающих резисторов PA11 (SW1), PA12 (SW2)*/
    GPIOA->PUPDR = GPIOA->PUPDR | (GPIO_PUPDR_PUPDR11_0 | GPIO_PUPDR_PUPDR12_0);
}

/* Макроопределения с цветами */
#define RED     0
#define GREEN   1
#define BLUE    2

/* Функция включения светодиодов и выбора цвета */
void led_set(uint16_t led, uint8_t color)
{
    /* Записываем в регистр данных порта C новое состояние светодиодов.
       Номер бита соответствует номеру светодиода: бит 0 - D1, бит 1 - D2 и
       так далее */
    GPIOC->ODR = led;

    /* Сброс трех битов управления цветом с помощью маски */
    GPIOA->ODR &= ~(7 << 6);

    /* Включение светодиодов нужного цвета */
    if (color == RED)
    {
        GPIOA->ODR |= (1 << 6);
    }
    else if (color == GREEN)
    {
        GPIOA->ODR |= (1 << 7);
    }
    else if (color == BLUE)
    {
        GPIOA->ODR |= (1 << 8);
    }
}

/* Функция инициализации таймера TIM1 */
void timer_init()
{
    /* Включение тактирования TIM1 */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* Расчет предделителя частоты и кода переполнения таймера
       (максимальный код таймера).
       Пусть таймер счетчик переключается каждую 1 мкс или 1 МГц,
       тогда при частоте тактирования МК fmcu = 8 МГц, предделитель требуется
       на 8 - 8 МГц / 1 МГц = 8.
       Пусть переполнение происходит каждую 1 мс или 1 кГц,
       тогда код переполнения должен быть 1 МГц / 1 кГц = 1000 */
    /* Предделитель частоты */

    /* Максимальный код таймера (счет идет от 0 до 999) */
    TIM1->ARR = 999;

    /* Начальный код в регистре выходного сравнения */
    TIM1->CCR1 = 250;

    /* Включение прерывания по переполнению */
    TIM1->DIER |= TIM_DIER_UIE;

    /* Включение прерывания по сравнению - канал 1 */
    TIM1->DIER |= TIM_DIER_CC1IE;

    /* Включить таймер */
    TIM1->CR1 |= TIM_CR1_CEN;

    /* Установка приоритета прерывания по переполнению таймера.
       В Cortex-M0 четыре уровня приоритета - 0-3.
       0 - наибольший приоритет, 3 - наименьший. */
    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0);

    /* Установка приоритета прерывания по сравнению. */
    NVIC_SetPriority(TIM1_CC_IRQn, 0);

    /* Разрешение прервания по переполнению */
    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);

    /* Разрешение прервания прерывания по сравнению. */
    NVIC_EnableIRQ(TIM1_CC_IRQn);
}

/* Переменная для сохранения состояния светодиодов */
uint32_t led = 0x30000; /* Начальное состояние - включен самый левый светодиод */
/* Переменная для сохранения цвета светодиодов */
uint16_t color = RED; /* Начальное состояние - красный цвет */
/* Переменная для подсчета количества переполнений */
uint16_t up_cnt = 0;
/* Переменная для сохранения яркости - код в канале выходного сравнения */
uint16_t brightness = 250;
/* Начальное состояние яркости */
uint16_t bright_cond = 0; 

/* Подпрограмма обработчик прерываний по переполнению таймера */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    /* Сброс флага вызвавшего прерывание */
    TIM1->SR &= ~TIM_SR_UIF;

    /* Подсчет количества переполнений таймера.
       Между каждым переполнением проходит по 1 мс */
    up_cnt++;

    /* Если прошло больше 250 мс */
    if (up_cnt > 250)
    {
        /* Подсчет заново */
        up_cnt = 0;
        /* Сдвиг маски светодиода на одну позицию вправо */
        led = led >> 1;
    }

    /* Если сдвиг произошел дальше 1 светодиода */
    if (led == 0x00)
    {
        /* Начинается сдвиг с шестнадцатого светодиода */
        led = 0x30000;

    }

    /* Включение светодиода с нужным цветом */
    led_set(led, color);

    /* Запись в регистр выходного сравнения нового кода */
    TIM1->CCR1 = brightness;
}

/* Подпрограмма обработчик прерываний по выходному сравнению таймера */
void TIM1_CC_IRQHandler(void)
{
    /* Сброс флага вызвавшего прерывание */
    TIM1->SR &= ~TIM_SR_CC1IF;
    /* Выключение всех светодиодов */
    led_set(0, color);
}
/* Временная задержка */
void software_delay(int ticks)
{
	while (ticks > 0)
	{
		ticks = ticks - 1;
	}
}

/* Функция main - точка входа в программу */
int main(void)
{
    /* Инициализация светодиодов D1-D8 и управления цветом */
    leds_init();
    /* Инициализация таймера TIM1 */
    timer_init();
    
    /* Бесконечный цикл */
    while (1)
    {
        /* Объявление переменной sb1 и чтение состояние линии PB4 (SB1) */
        int sb1 = GPIOB->IDR & (1 << 4);
        /* Объявление переменной sb2 и чтение состояние линии PB5 (SB2) */
        int sb2 = GPIOB->IDR & (1 << 5);

			  /* Изменение яркости */
				if (sb1 == 0)
				{
						if (bright_cond == 0)
						{
								brightness = 999;
								bright_cond = 1;
								software_delay(500000);
						}
						else if (bright_cond == 1)
						{	
								brightness = 750;
								bright_cond = 2;
								software_delay(500000);
						}
						else if (bright_cond == 2)
						{	
								brightness = 500;
								bright_cond = 3;
								software_delay(500000);
						}
						else if (bright_cond == 3)
						{	
								brightness = 250;
								bright_cond = 0;
								software_delay(500000);
						}
				}
			  /* Изменение цвета */
				if (sb2 == 0)
				{
						if (color == RED)
						{
								color = GREEN;
								software_delay(500000);
						}
						else if (color == GREEN)
						{	
								color = BLUE;
								software_delay(500000);
						}
						else if (color == BLUE)
						{	
								color = RED;
								software_delay(500000);
						}
				}
			
        /* Объявление переменной sw1 и чтение состояние линии PA11 (SW1) */
        int sw1 = GPIOA->IDR & (1 << 11);
        /* Объявление переменной sw2 и чтение состояние линии PA12 (SW2) */
        int sw2 = GPIOA->IDR & (1 << 12);
				
				/* Изменение скорости */
        if (sw2 == 0 && sw1 == 0){
					TIM1->PSC = 8;
				}
				else if (sw2 == 0 && sw1 != 0){
					TIM1->PSC = 6;
				}
				else if (sw2 != 0 && sw1 == 0){
					TIM1->PSC = 4;
				}
				else if (sw2 != 0 && sw1 != 0){
					TIM1->PSC = 2;
				}
    }
}

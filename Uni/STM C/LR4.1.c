#include <stm32f0xx.h>

/* Функция инициализации светодиодов D1-D8 и линий управления цветом */
void leds_init(void)
{
    /* Включение тактирования порта A */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* Включение тактирования порта C */
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    /* Настройка на вывод линий PA6, PA7, PA8 (RED, GREEN, BLUE) */
    GPIOA->MODER |= (GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0);

    /* Настройка на вывод линий PC0 - PC7 (D1 - D8) */
    GPIOC->MODER |= GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 |
                    GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 |
                    GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0;
}

/* Макроопределения с цветами */
#define RED     0x1
#define GREEN   0x2
#define BLUE    0x4

//uint16_t led = 0xFFFF;
int speed = 400000;

void software_delay(int ticks)
{
	while (ticks > 0)
	{
		ticks = ticks - 1;
	}
}
int blink = 1;
/* Функция включения светодиодов и выбора цвета */
void led_set(uint8_t led, uint8_t color)
{
    /* Записываем в регистр данных порта C новое состояние светодиодов.
       Номер бита соответствует номеру светодиода: бит 0 - D1, бит 1 - D2 и
       так далее */
				GPIOC->ODR = led;
}

/* Функция инициализации приемника/передатчика USART2 */
void usart_init(void)
{
    /* Включить тактирование порта A */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* Включить тактирование USART2 */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* Включить режим альтернативной функции 1 (USART2) на линиях 2 и 3 */
    GPIOA->AFR[0] |= (1 << GPIO_AFRL_AFRL2_Pos) | (1 << GPIO_AFRL_AFRL3_Pos);

    /* Включить режим альтернативной функции на линиях PA2 и PA3 */
    GPIOA->MODER |=
        GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1;

    /* Установить коэффициент деления.
       BRR = fbus / baudrate
       Пусть baudrate = 9600 бит/с, частота шины fbs = 8 МГц,
       тогда BRR = 8000000 / 9600 */
    USART2->BRR = 8000000 / 9600;
    /* Включаем передатчик */
    USART2->CR1 |= USART_CR1_TE;
    /* Включаем приемник */
    USART2->CR1 |= USART_CR1_RE;
    /* Включаем USART */
    USART2->CR1 |= USART_CR1_UE;

    /* Чтение регистра данных для сброса флагов */
    uint16_t dummy = USART2->RDR;
}

/* Функция передачи байта в бесконечном цикле */
void usart_transmit(uint8_t data)
{
    /* Записать байт в регистр данных */
    USART2->TDR = data;
    /* Ожидание флага окончания передачи TC (Transmission Complete) */
    while(!(USART2->ISR & USART_ISR_TC));
}

/* Функция приема байта в бесконечном цикле */
uint8_t usart_receive(void)
{
    /* Ожидание флага буфер приемника не пуст RXNE (Receiver buffer not empty) */
    while(!(USART2->ISR & USART_ISR_RXNE));
    /* Чтение приятого байта и возврат */
    return USART2->RDR;
}

/* Функция сравнения двух строк.
   В стандартной библиотеке C уже есть функция strcmp.
   В примере для учебных целей показана простейшая реализация
   такой функции. */
int32_t _strcmp(char s1[], char s2[])
{
    int32_t i = 0;

    while (s1[i] != '\0' && s2[i] != '\0')
    {
        if (s1[i] != s2[i])
        {
            break;
        }
        i++;
    }

    return s1[i] - s2[i];
}

void timer_init()
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    TIM1->PSC = 8;

    TIM1->ARR = 999;

    TIM1->CCR1 = 0;

    TIM1->DIER |= TIM_DIER_UIE;

    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0);

    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);

}

uint16_t up_cnt = 0;
uint8_t led = 0xFF;

void TIM1_BRK_UP_TRG_COM_IRQHandler(int time_speed)
{
    TIM1->SR &= ~TIM_SR_UIF;

    up_cnt++;

    if (up_cnt > speed)
    {
        up_cnt = 0;
        led = ~led;
    }

    led_set(led);
}

void led_blink()
{
	TIM1->CR1 |= TIM_CR1_CEN;
}

void led_speed()
{
	if (speed == 2000)
        speed = 1000;
    else if(speed == 1000)
        speed = 500;
    else if(speed == 500)
        speed = 250;
    else if(speed == 250)
        speed = 2000;
}

void led_stop()
{
	TIM1->CR1 &= ~TIM_CR1_CEN;
}

/* Функция main - точка входа в программу */
int main(void)
{
    /* Инициализация светодиодов D1-D8 и управления цветом */
    leds_init();
	
    /* Инициализация таймера TIM1 */
    usart_init();

    timer_init();
	
    /* Объявления массива buf и инициализация нулями. */
    char buf[20] = {0};
    int32_t pos = 0;

    /* Бесконечный цикл */
    while (1)
    {
        char ch = usart_receive();

        /* Наполнение буфера если не нажата клавиша Enter.
           Символ `\r` передается при нажатии клавиши Enter в терминале. */
        if (ch != '\r')
        {
            if (pos < 20)
            {
                buf[pos] = ch;
                pos++;
            }
        }
        else
        {
            /* Сравнение содержимого буфера с командой */
            if (_strcmp(buf, "BLINK") == 0)
            {
                led_blink();
            }
            else if (_strcmp(buf, "STOP") == 0)
            {
                led_stop();
            }
            else if (_strcmp(buf, "SPEED") == 0)
            {
                led_speed();
            }
            else /* Команда не найдена */
            {
                char* str = "ERROR\n";
                for (int32_t i = 0; i < sizeof("ERROR\n") - 1; i++)
                {
                    usart_transmit(str[i]);
                }
                
            }

            pos = 0; /* Буфер наполняется заново */
            for(int32_t i = 0; i < 20; i++) /* Очистка буфера */
            {
                buf[i] = '\0';
            }
        }
    }
}

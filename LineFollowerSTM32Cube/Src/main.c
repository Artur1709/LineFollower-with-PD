
#include <stdint.h>
#include "stm32g030xx.h"
// Definicje makr dla wlaczania i wylaczania diod oraz threshold
#define LD0_ON GPIOB->BSRR |= GPIO_BSRR_BS4
#define LD1_ON GPIOB->BSRR |= GPIO_BSRR_BS3
#define LD2_ON GPIOB->BSRR |= GPIO_BSRR_BS2
#define LD3_ON GPIOB->BSRR |= GPIO_BSRR_BS1
#define LD4_ON GPIOB->BSRR |= GPIO_BSRR_BS0
#define LD5_ON GPIOB->BSRR |= GPIO_BSRR_BS5
#define LD6_ON GPIOB->BSRR |= GPIO_BSRR_BS6

#define LD0_OFF GPIOB->BSRR |= GPIO_BSRR_BR4
#define LD1_OFF GPIOB->BSRR |= GPIO_BSRR_BR3
#define LD2_OFF GPIOB->BSRR |= GPIO_BSRR_BR2
#define LD3_OFF GPIOB->BSRR |= GPIO_BSRR_BR1
#define LD4_OFF GPIOB->BSRR |= GPIO_BSRR_BR0
#define LD5_OFF GPIOB->BSRR |= GPIO_BSRR_BR5
#define LD6_OFF GPIOB->BSRR |= GPIO_BSRR_BR6

// Definicja wartosci ADC wykrywajaca linie
#define threshold 4050

// Definicja funkcji wykorzystywanych w dalszej czesci kodu
void LED_init(void); // funkcja do zainicjalizowania wyjsc mikrokontrolera do sterowania Diodami;
void LED_set(void);  // funkcja zalaczajaca diody led informujace o wykryciu czarnej linii
void ADC_init(void); // funkcja inicjalizujaca piny na konwerter analogowo cyfrowy do odczytywania wartosci z, czujnikow
void ADC_enable(void); // funkcja zalaczajaca konwerter analogowo cyfrowy do odczytywania wartosci z czujnikww
uint32_t ADC_read(uint32_t channel); // funkcja odczytujaca wartosci z konkretnych czujnikow podlaczonych do mikrokontrolera (channel od 0-7)
void PWM_init(void); // funkcja inicjalizujaca piny na wyjscie sygnalu PWM
void PWM1_set(uint16_t pwmValue); // funkcja ustawiajaca wyjscie na konkretna wartosc wypelnienia sygnalu PWM
void PWM2_set(uint16_t pwmValue); // funkcja ustawiajaca wyjscie na konkretna wartosc wypelnienia sygnalu PWM
void LD293_init(void); // funkcja inicjalizujaca pinow kontrolera podlaczonego do silnikow

int main(void) {
	LED_init();
	ADC_init();
	PWM_init();
	LD293_init();
	ADC_enable();
	// Czlon proporcjonalny, wartosc dobrana testowo
	int Kp = 200;
	// Czlon rozniczkujacy, wartosc dobrana testowo
	int Kd = 120;
	// Etykieta przypisywana czujnikowi gdy wykryje linie
	int error = 0;
	// Aktualna etykieta przy wykryciu linii przez czujnik
	int current_error = 0;
	// Zmienna sluzaca do obliczenia rozniczki
	int derivative = 0;
	// Poprzednia etykieta przy wykryciu linii przez czujnik
	int previous_error = 0;



	/* Glowna petla programu */
	while (1) {
		LED_set();
// Wartosc threshold zostala obliczona ze wzoru z dokumentacji
//na podstawie zmierzonego napiecia z czujnika podczas przylozenia go do czarnej linii
		if (ADC_read(0) > threshold)
			error = -5;
		else if (ADC_read(1) > threshold)
			error = -3;
		else if (ADC_read(2) > threshold)
			error = -2;
		else if (ADC_read(3) > threshold)
			error = 0;
		else if (ADC_read(4) > threshold)
			error = 2;
		else if (ADC_read(5) > threshold)
			error = 3;
		else if (ADC_read(6) > threshold)
			error = 5;

		current_error = error;
		derivative = current_error - previous_error;

		uint16_t PWM_left = 0;
		uint16_t PWM_right = 0;
// Sprawdzenie z ktorej strony czujnik wykryl linie
		if(current_error>0){
// Odejmowanie regulatora PD
			PWM_left = 900 - ((Kp*error) + (Kd*derivative));
// Blokada przed przekroczeniem max wartosci PWM
			if(PWM_left >= 999)
				PWM_left = 999;

			PWM_right = 900;
// Blokada przed przekroczeniem max wartosci PWM
			if(PWM_right >= 999)
				PWM_right = 999;
		}
		else{
			PWM_left = 900;
			if(PWM_left >= 999)
				PWM_left = 999;
// Dodawanie regulatora PD
			PWM_right = 900 + ((Kp*error) + (Kd*derivative));
			if(PWM_right >= 999)
				PWM_right = 999;
		}
// Przekazywanie obliczonych parametrow do funkcji
		PWM1_set(PWM_left);
		PWM2_set(PWM_right);
		previous_error = current_error;

	}
}

void LED_init() {

// Wlaczenie zegara dla portu B
	RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
// Ustawienie pinow dla diod jako wyjscia, aby zapalic je w potrzebnym momencie
	GPIOB->MODER &= ~GPIO_MODER_MODE0_1;
	GPIOB->MODER &= ~GPIO_MODER_MODE1_1;
	GPIOB->MODER &= ~GPIO_MODER_MODE2_1;
	GPIOB->MODER &= ~GPIO_MODER_MODE3_1;
	GPIOB->MODER &= ~GPIO_MODER_MODE4_1;
	GPIOB->MODER &= ~GPIO_MODER_MODE5_1;
	GPIOB->MODER &= ~GPIO_MODER_MODE6_1;

}

void ADC_init() {
// Wlaczenie zegara dla portu A
	RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
// Wlaczenie zegara dla modulu ADC
	RCC->APBENR2 |= RCC_APBENR2_ADCEN;


}

void ADC_enable() {
// Wlaczenie gotowosci ADC
	ADC1->ISR |= ADC_ISR_ADRDY;
// Wlaczenie modulu ADC
	ADC1->CR |= ADC_CR_ADEN;
// Oczekiwanie na gotowosc ADC
	while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);

}
uint32_t ADC_read(uint32_t channel) {
// Wybor kanalu ADC, aby zarejestrowac odczyt z danego czujnika
	ADC1->CHSELR = (1 << channel);
// Start konwersji na wartosc cyfrowa
	ADC1->CR |= ADC_CR_ADSTART;
// Oczekiwanie na zakonczenie konwersji
	while ((ADC1->ISR & ADC_ISR_EOC) == 0);
// Zwrocenie wyniku
	return ADC1->DR;

}

void PWM_init(void) {
// Wlacz zegar dla portu A
	RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

// Wlacz zegar dla timera 1
	RCC->APBENR2 |= RCC_APBENR2_TIM1EN;

// Konfiguracja pinow jako wyjscia alternatywne ( alternate function mode)
	GPIOA->MODER &= ~(GPIO_MODER_MODE9_0); // Wyczyszenie poprzednich ustawien bitow
	GPIOA->MODER &= ~(GPIO_MODER_MODE11_0);
	GPIOA->MODER |= (GPIO_MODER_MODE9_1); // Ustawienie jedynek na konkertnych bitach
	GPIOA->MODER |= (GPIO_MODER_MODE11_1);
// Podpiecie pinow do poszczegolnych kanalow
// TIM1_CH2 PA9, pin lewego silnika na ktory jest wysylany sygnal PWM
	GPIOA->AFR[1] |= GPIO_AFRH_AFSEL9_1;
// TIM1_CH2 PA11, pin prawego silnika na ktory jest wysylany sygnal PWM
	GPIOA->AFR[1] |= GPIO_AFRH_AFSEL11_1;
// Ustawienie PA8 i PA10 jako wyjscia
	GPIOA->MODER &= ~GPIO_MODER_MODE8_1;
	GPIOA->MODER &= ~GPIO_MODER_MODE10_1;
// Ustawienie logicznego 0 na pinach PA8 i PA9
// W taki sposob uzyskujemy na pinach mase co pozwala na prawidłowy kierunek obrotu silnikow
	GPIOA->BSRR |= GPIO_BSRR_BR8;
	GPIOA->BSRR |= GPIO_BSRR_BR10;

// Konfiguracja timera TIM1
	TIM1->CR1 &= ~TIM_CR1_DIR; // Kierunek zliczania w gore
	TIM1->CR1 &= ~TIM_CR1_CMS; // Tryb zliczania w gore
	TIM1->PSC = 15;             // Prescaler
	TIM1->ARR = 999;           // Wartosc autoreload (maksymalny zakres PWM)
// Wlaczenie glownego sygnalu TIM1
	TIM1 -> BDTR |= TIM_BDTR_MOE;
// Konfiguracja kanalow PWM
	TIM1->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE; // Tryb PWM1 na kanale 2
	TIM1->CCMR2 |= TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4PE; // Tryb PWM1 na kanale 4

// Wlaczenie kanalow
	TIM1->CCER |= TIM_CCER_CC2E; // Wlacz kanal 2
	TIM1->CCER |= TIM_CCER_CC4E; // Wlacz kanal 4

// Wlaczenie licznika timera
	TIM1->CR1 |= TIM_CR1_CEN;

// Aktualizowanie generacji
	TIM1 -> EGR |= TIM_EGR_UG;
}
// Funkcja ustawiajaca predkosc lewego kola
void PWM1_set(uint16_t pwmValue) {
// Wartosc PWM dla kanalu 2,ustawienie wypelnienia sygnalu PWM z zakresu wartosci (0-1000),
// 100 to 10% wypelnienia itd
	TIM1->CCR2 = pwmValue;
}
// Funkcja ustawiajaca predkosc prawego kola
void PWM2_set(uint16_t pwmValue) {
// Wartosc PWM dla kanalu 4,ustawienie wypelnienia sygnalu PWM z zakresu wartosci (0-1000),
// 100 to 10% wypelnienia itd
	TIM1->CCR4 = pwmValue;
}
void LD293_init(void) {
// Wlaczenie zegara dla portu A
	RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
// Ustawienie pinu PA7 jako wyjscie
	GPIOA->MODER &= ~(GPIO_MODER_MODE7_1);
// Ustawienie logicznej 1 na pinie PA7. Wlaczenie sterownika od silnikow
	GPIOA -> BSRR |= GPIO_BSRR_BS7;
}

void LED_set(void){
// Wartosc threshold jest powiazana z czujnikami,
// gdy dany czujnik wykryje linie swieci sie takze odpowiednia dioda
	if (ADC_read(0) > threshold) {
		LD0_ON;
	} else
		LD0_OFF;

	if (ADC_read(1) > threshold) {
		LD1_ON;
	} else
		LD1_OFF;

	if (ADC_read(2) > threshold) {
		LD2_ON;
	} else
		LD2_OFF;

	if (ADC_read(3) > threshold) {
		LD3_ON;
	} else
		LD3_OFF;

	if (ADC_read(4) > threshold) {
		LD4_ON;
	} else
		LD4_OFF;

	if (ADC_read(5) > threshold) {
		LD5_ON;
	} else
		LD5_OFF;

	if (ADC_read(6) > threshold) {
		LD6_ON;
	} else
		LD6_OFF;

}







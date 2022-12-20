/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

/* Definiranje funkcij */
void Prekinitev_konfiguracija(void);
void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void EXTI15_10_IRQHandler(void);

/* Deklaracija spremenljivk */
//take spremenljivke so no go, ker ne pomenijo ničesar. smiselno bi bilo kaj v smislu "led_blink" in "button_allowed"
//je potreben int tip? mogoče bi bilo bolje uporabiti uint8_t
//int flag=0;
//int flag1=0;

uint8_t led_blink;		//1 - led should blink, 0 - led off
uint8_t button_allowed = 1; //1 - gumb se lahko pritisne, 0 - če je pritisnjen gumb, ne naredi ničesar

void Prekinitev_konfiguracija (void){
	//Konfiguracija tipke (PC13) kot vhod -> načeloma ne bi bilo potrebno, ker je input reset state za f4
	CLEAR_BIT(GPIOC->MODER,1<<27);
	CLEAR_BIT(GPIOC->MODER,1<<26);

	//Vklop internega pull-up upora na tipki
	SET_BIT(GPIOC->PUPDR,  1<<26);
	CLEAR_BIT(GPIOC->PUPDR,1<<27);

	SET_BIT(RCC->APB2ENR, 1<<14);			// Vklop sistemske ure in periferije - Bit 14 SYSCFGEN: System configuration controller clock enable
	SET_BIT(SYSCFG->EXTICR[3],0x20);		// Uporaba porta C za EXTI13
	SET_BIT(EXTI->IMR,1<<13); 				// Izklop interrupt mask registra za EXTI13
	SET_BIT(EXTI->RTSR,1<<13);				// Vklop rising edge trigger registra (pull-up vkljucen)
//	CLEAR_BIT(EXTI->FTSR,1<<0);				// Izklop falling edge trigger registra

	NVIC_EnableIRQ(EXTI0_IRQn);

}

int main(void)
{
	SET_BIT(RCC->APB1ENR,RCC_APB1ENR_TIM2EN); // Vklop ure in periferije - TIM2
	SET_BIT(RCC->APB1ENR,RCC_APB1ENR_TIM3EN); // Vklop ure in periferije - TIM3
	SET_BIT(RCC->AHB1ENR,RCC_AHB1ENR_GPIOAEN);// Vklop ure in periferije registra A
	SET_BIT(RCC->AHB1ENR,RCC_AHB1ENR_GPIOCEN);// Vklop ure in periferije registra C

	SET_BIT(GPIOA->MODER,1<<10);	//set PC13 as output

	//TIM2 - postavitev PSCA registra in ARR registra, DIER register za prekinitev
	//perioda timerja 1 s/1 Hz -> za utripanje led diode
	WRITE_REG(TIM2->PSC,16000-1);	//prescaler - zniža clock timerja iz 16 MHz (HSI oscilator za core) na 1 kHz
	WRITE_REG(TIM2->ARR,1000-1);	//auto-reload register - nastavi do koliko bo štel timer 0-999
	SET_BIT(TIM2->DIER,1);//enable interrupt
	NVIC_EnableIRQ(TIM2_IRQn);
//	SET_BIT(TIM2->CR1,TIM_CR1_CEN); //enable counter

	//TIM3 - postavitev PSCA registra in ARR registra, DIER register za prekinitev
	//perioda timerja 200 ms -> 5 Hz za debounce led diode
	WRITE_REG(TIM3->PSC,16000-1);
	WRITE_REG(TIM3->ARR,200-1);		//auto-reload register - nastavi do koliko bo štel timer 0-199
	SET_BIT(TIM3->DIER,1);	//enable interrupt
	NVIC_EnableIRQ(TIM3_IRQn);

	//counterja se tukaj še ne sme omogočiti, ker ga je potrebno zagnati šele po pritisku gumba

	//tipka
	Prekinitev_konfiguracija();

	//to ni potrebno onemogočiti in ponovno omogočiti, ker se naredi v funkciji "Prekinitev_konfiguracija"
//	NVIC_DisableIRQ(EXTI15_10_IRQn);// disable button
//	NVIC_EnableIRQ(EXTI15_10_IRQn);// enable button interrupt



	while(1){}
}




//void TIM2_IRQHandler(void){ //za blinkanje led
//	WRITE_REG(TIM2->SR,0); //clear status register
//
//	//toggle PA5
////	if(flag%2 == 0){GPIOA->ODR^=GPIO_ODR_ODR_5;}
//
//}

void TIM2_IRQHandler(void)
{
	WRITE_REG(TIM2->SR,0); //clear status register, da se lahko ponovno proži interrupt
	GPIOA->ODR^=GPIO_ODR_ODR_5;
}






//mogoče bi bilo smiselno uporabiti one-pulse mode pri timerju, ker potem ga ne rabiš vsakič izklopiti in postavljati na 0
//void TIM3_IRQHandler(void){ //za debouncing
//	WRITE_REG(TIM3->SR,0); //clear status register
//	flag1=0;
//	CLEAR_BIT(TIM3->CR1,TIM_CR1_CEN); //disable counter
//
//}

void TIM3_IRQHandler(void) //za debouncing
{
	WRITE_REG(TIM3->SR,0); //clear status register, da se lahko ponovno proži interrupt
	CLEAR_BIT(TIM3->CR1,TIM_CR1_CEN); //ugasni timer za debouncing do ponovnega pritiska gumba

	button_allowed = 1; //postavi flag, da se gumb lahko spet pritisne
}

//void EXTI15_10_IRQHandler(void) {
//	SET_BIT(TIM3->CR1,TIM_CR1_CEN); //enable counter
//	if(flag1==0){
//		WRITE_REG(TIM3->CNT,0);
//		flag=flag+1;
//		if(flag%2 == 0){
//			SET_BIT(TIM2->CR1,TIM_CR1_CEN); //enable counter
//			WRITE_REG(TIM2->CNT,0);
//			SET_BIT(GPIOA->ODR,GPIO_ODR_ODR_5);
//			flag=0;
//		} else {
//			CLEAR_BIT(GPIOA->ODR,GPIO_ODR_ODR_5);
//			CLEAR_BIT(TIM2->CR1,TIM_CR1_CEN); //disable counter
//		}	}
//	flag1=1;
//	WRITE_REG(EXTI->PR,0x2000);          // clear interrupt pending flag
//}



void EXTI15_10_IRQHandler(void)
{
	if (button_allowed)
	{
		//če je prej utripala led dioda -> zdaj ugasni in izklopi timer, drugače zaženi timer za utripanje
		if(led_blink)
		{
			led_blink = 0;
			CLEAR_BIT(TIM2->CR1,TIM_CR1_CEN); //ugasni counter za utripanje
			CLEAR_BIT(GPIOC->ODR,GPIO_ODR_ODR_13); //ugasni led diodo
		}
		else
		{
			led_blink = 1;
			SET_BIT(TIM2->CR1,TIM_CR1_CEN); //zaženi counter za utripanje
		}

		WRITE_REG(TIM3->CNT,0); //vpiši 0 v counter, da začne šteti vedno od začetka
		SET_BIT(TIM3->CR1,TIM_CR1_CEN); //zaženi counter za debounce -> da se lahko gumb ponovno pritisne

		button_allowed = 0; //gumb se ne sme pritisnitis
	}
	WRITE_REG(EXTI->PR,0x01);          // clear interrupt pending flag, brišeš tako da zapišeš 1 (čudna logika, ali ipak)
}

#include "settings.h"
#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_spi.h"
#include "uart.h"
#include "linux.h"
#include "math.h"
#include "misc.h"
#include "cp33.h"
#include "usb.h"



#define BUFFER_SIZE 65536
cp33_t cp33;

void init_i2s()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_12;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}


int main(void)
{
	init_linux();

/* Enable the GPIOs */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA |
			RCC_AHB1Periph_GPIOB |
			RCC_AHB1Periph_GPIOC |
			RCC_AHB1Periph_GPIOD |
			RCC_AHB1Periph_GPIOE |
			RCC_AHB1Periph_CCMDATARAMEN, 
		ENABLE);
// general purpose timer
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM10, ENABLE);
	TIM_DeInit(TIM10);
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = 50;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM10, &TIM_TimeBaseStructure);
	TIM_Cmd(TIM10, ENABLE);

// debug pin
	GPIO_InitTypeDef  GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	CLEAR_PIN(GPIOA, GPIO_Pin_14);


	NVIC_SetVectorTable(NVIC_VectTab_FLASH, PROGRAM_START - 0x08000000);



	init_uart();
	print_lf();
	print_text("**********************\n");
	print_text("* CP33 recorder      *\n");
	print_text("**********************\n");
	flush_uart();

	memset(&cp33, 0, sizeof(cp33_t));
	cp33.buffer = kmalloc(BUFFER_SIZE, 0);
	cp33.fragment_size = FRAGMENT_SIZE;




	init_usb();

// have to use polling for all but sample capture
  	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
 	NVIC_Init(&NVIC_InitStructure);



	init_i2s();
	usb_start_receive();

	
	unsigned int left_value;
	unsigned int right_value;
	int current_channel = 0;
	while(1)
	{
// 		if(TIM10->SR & TIM_FLAG_Update)
// 		{
// 			cp33.timer_high++;
// 			TIM10->SR = ~TIM_FLAG_Update;
// 
// 			if((cp33.timer_high % 50) == 0)
// 			{
// 				TRACE2
// 				print_text("total_bytes=");
// 				print_number(cp33.total_bytes);
// 				print_text("timer_high=");
// 				print_number(cp33.timer_high);
// 			}
// 		}


		handle_uart();

#ifndef TEST_STREAM

		SET_PIN(GPIOA, GPIO_Pin_14);

		if(current_channel)
		{
// rising edge of word select
//			if((GPIOA->IDR & GPIO_Pin_15))
//				TRACE2

			while(!(GPIOA->IDR & GPIO_Pin_15))
				;
		}
		else
		{
// falling edge of word select
//			if(!(GPIOA->IDR & GPIO_Pin_15))
//				TRACE2

			while((GPIOA->IDR & GPIO_Pin_15))
				;
		}
		

		unsigned int new_value = 0;
		int i;
// falling clock
		while((GPIOC->IDR & GPIO_Pin_10))
			;
		for(i = 0; i < 24; i++)
		{
// rising clock
			while(!(GPIOC->IDR & GPIO_Pin_10))
				;
		
			new_value <<= 1;
			new_value |= PIN_IS_SET(GPIOC, GPIO_Pin_12);

// falling clock
			if(i < 23)
			{
				while((GPIOC->IDR & GPIO_Pin_10))
					;
			}
		}
		CLEAR_PIN(GPIOA, GPIO_Pin_14);

		
		if(!current_channel)
		{
			left_value = new_value;
		}
		else
		{
			right_value = new_value;
			if(cp33.size < BUFFER_SIZE)
			{
				*(int32_t*)(cp33.buffer + cp33.write_offset) = left_value;
				cp33.write_offset += 4;
				*(int32_t*)(cp33.buffer + cp33.write_offset) = right_value;
				cp33.write_offset += 4;
				cp33.size += 8;

				if(cp33.write_offset >= BUFFER_SIZE) cp33.write_offset = 0;
			}
		}

		current_channel = !current_channel;

		
#else // !TEST_STREAM
		if(cp33.size < BUFFER_SIZE)
		{
			*(int32_t*)(cp33.buffer + cp33.write_offset) = test_value;
			cp33.write_offset += 4;
			*(int32_t*)(cp33.buffer + cp33.write_offset) = -test_value;
			cp33.write_offset += 4;
			cp33.size += 8;
			cp33.total_bytes += 8;

			if(cp33.write_offset >= BUFFER_SIZE) cp33.write_offset = 0;
			test_value++;
		}

#endif // TEST_STREAM

		if(cp33.size >= cp33.fragment_size &&
			usb.have_response &&
			usb.have_beacon)
		{
			usb_start_transmit(cp33.buffer + cp33.read_offset, cp33.fragment_size);
			cp33.read_offset += cp33.fragment_size;
			if(cp33.read_offset >= BUFFER_SIZE) cp33.read_offset = 0;
			cp33.size -= cp33.fragment_size;
		}
		else
		{
//int pin_state = (GPIOA->IDR & GPIO_Pin_15);
			USBD_OTG_ISR_Handler(&USB_OTG_dev);
//if(pin_state != (GPIOA->IDR & GPIO_Pin_15))
//TRACE2
		}
		
	}
	
}














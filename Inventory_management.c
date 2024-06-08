/*
 * Inventory_management.c
 *
 *  Created on: Jun 8, 2024
 *      Author: 91626
 */

#include<stdio.h>
#include "ds1307.h"
#include "stm32f407xx.h"
#include<string.h>
#include "lcd.h"


#define LOW 0
#define BTN_PRESSED LOW

/*• dd: uint8_t (range: 1 to 31)

• mm: uint8_t (range: 1 to 12)

• yyyy: uint16_t (range: 1 to 9999)

• hh: uint8_t (range: 0 to 23)

• mm: uint8_t (range: 0 to 59)

• ss: uint8_t (range: 0 to 59)*/



char msg1[128]="Carton 1 has been used at  ";
char msg2[128]="Carton 2 has been used at  ";
char msg3[128]="Carton 3 has been used at  ";
char msg[128]="All Cartons have been used. Item out of stock ";

char* concatenate_string(char *s, char *s1)
{
    int i;

    int j = strlen(s);

    for (i = 0; s1[i] != '\0'; i++) {
        s[i + j] = s1[i];
    }

    s[i + j] = '\0';

    return s;
}

char* get_day_of_week(uint8_t i)
{
	char* days[] = { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

	return days[i-1];
}


void number_to_string(uint8_t num , char* buf)
{

	if(num < 10){
		buf[0] = '0';
		buf[1] = num+48;
	}else if(num >= 10 && num < 99)
	{
		buf[0] = (num/10) + 48;
		buf[1]= (num % 10) + 48;
	}
}



//hh:mm:ss
char* time_to_string(RTC_time_t *rtc_time)
{
	static char buf[9];

	buf[2]= ':';
	buf[5]= ':';

	number_to_string(rtc_time->hours,buf);
	number_to_string(rtc_time->minutes,&buf[3]);
	number_to_string(rtc_time->seconds,&buf[6]);

	buf[8] = '\0';

	return buf;

}

//dd/mm/yy
char* date_to_string(RTC_date_t *rtc_date)
{
	static char buf[9];

	buf[2]= '/';
	buf[5]= '/';

	number_to_string(rtc_date->date,buf);
	number_to_string(rtc_date->month,&buf[3]);
	number_to_string(rtc_date->year,&buf[6]);

	buf[8]= '\0';

	return buf;

}

USART_Handle_t usart2_handle;

void USART2_Init(void)
{
	usart2_handle.pUSARTx = USART2;
	usart2_handle.USART_Config.USART_Baud = USART_STD_BAUD_115200;
	usart2_handle.USART_Config.USART_HWFlowControl = USART_HW_FLOW_CTRL_NONE;
	usart2_handle.USART_Config.USART_Mode = USART_MODE_ONLY_TX;
	usart2_handle.USART_Config.USART_NoOfStopBits = USART_STOPBITS_1;
	usart2_handle.USART_Config.USART_WordLength = USART_WORDLEN_8BITS;
	usart2_handle.USART_Config.USART_ParityControl = USART_PARITY_DISABLE;
	USART_Init(&usart2_handle);
}

void 	USART2_GPIOInit(void)
{
	GPIO_Handle_t usart_gpios;

	usart_gpios.pGPIOx = GPIOA;
	usart_gpios.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	usart_gpios.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	usart_gpios.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	usart_gpios.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	usart_gpios.GPIO_PinConfig.GPIO_PinAltFunMode =7;

	//USART2 TX
	usart_gpios.GPIO_PinConfig.GPIO_PinNumber  = GPIO_PIN_NO_2;
	GPIO_Init(&usart_gpios);

	//USART2 RX
	usart_gpios.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_3;
	GPIO_Init(&usart_gpios);


}

void GPIO_ButtonInit1(void)
{//PA0 is onboard push button
	GPIO_Handle_t GPIOBtn1;

	//this is btn gpio configuration
	GPIOBtn1.pGPIOx = GPIOA;
	GPIOBtn1.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
	GPIOBtn1.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	GPIOBtn1.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	GPIOBtn1.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

	GPIO_Init(&GPIOBtn1);



}

void GPIO_ButtonInit2(void)
{
	GPIO_Handle_t GPIOBtn2;

	//this is btn gpio configuration
	GPIOBtn2.pGPIOx = GPIOA;
	GPIOBtn2.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
	GPIOBtn2.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	GPIOBtn2.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	GPIOBtn2.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;//no internal pull up resistor

	GPIO_Init(&GPIOBtn2);


}

void GPIO_ButtonInit3(void)
{
	GPIO_Handle_t GPIOBtn;

	//this is btn gpio configuration
	GPIOBtn.pGPIOx = GPIOC;
	GPIOBtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
	GPIOBtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	GPIOBtn.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	GPIOBtn.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;//no internal pull up resistor

	GPIO_Init(&GPIOBtn);



}

void delay(void)
{
	for(uint32_t i = 0 ; i < 500000/2 ; i ++);
}


int main(void)
{

	GPIO_ButtonInit1();

	GPIO_ButtonInit2();

	GPIO_ButtonInit3();

	USART2_GPIOInit();

    USART2_Init();

    USART_PeripheralControl(USART2,ENABLE);

    RTC_time_t current_time;
    RTC_date_t current_date;

    #ifndef PRINT_LCD //if not defined
    	printf("RTC test\n");
    #else
    	lcd_init();

    	lcd_print_string("RTC Test...");


    	mdelay(2000);

    	lcd_display_clear();
    	lcd_display_return_home();
    #endif


    	if(ds1307_init()){
    		printf("RTC init has failed\n");
    while(1);
    	}


    	                    current_date.day = SATURDAY;
    						current_date.date = 8;
    						current_date.month = 06;
    						current_date.year = 24;

    						current_time.hours = 10;
    						current_time.minutes = 33;
    						current_time.seconds = 10;
    						current_time.time_format = TIME_FORMAT_12HRS_PM;
    						ds1307_set_current_date(&current_date);
    						ds1307_set_current_time(&current_time);

    						char *am_pm;
    						if(current_time.time_format != TIME_FORMAT_24HRS){
    							am_pm = (current_time.time_format) ? "PM" : "AM";
    						}


		//wait till button1 is pressed
		while( ! GPIO_ReadFromInputPin(GPIOA,GPIO_PIN_NO_0) );

		//to avoid button de-bouncing related issues use delay
		delay();

		                ds1307_get_current_time(&current_time);
		                ds1307_get_current_date(&current_date);

					        concatenate_string(msg1, time_to_string(&current_time));
					        concatenate_string(msg1, am_pm);
						    concatenate_string(msg1, " ");
						    concatenate_string(msg1, date_to_string(&current_date));
						    concatenate_string(msg1, " ");
						    concatenate_string(msg1, get_day_of_week(current_date.day));
						    concatenate_string(msg1, "\n\r");
					USART_SendData(&usart2_handle,(uint8_t*)msg1,strlen(msg1));

		//wait till button2 is pressed
		while( GPIO_ReadFromInputPin(GPIOA,GPIO_PIN_NO_7) != BTN_PRESSED );

		//to avoid button de-bouncing related issues use delay
		delay();

		                   ds1307_get_current_time(&current_time);
				           ds1307_get_current_date(&current_date);

						   concatenate_string(msg2, time_to_string(&current_time));
						   concatenate_string(msg2, am_pm);
						   concatenate_string(msg2, " ");
					       concatenate_string(msg2, date_to_string(&current_date));
					       concatenate_string(msg2, " ");
						   concatenate_string(msg2, get_day_of_week(current_date.day));
						   concatenate_string(msg2, "\n\r");

		USART_SendData(&usart2_handle,(uint8_t*)msg2,strlen(msg2));


	    //wait till button3 is pressed
		 while( GPIO_ReadFromInputPin(GPIOC,GPIO_PIN_NO_5) != BTN_PRESSED );

		//to avoid button de-bouncing related issues use delay
		delay();

		ds1307_get_current_time(&current_time);
		ds1307_get_current_date(&current_date);

						     concatenate_string(msg3, time_to_string(&current_time));
						     concatenate_string(msg3, am_pm);
						     concatenate_string(msg3, " ");
							 concatenate_string(msg3, date_to_string(&current_date));
							 concatenate_string(msg3, " ");
						     concatenate_string(msg3, get_day_of_week(current_date.day));
						     concatenate_string(msg3, "\n\r");


		USART_SendData(&usart2_handle,(uint8_t*)msg3,strlen(msg3));

		delay();

		USART_SendData(&usart2_handle,(uint8_t*)msg,strlen(msg));




	return 0;
}



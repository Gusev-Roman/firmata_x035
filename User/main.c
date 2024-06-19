/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/26
 * Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

/*
 *@Note
 *SDI_Printf routine:
 *It needs to be used with WCH-LinkUtility 1.8.
 *Note:version of WCH-LinkUtility must after 1.8
 *
 */

#include "debug.h"
#include <stdbool.h>
#include <stdlib.h>	// malloc/free
#include "firmata_def.h"
#include <assert.h>
#include "FIFO.h"

/* Global define */

/* Global Variable */
// указатель на FIFO-ring 1 кб для исходящих данных
fifo_t _send_buf;
// для входящих данных тоже можно приделать буфер, но вытаскивать все равно будем по готовности и побайтово
fifo_t _rcv_buf;
// запись в буфер увеличивает last до тех пор, пока не будет достигнут правый край; last перепрыгнет на начало
// запись в регистр увеличит first, пока не достигнет last

/*********************************************************************
 * @fn      GPIO_Toggle_INIT
 *
 * @brief   Initializes GPIOA.0
 *
 * @return  none
 */
void GPIO_Toggle_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void uart_echo(u8 *buf, u8 sz){
	u8 tmpb[4];
	if(sz > 0){
		u8 chr = buf[sz-1];
		sprintf(tmpb, "%02X ", chr);
		printf(tmpb);
	}
}

u8 uart_fetch(){
	while(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET){} // wait for symbol accepted in USART input buffer
	return (u8)USART_ReceiveData(USART2);
}

const u16 _pins_[] = {
		// A0
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		// A1
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		// A2 & A3 disabled (UART Function)
		0,
		0,
		// A4-A7
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		// B0
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		// B1
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		// B3
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		// B11
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		// B12
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		// C14
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP,
		// C15
		PIN_DIG_INPUT | PIN_DIG_OUTPUT | PIN_DIG_PULLUP
};

const u32 _pin_port_[] = {
		RCC_APB2Periph_GPIOA,
		RCC_APB2Periph_GPIOA,
		RCC_APB2Periph_GPIOA,
		RCC_APB2Periph_GPIOA,
		RCC_APB2Periph_GPIOA,
		RCC_APB2Periph_GPIOA,
		RCC_APB2Periph_GPIOA,
		RCC_APB2Periph_GPIOA,
		RCC_APB2Periph_GPIOB,
		RCC_APB2Periph_GPIOB,
		RCC_APB2Periph_GPIOB,
		RCC_APB2Periph_GPIOB,
		RCC_APB2Periph_GPIOB,
		RCC_APB2Periph_GPIOC,
		RCC_APB2Periph_GPIOC
};

const u32 _pin_num_[] = {
		// A pins
		GPIO_Pin_0,GPIO_Pin_1,GPIO_Pin_2,GPIO_Pin_3,GPIO_Pin_4,GPIO_Pin_5,GPIO_Pin_6,GPIO_Pin_7,
		// B pins
		GPIO_Pin_0,GPIO_Pin_1,GPIO_Pin_3,GPIO_Pin_11,GPIO_Pin_12,
		// C pins
		GPIO_Pin_14,GPIO_Pin_15
};

/*
 * wait for output buffer empty and send next byte
 */
void uart_send(u8 *buf, u8 len){
	for(int i=0; i<len; i++){
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET){}
	    USART_SendData(USART2, buf[i]);
	}
}

/*
 * Позже тут будет экзотика: ШИМ, серво, и т.п.
 * пока простейший ногодрыг, даже баз аналоговых входов
 */
void make_response(u8 *buff, u8 *len){
	// GPIO_Mode_IPU - input with pullup
	// GPIO_Mode_IN_FLOATING simple input
	int i = 0;
	buff[i++] = START_SYSEX;
	buff[i++] = CAPABILITY_RESPONSE;

	for(int k=0; k < (sizeof(_pins_)/2); k++){
		if(_pins_[k] & PIN_DIG_INPUT) { buff[i++] = 0; buff[i++] = 1; }
		if(_pins_[k] & PIN_DIG_OUTPUT){ buff[i++] = 1; buff[i++] = 1; }
		if(_pins_[k] & PIN_DIG_PULLUP){ buff[i++] = 0xB; buff[i++] = 1; }
		buff[i++] = 0x7F;	// end of pin
	}
	buff[i++] = END_SYSEX;
	*len = (u8)i;
}

void call_sysex(u8 *buf, u8 *sz){
	bool wr = false;
	u8 uchr, caps_len, *firmata_caps;
	caps_len = 2;
    while(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET)
    {
    	// wait first symbol
    }
    uchr = USART_ReceiveData(USART2);
    buf[(*sz)++] = uchr;
    if(uchr == REPORT_FIRMWARE) wr = true;
    else if(uchr == CAPABILITY_QUERY){ // make switch case...
    	// fetch sysex_end char
    	uart_fetch();
    	// make response
    	firmata_caps = malloc(256); // надеюсь этого хватит)
    	make_response(firmata_caps, &caps_len);
    	// send response
    	uart_send(firmata_caps, caps_len); // длина будет изменяться в процессе
    	free(firmata_caps);
    }

    while(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET){} // wait 3th char
    uchr = USART_ReceiveData(USART2);
    buf[(*sz)++] = uchr;
    if(uchr == END_SYSEX){ // make response & exit
    	if(wr){
    		static u8 resp[] = "\xF0\x79\2\5S\0t\0a\0n\0d\0a\0r\0d\0F\0i\0r\0m\0a\0t\0a\0\xF7";
    		u8 chr, i=0;
    		// loop to send Firmata string
    		do{
    			chr = resp[i++];
    			while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET){}
     		    USART_SendData(USART2, chr);
    		}while(chr != 0xF7);
    	}
    }
    else { // wrong request, exit w/o response
    	return;
    }
}
/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    u8 u_char, s_char;
    bool wait_sysex = false;
    static u8 RxCnt1 = 0;
#define TxSize1 100
    u8 RxBuffer1[TxSize1] = {0};               /* USART2 Read buffer on stack */
    _send_buf = fifo_create(1024, sizeof(char));

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

#if (SDI_PRINT == SDI_PR_OPEN)
    SDI_Printf_Enable();
#else
    USART_Printf_Init(57600); // firmata speed
#endif

    GPIO_Toggle_INIT();

    while(1)
    {
    	//process_tx_queue(); // send next char if out buffer empty
    	//process_rx_queue(); // get next char if rcv buff ready
        if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
        {
        	u_char = USART_ReceiveData(USART2);
        	fifo_add(_rcv_buf, &u_char);
        }
        if(!fifo_is_empty(_send_buf)){
        	if(USART_GetFlagStatus(USART2, USART_FLAG_TXE) != RESET){
        		fifo_get(_send_buf, &s_char);
        		USART_SendData(USART2, s_char);
        	}
        }
        if(wait_sysex){
        	switch(u_char){
        	case REPORT_FIRMWARE:
        		break;
        	}
        	wait_sysex = false;
        }
        // echo our char as hex
        if(u_char == START_SYSEX){
        	wait_sysex = true;
        }
        // maybe use USART3 as debug device?

        // ��ڧާӧ�� ����ӧ֧��֧��� �ߧ� F0 (start sysex) �ڧݧ� F9 (get_ver) �ڧݧ� D7 (report dig port)
        // ����ݧ� SYSEX ���ڧ��էڧ� 79
    }
}

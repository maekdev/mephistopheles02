// main.c
//
// 200322
// markus ekler
//
// romdump


// INCLUDES
#include <avr/io.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "usart.h"


// DEFINES

// PROTOTYPES

// FUNCTIONS
void eyecandy(void) {
	usart_puts_P(PSTR("\r\n"));
	usart_puts_P(PSTR("\r\n"));
	usart_puts_P(PSTR("\r\n"));

	usart_puts_P(PSTR(" _   _                             _      _   _                   \r\n"));
	usart_puts_P(PSTR("| | | | ___ _   _   _   _  ___    | | ___| |_( )___    __ _  ___  \r\n"));
	usart_puts_P(PSTR("| |_| |/ _ \\ | | | | | | |/ _ \\   | |/ _ \\ __|// __|  / _` |/ _ \\ \r\n"));
	usart_puts_P(PSTR("|  _  |  __/ |_| | | |_| | (_) |  | |  __/ |_  \\__ \\ | (_| | (_) |\r\n"));
	usart_puts_P(PSTR("|_| |_|\\___|\\__, |  \\__, |\\___( ) |_|\\___|\\__| |___/  \\__, |\\___/ \r\n"));
	usart_puts_P(PSTR("            |___/   |___/     |/                      |___/       \r\n"));
}

void setaddr(uint16_t a) {
	PORTD &= 0x03;
	PORTD |= (uint8_t)(a>>6);
	PORTA = (uint8_t)(a);	
}


uint8_t readaddr(void) {
	return (PINB);
}

// MAIN
int main() {
	uint16_t addr;
	int i;

	// sysclk
	clock_prescale_set(clock_div_1);

	// portio
	DDRA |= 0xFF;
	DDRB &= 0x00;
	DDRD |= 0xFC;
	
	// serial
	usart_init();


	for (addr=0;addr!=16*1024;addr++) {
		if (addr % 16 == 0) {
			usart_puts_P(PSTR("\r\n0000"));
			usart_putx((uint8_t)(addr>>8));
			usart_putx((uint8_t)(addr));
			usart_putc(':');
		}
		setaddr(addr);
		//_delay_ms(1);
		_delay_us(50);
		if (addr % 2 == 0) 
			usart_putc(' ');
		usart_putx(readaddr());
	}

	eyecandy();
	
	while (1) {
		// endless loop will never stop
	}
	return 0;
}

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "usart.h"

//#define BAUDRATE 9600
#define BAUDRATE 9600
#define MYUBRR F_CPU/16/BAUDRATE-1


void usart_init() {
	UBRR0 = MYUBRR;
  	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void usart_putc(unsigned char c) {
	while ( !( UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

void usart_putx(unsigned char c) {
	char hex[] = "0123456789ABCDEF";

	usart_putc(hex[(c>>4)&0x0F]);
	usart_putc(hex[(c&0x0F)]);
}

void usart_puts(char *str) {
	while (*str) {
		usart_putc(*str);
		str++;
	}
}

void usart_puts_P(const char *s) {
	while (pgm_read_byte(s)) {
		usart_putc(pgm_read_byte(s));
		s++;
	}
}

void usart_puti(unsigned int i) {
	char str[5]="\0\0\0\0\0";
	char o=0;

	if (i == 0) {
		usart_putc('0');
		return;
	}

	while (i != 0) {
		str[o] = (char)(i%10);
		o++;
		i/=10;
	}

	while (o > 0) {
		usart_putc(str[o-1]+'0');
		o--;
	}

}

unsigned char usart_getc(void) {
	while ( !(UCSR0A & (1<<RXC0)) );
	return UDR0;
}

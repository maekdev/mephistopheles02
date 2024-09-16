
#ifndef __USART_H
#define __USART_H


void usart_init();
void usart_putc(unsigned char c);
void usart_putx(unsigned char c);
void usart_puts(char *str);
void usart_puts_P(const char *s);
void usart_puti(unsigned int i);
unsigned char usart_getc(void);

#endif 

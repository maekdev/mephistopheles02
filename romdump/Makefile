CC = @avr-gcc
RM = rm -rf

PROJECT = romdump

MCU = atmega1284p
CLK = 8000000L

SRC = \
	main.c \
	usart.c 

OBJ = $(SRC:.c=.o)

CFLAGS = -x c -mno-interrupts -funsigned-char -funsigned-bitfields -DF_CPU=$(CLK) -Os -fno-inline-small-functions -fno-split-wide-types -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -Wall -mmcu=$(MCU) -c -std=gnu99 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" 
LDFLAGS = -Wl,-s -Wl,-static -Wl,-Map="$(PROJECT).map" -Wl,--start-group  -Wl,--end-group -Wl,--gc-sections -mrelax -Wl,-section-start=.text=0x0000 -mmcu=$(MCU)  

all: out

rebuild: clean all

clean:
	$(RM) *.o *.elf *.hex *.d .map
	$(RM) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(PROJECT).elf: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) -lm
	
$(PROJECT).hex: $(PROJECT).elf
	avr-objcopy -O ihex -R .eeprom $< $@ 

out: $(PROJECT).hex
	avr-size $(PROJECT).elf

program: $(PROJECT).hex
	sudo avrdude -c stk600 -p $(MCU) -U flash:w:$(PROJECT).hex:i

reset: 
	sudo avrdude -c stk600 -p $(MCU)

CC = gcc
RM = @rm -rf
ECHO = @echo
STRIP = @strip
CFLAGS = -O2 -Wall $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image

SRC = \
	main.c \
	65c02cpu.c \
	65c02mem.c 
OBJ = $(SRC:.c=.o)

BIN = meph
BACKUPDIR = /home/donovan/tmp

all: $(BIN)
	$(ECHO) "All done."

clean: 
	$(RM) $(OBJ)
	$(RM) $(BIN)

rebuild: clean all

run: $(BIN)
	./$(BIN)
	

backup: clean
	@tar czf $(BACKUPDIR)/`date +"%y%m%d_%H%M"`_`pwd | rev | cut -f1 -d'/' - | rev`.tar.gz *
	$(ECHO) "Backup stored in target folder."

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^
	

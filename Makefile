# meph02 Makefile 

# Compiler, remove, echo, and strip commands
CC = @gcc
RM = @rm -rf
ECHO = @echo
STRIP = @strip
AR = @ar

# Compiler and linker flags
INC = -I$(SRC_DIR)
INC += -I./lib/6502 
INC += -I./lib/vrEmu6502/src 
INC += -I./lib/65c02
INC += -I./lib/miniz
INC += -I./lib/md5-c
# Configurables
# choose emulator instance VREMU6502 or M6502
#DEF = -D_VREMU6502
DEF = -D_M6502
# choose to include rom.h
#DEF += -D_BUILTINROM
# enable DEBUG Options (uncomment to enable)
#DEF += -D_DEBUG

# Directories for sources, objects, and binaries
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
LIB_DIR = lib

# Source files, object files, and binary name
SRC = 	$(SRC_DIR)/main.c \
		$(SRC_DIR)/cfg.c \
		$(SRC_DIR)/gfx.c \
		$(SRC_DIR)/emu.c 
OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
#OBJ += $(OBJ_DIR)/vrEmu6502.o
#OBJ += $(OBJ_DIR)/m6502.o
BIN = $(BIN_DIR)/meph

# Libraries
LIBS = -lvrEmu6502 -l6502 -lminiz -lmd5

# Backup directory
BACKUPDIR = /home/donovan/tmp

# Flags
CFLAGS = -O2 -Wall $(shell sdl2-config --cflags) $(INC) $(DEF)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image
LDFLAGS += -L$(LIB_DIR) $(LIBS)


# Default target: build the binary
all: libs $(BIN) 
	$(ECHO) "All done."

# Clean object files and binaries
clean:
	$(ECHO) "=== clean project directory ==="
	$(MAKE) -C $(LIB_DIR) clean "RM=$(RM)" "ECHO=$(ECHO)"
	$(RM) $(OBJ_DIR)/*.o
	$(RM) $(BIN)

# Rebuild target: clean and build all
rebuild: clean all

# Run the binary
run: $(BIN)
	$(ECHO) "=== execute binary ==="
	./$(BIN)

# Create a backup (clean first, then archive)
backup: clean
	$(ECHO) "=== generating backup file ==="
	@tar czf $(BACKUPDIR)/`date +"%y%m%d_%H%M"`_`pwd | rev | cut -f1 -d'/' - | rev`.tar.gz *
	$(ECHO) "Backup stored in target folder."

# Rule to compile .c files to .o files (in the obj directory)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(ECHO) "=== compiling ($<) ==="
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# extrawuerste
#$(OBJ_DIR)/vrEmu6502.o: $(LIB_DIR)/vrEmu6502/src/vrEmu6502.c
#	$(ECHO) "=== compiling ($<) ==="
#	@mkdir -p $(OBJ_DIR)
#	$(CC) $(CFLAGS) -c $< -o $@

#$(OBJ_DIR)/m6502.o: $(LIB_DIR)/6502/m6502.c
#	$(ECHO) "=== compiling ($<) ==="
#	@mkdir -p $(OBJ_DIR)
#	$(CC) $(CFLAGS) -c $< -o $@

# not sure if this looks better but works though
#libs: $(LIB_DIR)/libminiz.a $(LIB_DIR)/lib6502.a $(LIB_DIR)/libvrEmu6502.a

libs:
	$(MAKE) -C $(LIB_DIR) all "RM=$(RM)" "ECHO=$(ECHO)" "CC=$(CC)" "AR=$(AR)"

libclean:
	$(MAKE) -C $(LIB_DIR) clean "RM=$(RM)" "ECHO=$(ECHO)"

#$(LIB_DIR)/libminiz.a: 
#	$(MAKE) -C $(LIB_DIR) libminiz.a CC=$(CC) ECHO=$(ECHO)
#
#$(LIB_DIR)/libminiz.a: 
#	$(MAKE) -C $(LIB_DIR) lib6502.a CC=$(CC) ECHO=$(ECHO)
#
#$(LIB_DIR)/libminiz.a: 
#	$(MAKE) -C $(LIB_DIR) libvrEmu6502.a CC=$(CC) ECHO=$(ECHO)

# Link the object files into the final binary (in the bin directory)
$(BIN): $(OBJ)
	$(ECHO) "=== linking ($@) ==="
	@mkdir -p $(BIN_DIR)
	#$(CC) $(LDFLAGS) -o $@ $^
	$(CC) -o $@ $^ $(LDFLAGS)
	$(ECHO) $(CC) $(LDFLAGS) -o $@ $^

# Phony targets to prevent confusion with files of the same name
.PHONY: all clean rebuild run backup

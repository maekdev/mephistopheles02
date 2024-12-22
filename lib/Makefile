# meph02 libraries Makefile (create required libraries)

clean: libminiz_clean lib6502_clean libvrEmu6502_clean libmd5_clean

all: libminiz.a lib6502.a libvrEmu6502.a libmd5.a

libminiz.a: 
	$(ECHO) "=== building library ($@) ==="
	$(CC) -I./miniz -c ./miniz/miniz.c -o ./miniz/miniz.o
	$(AR) cr libminiz.a ./miniz/miniz.o

libminiz_clean: 
	$(ECHO) "=== cleaning miniz library ==="
	$(RM) ./miniz/miniz.o libminiz.a

lib6502.a:
	$(ECHO) "=== building library ($@) ==="
	$(CC) -I./6502 -c ./6502/m6502.c -o ./6502/m6502.o
	$(AR) cr lib6502.a ./6502/m6502.o

lib6502_clean:
	$(ECHO) "=== cleaning m6502 library ==="
	$(RM) ./6502/m6502.o lib6502.a

libvrEmu6502.a:
	$(ECHO) "=== building library ($@) ==="
	$(CC) -I./vrEmu6502/src -c ./vrEmu6502/src/vrEmu6502.c -o ./vrEmu6502/src/vrEmu6502.o
	$(AR) cr libvrEmu6502.a ./vrEmu6502/src/vrEmu6502.o

libvrEmu6502_clean:
	$(ECHO) "=== cleaning vrEmu6502 library ==="
	$(RM) libvrEmu6502.a ./vrEmu6502/src/vrEmu6502.o

libmd5.a:
	$(ECHO) "=== building library ($@) ==="
	$(CC) -I./md5-c -c ./md5-c/md5.c -o ./md5-c/md5.o
	$(AR) cr libmd5.a ./md5-c/md5.o

libmd5_clean:
	$(ECHO) "=== cleaning md5 library ==="
	$(RM) ./md5-c/md5.o libmd5.a

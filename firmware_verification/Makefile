CC = clang

src = crc.c firmware_verification.c

firmware_verification: $(src)
	$(CC) -o $@ $(src)

clean: firmware_verification
	rm firmware_verification

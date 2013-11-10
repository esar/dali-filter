all:
	avr-gcc -mmcu=attiny861 DaliFilter.c -o DaliFilter
	avr-objcopy -O ihex DaliFilter DaliFilter.hex
	avrdude -p t861 -c avrispmkii -P usb -U flash:w:DaliFilter.hex 

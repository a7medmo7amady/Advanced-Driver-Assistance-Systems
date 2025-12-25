# AVR Makefile for ATmega328P

# Project
TARGET = Main
SRC = Main.c

# MCU Settings
MCU = atmega328p
F_CPU = 16000000UL

# Compiler
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

# Flags
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -std=c99
LDFLAGS = -mmcu=$(MCU)

# Programmer (Arduino as ISP)
PROGRAMMER = arduino
PORT = COM3
BAUD = 115200

# Targets
all: $(TARGET).hex

$(TARGET).elf: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

flash: $(TARGET).hex
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$

clean:
	del /Q *.elf *.hex 2>nul

size: $(TARGET).elf
	avr-size --mcu=$(MCU) -C $

.PHONY: all flash clean size
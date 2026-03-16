# Linux
PORT_GPU ?= /dev/ttyUSB0
PORT_CPU ?= /dev/ttyUSB1
# Windows
#PORT ?= COM1
 
all: gpu cpu

gpu: gpu.c
	avr-gcc -mmcu=atmega328p -DF_CPU=12000000 -Os -Wall -o gpu.elf gpu.c
	avr-objcopy -j .text -j .data -O ihex gpu.elf gpu.hex

cpu: cpu.c
	avr-gcc -mmcu=atmega328p -DF_CPU=12000000 -Os -Wall -o cpu.elf cpu.c
	avr-objcopy -j .text -j .data -O ihex cpu.elf cpu.hex


upload_gpu: gpu.hex
	avrdude -c arduino -p atmega328p -P $(PORT_GPU) -b 115200 -D -U flash:w:gpu.hex:i

upload_cpu: cpu.hex
	avrdude -c urclock -P $(PORT_CPU) -b 57600 -p atmega328p -D -xnometadata -U flash:w:$<:a

clean:
	rm -rf cpu.hex gpu.hex cpu.elf gpu.elf
# Linux
PORT_GPU ?= /dev/ttyUSB0
PORT_CPU ?= /dev/ttyACM0
# Windows
#PORT ?= COM1

GPU_FILES = gpu.c spi.c lcd_screen.c geometry.c uart.c buffer.c

GPU_HEADERS = lcd_screen.h spi.h geometry.h bits.h gpu_commands.h buffer.h

CPU_FILES = cpu.c uart.c timer.c

CPU_HEADERS = uart.h gpu_commands.h bits.h timer.h

all: gpu cpu

gpu: $(GPU_FILES) $(GPU_HEADERS)
	avr-gcc -mmcu=atmega328p -DF_CPU=16000000 -Os -Wall -lm -o gpu.elf $(GPU_FILES)
	avr-objcopy -j .text -j .data -O ihex gpu.elf gpu.hex

cpu: $(CPU_FILES) $(CPU_HEADERS)
	avr-gcc -mmcu=atmega328p -DF_CPU=16000000 -Os -Wall -o cpu.elf $(CPU_FILES)
	avr-objcopy -j .text -j .data -O ihex cpu.elf cpu.hex

upload: upload_cpu upload_gpu

upload_gpu: gpu.hex
	avrdude -c arduino -p atmega328p -P $(PORT_GPU) -b 115200 -D -U flash:w:gpu.hex:i

upload_cpu: cpu.hex
	avrdude -c xplainedmini -p m328p -U flash:w:cpu.hex:i

clean:
	rm -rf cpu.hex gpu.hex cpu.elf gpu.elf
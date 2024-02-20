GCC := gcc
GCC_ARM := /opt/arm/bin/arm-elf-gcc
OBJCOPY := /opt/arm/bin/arm-elf-objcopy
ARM_CFLAGS := -O2 -c -I. -Irtlwifi -Iarm -Istm32f4 -fno-builtin -mthumb -mcpu=cortex-m4 -mlittle-endian -ffreestanding 
ARM_LFLAGS := -fno-builtin -mthumb -mcpu=cortex-m4 -mlittle-endian -ffreestanding -nostdlib -nostdinc -L/opt/arm/lib/gcc/arm-elf/4.1.1/
OBJDIR := $(shell uname --machine)
USB_DIR := libusb-1.0.0

ARM_OBJS := \
	stm32f4/startup_main.o \
	stm32f4/usb_hcd_int.o \
	cp33.o \
	fft.o \
	uart.o \
	usb.o \
	stm32f4/misc.o \
	stm32f4/stm32f4xx_rcc.o \
	stm32f4/stm32f4xx_usart.o \
	stm32f4/stm32f4xx_gpio.o \
	stm32f4/stm32f4xx_dac.o \
	stm32f4/stm32f4xx_dcmi.o \
	stm32f4/stm32f4xx_dma.o \
	stm32f4/stm32f4xx_i2c.o \
	stm32f4/stm32f4xx_it.o \
	stm32f4/stm32f4xx_iwdg.o \
	stm32f4/stm32f4xx_tim.o \
	stm32f4/stm32f4xx_adc.o \
	stm32f4/stm32f4xx_flash.o \
	stm32f4/stm32f4xx_spi.o \
	stm32f4/usb_bsp.o \
	stm32f4/usb_core.o \
	stm32f4/usb_dcd.o \
	stm32f4/usb_hcd.o \
	stm32f4/usbd_core.o \
	stm32f4/usbd_req.o \
	stm32f4/usbd_ioreq.o \
	stm32f4/usb_dcd_int.o \
	stm32f4/usbh_core.o \
	stm32f4/usbh_hcs.o \
	stm32f4/usbh_ioreq.o \
	stm32f4/usbh_stdreq.o \
	math.o \
	linux.o


ARM_BOOTLOADER_OBJS := \
	bootloader.o \
	stm32f4/startup_boot.o \
	stm32f4/system_stm32f4xx.o \
	uart.o \
	stm32f4/misc.o \
	stm32f4/stm32f4xx_flash.o \
	stm32f4/stm32f4xx_gpio.o \
	stm32f4/stm32f4xx_rcc.o \
	stm32f4/stm32f4xx_tim.o \
	stm32f4/stm32f4xx_usart.o




all: cp33.bin bootloader.bin recordcp33 passthrough

piano: piano.c pianoserver.o metro.o
	$(GCC) -o piano piano.c pianoserver.c metro.o $(USB_DIR)/libusb/.libs/libusb-1.0.a -I$(USB_DIR)/libusb -lrt -lpthread -lm -lasound

metro.o: metro.wav
#	objcopy -B arm -I binary -O elf32-littlearm metro.wav metro.o
	objcopy -B i386 -I binary -O elf64-x86-64 metro.wav metro.o

cp33.bin: $(ARM_OBJS)
	$(GCC_ARM) -o cp33.elf \
		$(ARM_OBJS) \
		$(ARM_LFLAGS) \
		-Tstm32f4/main.ld
	$(OBJCOPY) -O binary cp33.elf cp33.bin

bootloader.bin: $(ARM_BOOTLOADER_OBJS)
	$(GCC_ARM) -o bootloader.elf $(ARM_BOOTLOADER_OBJS) $(ARM_BOOTLOADER_OBJS2) \
		-Tstm32f4/bootloader.ld $(ARM_LFLAGS)
	$(OBJCOPY) -O binary bootloader.elf bootloader.bin


recordcp33: recordcp33.c
	$(GCC) -o recordcp33 recordcp33.c $(USB_DIR)/libusb/.libs/libusb-1.0.a -I$(USB_DIR)/libusb -lrt -lpthread

passthrough: passthrough.c
	$(GCC) -o passthrough passthrough.c $(USB_DIR)/libusb/.libs/libusb-1.0.a -I$(USB_DIR)/libusb -lrt -lpthread -lasound

bootloader.o stm32f4/startup_boot.o stm32f4/system_stm32f4xx.o: 
	$(GCC_ARM) $(ARM_CFLAGS) -c $< -o $*.o




$(ARM_OBJS):
	$(GCC_ARM) $(ARM_CFLAGS) -c $< -o $*.o

clean:
	rm -f *.o stm32f4/*.o cp33.elf cp33.bin recordcp33 passthrough





stm32f4/startup_main.o:       stm32f4/startup_main.s
stm32f4/usb_hcd_int.o:	      stm32f4/usb_hcd_int.c
hardi2c.o:		  hardi2c.c
cp33.o:		  cp33.c  
fft.o:		  fft.c
idg3200.o:                idg3200.c
imu.o:                    imu.c
uart.o: 		  uart.c
usb.o: 		  usb.c
stm32f4/misc.o: 	      stm32f4/misc.c
stm32f4/stm32f4xx_rcc.o:      stm32f4/stm32f4xx_rcc.c
stm32f4/stm32f4xx_usart.o:    stm32f4/stm32f4xx_usart.c
stm32f4/stm32f4xx_gpio.o:     stm32f4/stm32f4xx_gpio.c
stm32f4/stm32f4xx_dac.o:      stm32f4/stm32f4xx_dac.c
stm32f4/stm32f4xx_dcmi.o:     stm32f4/stm32f4xx_dcmi.c
stm32f4/stm32f4xx_dma.o:      stm32f4/stm32f4xx_dma.c
stm32f4/stm32f4xx_i2c.o:      stm32f4/stm32f4xx_i2c.c
stm32f4/stm32f4xx_it.o:       stm32f4/stm32f4xx_it.c
stm32f4/stm32f4xx_iwdg.o:     stm32f4/stm32f4xx_iwdg.c
stm32f4/stm32f4xx_tim.o:      stm32f4/stm32f4xx_tim.c
stm32f4/stm32f4xx_adc.o:      stm32f4/stm32f4xx_adc.c
stm32f4/stm32f4xx_flash.o:    stm32f4/stm32f4xx_flash.c
stm32f4/stm32f4xx_spi.o:      stm32f4/stm32f4xx_spi.c
stm32f4/usb_bsp.o:	      stm32f4/usb_bsp.c
stm32f4/usb_core.o:	      stm32f4/usb_core.c
stm32f4/usb_dcd.o:	      stm32f4/usb_dcd.c
stm32f4/usb_hcd.o:	      stm32f4/usb_hcd.c
stm32f4/usbd_core.o:	      stm32f4/usbd_core.c
stm32f4/usbd_req.o:	      stm32f4/usbd_req.c
stm32f4/usbd_ioreq.o:	      stm32f4/usbd_ioreq.c
stm32f4/usb_dcd_int.o:	      stm32f4/usb_dcd_int.c
stm32f4/usbh_core.o:	      stm32f4/usbh_core.c
stm32f4/usbh_hcs.o:	      stm32f4/usbh_hcs.c
stm32f4/usbh_ioreq.o:	      stm32f4/usbh_ioreq.c
stm32f4/usbh_stdreq.o:	      stm32f4/usbh_stdreq.c
mpu9150.o:		  mpu9150.c
math.o: 		  math.c
linux.o:		  linux.c


bootloader.o:		  bootloader.c
stm32f4/startup_boot.o: stm32f4/startup_boot.s
stm32f4/system_stm32f4xx.o: stm32f4/system_stm32f4xx.c



mwindow.o: mwindow.C
spectrogram.o: spectrogram.C






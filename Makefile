CC      = i686-elf-gcc
AS      = nasm
LD      = i686-elf-ld

CFLAGS  = -ffreestanding -O2 -Wall -Wextra -std=c11
ASFLAGS = -f elf32
LDFLAGS = -T linker.ld -nostdlib

BUILD   = build
SRC     = src

OBJS = $(BUILD)/boot.o \
       $(BUILD)/kernel.o \
       $(BUILD)/pmm.o \
       $(BUILD)/idt.o \
       $(BUILD)/isr.o \
       $(BUILD)/keyboard.o

.PHONY: all clean run debug

all: $(BUILD)/kernel.iso

$(BUILD)/boot.o: $(SRC)/boot/boot.asm | $(BUILD)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/kernel.o: $(SRC)/kernel/kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD)/pmm.o: $(SRC)/memory/pmm.c | $(BUILD)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD)/idt.o: $(SRC)/cpu/idt.c | $(BUILD)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD)/isr.o: $(SRC)/cpu/isr.asm | $(BUILD)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/keyboard.o: $(SRC)/drivers/keyboard.c | $(BUILD)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BUILD)/kernel.elf: $(OBJS)
	$(CC) -T linker.ld -ffreestanding -O2 -nostdlib $^ -lgcc -o $@

$(BUILD)/kernel.iso: $(BUILD)/kernel.elf
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(BUILD)/kernel.elf $(BUILD)/iso/boot/
	printf 'menuentry "Liege" {\n    multiboot /boot/kernel.elf\n}\n' > $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD)/iso > /dev/null 2>&1

$(BUILD):
	mkdir -p $(BUILD)

run: $(BUILD)/kernel.iso
	qemu-system-i386 -cdrom $(BUILD)/kernel.iso -m 64M -no-reboot -no-shutdown

debug: $(BUILD)/kernel.iso
	qemu-system-i386 -cdrom $(BUILD)/kernel.iso -m 64M -s -S

clean:
	rm -rf $(BUILD)
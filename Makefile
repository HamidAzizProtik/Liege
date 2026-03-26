CC      = i686-elf-gcc
AS      = nasm
LD      = i686-elf-ld
CFLAGS  = -ffreestanding -O2 -Wall -Wextra -std=c11
ASFLAGS = -f elf32
LDFLAGS = -T linker.ld -nostdlib

BUILD   = build
SRC     = src

OBJS    = $(BUILD)/boot.o $(BUILD)/kernel.o

.PHONY: all clean run debug

all: $(BUILD)/kernel.iso

$(BUILD)/boot.o: $(SRC)/boot/boot.asm | $(BUILD)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/kernel.o: $(SRC)/kernel/kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel.elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -lgcc -o $@

$(BUILD)/kernel.iso: $(BUILD)/kernel.elf
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(BUILD)/kernel.elf $(BUILD)/iso/boot/
	echo 'menuentry "Liege" { multiboot /boot/kernel.elf }' > $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD)/iso 2>/dev/null

$(BUILD):
	mkdir -p $(BUILD)

run: $(BUILD)/kernel.iso
	qemu-system-i386 -cdrom $(BUILD)/kernel.iso -m 32M

debug: $(BUILD)/kernel.iso
	qemu-system-i386 -cdrom $(BUILD)/kernel.iso -m 32M \
	  -s -S &
	@echo "Listening on :1234 — attach GDB in VS Code"

clean:
	rm -rf $(BUILD)
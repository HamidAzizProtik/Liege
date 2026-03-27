# Liege OS

Liege is a minimal operating system built from scratch to understand how computers actually work at the deepest level.
No Linux underneath it. No Windows. No standard library. Just code talking directly to hardware.


## Why make an OS

I've been programming for a few years but always at a high level — Python with frameworks underneath them doing the hard work.
I wanted to understand what was underneath. Not just read about it, but build it. An OS felt like the hardest possible thing to start with, which is exactly what sounded right.

I didn't expect to enjoy low level programming as much as I do. I didn't expect to like C. I didn't expect that staring at a black screen and slowly understanding it would be satisfying. But it is.

## What Liege is

Liege is personal. It's built the way I think, designed around simplicity, and every line of it is something I try to understand or am working to understand.

The aesthetic goal is retrofuturistic — the feeling of a terminal from a future that never happened. Simple, clean, fast, with character.

The technical goal is educational — this is a project I'm building to learn, and the code reflects that.

## Current State

Liege is early in development. Right now it:

- Boots on x86 via GRUB multiboot
- Runs in 32 bit protected mode
- Has a working print system with cursor tracking and center alignment
- Displays a startup screen with a prompt

## Roadmap

The honest next steps in order:

- Keyboard input 
- A working shell 
- Memory management
- A simple filesystem

## Building

You need a cross compiler targeting i686-elf, NASM, QEMU, and GRUB tools.
Full environment setup documentation for WSL2 on Windows coming soon.

```bash
make        # build the kernel and ISO
make run    # boot in QEMU
make clean  # remove build artifacts
```

## Toolchain

- i686-elf-gcc — cross compiler targeting bare metal x86
- NASM — assembler for the boot code
- QEMU — emulator for development and testing
- GRUB — bootloader (will be replaced with a custom one)
- GDB — debugger attached to QEMU over a socket

## About

I'm Hamid, 15 years old, and I like making things that i would use myself and Liege is one of those things.

The hardest parts have been understanding assembly and knowing where I am in a system that gives you no feedback when something goes wrong. The most surprising part has been how much I enjoy it.

There's a lot more coming.
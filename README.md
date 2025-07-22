# ZoraVM

**A tiny virtual machine embedded with Zora — because we needed it, and nobody else was gonna do it.**

---

## What is ZoraVM?

ZoraVM is our minimal, purpose-built virtual machine designed specifically for Zora and its ecosystem. Think of it like a sandboxed environment for executing low-level code, scripts, or even entire apps — without bloating the system or reinventing QEMU.

If Zora had a napkin, this is what we scribbled on it:
```
run bytecode
support Kairos
simulate hardware (eventually)
don't suck
```

---

## Features (So Far)

- Simple instruction set (custom ISA)  
- Kairos bytecode support  
- Built-in debugging mode  
- Super low RAM/disk usage  
- Terminal-friendly (can run headless)  
- Features Meisei Virtual Silicon

---

## Status

> **Actively being worked on.**  
It boots, it runs stuff, it throws errors when it should — and sometimes when it shouldn’t. Still, it’s a VM. A *Zora* VM.

---

## Building

```sh
git clone https://github.com/Plutomaster28/zora_vm.git
cd zora_vm
mkdir build
cd build
cmake ..
ninja # run "ninja clean" when recompiling
```
> Works best on systems that don't ask too many questions.
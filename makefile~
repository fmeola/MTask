INCLUDE_DIR = include/
GCC_FLAGS = -Wall -fno-stack-protector -fno-builtin -m32 -I $(INCLUDE_DIR)
NASM_FLAGS = -f elf32 -I $(INCLUDE_DIR)

# kstart debe ser el primero pues debe linkearse al principio del ejecutable
MODULES = kstart libasm interrupts kernel gdt_idt irq string sprintf malloc \
			cons io timer queue math sem mutex monitor pipe msgqueue rand \
			filo sfilo xfilo keyboard printk getline shell split setkb camino \
			camino_ns atoi prodcons afilo divz

OBJECTS = $(MODULES:%=obj/%.o)
mtask: $(OBJECTS)
	cc -nostdlib -m32 -Wl,-Ttext-segment,0x100000,-Map,mtask.map -o mtask $(OBJECTS) 
	mkdir -p iso/boot/grub
	cp mtask iso/boot/
	cp boot/stage2_eltorito boot/menu.lst iso/boot/grub/
	mkisofs -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o mtask.iso iso

.PHONY: clean
clean:
	rm -f obj/* dep/* s/* mtask mtask.map mtask.iso
	rm -rf iso

obj/%.o: src/%.asm
	nasm $(NASM_FLAGS) $< -o $@

obj/%.o: src/%.c
	cc $(GCC_FLAGS) -c $< -o $@

s/%.s: src/%.c
	cc $(GCC_FLAGS) -S $< -o $@

# Generación automática de dependencias

dep/%.d: src/%.c
	cc $(GCC_FLAGS) $< -MM -MT 'obj/$*.o $@' > $@

dep/%.d: src/%.asm
	nasm $(NASM_FLAGS) $< -M -o 'obj/$*.o $@' > $@

DEPS = $(MODULES:%=dep/%.d)
-include $(DEPS)

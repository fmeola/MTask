bits 32

global inb
global outb

section .text

; unsigned inb(unsigned ioaddr);
inb:
	push edx
	mov dx, [esp + 8]
	xor eax, eax
	in al, dx
	pop edx
	ret

; void outb(unsigned ioaddr, unsigned data);
outb:
	push edx
	mov dx, [esp + 8]
	mov eax, [esp + 12]
	out dx, al
	pop edx
	ret


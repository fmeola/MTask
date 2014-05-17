MULTIBOOT_PAGE_ALIGN	equ 1<<0
MULTIBOOT_MEMORY_INFO	equ 1<<1        
MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM		equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

global _start
extern mt_main			; en kernel.c

bits 32

section .text

; Header multiboot, requerido por GRUB. Debe aparecer en los primeros 8 KB del
; ejecutable alineado a 4 bytes.

	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd MULTIBOOT_CHECKSUM

; Punto de entrada del kernel.
; Las interrupciones están deshabilitadas, el stack es indefinido. Los registros
; de segmentos están cargados pero no es seguro que haya una GDT válida, por lo
; tanto no pueden tocarse hasta no establecer una GDT propia. Lo primero que
; hacemos es poner un stack para poder ejecutar código en C. El resto lo
; inicializamos en C.

_start:
	mov esp, stack_end	; establecer stack
	push ebx			; puntero a estructura de informacion del bootloader
	push eax			; numero magico del bootloader
	call mt_main		; seguimos en C
	jmp $				; mt_main no deberia retornar

section .bss

align 4
stack:
	resb 0x4000			; 16 KB
stack_end:

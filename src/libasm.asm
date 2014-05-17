bits 32

; Mantener sincronizado con la definición de la estructura Task_t en mtask.h
Task_t.esp equ 20

BIT_TS equ 8

global mt_load_gdt
global mt_load_idt
global mt_context_switch
global mt_sti
global mt_cli
global mt_finit
global mt_fsave
global mt_frstor
global mt_stts
global mt_clts

extern mt_curr_task
extern mt_last_task

section .text

; void mt_load_gdt(const region_desc *gdt, unsigned cs, unsigned ds);
; Cargar la GDT e inicializar todos los registros de segmento
mt_load_gdt:
	mov eax, [esp + 4]
	lgdt [eax]
	mov ax, [esp + 12]			; nuevo segmento de datos
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov ax, [esp + 8]			; nuevo segmento de codigo
	mov [longptr.selector], ax
	mov dword [longptr.offset], end_load_gdt
	jmp	far [longptr]			; cargar nuevo segmento de codigo
end_load_gdt:
	ret

; void mt_load_idt(const region_desc *idt);
; Cargar la IDT
mt_load_idt:
	mov eax, [esp + 4]
	lidt [eax]
	ret

; void context_switch(void);
; Cambio de contexto fuera de una interrupción.
; Mantener el stack frame sincronizado con el manejador de interrupciones
; en interrupts.asm y con la estructura mt_regs_t en kernel.h
mt_context_switch:
	pushfd
	push eax
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp

	mov eax, [mt_last_task]
	mov [eax + Task_t.esp], esp
	mov eax, [mt_curr_task]
	mov esp, [eax + Task_t.esp]

	pop ebp
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	popfd
	ret

; void mt_sti()
; Habilitar interrupciones
mt_sti:
	sti
	ret

; void mt_cli()
; Deshabilitar interrupciones
mt_cli:
	cli
	ret

; void mt_finit(void);
; Resetear el coprocesador aritmético
mt_finit:
	finit
	ret

; void mt_fsave(void *buf)
; Guardar el contexto del coprocesador aritmético
mt_fsave:
	mov eax, [esp + 4]
	fsave [eax]
	ret

; void mt_frstor(void *buf)
; Restaurar el contexto del coprocesador aritmético
mt_frstor:
	fwait
	mov eax, [esp + 4]
	frstor [eax]
	ret

; void mt_stts(void);
mt_stts:
	mov eax, cr0
	or eax, BIT_TS
	mov cr0, eax
	ret

; void mt_clts(void);
mt_clts:
	clts
	ret

section .bss

longptr:
	.offset: resd 1
	.selector: resw 1

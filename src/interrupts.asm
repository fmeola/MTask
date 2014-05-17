bits 32

; Mantener sincronizado con la definición de la estructura Task_t en mtask.h.
Task_t.esp equ 20	

extern mt_curr_task
extern mt_select_task
extern mt_int_handler
extern mt_int_level

global mt_int_stubs

section .text

%macro define_stub 2
	align 16
	%ifidn %2, noerror
		push 0								; código de error dummy
	%endif
		push %1								; número de interrupción
		jmp common_handler					; ir al código común
%endmacro

; Mantener la alineacion y cantidad de stubs sincronizados con las definiciones
; de INT_STUB_SIZE y NUM_INTS en kernel.h

; typedef char int_stub[INT_STUB_SIZE];
; extern int_stub mt_int_stubs[NUM_INTS];

align 16
mt_int_stubs:								; array de stubs, cada uno ocupa 16 bytes

; Excepciones

define_stub  0, noerror
define_stub  1, noerror
define_stub  2, noerror
define_stub  3, noerror
define_stub  4, noerror
define_stub  5, noerror
define_stub  6, noerror
define_stub  7, noerror
define_stub  8, error
define_stub  9, noerror
define_stub 10, error
define_stub 11, error
define_stub 12, error
define_stub 13, error
define_stub 14, error
define_stub 15, noerror
define_stub 16, noerror
define_stub 17, error
define_stub 18, noerror
define_stub 19, noerror
define_stub 20, noerror
define_stub 21, noerror
define_stub 22, noerror
define_stub 23, noerror
define_stub 24, noerror
define_stub 25, noerror
define_stub 26, noerror
define_stub 27, noerror
define_stub 28, noerror
define_stub 29, noerror
define_stub 30, noerror
define_stub 31, noerror

; Interrupciones de hardware

define_stub 32, noerror
define_stub 33, noerror
define_stub 34, noerror
define_stub 35, noerror
define_stub 36, noerror
define_stub 37, noerror
define_stub 38, noerror
define_stub 39, noerror
define_stub 40, noerror
define_stub 41, noerror
define_stub 42, noerror
define_stub 43, noerror
define_stub 44, noerror
define_stub 45, noerror
define_stub 46, noerror
define_stub 47, noerror

; Código común para todos los manejadores
common_handler:
	; El stack no está como lo queremos. 
	; Bajamos todo a variables estáticas, están protegidas
	; por estar deshabilitadas las interrupciones
	cli										; ¿es necesario?
	pop dword [int_number]					; número de interrupción
	pop dword [except_error]				; código de error
	pop dword [int_eip]						; dirección de retorno
	add esp, 4								; descartar segmento de código
	pop dword [int_eflags]					; flags

	; Ahora armamos nuestro stack frame. Esto tiene que estar
	; sincronizado con la estructura mt_regs_t en kernel.h y con la función
	; mt_context_switch() en libasm.asm.
	; Empujar dirección de retorno y contexto.
	push dword [int_eip]
	push dword [int_eflags]
	push eax
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	mov [regs_ptr], esp		; puntero a los registros

	; Si se trata de una interrupción de primer nivel,
	; cambiamos a un stack interno. Para interrupciones anidadas
	; mantenemos el mismo stack.
	inc dword [mt_int_level]
	cmp dword [mt_int_level], 1
	jne stack_ok1
	mov eax, [mt_curr_task]
	mov [eax + Task_t.esp], esp				; guardar stack actual
	mov esp, int_stack_end					; cambiar a stack interno

stack_ok1:
	; Llamamos al manejador genérico en C, pasándole
	; como argumentos el número de interrupción, código de error,
	; que solamente será distinto de cero para las excepciones que
	; empujan un código de error al stack, y puntero a la estructura de
	; registros.
	push dword [regs_ptr]
	push dword [except_error]
	push dword [int_number]
	call mt_int_handler						; mt_int_handler(int_number, except_error, regs)
	cli										; por si el manejador habilito interrupciones
	add esp, 12

	; Si estamos retornando de una interrupción de primer nivel,
	; llamamos a mt_select_task() para que eventualmente cambie el
	; proceso actual, y cambiamos al stack de ese proceso. Si se trata
	; de una interrupción anidada seguimos con el mismo stack.
	dec dword [mt_int_level]
	jnz stack_ok2
	call mt_select_task	
	mov eax, [mt_curr_task]
	mov esp, [eax + Task_t.esp]				; cambiar al stack del proceso actual

stack_ok2:
	; Recuperar contexto y retornar
	pop ebp
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	popfd
	ret

section .bss

except_error: resd 1
int_number:resd 1
int_eip: resd 1
int_eflags: resd 1
regs_ptr: resd 1

align 4
int_stack: resb 0x4000
int_stack_end:



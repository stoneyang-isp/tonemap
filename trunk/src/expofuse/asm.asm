%define ARG1 [ebp+8]

%macro Entrar 0
	push ebp
	mov ebp,esp
	sub esp, 4
	push esi
	push edi
	push ebx
%endmacro

%macro Salir 0
	pop ebx
	pop edi
	pop esi
	add esp, 4
	pop ebp
	ret
%endmacro

section .data
	C255: dd 255		; constante 255
section .text

	; void asmInvertir(Matrix* A);
	global asmInvertir

asmInvertir:
	Entrar					; Convencion C
	mov esi,ARG1			; Cargo en es1 el 1er arg de la funcion
	mov eax,[esi]			; eax= A->rows
	mov ecx,[esi+4]		; ecx= A->cols
	mul ecx					; eax=eax*ecx (cant de celdas)
	mov ecx,eax				; ecx=cant celdas
	mov edi,[esi+8]		; edi= *(A->data)
	finit
	.loop:
		;fild dword [C255]
		fld1
		fld qword [edi]
		fsub
		fst qword [edi]
		ffree
		lea edi,[edi+8]
		loop .loop
	Salir

%define ARG1 [ebp+8]

%macro Entrar 0
  push rbp
  mov ebp,esp
  sub esp, 4
  push rsi
  push rdi
  push rbx
%endmacro

%macro Salir 0
  pop rbx
  pop rdi
  pop rsi
  add esp, 4
  pop rbp
  ret
%endmacro

section .data
  C255: dd 255      ; constante 255
section .text

  ; void asmInvertir(Matrix* A);
  global asmInvertir

asmInvertir:
  Entrar            ; Convencion C
  mov esi, ARG1     ; Cargo en es1 el 1er arg de la funcion
  mov eax, [esi]    ; eax = A->rows
  mov ecx, [esi+4]  ; ecx = A->cols
  mul ecx           ; eax = eax*ecx (cant de celdas)
  mov ecx, eax      ; ecx = cant celdas
  mov edi, [esi+8]  ; edi = *(A->data)
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

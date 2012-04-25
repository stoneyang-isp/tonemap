%define ARG1 rdi
%define ARG2 rsi
%define ARG3 r10 ; ARG3 en realidad es rdx, pero en Entrar se renombra
%define ARG4 rcx
%define ARG5 r8
%define ARG6 r9

%macro Entrar 0
  push rbp
  mov rbp, rsp
  sub rsp, 8
  push rbx
  push r12
  push r15
  mov r10, rdx  ; little hack, para guardar el ARG3 en r10 en vez de rdx (y que no se pise con mul por ej)
%endmacro

%macro Salir 0
  pop r15
  pop r12
  pop rbx
  add rsp, 8
  pop rbp
  ret
%endmacro

section .data

section .text

  ; Matrix* _asmSubstract(const double* A, const double* B, double* C, int rows, int cols);
  global _asmSubstract

  ; Matrix* _asmAddMatrix(const double* A, const double* B, double* C, int rows, int cols);
  global _asmAddMatrix

  ; Matrix* _asmAddEqualsMatrix(double* A, const double* B, int rows, int cols);
  global _asmAddEqualsMatrix

_asmSubstractFPU:
  Entrar            ; Convencion C
  mov rax, ARG4     ; rax = rows
  mul ARG5          ; rax = rows*cols
  mov rcx, rax      ; rcx = total celdas
  finit
  .loop:
    fld qword [ARG1]
    fld qword [ARG2]
    fsub
    fst qword [ARG3]
    ffree
    lea ARG1, [ARG1+8]
    lea ARG2, [ARG2+8]
    lea ARG3, [ARG3+8]
    loop .loop
  Salir

_asmSubstract:
  Entrar               ; Convencion C
  mov rax, ARG4        ; rax = rows
  mul ARG5             ; rax = rows*cols
  mov rcx, rax         ; rcx = total celdas
  shr rcx, 1           ; rcx = total celdas / 2
  jnc .loop            ; if (celdas impares) {
    movsd xmm1, [ARG1] ;   C[0] = A[0] - B[0]
    movsd xmm2, [ARG2]
    subsd xmm1, xmm2
    movsd [ARG3], xmm1
    lea ARG1, [ARG1+8]
    lea ARG2, [ARG2+8] ;   i++
    lea ARG3, [ARG3+8]   ; }
  .loop:               ; while (RCX) {
    movupd xmm1, [ARG1];   C[i] = A[i] - B[i]; C[i+1] = A[i+1] - B[i+1]
    movupd xmm2, [ARG2]
    subpd xmm1, xmm2
    movupd [ARG3], xmm1
    lea ARG1, [ARG1+16]
    lea ARG2, [ARG2+16]
    lea ARG3, [ARG3+16]  ;   i+=2; RCX-=2
    loop .loop         ; }
  Salir

_asmAddMatrix:
  Entrar               ; Convencion C
  mov rax, ARG4        ; rax = rows
  mul ARG5             ; rax = rows*cols
  mov rcx, rax         ; rcx = total celdas
  shr rcx, 1           ; rcx = total celdas / 2
  jnc .loop            ; if (celdas impares) {
    movsd xmm1, [ARG1] ;   C[0] = A[0] + B[0]
    movsd xmm2, [ARG2]
    addsd xmm1, xmm2
    movsd [ARG3], xmm1
    lea ARG1, [ARG1+8]
    lea ARG2, [ARG2+8] ;   i++
    lea ARG3, [ARG3+8]   ; }
  .loop:               ; while (RCX) {
    movupd xmm1, [ARG1];   C[i] = A[i] + B[i]; C[i+1] = A[i+1] + B[i+1]
    movupd xmm2, [ARG2]
    addpd xmm1, xmm2
    movupd [ARG3], xmm1
    lea ARG1, [ARG1+16]
    lea ARG2, [ARG2+16]
    lea ARG3, [ARG3+16]  ;   i+=2; RCX-=2
    loop .loop         ; }
  Salir

_asmAddEqualsMatrix:
  Entrar               ; Convencion C
  mov rax, ARG3        ; rax = rows
  mul ARG4             ; rax = rows*cols
  mov rcx, rax         ; rcx = total celdas
  shr rcx, 1           ; rcx = total celdas / 2
  jnc .loop            ; if (celdas impares) {
    movsd xmm1, [ARG1] ;   A[0] = A[0] + B[0]
    movsd xmm2, [ARG2]
    addsd xmm1, xmm2
    movsd [ARG1], xmm1
    lea ARG1, [ARG1+8] ;   i++
    lea ARG2, [ARG2+8] ; }
  .loop:               ; while (RCX) {
    movupd xmm1, [ARG1];   A[i] = A[i] + B[i]; A[i+1] = A[i+1] + B[i+1]
    movupd xmm2, [ARG2]
    addpd xmm1, xmm2
    movupd [ARG1], xmm1
    lea ARG1, [ARG1+16]
    lea ARG2, [ARG2+16];   i+=2; RCX-=2
    loop .loop         ; }
  Salir

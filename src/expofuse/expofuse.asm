%define ARG1 rdi
%define ARG2 rsi
%define ARG3 r10 ; ARG3 en realidad es rdx, pero en Entrar se renombra
%define ARG4 r11 ; ARG3 en realidad es rcx, pero en Entrar se renombra
%define ARG5 r8
%define ARG6 r9

%macro Entrar 0
  push rbp
  mov rbp, rsp
  sub rsp, 8
  push rbx
  push r12
  push r15
  mov r10, rdx  ; little hack, para guardar el ARG3 en r10 en vez de rdx
  mov r11, rcx  ; little hack, para guardar el ARG4 en r11 en vez de rcx
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
  C.299 dq 0.299
  C.587 dq 0.587
  C.114 dq 0.114
  C3    dq 3.0
  C.5   dq 0.5
  Cm.5  dq -0.5

section .text
  extern exp
  
  ; _asmTruncate(double* A, int rows, int cols);
  global _asmTruncate
  
  ; _asmDesaturate(double* J, const double* R, const double* G, const double* B, int rows, int cols);
  global _asmDesaturate
  
  ; _asmAbs(double* A, int rows, int cols);
  global _asmAbs
  
  ; _asmSaturation(double* A, const double* R, const double* G, const double* B, int rows, int cols);
  global _asmSaturation
  
  ; _asmExposeness(double* A, const double* R, const double* G, const double* B, int rows, int cols, double sigma2);
  global _asmExposeness

_asmTruncate:
  Entrar
  mov rax, ARG2
  mul ARG3
  mov rcx, rax
  finit
  .loop:
    fld qword [ARG1]
    fld1
    fcomip
    ja .listo1
    fld1
    fstp qword [ARG1]
    jmp .listo
    .listo1:
    fldz
    fcomip
    jb .listo
    fldz
    fstp qword [ARG1]
    .listo:
    fcomip
    lea ARG1, [ARG1+8]
    loop .loop
  Salir

_asmDesaturate:
  Entrar
  movsd xmm1, [C.299]
  movddup xmm1, xmm1
  movsd xmm2, [C.587]
  movddup xmm2, xmm2
  movsd xmm3, [C.114]
  movddup xmm3, xmm3

  mov r12, ARG4
  mov rax, ARG5
  mul ARG6
  mov rcx, rax
  shr rcx, 1
  jnc .loop
    movsd xmm4, [ARG2]
    movsd xmm5, [ARG3]
    movsd xmm6, [ARG4]
    mulsd xmm4, xmm1
    mulsd xmm5, xmm2
    mulsd xmm6, xmm3
    addsd xmm4, xmm5
    addsd xmm4, xmm6
    movsd [ARG1], xmm4
    lea ARG1, [ARG1+8]
    lea ARG2, [ARG2+8]
    lea ARG3, [ARG3+8]
    lea ARG4, [ARG4+8]
  .loop:
    movupd xmm4, [ARG2]
    movupd xmm5, [ARG3]
    movupd xmm6, [ARG4]
    mulpd xmm4, xmm1
    mulpd xmm5, xmm2
    mulpd xmm6, xmm3
    addpd xmm4, xmm5
    addpd xmm4, xmm6
    movupd [ARG1], xmm4
    lea ARG1, [ARG1+16]
    lea ARG2, [ARG2+16]
    lea ARG3, [ARG3+16]
    lea ARG4, [ARG4+16]
    loop .loop
  Salir

_asmAbs:
  Entrar
  mov rax, ARG2
  mul ARG3
  mov rcx, rax
  finit
  .loop:
    fld qword [ARG1]
    fabs
    fstp qword [ARG1]
    lea ARG1, [ARG1+8]
    loop .loop
  Salir

_asmSaturation:
  Entrar
  mov r12, ARG4
  mov rax, ARG5
  mul ARG6
  mov rcx, rax
  shr rcx, 1
  movsd xmm8, [C3]
  movddup xmm8, xmm8
  jnc .loop
    movsd xmm4, [ARG2]
    movsd xmm5, [ARG3]
    movsd xmm6, [ARG4]
    subpd xmm7, xmm7
    addsd xmm7, xmm4
    addsd xmm7, xmm5
    addsd xmm7, xmm6
    divsd xmm7, xmm8 ; mu = (r+g+b)/3
    subsd xmm4, xmm7 ; r-mu
    subsd xmm5, xmm7 ; g-mu
    subsd xmm6, xmm7 ; b-mu
    mulsd xmm4, xmm4 ; (r-mu)^2
    mulsd xmm5, xmm5 ; (g-mu)^2
    mulsd xmm6, xmm6 ; (b-mu)^2
    addsd xmm4, xmm5
    addsd xmm4, xmm6 ; (r-mu)^2 + (g-mu)^2 + (b-mu)^2
    divsd xmm4, xmm8 ; ( (r-mu)^2 + (g-mu)^2 + (b-mu)^2 ) / 3
    movsd [ARG1], xmm4
    lea ARG1, [ARG1+8]
    lea ARG2, [ARG2+8]
    lea ARG3, [ARG3+8]
    lea ARG4, [ARG4+8]
  .loop:
    movupd xmm4, [ARG2] ; r
    movupd xmm5, [ARG3] ; g
    movupd xmm6, [ARG4] ; b
    subpd xmm7, xmm7
    addpd xmm7, xmm4
    addpd xmm7, xmm5
    addpd xmm7, xmm6
    divpd xmm7, xmm8 ; mu = (r+g+b)/3
    subpd xmm4, xmm7 ; r-mu
    subpd xmm5, xmm7 ; g-mu
    subpd xmm6, xmm7 ; b-mu
    mulpd xmm4, xmm4 ; (r-mu)^2
    mulpd xmm5, xmm5 ; (g-mu)^2
    mulpd xmm6, xmm6 ; (b-mu)^2
    addpd xmm4, xmm5
    addpd xmm4, xmm6 ; (r-mu)^2 + (g-mu)^2 + (b-mu)^2
    divpd xmm4, xmm8 ; ( (r-mu)^2 + (g-mu)^2 + (b-mu)^2 ) / 3
    movupd [ARG1], xmm4
    lea ARG1, [ARG1+16]
    lea ARG2, [ARG2+16]
    lea ARG3, [ARG3+16]
    lea ARG4, [ARG4+16]
    loop .loop
  Salir

_asmExposeness:
  Entrar
  mov r12, ARG4
  mov rax, ARG5
  mul ARG6
  mov rcx, rax
  movsd xmm8, [C.5]
  movsd xmm9, xmm0
  movsd xmm10, [Cm.5]
  .loopExp:
    push rcx
    push ARG1
    push ARG2
    push ARG3
    push ARG4
    movsd xmm4, [ARG2] ; r
    movsd xmm5, [ARG3] ; g
    movsd xmm6, [ARG4] ; b
    subsd xmm4, xmm8   ; r-.5
    subsd xmm5, xmm8   ; g-.5
    subsd xmm6, xmm8   ; b-.5
    mulsd xmm4, xmm4   ; (r-.5)^2
    mulsd xmm5, xmm5   ; (g-.5)^2
    mulsd xmm6, xmm6   ; (b-.5)^2
    mulsd xmm4, xmm10  ; -.5*((r-.5)^2)
    mulsd xmm5, xmm10  ; -.5*((g-.5)^2)
    mulsd xmm6, xmm10  ; -.5*((b-.5)^2)
    divsd xmm4, xmm9   ; -.5*((r-.5)^2) / sigma2
    divsd xmm5, xmm9   ; -.5*((g-.5)^2) / sigma2
    divsd xmm6, xmm9   ; -.5*((b-.5)^2) / sigma2
    movsd xmm0, xmm4
    call exp
    movsd xmm4, xmm0
    movsd xmm0, xmm5
    call exp
    mulps xmm4, xmm0
    movsd xmm0, xmm6
    call exp
    mulsd xmm4, xmm0
    pop ARG4
    pop ARG3
    pop ARG2
    pop ARG1
    pop rcx
    movsd [ARG1], xmm4
    lea ARG1, [ARG1+8]
    lea ARG2, [ARG2+8]
    lea ARG3, [ARG3+8]
    lea ARG4, [ARG4+8]
    dec rcx
    jnz .loopExp ; loop .loop
  Salir

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

  ; _asmSubstract(const double* A, const double* B, double* C, int rows, int cols);
  global _asmSubstract

  ; _asmAddMatrix(const double* A, const double* B, double* C, int rows, int cols);
  global _asmAddMatrix

  ; _asmAddEqualsMatrix(double* A, const double* B, int rows, int cols);
  global _asmAddEqualsMatrix
  
  ; _asmDownsample(double* A, const double* B, int Arows, int Acols, int Bcols);
  global _asmDownsample

  ; _asmUpsample(const double* I, double* upsampled, int rows, int cols, int Urows, int Ucols);
  global _asmUpsample
  
  ; _asmConvolve1x5(const double* A, double* C, int rows, int cols, const double* kern);
  global _asmConvolve1x5
  global _asmConvolve1x5Symm

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

_asmDownsample:
  Entrar                  ; Convencion C
  mov r12, ARG4           ; r12 = cols
  mov r13, ARG5           ; r13 = Bcols
  shl r13, 4              ; r13 = 2*8*Bcols (2 filas)
  mov rcx, ARG3           ; rcx = rows
  mov rbx, ARG2           ; rbx = B
  .loopRows:
    push rcx
    mov rcx, r12          ; rcx = cols
    .loopCols:
      mov r14, [ARG2]
      mov [ARG1], r14     ; A[i][j] = B[2*i][2*j]
      lea ARG1, [ARG1+8]  ; ARG1 += 1 (ARG1 = A[i][j])
      lea ARG2, [ARG2+16] ; ARG2 += 2 (ARG2 = B[2*i][2*j])
      loop .loopCols
    pop rcx
    add rbx, r13          ; rbx += 2 filas (rbx = B[2*i])
    mov ARG2, rbx         ; ARG2 = B[2*i]
    loop .loopRows
  Salir

_asmUpsample:
  Entrar                  ; Convencion C
  mov r13, ARG4
  mov rax, ARG5           ; rax = Urows
  mul ARG6                ; rax = Urows * Ucols
  mov rcx, rax
  mov rdx, ARG2
  
  ; poner upsampled en 0
  ; (los bordes tmb, si es odd (!))
  subpd xmm1, xmm1
  shr rcx, 1
  jnc .loopZero
    movsd [rdx], xmm1
    lea rdx, [rdx+8]
  .loopZero:
    movupd [rdx], xmm1
    lea rdx, [rdx+16]
    loop .loopZero

  ; upsampled[2*i, 2*j] = I[i, j]
  shl ARG6, 4
  mov r14, 4
  cvtsi2sd xmm1, r14
  mov rcx, ARG3
  mov rdx, ARG2
  .loopRows:
    push rcx
    mov rcx, r13
    .loopCols:
      movsd xmm2, [ARG1]
      mulsd xmm2, xmm1
      movsd [rdx], xmm2
      lea rdx, [rdx+16]
      lea ARG1, [ARG1+8]
      loop .loopCols
    add ARG2, ARG6
    mov rdx, ARG2
    pop rcx
    loop .loopRows
  Salir

_asmConvolve1x5:
  Entrar               ; Convencion C
  mov rax, ARG3        ; rax = rows
  mul ARG4             ; rax = rows*cols
  mov rcx, rax
  sub rcx, 4           ; rcx = total celdas - 4
  lea ARG2, [ARG2+16]  ; (empezamos de la posicion 3 en C)
  movupd xmm1, [ARG5]
  lea ARG5, [ARG5+16]
  movupd xmm2, [ARG5]
  lea ARG5, [ARG5+16]
  movsd xmm3, [ARG5]   ; xmm1:xmm2:xmm3 = kern
  .loop:               ; while (rcx) {
    mov r12, ARG1
    movupd xmm4, [r12]
    lea r12, [r12+16]
    movupd xmm5, [r12]
    lea r12, [r12+16]
    movsd xmm6, [r12]  ;   xmm4:xmm5:xmm6 = A[i]...A[i+5]
    mulpd xmm4, xmm1
    mulpd xmm5, xmm2
    mulsd xmm6, xmm3   ;   xmm4:xmm5:xmm6 = A[i]...A[i+5] x KERN
    addsd xmm4, xmm6
    haddpd xmm4, xmm5
    haddpd xmm4, xmm4  ;   xmm4 = sum(A[i..i+5] x KERN):..
    movsd [ARG2], xmm4
    lea ARG1, [ARG1+8]
    lea ARG2, [ARG2+8] ;   i++
    loop .loop         ; rcx--; }
  Salir

_asmConvolve1x5Symm:
  Entrar                  ; Convencion C
  mov r12, ARG4           ; r12 = cols
  mov r13, r12
  shl r13, 3              ; r13 = 8*cols (1 fila)
  mov rcx, ARG3           ; rcx = rows
  mov rbx, ARG2           ; rbx = C
  mov rax, ARG1           ; rax = A
  movupd xmm1, [ARG5]
  lea ARG5, [ARG5+16]
  movupd xmm2, [ARG5]
  lea ARG5, [ARG5+16]
  movsd xmm3, [ARG5]      ; xmm1:xmm2:xmm3 = kern
  .loopRows:
    movlpd xmm4, [ARG1+16]
    movhpd xmm4, [ARG1+8]
    movupd xmm5, [ARG1]
    movsd xmm6, [ARG1+16] ; xmm4:xmm5:xmm6 = A[2],A[1],A[0],A[1],A[2]

    mulpd xmm4, xmm1
    mulpd xmm5, xmm2
    mulsd xmm6, xmm3
    addsd xmm4, xmm6
    haddpd xmm4, xmm5
    haddpd xmm4, xmm4  ;   xmm4 = sum(A[..] x KERN):..
    movsd [ARG2], xmm4

    movlpd xmm4, [ARG1+8]
    movhpd xmm4, [ARG1]
    movupd xmm5, [ARG1+8]
    movsd xmm6, [ARG1+24] ; xmm4:xmm5:xmm6 = A[1],A[0],A[1],A[2],A[3]
    
    mulpd xmm4, xmm1
    mulpd xmm5, xmm2
    mulsd xmm6, xmm3
    addsd xmm4, xmm6
    haddpd xmm4, xmm5
    haddpd xmm4, xmm4  ;   xmm4 = sum(A[..] x KERN):..
    movsd [ARG2+8], xmm4
    
    lea ARG2, [ARG2+16]
    push rcx
    mov rcx, r12          ; rcx = cols
    sub rcx, 4
    .loopCols:
      movupd xmm4, [ARG1]
      movupd xmm5, [ARG1+16]
      movsd xmm6, [ARG1+32] ; xmm4:xmm5:xmm6 = A[0],A[1],A[2],A[3],A[4]

      mulpd xmm4, xmm1
      mulpd xmm5, xmm2
      mulsd xmm6, xmm3
      addsd xmm4, xmm6
      haddpd xmm4, xmm5
      haddpd xmm4, xmm4  ;   xmm4 = sum(A[..] x KERN):..
      movsd [ARG2], xmm4
      
      lea ARG1, [ARG1+8]
      lea ARG2, [ARG2+8]
      loop .loopCols
    
    pop rcx
    
    movupd xmm4, [ARG1]
    movupd xmm5, [ARG1+16]
    movsd xmm6, [ARG1+16] ; xmm4:xmm5:xmm6 = A[-3],A[-2],A[-1],A[0],A[-1]

    mulpd xmm4, xmm1
    mulpd xmm5, xmm2
    mulsd xmm6, xmm3
    addsd xmm4, xmm6
    haddpd xmm4, xmm5
    haddpd xmm4, xmm4  ;   xmm4 = sum(A[..] x KERN):..
    movsd [ARG2], xmm4

    movupd xmm4, [ARG1+8]
    movlpd xmm5, [ARG1+24]
    movhpd xmm5, [ARG1+16]
    movsd xmm6, [ARG1+8] ; xmm4:xmm5:xmm6 = A[-2],A[-1],A[0],A[-1],A[-2]
    
    mulpd xmm4, xmm1
    mulpd xmm5, xmm2
    mulsd xmm6, xmm3
    addsd xmm4, xmm6
    haddpd xmm4, xmm5
    haddpd xmm4, xmm4  ;   xmm4 = sum(A[..] x KERN):..
    movsd [ARG2+8], xmm4
    
    add rbx, r13
    mov ARG2, rbx
    add rax, r13
    mov ARG1, rax
    dec rcx
    jnz .loopRows ;loop .loopRows
  Salir

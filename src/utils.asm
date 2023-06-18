; utils.asm
section .data
    stdout equ 1
    sys_write equ 1
    sys_flush equ 2

section .bss
    num    resd 1
    buffer resb 11
    
section .text
global putchar
global getchar
global putint
global getint

global _start
putchar:
    push rbp
    mov rbp, rsp
    push rdi
    mov rax, 1
    mov rdi, 1
    mov rsi, rsp
    mov rdx, 1
    syscall
    pop rax
    mov rsp, rbp
    pop rbp
    ret

getchar:
    push rbp
    mov rbp, rsp

    sub rsp, 8
    
    mov rax, 0
    mov rdi, 0
    mov rsi, rsp
    mov rdx, 1
    syscall

    pop rax ; résultat dans rax

    mov rsp, rbp
    pop rbp
    ret

getint:
    push rbp
    mov rbp, rsp

    push 0 ; is neg
    push 0 ; digit : rsp-16
    push 0 ; number: rsp-8

    _getint_loop_rchar:
        call getchar 
        mov dword [rbp-16], eax ; met caractère dans digit

        ; digit < 0 ou digit > 9 ?  
        cmp dword [rbp-16], '0'
        jl _getint_loop_break
        cmp dword [rbp-16], '9'
        jg _getint_loop_break

        ; on garde le chiffre ex: '1' (49) - '0' (48) = 1
        sub dword [rbp-16], '0' 
        mov eax, dword [rbp-8] ; charge number
        imul eax, 10
        add eax, dword [rbp-16]
        mov dword [rbp-8], eax

        jmp _getint_loop_rchar
    _getint_loop_break:
        ; signe déjà pris en compte ?
        cmp dword [rbp-24], 1
        je _getint_end ; sort
        cmp dword [rbp-16], '-'
        jne _getint_end ; sort
        mov dword [rbp-24], 1
        jmp _getint_loop_rchar ; continue lecture


    _getint_end:
    mov eax, dword [rbp-8]    

    cmp dword [rbp-24], 1
    jne _getint_no_neg
    neg eax

    _getint_no_neg:

    mov rsp, rbp
    pop rbp
    ret


putint:
    push rbp
    mov rbp, rsp
    push 0
    push 0

    mov rax, rdi
    mov esi, buffer
    add esi,9
    mov byte [esi], 0    ; Null en fin de de string
    
    test eax, eax        ; test si la veleur est negative
    jns not_neg          
    mov dword [rbp+16], 1   ; met le flag negatif à vrai
    neg rax
    jmp neg
    not_neg:
    mov byte [rbp+16], 0
    neg:

    mov ebx,10
    .next_digit:
    xor edx,edx         ; nettoie edx avant la division de edx:eax par ebx
    div ebx             ; eax /= 10
    add dl,'0'          ; Conversion du reste en ascii
    dec esi             ; stocker le caractère en ordre inverse
    mov [esi],dl
    test eax,eax            
    jnz .next_digit     ; répéter tant que eax != 0

    mov r8d, dword [rbp+16]
    cmp r8d, 1
    jne not_minus
        dec esi             ; store le signe '-' si negatif
        mov byte [esi],'-'
    not_minus:

    mov eax,esi
    push rcx
    ; affichage de la chaîne 
    mov rsi, rax
    pop rdx
    mov rax, 1
    mov rdi, 1
    syscall


 mov rax, sys_flush
    mov rdi, stdout
    syscall

    mov rsp, rbp
    pop rbp
    ret




_start:
    ; flush stdout
    mov rax, sys_flush
    mov rdi, stdout
    syscall
    
    call main
    push rax
    pop rdi
    mov rax, 60
    syscall

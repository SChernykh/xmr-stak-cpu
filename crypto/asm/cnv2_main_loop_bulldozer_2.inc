        push    r15
        push    r14
        push    r13
        push    r12
        mov     r12d, 524288
        push    rbp
        push    rdi
        push    rsi
        push    rbx
        sub     rsp, 88
        mov     rax, QWORD PTR 8[rcx]
        xor     rax, QWORD PTR 40[rcx]
        mov     r14, QWORD PTR [rcx]
        xor     r14, QWORD PTR 32[rcx]
        movaps  XMMWORD PTR 48[rsp], xmm6
        mov     QWORD PTR 168[rsp], rdx
        mov     rdi, QWORD PTR 224[rcx]
        movaps  XMMWORD PTR 64[rsp], xmm7
        mov     r9, r14
        mov     QWORD PTR [rsp], rax
        mov     rax, QWORD PTR 16[rcx]
        xor     rax, QWORD PTR 48[rcx]
        mov     QWORD PTR 8[rsp], rax
        mov     rax, QWORD PTR 24[rcx]
        xor     rax, QWORD PTR 56[rcx]
        movq    xmm5, QWORD PTR 8[rsp]
        mov     QWORD PTR 8[rsp], rax
        mov     rax, QWORD PTR 64[rcx]
        xor     rax, QWORD PTR 80[rcx]
        movhps  xmm5, QWORD PTR 8[rsp]
        mov     QWORD PTR 8[rsp], rax
        mov     rax, QWORD PTR 72[rcx]
        xor     rax, QWORD PTR 88[rcx]
        movq    xmm4, QWORD PTR 8[rsp]
        mov     QWORD PTR 8[rsp], rax
        mov     r11, QWORD PTR 104[rcx]
        mov     rax, r14
        movhps  xmm4, QWORD PTR 8[rsp]
        and     eax, 2097136
        mov     QWORD PTR 40[rsp], r12
        movq    xmm0, QWORD PTR 96[rcx]
        mov     r13, r11
        jmp     L43

	ALIGN 16
L44:
        movdqa  xmm5, xmm2
L43:
        lea     r8, [rdi+rax]
        mov     QWORD PTR 8[rsp], r9
        mov     rcx, rax
        movq    xmm1, QWORD PTR 8[rsp]
        mov     rdx, rax
        movdqa  xmm2, XMMWORD PTR [r8]
        movhps  xmm1, QWORD PTR [rsp]
        xor     rcx, 16
        xor     rdx, 32
        xor     rax, 48
        aesenc  xmm2, xmm1
        add     rcx, rdi
        add     rdx, rdi
        add     rax, rdi
        mov     rsi, r13
        movdqa  xmm6, XMMWORD PTR [rcx]
        sal     rsi, 32
        movq    r11, xmm2
        movq    r10, xmm2
        movdqa  xmm3, XMMWORD PTR [rdx]
        paddq   xmm6, xmm5
        and     r10d, 2097136
        movdqa  xmm7, XMMWORD PTR [rax]
        paddq   xmm3, xmm1
        lea     rbp, [rdi+r10]
        paddq   xmm7, xmm4
        movaps  XMMWORD PTR [rcx], xmm7
        lea     ecx, [r11+r13*2]
        movaps  XMMWORD PTR [rdx], xmm6
        or      ecx, -2147483647
        xor     edx, edx
        movaps  XMMWORD PTR [rax], xmm3
        movq    rax, xmm0
        movdqa  xmm0, xmm2
        movdqa  xmm3, xmm2
        xor     rsi, rax
        psrldq  xmm0, 8
        movq    rax, xmm0
        pxor    xmm3, xmm5
        div     rcx
        movaps  XMMWORD PTR [r8], xmm3
        xor     rsi, QWORD PTR 0[rbp]
        mov     r15, QWORD PTR 8[rbp]
        sal     rdx, 32
        mov     eax, eax
        lea     rbx, [rdx+rax]
        lea     r8, [r11+rbx]
        mov     QWORD PTR 8[rsp], rbx
        mov     rbx, QWORD PTR 168[rsp]
        mov     rax, r8
        shr     rax, 53
        lea     rdx, [rbx+rax*8]
        mov     rax, r8
        mov  rbx, 8589934591
        shr     rax, 25
        mov     ecx, DWORD PTR 4[rdx]
        and     rbx, QWORD PTR [rdx]
        and     eax, 268435455
        mov   edx, ecx
        sub     rax, 134217728
        shr     ecx, 1
        and     ecx, 4194303
        sal     rbx, 28
        imul    rcx, rax
        shr     edx, 23
        imul    rax, rax
        add     rcx, rbx
        imul    rax, rdx
        shr     rax, 28
        sub     rcx, rax
        movq    rax, xmm2
        mov     r13, rcx
        shr     rcx, 30
        shr     r13, 29
        mov     rbx, r13
        and     ebx, 1
        mul     rsi
        mov     r12, rdx
        mov     r11, rax
        mov     rax, rdx
        mov     QWORD PTR 16[rsp], r12
        mov     QWORD PTR 32[rsp], r11
        mov     rdx, r10
        movq    xmm3, QWORD PTR 16[rsp]
        xor     rdx, 16
        mov     QWORD PTR 24[rsp], r12
        movhps  xmm3, QWORD PTR 32[rsp]
        mov     r12, r10
        add     rdx, rdi
        xor     r12, 32
        xor     r10, 48
        pxor    xmm3, XMMWORD PTR [rdx]
        add     r12, rdi
        add     r10, rdi
        xor     rax, QWORD PTR [r12]
        mov     QWORD PTR 16[rsp], r11
        movdqa  xmm0, XMMWORD PTR [r12]
        paddq   xmm4, XMMWORD PTR [r10]
        mov     r11, QWORD PTR 16[rsp]
        xor     r11, QWORD PTR 8[r12]
        movaps  XMMWORD PTR [rdx], xmm4
        paddq   xmm1, xmm0
        mov     rdx, r13
        add     r9, rax
        paddq   xmm3, xmm5
        sal     rdx, 32
        movdqa  xmm4, xmm5
        add     rdx, rbx
        add     rbx, rcx
        imul    rcx, rbx
        add rdx, rcx
        movaps  XMMWORD PTR [r12], xmm3
        movaps  XMMWORD PTR [r10], xmm1
        cmp     r8, rdx
        sbb     r13, 0
        mov     rax, QWORD PTR [rsp]
        mov     QWORD PTR 0[rbp], r9
        xor     r9, rsi
        movq    xmm0, QWORD PTR 8[rsp]
        add     rax, r11
        xor     r15, rax
        mov     QWORD PTR 8[rbp], rax
        mov     rax, r9
        and     eax, 2097136
        sub     QWORD PTR 40[rsp], 1
        mov     QWORD PTR [rsp], r15
        jne     L44

        movaps  xmm6, XMMWORD PTR 48[rsp]
        movaps  xmm7, XMMWORD PTR 64[rsp]
        add     rsp, 88
        pop     rbx
        pop     rsi
        pop     rdi
        pop     rbp
        pop     r12
        pop     r13
        pop     r14
        pop     r15

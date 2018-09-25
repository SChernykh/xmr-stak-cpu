tweak1_2$1$ = 0
ctx0$ = 96
idx1$2$ = 104
tv650 = 112
l0$1$ = 120

	mov	QWORD PTR [rsp+8], rcx
	push	rbx
	push	rbp
	push	rsi
	push	rdi
	push	r12
	push	r13
	push	r14
	push	r15
	sub	rsp, 24
	mov	rax, QWORD PTR [rcx+48]
	xor	rax, QWORD PTR [rcx+16]
	mov	rdx, QWORD PTR [rcx+56]
	xor	rdx, QWORD PTR [rcx+24]
	mov	r8, QWORD PTR [rcx+32]
	xor	r8, QWORD PTR [rcx]
	movq	xmm4, rax
	mov	rax, QWORD PTR [rcx+256]
	mov	r13, QWORD PTR [rcx+40]
	movq	xmm0, rdx
	xor	r13, QWORD PTR [rcx+8]
	mov	rdx, r8
	mov	rdi, QWORD PTR [rcx+224]
	and	edx, 2097136
	mov	rax, QWORD PTR [rax+35]
	xor	rax, QWORD PTR [rcx+192]
	mov	QWORD PTR tweak1_2$1$[rsp], rax
	mov	QWORD PTR l0$1$[rsp], rdi
	punpcklqdq xmm4, xmm0
	mov	QWORD PTR idx1$2$[rsp], rdx
	mov	QWORD PTR tv650[rsp], 524288

	ALIGN 64
cnv1_mainloop_soft_aes_sandybridge:
	mov	r12, QWORD PTR [rcx+272]
	mov	esi, DWORD PTR [rdx+rdi]
	mov	r10d, DWORD PTR [rdx+rdi+4]
	mov	ebp, DWORD PTR [rdx+rdi+12]
	mov	r14d, DWORD PTR [rdx+rdi+8]
	mov	rdx, QWORD PTR idx1$2$[rsp]
	movzx	ecx, sil
	shr	esi, 8
	mov	r15d, DWORD PTR [r12+rcx*4]
	movzx	ecx, r10b
	shr	r10d, 8
	mov	edi, DWORD PTR [r12+rcx*4]
	movzx	ecx, r14b
	shr	r14d, 8
	mov	ebx, DWORD PTR [r12+rcx*4]
	movzx	ecx, bpl
	shr	ebp, 8
	mov	r9d, DWORD PTR [r12+rcx*4]
	movzx	ecx, r10b
	shr	r10d, 8
	xor	r15d, DWORD PTR [r12+rcx*4+1024]
	movzx	ecx, r14b
	shr	r14d, 8
	mov	eax, r14d
	shr	eax, 8
	xor	edi, DWORD PTR [r12+rcx*4+1024]
	add	eax, 256
	movzx	ecx, bpl
	shr	ebp, 8
	xor	ebx, DWORD PTR [r12+rcx*4+1024]
	movzx	ecx, sil
	shr	esi, 8
	xor	r9d, DWORD PTR [r12+rcx*4+1024]
	add	r12, 2048
	movzx	ecx, r10b
	shr	r10d, 8
	add	r10d, 256
	mov	r11d, DWORD PTR [r12+rax*4]
	xor	r11d, DWORD PTR [r12+rcx*4]
	xor	r11d, r9d
	movzx	ecx, sil
	mov	r10d, DWORD PTR [r12+r10*4]
	shr	esi, 8
	add	esi, 256
	xor	r10d, DWORD PTR [r12+rcx*4]
	movzx	ecx, bpl
	xor	r10d, ebx
	shr	ebp, 8
	add	ebp, 256
	movd	xmm1, r11d
	mov	r9d, DWORD PTR [r12+rcx*4]
	xor	r9d, DWORD PTR [r12+rsi*4]
	mov	eax, DWORD PTR [r12+rbp*4]
	xor	r9d, edi
	mov	rdi, QWORD PTR l0$1$[rsp]
	movzx	ecx, r14b
	movd	xmm0, r10d
	movd	xmm2, r9d
	punpckldq xmm2, xmm1
	movq	xmm1, r8
	xor	eax, DWORD PTR [r12+rcx*4]
	xor	eax, r15d
	movd	xmm3, eax
	punpckldq xmm3, xmm0
	movq	xmm0, r13
	punpcklqdq xmm1, xmm0
	punpckldq xmm3, xmm2
	pxor	xmm3, xmm1
	movdqa	xmm0, xmm3
	pxor	xmm0, xmm4
	movdqu	XMMWORD PTR [rdx+rdi], xmm0
	psrldq	xmm0, 11
	movq	rax, xmm0
	movzx	ecx, al
	mov	rax, QWORD PTR ctx0$[rsp]
	mov	rax, QWORD PTR [rax+264]
	movzx	ecx, BYTE PTR [rcx+rax]
	mov	BYTE PTR [rdi+rdx+11], cl
	movq	rcx, xmm3
	mov	rax, rcx
	and	eax, 2097136
	mov	rbx, QWORD PTR [rax+rdi]
	lea	r9, QWORD PTR [rax+rdi]
	mov	r11, QWORD PTR [rdi+rax+8]
	lea	r10, QWORD PTR [rdi+rax]
	mov	rax, rbx
	movdqa	xmm4, xmm3
	mul	rcx
	mov	rcx, QWORD PTR ctx0$[rsp]
	add	r8, rdx
	add	r13, rax
	mov	rax, QWORD PTR tweak1_2$1$[rsp]
	xor	rax, r13
	mov	QWORD PTR [r9], r8
	xor	r8, rbx
	mov	QWORD PTR [r10+8], rax
	mov	rdx, r8
	xor	r13, r11
	and	edx, 2097136
	sub	QWORD PTR tv650[rsp], 1
	mov	QWORD PTR idx1$2$[rsp], rdx
	jne	cnv1_mainloop_soft_aes_sandybridge

	add	rsp, 24
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	rdi
	pop	rsi
	pop	rbp
	pop	rbx

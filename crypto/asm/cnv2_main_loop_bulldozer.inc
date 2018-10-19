	mov	QWORD PTR [rsp+8], rbx
	mov	QWORD PTR [rsp+24], rbp
	mov	QWORD PTR [rsp+32], rsi
	mov	rsi, rdx
	push	rdi
	push	r12
	push	r13
	push	r14
	push	r15
	sub	rsp, 48
	mov	rax, QWORD PTR [rcx+48]
	mov	r9, rcx
	xor	rax, QWORD PTR [rcx+16]
	mov	r13d, 524288
	mov	r15, QWORD PTR [rcx+32]
	xor	r15, QWORD PTR [rcx]
	mov	r12, QWORD PTR [rcx+40]
	mov	r10, r15
	mov	r8, QWORD PTR [rcx+56]
	xor	r8, QWORD PTR [rcx+24]
	xor	r12, QWORD PTR [rcx+8]
	mov	r14, QWORD PTR [rcx+224]
	mov	rcx, QWORD PTR [rcx+88]
	xor	rcx, QWORD PTR [r9+72]
	movq	xmm0, r8
	movaps	XMMWORD PTR [rsp+32], xmm6
	movq	xmm6, rax
	mov	rax, QWORD PTR [r9+80]
	xor	rax, QWORD PTR [r9+64]
	movaps	XMMWORD PTR [rsp+16], xmm7
	and	r10d, 2097136
	punpcklqdq xmm6, xmm0
	movq	xmm0, rcx
	mov	rcx, QWORD PTR [r9+104]
	movaps	XMMWORD PTR [rsp], xmm8
	movq	xmm7, rax
	movq	xmm8, QWORD PTR [r9+96]
	mov	r9d, -2147483647
	punpcklqdq xmm7, xmm0

	ALIGN 64
main_loop_bulldozer:
	movdqu	xmm5, XMMWORD PTR [r10+r14]
	lea	r8, QWORD PTR [r10+r14]
	mov	rdx, r10
	mov	rax, r10
	xor	rax, 32
	xor	rdx, 16
	xor	r10, 48
	movq	xmm0, r12
	movq	rdi, xmm8
	movq	xmm4, r15
	punpcklqdq xmm4, xmm0
	movdqu	xmm1, XMMWORD PTR [rax+r14]
	movdqu	xmm2, XMMWORD PTR [rdx+r14]
	paddq	xmm1, xmm4
	movdqu	xmm0, XMMWORD PTR [r10+r14]
	paddq	xmm2, xmm6
	paddq	xmm0, xmm7
	movdqu	XMMWORD PTR [rdx+r14], xmm0
	xor	edx, edx
	movdqu	XMMWORD PTR [rax+r14], xmm2
	mov	rax, rcx
	shl	rax, 32
	xor	rdi, rax
	movdqu	XMMWORD PTR [r10+r14], xmm1
	aesenc	xmm5, xmm4
	movq	r11, xmm5
	mov	r10, r11
	and	r11d, 2097136
	movdqa	xmm0, xmm5
	pxor	xmm0, xmm6
	movdqu	XMMWORD PTR [r8], xmm0
	lea	rbx, QWORD PTR [r11+r14]
	xor	rdi, QWORD PTR [r11+r14]
	lea	r8, QWORD PTR [rcx+rcx]
	mov	rbp, QWORD PTR [rbx+8]
	add	r8d, r10d
	movdqa	xmm0, xmm5
	or	r8, r9
	psrldq	xmm0, 8
	movq	rax, xmm0
	div	r8
	mov	eax, eax
	shl	rdx, 32
	add	rdx, rax
	lea	r9, QWORD PTR [rdx+r10]
	movq	xmm8, rdx
	mov	rax, r9
	mov	rdx, r9
	shr	rax, 53
	shr	rdx, 25
	and	edx, 268435455
	sub	rdx, 134217728
	mov	r8, QWORD PTR [rsi+rax*8]
	mov	rax, r8
	mov	rcx, r8
	shr	rax, 55
	shr	rcx, 33
	imul	rax, rdx
	and	ecx, 4194303
	shl	r8, 28
	imul	rcx, rdx
	imul	rax, rdx
	shr	rax, 28
	sub	rcx, rax
	add	rcx, r8
	shr	rcx, 29
	mov	r8d, ecx
	mov	edx, ecx
	shr	edx, 1
	and	r8d, 1
	lea	rax, QWORD PTR [r8+rdx]
	imul	rax, rdx
	mov	edx, ecx
	shl	rdx, 32
	add rdx, r8
	add rdx, rax
	cmp	r9, rdx
	sbb	rcx, 0
	mov	r8d, r11d
	xor	r8, 32
	mov	r9d, r11d
	mov	rax, rdi
	mul	r10
	movdqu	xmm1, XMMWORD PTR [r8+r14]
	paddq	xmm1, xmm4
	xor	r9, 16
	xor	r11, 48
	movdqu	xmm2, XMMWORD PTR [r9+r14]
	movq	xmm0, rax
	xor	rax, QWORD PTR [r14+r8+8]
	movq	xmm3, rdx
	xor	rdx, QWORD PTR [r8+r14]
	add	r12, rax
	punpcklqdq xmm3, xmm0
	add	r15, rdx
	movdqu	xmm0, XMMWORD PTR [r11+r14]
	pxor	xmm2, xmm3
	paddq	xmm0, xmm7
	paddq	xmm2, xmm6
	movdqu	XMMWORD PTR [r9+r14], xmm0
	movdqa	xmm7, xmm6
	movdqu	XMMWORD PTR [r8+r14], xmm2
	movdqa	xmm6, xmm5
	movdqu	XMMWORD PTR [r11+r14], xmm1
	mov	r9d, -2147483647
	mov	QWORD PTR [rbx], r15
	xor	r15, rdi
	mov	r10, r15
	mov	QWORD PTR [rbx+8], r12
	and	r10d, 2097136
	xor	r12, rbp
	sub	r13, 1
	jne	main_loop_bulldozer

	mov	rbx, QWORD PTR [rsp+96]
	mov	rbp, QWORD PTR [rsp+112]
	mov	rsi, QWORD PTR [rsp+120]
	movaps	xmm6, XMMWORD PTR [rsp+32]
	movaps	xmm7, XMMWORD PTR [rsp+16]
	movaps	xmm8, XMMWORD PTR [rsp]
	add	rsp, 48
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	rdi

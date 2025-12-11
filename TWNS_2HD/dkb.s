	.text
	.data

.global _DKB_read

/*


void DKB_read(char *dst) 

*/
//.extern _datasegment
.extern _ax
.extern _bx
.extern _cx
.extern _dx

	.align 4


/*

マトリクス入力 

void DYB_read(char *dst) 

*/
	.align 2
_DKB_read:
	push	%ebx
	push	%edi

	leal	_RIN_dataBuf2,%ebx
	movl	$16,%ecx
	call	_RIN_nativetoreal

	movw	%cx,%di
	shrl	$16,%ecx
	movw	%cx,(_RIN_ds)
//	movb	$0xa,(_RIN_ah)

	push	%ecx
	push	%edx

	mov	(_ax),%eax	/* Device No, */
	mov	%ax,(_RIN_ax)
	mov	(_bx),%ebx	/* sector num */
	mov	(_cx),%ecx	/* cylinder */
	mov	(_dx),%edx	/* head & sector no */
	mov	%dx,(_RIN_dx)

	movb	$5,(_RIN_ah)

	movw	$0x93,(_RIN_rint)

	movw	$0x2511,%ax	/* リアルモード割り込み */
	leal	_RIN_rintpar,%edx

//	cli
//	hlt
	int	$0x21

	mov	%ecx,(_cx)

	pop	%ecx
	pop	%edx

	cmp	$0,%ah
	jnz	error
//	jmp	error

	mov	%edx,%eax

	movl	12(%esp),%edx

	push	%eax
	push	%ecx

	lea	_RIN_dataBuf2,%eax
	mov	$1024,%ecx
loop:
	movb	(%eax),%bl
	movb	%bl,(%edx)
	inc	%eax
	inc	%edx
	dec	%ecx
	jnz	loop

	xorl	%eax,%eax

	pop	%ecx
	pop	%edx

error:

	pop	%edi
	pop	%ebx

	ret


_RIN_dataBuf2:
		.space	8192

_dkb_destAdr:	.long	0


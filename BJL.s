//
// low-level routines for centronix
// written by 42Bastian Schick
//
//
// globals (defined in main)
// _gBase : port-address
// gWait : wait-loop counter (no longer used, see below)
//
// functions:
//
// void _InitPortNormal();
// void _InitPortHyper();
// int _SendWord(short w);   /* 1 => timeout */
// int _SendWordLE(short w); /* _Send little-endian word */ 
// int _SendLong(long l);    /* 1 => timeout */
// void _SendLongHyper(long l);
//
// All functions expect little-endian words/longs !!!

//#define 4BIT

//
// _SendLongHyper(ulong l)
//


.globl _SendLongHyper
_SendLongHyper:
  pushl %ebx

  movl 8(%esp),%ebx
  call _SendByte
  shrl $8,%ebx
  call _SendByte
  shrl $8,%ebx
  call _SendByte
  shrl $8,%ebx
  call _SendByte

  popl %ebx
  ret

//
// _SendByte
// in : bl = data
//
  .align 2
_SendByte:
  movl _g4BitMode,%eax
  testl %eax,%eax
  jz _SendNibble

  pushl %ebx
  shlb $4,%bl
  call _SendNibble
  popl %ebx


_SendNibble: 
  movl _gBase,%edx
  incw %dx
L94a:
  inb %dx, %al
  testb %al,%al
  jns L94a
  
  movl _gBase,%edx
  movb %bl,%al
  outb %al, %dx

  incw %dx
  incw %dx
  movb $1,%al
  outb %al, %dx

  decw %dx
L100a:
  inb %dx, %al
  testb %al,%al
  js L100a
  
  movl _gBase,%edx
  incw %dx
  incw %dx

  xorb %al,%al
  outb %al, %dx

//using out is better,AFAIK a port-access is 1us even on a PII-450 !

  movl _gWait,%edx
wait0:
  outb %al,$0x80
  decl %edx
  jnz wait0
  ret

//
// _Initport
//
  .align 2
.globl _InitPortHyper

_InitPortHyper:
  movl _gBase,%edx
  xorb %al,%al
  
  outb %al,%dx
  incw %dx
  outb %al,%dx
  incw %dx
  outb %al,%dx

  ret

//
// _InitPortNormal
//
.align 2
.globl _InitPortNormal
_InitPortNormal:
  movl _gBase,%edx
  xorb %al,%al
  
  outb %al,%dx
  incw %dx
  outb %al,%dx
  incw %dx
  incb %al
  outb %al,%dx

  outb %al,%dx
  ret

//
// _SendWordLE(ushort w)
//
	.align 2
  .globl _SendWordLE
_SendWordLE:
	pushl %ebx
	pushl %esi
	movl 12(%esp),%ebx
	rorw $8,%bx
	jmp cont
//
// _SendWord(ushort w)
//
  .align 2
.globl _SendWord
_SendWord:
  pushl %ebx
  pushl %esi
  movl 12(%esp),%ebx
cont:	
  movl _gBase,%edx
  rolw $3,%bx
  movw $8,%cx

  movw $20000,%si
  incw %dx
l00:
  decw %si
  jz l99
  inb %dx,%al
  testb %al,%al
  js l00

l1:
  inb %dx,%al
  testb %al,%al
  js l1
	  
  decw %dx
  movb $6,%al
  andb %bl,%al
  outb %al,%dx

	
  incw %dx
  incw %dx
  xorb %al,%al
  outb %al,%dx   
	
  rolw $2,%bx

  decw %dx
l2:
  inb %dx,%al
  testb %al,%al
  jns l2

  incw %dx
  movb $1,%al
  outb %al,%dx

  outb %al,$0x80

  decw %dx

  decw %cx
  jnz l1

  popl %esi
  popl %ebx
  xorl %eax,%eax
  ret

l99:
  popl %esi
  popl %ebx
  xorl %eax,%eax
  decl %eax
  ret


//
// _SendLong(ulong l)
//
  .align 2
.globl _SendLong
_SendLong:
  pushl %ebx
  pushl %esi
  movl _gBase,%edx
  movl 12(%esp),%ebx
  rorl $2,%ebx
  movw $10,%cx     // 10*3Bit + 2Bit

  movw $20000,%si  // time-out
  incw %dx
l00a:
  decw %si
  jz l99a
  inb %dx,%al
  testb %al,%al
  js l00a


l1a:
  inb %dx,%al
  testb %al,%al
  js l1a
  
  decw %dx
  movb $7,%al
  andb %bl,%al
  outb %al,%dx

  incw %dx
  incw %dx
  xorb %al,%al
  outb %al,%dx   

  roll $3,%ebx

  decw %dx
l2a:
  inb %dx,%al
  testb %al,%al
  jns l2a

  incw %dx
  movb $1,%al
  outb %al,%dx

  decw %dx

  decw %cx
  jnz l1a

l3a:
  inb %dx,%al
  testb %al,%al
  js l3a
  
  decw %dx
  movb $6,%al
  andb %bl,%al
  orb $0xf0,%al
  outb %al,%dx

  incw %dx
  incw %dx
  xorb %al,%al
  outb %al,%dx   

  decw %dx
l4a:
  inb %dx,%al
  testb %al,%al
  jns l4a

  incw %dx
  movb $1,%al
  outb %al,%dx

  popl %esi
  popl %ebx
  xorl %eax,%eax
  ret

l99a:
  popl %esi
  popl %ebx
  xorl %eax,%eax
  decl %eax
  ret



tests/cim/test_my_add42.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <_start>:
   0:	e8 a9 00 00 00       	call   ae <alloc_and_touch_mem>
   5:	48 bf 00 00 00 00 00 	movabs $0x0,%rdi
   c:	00 00 00 
   f:	be 13 00 00 00       	mov    $0x13,%esi
  14:	e8 f7 00 00 00       	call   110 <print_string>
  19:	bf 90 78 56 09       	mov    $0x9567890,%edi
  1e:	be 01 89 67 09       	mov    $0x9678901,%esi
  23:	ba 01 89 67 09       	mov    $0x9678901,%edx
  28:	66 0f 38 41 ca       	phminposuw %xmm2,%xmm1
  2d:	90                   	nop
  2e:	90                   	nop
  2f:	90                   	nop
  30:	66 0f 38 42          	data16 (bad)
  34:	90                   	nop
  35:	90                   	nop
  36:	90                   	nop
  37:	90                   	nop
  38:	90                   	nop
  39:	90                   	nop
  3a:	90                   	nop
  3b:	90                   	nop
  3c:	90                   	nop
  3d:	90                   	nop
  3e:	90                   	nop
  3f:	90                   	nop
  40:	66 0f 38 43          	data16 (bad)
  44:	90                   	nop
  45:	90                   	nop
  46:	90                   	nop
  47:	90                   	nop
  48:	90                   	nop
  49:	90                   	nop
  4a:	90                   	nop
  4b:	90                   	nop
  4c:	90                   	nop
  4d:	90                   	nop
  4e:	90                   	nop
  4f:	90                   	nop
  50:	66 0f 38 44          	data16 (bad)
  54:	90                   	nop
  55:	90                   	nop
  56:	90                   	nop
  57:	90                   	nop
  58:	90                   	nop
  59:	90                   	nop
  5a:	90                   	nop
  5b:	90                   	nop
  5c:	90                   	nop
  5d:	90                   	nop
  5e:	90                   	nop
  5f:	90                   	nop
  60:	66 0f 38 45          	data16 (bad)
  64:	90                   	nop
  65:	90                   	nop
  66:	90                   	nop
  67:	90                   	nop
  68:	90                   	nop
  69:	90                   	nop
  6a:	90                   	nop
  6b:	90                   	nop
  6c:	90                   	nop
  6d:	90                   	nop
  6e:	90                   	nop
  6f:	90                   	nop
  70:	66 0f 38 46          	data16 (bad)
  74:	90                   	nop
  75:	90                   	nop
  76:	90                   	nop
  77:	90                   	nop
  78:	90                   	nop
  79:	90                   	nop
  7a:	90                   	nop
  7b:	90                   	nop
  7c:	90                   	nop
  7d:	90                   	nop
  7e:	90                   	nop
  7f:	90                   	nop
  80:	66 0f 38 47          	data16 (bad)
  84:	90                   	nop
  85:	90                   	nop
  86:	90                   	nop
  87:	90                   	nop
  88:	90                   	nop
  89:	90                   	nop
  8a:	90                   	nop
  8b:	90                   	nop
  8c:	90                   	nop
  8d:	90                   	nop
  8e:	90                   	nop
  8f:	90                   	nop
  90:	48 bf 00 00 00 00 00 	movabs $0x0,%rdi
  97:	00 00 00 
  9a:	be 13 00 00 00       	mov    $0x13,%esi
  9f:	e8 6c 00 00 00       	call   110 <print_string>
  a4:	b8 3c 00 00 00       	mov    $0x3c,%eax
  a9:	48 31 ff             	xor    %rdi,%rdi
  ac:	0f 05                	syscall

00000000000000ae <alloc_and_touch_mem>:
  ae:	bf 00 70 56 09       	mov    $0x9567000,%edi
  b3:	e8 33 00 00 00       	call   eb <mmap_alloc>
  b8:	48 89 c7             	mov    %rax,%rdi
  bb:	48 c7 07 2a 00 00 00 	movq   $0x2a,(%rdi)
  c2:	bf 00 80 67 09       	mov    $0x9678000,%edi
  c7:	e8 1f 00 00 00       	call   eb <mmap_alloc>
  cc:	48 89 c7             	mov    %rax,%rdi
  cf:	48 c7 07 2a 00 00 00 	movq   $0x2a,(%rdi)
  d6:	bf 00 90 78 09       	mov    $0x9789000,%edi
  db:	e8 0b 00 00 00       	call   eb <mmap_alloc>
  e0:	48 89 c7             	mov    %rax,%rdi
  e3:	48 c7 07 2a 00 00 00 	movq   $0x2a,(%rdi)
  ea:	c3                   	ret

00000000000000eb <mmap_alloc>:
  eb:	b8 09 00 00 00       	mov    $0x9,%eax
  f0:	be 00 10 00 00       	mov    $0x1000,%esi
  f5:	ba 03 00 00 00       	mov    $0x3,%edx
  fa:	41 ba 32 00 00 00    	mov    $0x32,%r10d
 100:	49 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%r8
 107:	41 b9 00 00 00 00    	mov    $0x0,%r9d
 10d:	0f 05                	syscall
 10f:	c3                   	ret

0000000000000110 <print_string>:
 110:	b8 01 00 00 00       	mov    $0x1,%eax
 115:	48 89 f2             	mov    %rsi,%rdx
 118:	48 89 fe             	mov    %rdi,%rsi
 11b:	bf 01 00 00 00       	mov    $0x1,%edi
 120:	0f 05                	syscall
 122:	c3                   	ret

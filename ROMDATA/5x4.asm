#include "instructions.asm"
init:
	mov a, 0x0
	mov b, 0x0
	sto b, 0x51
	mov m, end

main:
	add a, 0x5
	ld  b, 0x51
	add b, 0x1
	cmp b, 0xC
	sto b, 0x51
	mov f, ptr[b]
	and b, 0x4
	jnz b 
	jmp main

end:
	out a, 0
	hlt

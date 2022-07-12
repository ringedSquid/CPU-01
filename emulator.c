#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROMADD 0x40
#define RAMADD 0x100
#define SP 0x50

typedef unsigned char byte;

typedef struct {
	//A = 0
	//B = 1
	//M = 2
	//F = 3
	byte A, B, M, F, PC;
	byte RAM[RAMADD], ROM[ROMADD];
} CPU;

typedef struct {
	byte X : 4;
	byte Z : 3;
	byte Y : 1;
} INSTRUCTION;

//reset all registers, RAM, ROM, SP
void reset(CPU *cpu) {
	cpu->A = 0;
	cpu->B = 0;
	cpu->M = 0;
	cpu->PC = 0;
	cpu->F = 0;
	memset(cpu->RAM, 0, RAMADD);
	memset(cpu->ROM, 0, ROMADD);
	cpu->RAM[SP] = 0x4f;
	return;
}

//load data from specified file into ROM
void loadROM(CPU *cpu, FILE *fp) {
	long int size;
	//byte buff, i=0;
	//while ((fscanf(fp, "%hhu", &buff) != EOF) && (i < ROMADD)) {
	//		cpu->ROM[i] = buff;
	//		printf("%hhu : 0x%X\n", i, buff);	
	//		i++;
	//}
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp)/sizeof(byte);
	rewind(fp);
	if (fread(cpu->ROM, sizeof(byte), size, fp) != size) {
		fprintf(stderr, "Error reading file!\n");
		exit(1);
	}
	printf("File sucsessfully loaded into ROM!\n");
	return;
}

//fetch instruction identifier (first 4 bits), then increment PC
//if PC > ROM size, it loops back to 0
INSTRUCTION fetchInstruction(CPU *cpu) {
	byte currentByte = (cpu->PC > ROMADD-1) ? cpu->RAM[cpu->PC] : cpu->ROM[cpu->PC];
	INSTRUCTION buff = {
		currentByte>>4, //X 
		(currentByte<<4)>>5, //Z
		(currentByte<<7)>>7 //Y
	};
	cpu->PC++;
	//cpu->PC %= ROMADD;
	return buff; 
}

//translates number to corresponding register
byte *selReg(CPU *cpu, byte reg) {
	switch(reg) {
		case 0:
			return &cpu->A;
		case 1:
			return &cpu->B;
		case 2:
			return &cpu->M;
		case 3:
			return &cpu->F;
		}
}

//tReg = value or tReg = reg
void moveByte(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	byte s = cpu->ROM[cpu->PC];
	if (mode == 0) {
		*p_tReg = s;
	}
	else {
		byte *p_sReg = selReg(cpu, s);
		*p_tReg = *p_sReg;
	}
	cpu->PC++;
	return;
}

//load byte from RAM into register
void loadByte(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	byte s = cpu->ROM[cpu->PC];
	if (mode == 0) {
		*p_tReg = (s > ROMADD-1) ? cpu->RAM[s] : cpu->ROM[s];
		cpu->PC++;
	}
	else {
		*p_tReg = (cpu->M > ROMADD-1) ? cpu->RAM[cpu->M] : cpu ->ROM[cpu->M];
	}
	return;
}

//store byte from register into RAM
void storeByte(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	byte s = cpu->ROM[cpu->PC];
	if (mode == 0) {
		cpu->RAM[s] = (s > ROMADD-1) ? *p_tReg : 0;
		cpu->PC++;
	}
	else {
		cpu->RAM[cpu->M] = (cpu->M > ROMADD-1) ? *p_tReg : 0;
	}
	return;
}

//Addition operation on selected register
void add(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	byte s = cpu->ROM[cpu->PC];
	if (mode == 0) {
		if ((*p_tReg + s) > 255) {
			cpu->F |= 0x2;
		}
		*p_tReg += s;
	}
	else {
		byte *p_sReg = selReg(cpu, s);
		if ((*p_tReg + *p_sReg) > 255) {
			cpu->F |= 0x2;
		}
		*p_tReg += s;
		*p_tReg += *p_sReg;
	}
	cpu->PC++;
	return;
}

//Addition operation on selected register with carry
void addCarry(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	byte s = cpu->ROM[cpu->PC];
	byte carry = (cpu->F<<2)>>3;
	if (mode == 0) {
		if ((*p_tReg + s + carry) < 256) {
			cpu->F &= ~0x2;
		}
		*p_tReg += s + carry;
	}
	else {
		byte *p_sReg = selReg(cpu, s);
		if ((*p_tReg + *p_sReg + carry) <= 255) {
			cpu->F &= ~0x2;
		}
		*p_tReg += *p_sReg + carry;
	}
	cpu->PC++;
	return;
}
		
//Subtraction operation on selected register with borrow
void subBorrow(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	byte s = cpu->ROM[cpu->PC];
	byte borrow = (cpu->F<<3)>>3;
	if (mode == 0) {
		if ((*p_tReg - s - borrow) >= 0) {
			cpu->F &= ~0x1;
		}
		else {
			cpu->F |= 0x1;
		}
		*p_tReg -= s - borrow;
	}
	else {
		byte *p_sReg = selReg(cpu, s);
		if ((*p_tReg - *p_sReg - borrow) >= 0) {
			cpu->F &= ~0x1;
		}
		else {
			cpu->F |= 0x1;
		}
		*p_tReg -= *p_sReg - borrow;
	}
	cpu->PC++;
	return;
}

//Bitwise AND operation
void and(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	byte s = cpu->ROM[cpu->PC];
	if (mode == 0) {
		*p_tReg &= s;
	}
	else {
		byte *p_sReg = selReg(cpu, s);
		*p_tReg &= *p_sReg;
	}
	cpu->PC++;
	return;
}

//Bitwise OR operation
void or(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	byte s = cpu->ROM[cpu->PC];
	if (mode == 0) {
		*p_tReg |= s;
	}
	else {
		byte *p_sReg = selReg(cpu, s);
		*p_tReg |= *p_sReg;
	}
	cpu->PC++;
	return;
}

//Bitwise NOR operation
void nor(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	byte s = cpu->ROM[cpu->PC];
	if (mode == 0) {
		*p_tReg = ~(*p_tReg | s);
	}
	else {
		byte *p_sReg = selReg(cpu, s);
		*p_tReg = ~(*p_tReg |*p_sReg);
	}
	cpu->PC++;
	return;
}

//Compare two values
void cmp(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	byte s = cpu->ROM[cpu->PC];
	if (mode == 0) {
		if (*p_tReg < s) { 
			cpu->F |= 0x8; 
			cpu->F &= ~0x4;
		}
		else if (*p_tReg == s) {
			cpu->F |= 0x4;
			cpu->F &= ~0x8;
		}
	}
	else {
		byte *p_sReg = selReg(cpu, s);
		if (*p_tReg < *p_sReg) {
			cpu->F |= 0x8; 
			cpu->F &= ~0x4;
		}
		else if (*p_tReg == *p_sReg) {
			cpu->F |= 0x4; 
			cpu->F &= ~0x8;
		}
		
	}
	cpu->PC++;
	return;
}

//Push to stack 
void push(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	if (mode == 0) {
		byte s = cpu->ROM[cpu->PC];
		cpu->RAM[SP]--;
		cpu->RAM[SP] = s;
		cpu->PC++;
	}
	else {
		cpu->RAM[SP]--;
	       	cpu->RAM[SP] = *p_tReg;
	}
	return;
}

//Pop from stack
void pop(CPU *cpu, byte tReg) {
	byte *p_tReg = selReg(cpu, tReg);
	*p_tReg = cpu->RAM[SP];
	cpu->RAM[SP]++;
	return;
}

//Jump if not zero
void jnz(CPU *cpu, byte tReg, byte mode) {
	if (mode == 0) {
		byte s = cpu->ROM[cpu->PC];
		if (s != 0) { cpu->PC = cpu->M; }
	}
	else {
		byte *p_tReg = selReg(cpu, tReg);
		if (*p_tReg != 0) { cpu->PC = cpu->M; }
	}
	return;
}

//Jump to specified address
void jmp(CPU *cpu, byte mode) {
	if (mode == 0) {
		cpu->PC = cpu->ROM[cpu->PC];
	}
	else {
		cpu->PC = cpu->M;
	}
	return;
}

//TEMPORARY: print contents of register 0 = decimal, 1 = hex
void out(CPU *cpu, byte tReg, byte mode) {
	byte *p_tReg = selReg(cpu, tReg);
	if (mode == 0) {
		printf("%d\n", *p_tReg);
	}
	else {
		printf("%X\n", p_tReg);
	}
	return;
}

int main(int arc, char*argv[]) {
	FILE *fp;
	if ((fp = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Invalid file name supplied!\n");
		exit(1);
	};
	CPU mainCPU;
	CPU *p_mainCPU = &mainCPU;
	INSTRUCTION fetchedInstruction;
	reset(p_mainCPU);
	loadROM(p_mainCPU, fp);
	fclose(fp);
	//for (int i=0; i<ROMADD; i++) {
	//	printf("%d : %X\n", i, mainCPU.ROM[i]);
	//}
	
	//Main loop
	for (byte i=0; i<1;) {
		fetchedInstruction = fetchInstruction(p_mainCPU);
		//mainCPU.PC %= RAMADD;
		switch(fetchedInstruction.X) {
			case 0x0:
				moveByte(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0x1:
				loadByte(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0x2:
				storeByte(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0x3:
				add(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0x4:
				addCarry(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0x5:
				subBorrow(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0x6:
				and(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0x7:
				or(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0x8:
				nor(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0x9:
				cmp(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0xA:
				push(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0xB:
				pop(p_mainCPU, fetchedInstruction.Y);
				break;
			case 0xC:
				jnz(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
			case 0xD:
				jmp(p_mainCPU, fetchedInstruction.Y);
				break;
			case 0xE:
				printf("HALT\n");
				i++;
				break;
			//Temporarily output operation
			case 0xF:
				out(p_mainCPU, fetchedInstruction.Z, fetchedInstruction.Y);
				break;
		}
	}
	return 0;
}

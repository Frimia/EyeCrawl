// -----------------------------------------------------------
//  ______                 ____
// |        \  /   ____   /    \ | ____   ____,           |
// |____     \/  /_____/ |       |/      /    | \       / |
// |         /  |        |       |      |     |  \  |  /  |
// |______  /    \_____/  \____/ |       \____/\  \/ \/   |
//
// -----------------------------------------------------------
// 
#include "stdafx.h"
#include "eyecrawl.h"
#include <TlHelp32.h>
#include <Psapi.h>
//
// Things to keep in mind with updating:
// 
// Any updates to the destination side, MAKE SURE
// it applies to the source side as well.
// Unless, the second operand requires a different
// algorithm, and VISE VERSA.
// 

// Private namespace definitions
// Plus it won't allow this in header
namespace EyeCrawl {
	HANDLE proc;
	UINT_PTR base_address;
	UINT_PTR base_size;
	const char* _r8[8]	= {"al","cl","dl","bl","ah","ch","dh","bh"};
	const char* _r16[8] = {"ax","bx","cx","dx","sp","bp","si","di"};
	const char* _r32[8] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
	const char* _r64[8] = {"rax","rbx","rcx","rdx","rsp","rbp","rsi","rdi"}; // COMING SOON
	const char* _xmm[8] = {"xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7"};
	const char* _conds[16]	= {"o","no","b","nb","e","ne","na","a","s","ns","p","np","l","nl","lng","g"};
	const char c_ref1[16]	= {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	const int c_ref2[16]	= { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15};

	namespace util {
		const long buffersize = (64 * 64 * 64);
	}
}

void EyeCrawl::set(HANDLE handle) {
	proc = handle;
	HMODULE hMods[1024];
	unsigned long cbNeeded,mCurrent=0;
	if (EnumProcessModulesEx(handle,hMods,1024,&cbNeeded,LIST_MODULES_ALL)){
		for (int i=0; i<(cbNeeded/sizeof(HMODULE)); i++){
			MODULEINFO info;
			char szModPath[MAX_PATH];
			if (GetModuleFileNameExA(handle,hMods[i],szModPath,sizeof(szModPath)) && K32GetModuleInformation(handle,hMods[i],&info,cbNeeded)){
				if (mCurrent++==0){
					base_address = reinterpret_cast<UINT_PTR>(info.lpBaseOfDll);
					base_size = info.SizeOfImage;
				}
			}
		}
	}
}

void EyeCrawl::set() {
	proc = GetCurrentProcess();
	HMODULE hMods[1024];
	unsigned long cbNeeded,mCurrent=0;
	if (EnumProcessModulesEx(proc,hMods,1024,&cbNeeded,LIST_MODULES_ALL)){
		for (int i=0; i<(cbNeeded/sizeof(HMODULE)); i++){
			MODULEINFO info;
			char szModPath[MAX_PATH];
			if (GetModuleFileNameA(hMods[i],szModPath,sizeof(szModPath)) && K32GetModuleInformation(proc,hMods[i],&info,cbNeeded)){
				if (mCurrent++==0){
					base_address = reinterpret_cast<UINT_PTR>(info.lpBaseOfDll);
					base_size = info.SizeOfImage;
				}
			}
		}
	}
}

HANDLE EyeCrawl::get() {
	return proc;
}

UINT_PTR EyeCrawl::base_start() {
	return base_address;
}

UINT_PTR EyeCrawl::base_end() {
	return base_address + base_size;
}

UINT_PTR EyeCrawl::aslr(UINT_PTR addr) {
	return (addr - 0x400000 + base_address);
}

UINT_PTR EyeCrawl::non_aslr(UINT_PTR addr) {
	return (addr + 0x400000 - base_address);
}

UCHAR EyeCrawl::readb(UINT_PTR addr) {
	if (DLL_MODE) return *(UCHAR*)addr;
	UCHAR buffer = 0;
	ReadProcessMemory(proc,reinterpret_cast<void*>(addr),&buffer,1,0);
	return buffer;
}

char EyeCrawl::readc(UINT_PTR addr) {
	if (DLL_MODE) return *(char*)addr;
	char buffer = 0;
	ReadProcessMemory(proc,reinterpret_cast<void*>(addr),&buffer,1,0);
	return buffer;
}

USHORT EyeCrawl::readus(UINT_PTR addr) {
	if (DLL_MODE) return *(USHORT*)addr;
	USHORT buffer = 0;
	ReadProcessMemory(proc,reinterpret_cast<void*>(addr),&buffer,2,0);
	return buffer;
}

short EyeCrawl::reads(UINT_PTR addr) {
	if (DLL_MODE) return *(short*)addr;
	short buffer = 0;
	ReadProcessMemory(proc,reinterpret_cast<void*>(addr),&buffer,2,0);
	return buffer;
}

UINT_PTR EyeCrawl::readui(UINT_PTR addr) {
	if (DLL_MODE) return *(UINT_PTR*)addr;
	UINT_PTR buffer = 0;
	ReadProcessMemory(proc,reinterpret_cast<void*>(addr),&buffer,4,0);
	return buffer;
}

int EyeCrawl::readi(UINT_PTR addr) {
	if (DLL_MODE) return *(int*)addr;
	int buffer = 0;
	ReadProcessMemory(proc,reinterpret_cast<void*>(addr),&buffer,4,0);
	return buffer;
}

float EyeCrawl::readf(UINT_PTR addr) {
	if (DLL_MODE) return *(float*)addr;
	float buffer = 0;
	ReadProcessMemory(proc,reinterpret_cast<void*>(addr),&buffer,4,0);
	return buffer;
}

double EyeCrawl::readd(UINT_PTR addr) {
	if (DLL_MODE) return *(double*)addr;
	double buffer = 0;
	ReadProcessMemory(proc,reinterpret_cast<void*>(addr),&buffer,8,0);
	return buffer;
}

std::string EyeCrawl::sreads(UINT_PTR addr, int count) {
	std::string read = "";
	if (DLL_MODE) {
		char* str = *(char**)(addr);
		read += str;
	} else {
		int reader = 0, size = count;
		if (count == 0) size = STR_READ_MAX;
		while (reader < size) {
			char c = readc(addr+reader++);
			if (c >= 0x20 && c <= 0x7E)
				read += c;
			else
				break;
		}
	}
	return read;
}

bool EyeCrawl::write(UINT_PTR addr, UCHAR v){
	if (DLL_MODE) *(UCHAR*)(addr) = v;
	return WriteProcessMemory(proc,reinterpret_cast<void*>(addr),&v,1,0);
}

bool EyeCrawl::write(UINT_PTR addr, char v){
	if (DLL_MODE) *(char*)(addr) = v;
	return WriteProcessMemory(proc,reinterpret_cast<void*>(addr),&v,1,0);
}

bool EyeCrawl::write(UINT_PTR addr, USHORT v){
	if (DLL_MODE) *(USHORT*)(addr) = v;
	return WriteProcessMemory(proc,reinterpret_cast<void*>(addr),&v,2,0);
}

bool EyeCrawl::write(UINT_PTR addr, short v){
	if (DLL_MODE) *(short*)(addr) = v;
	return WriteProcessMemory(proc,reinterpret_cast<void*>(addr),&v,2,0);
}

bool EyeCrawl::write(UINT_PTR addr, UINT_PTR v){
	if (DLL_MODE) *(UINT_PTR*)(addr) = v;
	return WriteProcessMemory(proc,reinterpret_cast<void*>(addr),&v,4,0);
}

bool EyeCrawl::write(UINT_PTR addr, int v){
	if (DLL_MODE) *(int*)(addr) = v;
	return WriteProcessMemory(proc,reinterpret_cast<void*>(addr),&v,4,0);
}

bool EyeCrawl::write(UINT_PTR addr, float v){
	if (DLL_MODE) *(float*)(addr) = v;
	return WriteProcessMemory(proc,reinterpret_cast<void*>(addr),&v,4,0);
}

bool EyeCrawl::write(UINT_PTR addr, double v){
	if (DLL_MODE) *(double*)(addr) = v;
	return WriteProcessMemory(proc,reinterpret_cast<void*>(addr),&v,8,0);
}

bool EyeCrawl::write(UINT_PTR addr, std::string v){
	if (DLL_MODE) *(const char**)(addr) = v.c_str();
	return WriteProcessMemory(proc,reinterpret_cast<void*>(addr),v.c_str(),v.length(),0);
}


EyeCrawl::instruction* EyeCrawl::disassemble(UINT_PTR addr) {
	instruction* x = new instruction();
	x->address = addr;
	if (proc == INVALID_HANDLE_VALUE) return x; // return 0 size instruction (this is bad!!)

	UCHAR b=readb(addr), single_reg=0;

	// For single-byte instructions,
	// we will just use math to calculate
	// them here.
	// 
	// inc, dec, push, pop 
	if (b>=0x40 && b<0x48){
		strcpy_s(x->opcode, "inc");
		strcpy_s(x->data, "inc ");
		strcat_s(x->data, _r32[b-0x40]);
		x->r32[0] = (b-0x40);
		x->size++;
		set_d(x,r32);
		return x;
	} else if (b>=0x48 && b<0x50){
		strcpy_s(x->opcode, "dec");
		strcpy_s(x->data, "dec ");
		strcat_s(x->data, _r32[b-0x48]);
		x->r32[0] = (b-0x48);
		x->size++;
		set_d(x,r32);
		return x;
	} else if (b>=0x50 && b<0x58){
		strcpy_s(x->opcode, "push");
		strcpy_s(x->data, "push ");
		strcat_s(x->data, _r32[b-0x50]);
		x->r32[0] = (b-0x50);
		x->size++;
		set_d(x,r32);
		return x;
	} else if (b>=0x58 && b<0x60){
		strcpy_s(x->opcode, "pop");
		strcpy_s(x->data, "pop ");
		strcat_s(x->data, _r32[b-0x58]);
		x->r32[0] = (b-0x58);
		x->size++;
		set_d(x,r32);
		return x;
	}

	// identify the opcode
	switch (b){
		case 0x03:
			strcpy_s(x->opcode,"add");
			set_d(x,r16_32);
			set_s(x,r_m16_32);
			break;

		case 0x0F:{
			x->size++;
			UCHAR b=readb(addr+x->size);
			if (b>=0x40 && b<0x50){
				strcpy_s(x->opcode, "cmov");
				strcat_s(x->opcode, _conds[b-0x40]);
				set_d(x,r16_32);
				set_s(x,r_m16_32);
				break;
			} else if (b>=0x80 && b<0x90){
				strcpy_s(x->opcode, "j");
				strcat_s(x->opcode, _conds[b-0x80]);
				set_d(x,rel32);
				break;
			} else {
				switch (b) {
					case 0x1F:
						strcpy_s(x->opcode, "nop");
						strcpy_s(x->mark1, "dword ptr");
						set_d(x,r_m16_32);
					break;

					case 0x28:
						strcpy_s(x->opcode, "movaps");
						if (readb(addr+x->size+1) >= 0xC0){
							set_d(x,rxmm);
							set_s(x,rxmm);
						} else {
							strcpy_s(x->mark2, "qword ptr");
							set_d(x,rxmm);
							set_s(x,r_m16_32);
						}
					break;

					case 0x29:
						strcpy_s(x->opcode, "movaps");
						if (readb(addr+x->size+1) >= 0xC0){
							set_d(x,rxmm);
							set_s(x,rxmm);
						} else {
							strcpy_s(x->mark1, "qword ptr");
							set_d(x,r_m16_32);
							set_s(x,rxmm);
						}
					break;
					case 0x2E:
						strcpy_s(x->opcode, "ucomiss");
						if (readb(addr+x->size+1) >= 0xC0){
							set_d(x,rxmm);
							set_s(x,rxmm);
						} else {
							strcpy_s(x->mark2, "qword ptr");
							set_d(x,rxmm);
							set_s(x,r_m16_32);
						}
					break;

					case 0x57:
						strcpy_s(x->opcode, "xorps");
						if (readb(addr+x->size+1) >= 0xC0){
							set_d(x,rxmm);
							set_s(x,rxmm);
						} else {
							strcpy_s(x->mark2, "qword ptr");
							set_d(x,rxmm);
							set_s(x,r_m16_32);
						}
					break;
					
					case 0xB6:
						strcpy_s(x->opcode, "movzx");
						if (readb(addr+x->size+1) >= 0xC0){
							set_d(x,r16_32);
							set_s(x,r8);
						} else {
							strcpy_s(x->mark2, "byte ptr");
							set_d(x,r16_32);
							set_s(x,r_m16_32);
						}
					break;

					case 0xBA:
						// "0F BA '/7' ib"
						// 0x20-0x30-0x40, 0x60-0x70-0x80, 0xA0-0xB0-0xC0
						// has two modes (bt/btr)
						if ((readb(addr+x->size+1) / 0x20) % 2!=0){
							UCHAR m=((readb(addr+x->size+1) % 0x20) / 8);
							switch (m) {
								case 0:
									strcpy_s(x->opcode, "bt");
									set_d(x,r_m16_32);
									set_s(x,imm8);
									break;
								case 1:
									strcpy_s(x->opcode, "bts");
									set_d(x,r_m16_32);
									set_s(x,imm8);
									break;
								case 2:
									strcpy_s(x->opcode, "btr");
									set_d(x,r_m16_32);
									set_s(x,imm8);
									break;
								case 3:
									strcpy_s(x->opcode, "btc");
									set_d(x,r_m16_32);
									set_s(x,imm8);
								break;
							}
						}
					break;
					case 0xBB:
						strcpy_s(x->opcode, "btc");
						set_d(x,r_m16_32);	// BTC r/m16, r16
						set_s(x,r16_32);	// BTC r/m32, r32
					break;
				}
			}
		} break;

		case 0x23:
			strcpy_s(x->opcode,"and"); // performs AND operation
			set_d(x,r16_32);	// AND r16, r/m16 (set destination)
			set_s(x,r_m16_32);	// AND r32, r/m32 (set source)
			break;
		case 0x24: // single-register instruction, just do imm8 value
			strcpy_s(x->opcode,"and al");
			single_reg=true;
			set_s(x,imm8);
			break;
		case 0x2B:
			strcpy_s(x->opcode,"sub");
			set_d(x,r16_32);
			set_s(x,r_m16_32);
			break;
		case 0x33:
			strcpy_s(x->opcode,"xor");
			set_d(x,r16_32);
			set_s(x,r_m16_32);
			break;
		case 0x38:
			strcpy_s(x->opcode,"cmp");
			set_d(x,r_m16_32);
			set_s(x,r8);
			break;
		case 0x3B:
			strcpy_s(x->opcode,"cmp");
			set_d(x,r16_32);
			set_s(x,r_m16_32);
			break;
		case 0x66:
			x->size++;
			switch (readb(addr+x->size)){
				case 0x90:
					strcpy_s(x->opcode,"xchg");
					strcpy_s(x->data,"xchg ax,ax");
					x->size++;
					return x;
				break;
			}
			break;
		case 0x68:
			strcpy_s(x->opcode,"push");
			set_d(x,imm32);
			break;
		case 0x6A:
			strcpy_s(x->opcode,"push");
			set_d(x,imm8);
			break;
		case 0x70:
			strcpy_s(x->opcode, "jo short"); // jmp short if overflow
			set_d(x,rel8);
			break;
		case 0x71:
			strcpy_s(x->opcode, "jno short"); // jmp short if not overflow
			set_d(x,rel8);
			break;
		case 0x72:
			strcpy_s(x->opcode, "jb short"); // jmp short if below
			set_d(x,rel8);
			break;
		case 0x73:
			strcpy_s(x->opcode, "jnb short"); // jmp short if above or equal / not below
			set_d(x,rel8);
			break;
		case 0x74:
			strcpy_s(x->opcode, "je short"); // jmp short if equal
			set_d(x,rel8);
			break;
		case 0x75:
			strcpy_s(x->opcode, "jne short"); // jmp short if not equal
			set_d(x,rel8);
			break;
		case 0x76:
			strcpy_s(x->opcode, "jbe short"); // jmp short if not above / below or equal
			set_d(x,rel8);
			break;
		case 0x77:
			strcpy_s(x->opcode, "ja short"); // jmp short if above
			set_d(x,rel8);
			break;
		case 0x78:
			strcpy_s(x->opcode, "js short"); // jmp short if sign
			set_d(x,rel8);
			break;
		case 0x79:
			strcpy_s(x->opcode, "jns short"); // jmp short if not sign
			set_d(x,rel8);
			break;
		case 0x7A:
			strcpy_s(x->opcode, "jp short"); // jmp short if parity
			set_d(x,rel8);
			break;
		case 0x7B:
			strcpy_s(x->opcode, "jnp short"); // jmp short if not parity
			set_d(x,rel8);
			break;
		case 0x7C:
			strcpy_s(x->opcode, "jl short"); // jmp short if less
			set_d(x,rel8);
			break;
		case 0x7D:
			strcpy_s(x->opcode, "jnl short"); // jmp short if not less
			set_d(x,rel8);
			break;
		case 0x7E:
			strcpy_s(x->opcode, "jng short"); // jmp short if not great
			set_d(x,rel8);
			break;
		case 0x7F:
			strcpy_s(x->opcode, "jg short"); // jmp short if greater
			set_d(x,rel8);
			break;
		case 0x80:
			switch (readb(addr+1)%40/8){
				case 0:
					strcpy_s(x->opcode,"add");
					break;
				case 1:
					strcpy_s(x->opcode,"or");
					break;
				case 2:
					strcpy_s(x->opcode,"adc");
					break;
				case 3:
					strcpy_s(x->opcode,"sbb");
					break;
				case 4:
					strcpy_s(x->opcode,"and");
					break;
				case 5:
					strcpy_s(x->opcode,"sub");
					break;
				case 6:
					strcpy_s(x->opcode,"xor");
					break;
				case 7:
					strcpy_s(x->opcode,"cmp");
				break;
			}
			if (readb(addr+1) >= 0xC0){
				strcpy_s(x->mark1, "byte ptr");
				set_d(x,r8);
				set_s(x,imm8);
			} else {
				strcpy_s(x->mark1, "dword ptr");
				set_d(x,r_m16_32);
				set_s(x,imm8);
			}
			break;
		case 0x81:
			switch (readb(addr+1)%0x40/8){
				case 0:
					strcpy_s(x->opcode,"add");
					set_d(x,r_m16_32);
					set_s(x,imm32);
					break;
				case 1:
					strcpy_s(x->opcode,"or");
					set_d(x,r_m16_32);
					set_s(x,imm32);
					break;
				case 2:
					strcpy_s(x->opcode,"adc");
					set_d(x,r_m16_32);
					set_s(x,imm32);
					break;
				case 3:
					strcpy_s(x->opcode,"sbb");
					set_d(x,r_m16_32);
					set_s(x,imm32);
					break;
				case 4:
					strcpy_s(x->opcode,"and");
					set_d(x,r_m16_32);
					set_s(x,imm32);
					break;
				case 5:
					strcpy_s(x->opcode,"sub");
					set_d(x,r_m16_32);
					set_s(x,imm32);
					break;
				case 6:
					strcpy_s(x->opcode,"xor");
					set_d(x,r_m16_32);
					set_s(x,imm32);
					break;
				case 7:
					strcpy_s(x->opcode,"cmp");
					set_d(x,r_m16_32);
					set_s(x,imm32);
				break;
			}
			strcpy_s(x->mark1, "dword ptr");
			break;
		case 0x82:
			switch (readb(addr+1)%0x40/8){
				case 0:
					strcpy_s(x->opcode,"add");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 1:
					strcpy_s(x->opcode,"or");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 2:
					strcpy_s(x->opcode,"adc");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 3:
					strcpy_s(x->opcode,"sbb");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 4:
					strcpy_s(x->opcode,"and");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 5:
					strcpy_s(x->opcode,"sub");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 6:
					strcpy_s(x->opcode,"xor");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 7:
					strcpy_s(x->opcode,"cmp");
					set_d(x,r_m16_32);
					set_s(x,imm8);
				break;
			}
			strcpy_s(x->mark1, "byte ptr");
			break;
		case 0x83:
			switch (readb(addr+1)%0x40/8){
				case 0:
					strcpy_s(x->opcode,"add");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 1:
					strcpy_s(x->opcode,"or");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 2:
					strcpy_s(x->opcode,"adc");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 3:
					strcpy_s(x->opcode,"sbb");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 4:
					strcpy_s(x->opcode,"and");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 5:
					strcpy_s(x->opcode,"sub");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 6:
					strcpy_s(x->opcode,"xor");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 7:
					strcpy_s(x->opcode,"cmp");
					set_d(x,r_m16_32);
					set_s(x,imm8);
				break;
			}
			strcpy_s(x->mark1, "dword ptr");
			break;
		case 0x85:
			strcpy_s(x->opcode,"test");
			set_d(x,r_m16_32);
			set_s(x,r16_32);
			break;
		case 0x88:
			strcpy_s(x->opcode,"mov");
			set_d(x,r_m16_32);
			set_s(x,r8);
			break;
		case 0x89:
			strcpy_s(x->opcode,"mov");
			set_d(x,r_m16_32);
			set_s(x,r16_32);
			break;
		case 0x8A:
			strcpy_s(x->opcode,"mov");
			set_d(x,r8);
			set_s(x,r_m16_32);
			break;
		case 0x8B:
			strcpy_s(x->opcode,"mov");
			set_d(x,r16_32);
			set_s(x,r_m16_32);
			break;
		case 0x8D:
			strcpy_s(x->opcode,"lea");
			set_d(x,r16_32);
			set_s(x,r_m16_32);
			break;
		case 0xA1:
			strcpy_s(x->opcode, "mov eax");
			single_reg=true;
			set_s(x,imm32);
			break;
		case 0xA2:{
			strcpy_s(x->opcode, "mov");
			strcpy_s(x->data, "mov dword:");
			char c[16];
			sprintf_s(c,"%08X",readui(addr+1));
			strcat_s(x->data, c);
			strcat_s(x->data, ",al");
			x->size=5;
			return x;
		} break;
		case 0xA3:{
			strcpy_s(x->opcode, "mov");
			strcpy_s(x->data, "mov dword:");
			char c[16];
			sprintf_s(c,"%08X",readui(addr+1));
			strcat_s(x->data, c);
			strcat_s(x->data, ",eax");
			x->size=5;
			return x;
		} break;
		case 0xB8:
			strcpy_s(x->opcode, "mov eax");
			single_reg=true;
			set_s(x,imm32);
			break;
		case 0xB9:
			strcpy_s(x->opcode, "mov ecx");
			single_reg=true;
			set_s(x,imm32);
			break;
		case 0xBA:
			strcpy_s(x->opcode, "mov edx");
			single_reg=true;
			set_s(x,imm32);
			break;
		case 0xBB:
			strcpy_s(x->opcode, "mov ebx");
			single_reg=true;
			set_s(x,imm32);
			break;
		case 0xBC:
			strcpy_s(x->opcode, "mov esp");
			single_reg=true;
			set_s(x,imm32);
			break;
		case 0xBD:
			strcpy_s(x->opcode, "mov ebp");
			single_reg=true;
			set_s(x,imm32);
			break;
		case 0xBE:
			strcpy_s(x->opcode, "mov esi");
			single_reg=true;
			set_s(x,imm32);
			break;
		case 0xBF:
			strcpy_s(x->opcode, "mov edi");
			single_reg=true;
			set_s(x,imm32);
			break;
		case 0xC0:
			switch (readb(addr+1)%0x40/8){
				case 0:
					strcpy_s(x->opcode,"rol");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 1:
					strcpy_s(x->opcode,"ror");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 2:
					strcpy_s(x->opcode,"rcl");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 3:
					strcpy_s(x->opcode,"rcr");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 4:
					strcpy_s(x->opcode,"shl");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 5:
					strcpy_s(x->opcode,"shr");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				// CASE 6 NOT COVERED BY INTEL DOCUMENTATION
				case 7:
					strcpy_s(x->opcode,"sar");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
			}
			strcpy_s(x->mark1, "byte ptr");
			break;
		case 0xC1:
			switch (readb(addr+1)%0x40/8){
				case 0:
					strcpy_s(x->opcode,"rol");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 1:
					strcpy_s(x->opcode,"ror");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 2:
					strcpy_s(x->opcode,"rcl");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 3:
					strcpy_s(x->opcode,"rcr");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 4:
					strcpy_s(x->opcode,"shl");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				case 5:
					strcpy_s(x->opcode,"shr");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
				// CASE 6 NOT COVERED BY INTEL DOCUMENTATION
				case 7:
					strcpy_s(x->opcode,"sar");
					set_d(x,r_m16_32);
					set_s(x,imm8);
					break;
			}
			break;
		case 0xC2:
			strcpy_s(x->opcode, "ret");
			set_d(x,imm16);
			break;
		case 0xC3:
			strcpy_s(x->opcode, "retn");
			break;
		case 0xC6:
			if (readb(addr+1)%0x40<8){
				strcpy_s(x->opcode, "mov");
				strcpy_s(x->mark1, "byte ptr");
				set_d(x, r_m16_32);
				set_s(x, imm8);
				break;
			}
			break;
		case 0xC7:
			if (readb(addr+1)%0x40<8){
				strcpy_s(x->opcode, "mov");
				strcpy_s(x->mark1, "dword ptr");
				set_d(x, r_m16_32);
				set_s(x, imm32);
				break;
			}
			break;
		case 0xCC:
			strcpy_s(x->opcode,"align");
			break;
		case 0xD9:
			x->size++;
			switch (readb(addr+x->size)){
				case 0xE0:
					strcpy_s(x->opcode,"fchs"); // change sign
					break;
				case 0xE1:
					strcpy_s(x->opcode,"fabs"); // absolute value
					break;
				case 0xE4:
					strcpy_s(x->opcode,"ftst"); // test
					break;
				case 0xE5:
					strcpy_s(x->opcode,"fxam"); // examine
					break;
				case 0xE8:
					strcpy_s(x->opcode,"fld1"); // Push +1.0 onto FPU stack
					break;
				case 0xE9:
					strcpy_s(x->opcode,"fldl2t"); // Push log2(10) onto FPU stack
					break;
				case 0xEA:
					strcpy_s(x->opcode,"fldl2e"); // Push log2(e) onto FPU stack
					break;
				case 0xEB:
					strcpy_s(x->opcode,"fldpi"); // Push pi constant onto FPU stack
					break;
				case 0xEC:
					strcpy_s(x->opcode,"fldlg2"); // Push log10(2) onto FPU stack
					break;
				case 0xED:
					strcpy_s(x->opcode,"fldln2"); // Push log e(2) onto FPU stack
					break;
				case 0xEE:
					strcpy_s(x->opcode,"fldz"); // Push +0.0 onto FPU stack
					break;
			}
		case 0xDD:
			if (readb(addr+1) <= 0xC0){
				strcpy_s(x->opcode,"fld");
				strcpy_s(x->mark1,"qword ptr");
				set_d(x,r_m16_32);
			}
			break;
		case 0xE8:
			strcpy_s(x->opcode,"call");
			set_d(x,rel32);
			break;
		case 0xE9:
			strcpy_s(x->opcode,"jmp");
			set_d(x,rel32);
			break;
		case 0xEB:
			strcpy_s(x->opcode,"jmp short");
			set_d(x,rel8);
			break;

		case 0xF0:
			x->size++;
			switch (readb(addr+x->size)){
				case 0xFF:
					strcpy_s(x->opcode, "lock inc");
					set_d(x,r_m16_32);
				break;
			}
		break;

		case 0xF2:
			x->size++;
			switch (readb(addr+x->size)){
				case 0x0F:
					x->size++;
					switch (readb(addr+x->size)){
						case 0x10:
							strcpy_s(x->opcode, "movsd");
							if (readb(addr+x->size+1) >= 0xC0){
								set_d(x,rxmm);
								set_s(x,rxmm);
							} else {
								strcpy_s(x->mark2, "qword ptr");
								set_d(x,rxmm);
								set_s(x,r_m16_32);
							}
						break;

						case 0x11:
							strcpy_s(x->opcode, "movsd");
							if (readb(addr+x->size+1) >= 0xC0){
								set_d(x,rxmm);
								set_s(x,rxmm);
							} else {
								strcpy_s(x->mark1, "qword ptr");
								set_d(x,r_m16_32);
								set_s(x,rxmm);
							}
						break;
					}
				break;
			}
		break;

		case 0xF7:
			x->size++;
			if (readb(addr+x->size) % 0x40 < 8){
				strcpy_s(x->opcode, "test");
				set_d(x,r_m16_32);
				set_s(x,imm16); // short value test
			}
			if (readb(addr+x->size) >= 0xD0) {
				switch ((readb(addr+x->size)-0xD0)/8){
					case 0:
						strcpy_s(x->opcode, "not");
						set_d(x,r32);
						break;
					case 1:
						strcpy_s(x->opcode, "neg");
						set_d(x,r32);
						break;
					case 2:
						strcpy_s(x->opcode, "mul");
						set_d(x,r32);
						break;
					case 3:
						strcpy_s(x->opcode, "imul");
						set_d(x,r32);
						break;
					case 4:
						strcpy_s(x->opcode, "div");
						set_d(x,r32);
						break;
					case 5:
						strcpy_s(x->opcode, "idiv");
						set_d(x,r32);
					break;
				}
			}
		break;

		case 0xFF:
			switch (readb(addr+1)%0x40/8){
				case 0:
					strcpy_s(x->opcode,"inc");
					set_d(x,r_m16_32);
					break;
				case 1:
					strcpy_s(x->opcode,"dec");
					set_d(x,r_m16_32);
					break;
				case 2:
					strcpy_s(x->opcode,"call");
					set_d(x,r_m16_32);
					break;
				case 3:
					strcpy_s(x->opcode,"call");
					set_d(x,r_m16_32);
					break;
				case 4:
					strcpy_s(x->opcode,"jmp");
					set_d(x,r_m16_32);
					break;
				case 5:
					strcpy_s(x->opcode,"jmp far");
					set_d(x,r_m16_32);
					break;
				case 6:
					strcpy_s(x->opcode,"push");
					set_d(x,r_m16_32);
				break;
				// CASE 7 NOT COVERED BY INTEL DOCUMENTATION
			}
			strcpy_s(x->mark1, "dword ptr");
		break;

		default:
			strcpy_s(x->opcode,"?");
			strcpy_s(x->data,"?");
			x->size++;
			return x;
		break;
	}

	x->size++;
	UCHAR c,mode20,mode40,i,j,oldj=0,skip=0;

	strcpy_s(x->data, x->opcode);
	if (!single_reg) strcat_s(x->data, " ");
	
	char cnv[16]; // for future values we have to convert to string
	char second_op[8];

	auto check_mark = [&x]() {
		if (lstrlenA(x->mark1) > 0){
			strcat_s(x->data, x->mark1);
			strcat_s(x->data, " ");
		} else if (lstrlenA(x->mark2) > 0){
			strcat_s(x->data, x->mark2);
			strcat_s(x->data, " ");
		}
	};

	// Used for furthering the instruction
	// by updating the values corresponding
	// to the next byte
	// 
	auto update = [&c,&addr,&x,&mode20,&mode40,&i,&j]() {
		c=readb(addr+x->size);
		mode20=c/32;
		mode40=c/64;
		i=c%8;
		j=c%64/8;
	};

	// *calibrate and then extend size
	auto extend = [&c,&addr,&x,update]() {
		update();
		x->size++;
	};

	// Quick functions to help translate
	// the asm to readable text
	//
	auto w_offset8 = [&x,&cnv,&addr]() {
		UCHAR v=readb(addr+x->size);
		x->offset = v;
		if (v <= 0x7F){
			sprintf_s(cnv,"%02X",v);
			strcat_s(x->data, "+");
			strcat_s(x->data, cnv);
		} else {
			sprintf_s(cnv,"%02X",(UCHAR_MAX-v+1));
			strcat_s(x->data, "-");
			strcat_s(x->data, cnv);
		}
		x->size += 1;
	};

	auto w_offset16 = [&x,&cnv,&addr]() {
		USHORT v=readus(addr+x->size);
		x->offset = v;
		if (v <= 0x7FFF){
			sprintf_s(cnv,"%04X",v);
			strcat_s(x->data, "+");
			strcat_s(x->data, cnv);
		} else {
			sprintf_s(cnv,"%04X",(USHRT_MAX-v+1));
			strcat_s(x->data, "-");
			strcat_s(x->data, cnv);
		}
		x->size += sizeof(USHORT);
	};

	auto w_offset32 = [&x,&cnv,&addr]() {
		UINT_PTR v=readui(addr+x->size);
		x->offset = v;
		if (v <= 0x7FFFFFFF){
			sprintf_s(cnv,"%08X",v);
			strcat_s(x->data, "+");
			strcat_s(x->data, cnv);
		} else {
			sprintf_s(cnv,"%08X",(UINT32_MAX-v+1));
			strcat_s(x->data, "-");
			strcat_s(x->data, cnv);
		}
		x->size += sizeof(UINT_PTR);
	};

	auto w_mult32 = [&x,&mode40]() {
		if (mode40 != 0) {
			int mul=(mode40==1)?2:(mode40==2)?4:(mode40==3)?8:0;
			char s_mul[2];
			sprintf_s(s_mul,"%i",mul);
			strcat_s(x->data, "*");
			strcat_s(x->data, s_mul);
		}
	};

	update();

	// FIRST OPERAND
	switch (x->dest) {
		case _m::none: break;

		// Check 8bit value on the next byte
		case _m::imm8:{
			UCHAR v = readb(addr+x->size);
			x->v8 = v;
			sprintf_s(cnv,"%02X",v);
			strcat_s(x->data,cnv);
			x->size += 1;
		} break;
		// Check 16bit value on the next 2 bytes
		case _m::imm16:{
			USHORT v = readus(addr+x->size);
			x->v16 = v;
			sprintf_s(cnv,"%04X",v);
			strcat_s(x->data,cnv);
			x->size += sizeof(USHORT);
		} break;
		// Check 32bit value on the next 4 bytes
		case _m::imm32:{
			UINT_PTR v = readui(addr+x->size);
			x->v32 = v;
			sprintf_s(cnv,"%08X",v);
			strcat_s(x->data,cnv);
			x->size += sizeof(UINT_PTR);
		} break;

		case _m::rel8:{
			UCHAR v = ((addr+x->size+1) + readb(addr+x->size));
			x->v8 = v;
			sprintf_s(cnv,"%02X",v);
			strcat_s(x->data,cnv);
			x->size += 1;
		} break;
		case _m::rel16:{
			USHORT v = (addr+x->size+2) + readus(addr+x->size);
			x->v16 = v;
			sprintf_s(cnv,"%04X",v);
			strcat_s(x->data,cnv);
			x->size += sizeof(USHORT);
		} break;
		case _m::rel32:{
			UINT_PTR v = (addr+x->size+4) + readui(addr+x->size);
			x->v32 = v;
			sprintf_s(cnv,"%08X",v);
			strcat_s(x->data,cnv);
			x->size += sizeof(UINT_PTR);
		} break;

		case _m::r8:
			strcat_s(x->data,_r8[j]); // goes by 8ths
		break;
		case _m::r16:
			strcat_s(x->data,_r16[j]); // have not gotten here yet
		break;
		case _m::r32:
			strcat_s(x->data,_r32[j]);
		break;
		case _m::rxmm:
			strcat_s(x->data,_xmm[j]);
		break;

		case _m::r16_32:
			x->r32[0] = j;
			switch (mode40){
				case 0: // 0x00 through 0x3F
					switch (i){
						case 0x4:
							strcat_s(x->data,_r32[j]);
							break;
						case 0x5:
							strcat_s(x->data,_r32[(c-0x5)/8]);
							x->size++;
							x->src = _m::imm32;
							break;
						default:
							strcat_s(x->data,_r32[j]);
							break;
					}
				break;
				default: // 0x40-0xFF
					strcat_s(x->data,_r32[j]);
				break;
			}
		break;

		case _m::r_m16_32:{
			// This is done because a previous byte could
			// represent the second operand, which
			// we need to know, if it is a register
			UCHAR	can_solve_second_r8op = 
							(x->src==_m::r8),
					can_solve_second_r16op = 
							(x->src==_m::r16),
					can_solve_second_r32op =
							(x->src==_m::r32 ||
							x->src==_m::r16_32),
					can_solve_second_rxmmop =
							(x->src==_m::rxmm);
			// For any case, we need to skip the second
			// operand check
			skip = (can_solve_second_r8op ||
					can_solve_second_r16op ||
					can_solve_second_r32op ||
					can_solve_second_rxmmop);

			extend();
			x->r32[0] = i;
			switch (mode40) {
				case 3: // 0xC0 through 0xFF
					strcat_s(x->data, _r32[i]);
					break;
				case 2: // 0x80 through 0xBF
					check_mark();
					strcat_s(x->data, "[");
					oldj = j;
					switch(i){
						case 0x4:
							extend();
							x->r32[0] = i;
							strcat_s(x->data, _r32[i]);
							if (mode20%2!=1 || c%0x20>=16){ 
								strcat_s(x->data, "+");
								strcat_s(x->data, _r32[j]);
								w_mult32();
							} else if (c%0x20>=8) {
								strcat_s(x->data, "+");
								strcat_s(x->data, _r32[j]);
							}
							break;
						default:
							strcat_s(x->data, _r32[i]);
							break;
					}
					w_offset32();
					strcat_s(x->data, "]");
					break;
				case 1: // 0x40 through 0x80
					check_mark();
					strcat_s(x->data, "[");
					oldj = j;
					switch(i){
						case 0x4:
							extend();
							x->r32[0] = i;
							strcat_s(x->data, _r32[i]);
							if (mode20%2!=1 || c%0x20>=16){ 
								strcat_s(x->data, "+");
								strcat_s(x->data, _r32[j]);
								w_mult32();
							} else if (c%0x20>=8) {
								strcat_s(x->data, "+");
								strcat_s(x->data, _r32[j]);
							}
							break;
						default:
							strcat_s(x->data, _r32[i]);
							break;
					}
					w_offset8();
					strcat_s(x->data, "]");
					break;
				case 0: // 0x00 through 0x40
					check_mark();
					strcat_s(x->data, "[");
					oldj = j;
					switch(i){
						case 0x4:
							extend();
							x->r32[0] = i;
							switch(i){
								case 0x5:
									if ((mode20+1)%2==0){
										sprintf_s(cnv,"%08X",readui(addr+x->size));
										strcat_s(x->data, cnv);
										x->size += sizeof(UINT_PTR);
									} else {
										strcat_s(x->data, _r32[j]);
										w_mult32();
										w_offset32();
									}
								break;
								default:
									strcat_s(x->data, _r32[i]);
									if (mode20%2!=1 || c%0x20>=16){ 
										strcat_s(x->data, "+");
										strcat_s(x->data, _r32[j]);
										w_mult32();
									} else if (c%0x20>=8) {
										strcat_s(x->data, "+");
										strcat_s(x->data, _r32[j]);
										w_mult32();
									}
								break;
							}
							break;
						case 0x5:
							sprintf_s(cnv,"%08X",readui(addr+x->size));
							strcat_s(x->data, cnv);
							x->size += sizeof(UINT_PTR);
							break;
						default:
							strcat_s(x->data, _r32[i]);
							break;
					}
					strcat_s(x->data, "]");
					break;
				break;
			}
			if (skip){ // We are solving for the second operand
				if (can_solve_second_r8op){	x->r8[1]=oldj; strcpy_s(second_op,_r8[oldj]); }
				if (can_solve_second_r16op){ x->r16[1]=oldj; strcpy_s(second_op,_r16[oldj]); }
				if (can_solve_second_r32op){ x->r32[1]=oldj; strcpy_s(second_op,_r32[oldj]); }
				if (can_solve_second_rxmmop){ x->rxmm[1]=oldj; strcpy_s(second_op,_xmm[oldj]); }
				strcat_s(x->data, ",");
				strcat_s(x->data, second_op);
			}
		} break;
	}

	// SECOND OPERAND, if not already configured
	if (!skip && x->src != _m::none){
		strcat_s(x->data,",");

		switch (x->src) {
			// Check 8bit value on the next byte
			case _m::imm8:{
				UCHAR v = readb(addr+x->size);
				x->v8 = v;
				sprintf_s(cnv,"%02X",v);
				strcat_s(x->data,cnv);
				x->size += 1;
			} break;
			// Check 16bit value on the next 2 bytes
			case _m::imm16:{
				USHORT v = readus(addr+x->size);
				x->v16 = v;
				sprintf_s(cnv,"%04X",v);
				strcat_s(x->data,cnv);
				x->size += sizeof(USHORT);
			} break;
			// Check 32bit value on the next 4 bytes
			case _m::imm32:{
				UINT_PTR v = readui(addr+x->size);
				x->v32 = v;
				sprintf_s(cnv,"%08X",v);
				strcat_s(x->data,cnv);
				x->size += sizeof(UINT_PTR);
			} break;

			case _m::rel8:{
				UCHAR v = ((addr+x->size+1) + readb(addr+x->size));
				x->v8 = v;
				sprintf_s(cnv,"%02X",v);
				strcat_s(x->data,cnv);
				x->size += 1;
			} break;
			case _m::rel16:{
				USHORT v = (addr+x->size+2) + readus(addr+x->size);
				x->v16 = v;
				sprintf_s(cnv,"%04X",v);
				strcat_s(x->data,cnv);
				x->size += sizeof(USHORT);
			} break;
			case _m::rel32:{
				UINT_PTR v = (addr+x->size+4) + readui(addr+x->size);
				x->v32 = v;
				sprintf_s(cnv,"%08X",v);
				strcat_s(x->data,cnv);
				x->size += sizeof(UINT_PTR);
			} break;

			case _m::r8:
				strcat_s(x->data,_r8[i]);
				x->size++;
			break;
			case _m::r16:
				strcat_s(x->data,_r16[i]);
				x->size++;
			break;
			case _m::r32:
				strcat_s(x->data,_r32[i]);
				x->size++;
			break;
			case _m::rxmm:
				strcat_s(x->data,_xmm[i]);
				x->size++;
			break;

			case _m::r16_32:
				x->r32[1] = j;
				switch (mode40){
					case 0: // 0x00 through 0x3F
						switch (i){
							case 0x4:
								strcat_s(x->data,_r32[j]);
								break;
							case 0x5:
								strcat_s(x->data,_r32[(c-0x5)/8]);
								x->src = _m::imm32;
								break;
							default:
								strcat_s(x->data,_r32[j]);
								break;
						}
					break;
					default: // 0x40-0xFF
						strcat_s(x->data,_r32[j]);
					break;
				}
			break;

			case _m::r_m16_32:
				extend();
				x->r32[1] = i;
				switch (mode40){
					case 3: // 0xC0 through 0xFF		[op] eax,[eax]
						strcat_s(x->data,_r32[i]);
					break;
					case 2: // 0x80 through 0xBF		[op] eax,[eax+eax*?+????????]
						check_mark();
						strcat_s(x->data, "[");
						switch(i){
							case 0x4:
								extend();
								x->r32[1] = i;
								strcat_s(x->data, _r32[i]);
								// 0x60 = [op] eax,[eax+00000000]
								// 0xA0 = [op] eax,[eax+00000000]
								// but, 0x80 = [op] eax,[eax+eax*4+00000000]
								//
								strcat_s(x->data, "+");
								strcat_s(x->data, _r32[j]);
								w_mult32();
								break;
							default:
								strcat_s(x->data, _r32[i]);
								break;
						}
						w_offset32();
						strcat_s(x->data, "]");
					break;
					case 1: // 0x40 through 0x7F		[op] eax,[eax+eax*?+??]
						check_mark();
						strcat_s(x->data, "[");
						switch(i){
							case 0x4:
								extend();
								x->r32[1] = i;
								strcat_s(x->data, _r32[i]);
								if (!(mode20%2!=0 && c%0x20<8)){
									strcat_s(x->data, "+");
									strcat_s(x->data, _r32[j]);
									w_mult32();
								}
							break;
							default:
								strcat_s(x->data, _r32[i]);
							break;
						}
						w_offset8();
						strcat_s(x->data, "]");
					break;
					case 0:
						check_mark();
						strcat_s(x->data, "[");
						switch(i){
							case 0x5:
								sprintf_s(cnv,"%08X",readui(addr+x->size));
								strcat_s(x->data, cnv);
								x->size += sizeof(UINT_PTR);
								break;
							case 0x4:
								extend();
								x->r32[1] = i;
								switch(i){
									case 0x5:
										if (mode20%2!=1){
											strcat_s(x->data, _r32[j]);
											w_mult32();
											w_offset32();
										} else {
											sprintf_s(cnv,"%08X",readui(addr+x->size));
											strcat_s(x->data, cnv);
											x->size += sizeof(UINT_PTR);
										}
									break;
									default:
										strcat_s(x->data, _r32[i]);
										if (mode20%2!=1){
											strcat_s(x->data, "+");
											strcat_s(x->data, _r32[j]);
											w_mult32();
										} else if (c%0x20>=8) {
											strcat_s(x->data, "+");
											strcat_s(x->data, _r32[j]);
											w_mult32();
										}
									break;
								}
							break;
							default:
								strcat_s(x->data, _r32[i]);
							break;
						}
						strcat_s(x->data, "]");
					break;
				}
			break;
		}
	}

	return x;
}

std::string replaceex(std::string str, const char* replace, const char* mask, const char* newstr) {
	std::string x;
	int size=lstrlenA(mask);
	for (int i=0; i<str.length(); i++){
		bool matched=(i<(str.length()-size));
		if (matched) // dont check past the string size
			for (int j=0; j<size; j++)
				if (mask[j]=='.' && str[i+j]!=replace[j])
					matched=false;
		if (matched){
			i += (size-1);
			x += newstr;
		} else {
			x += str[i];
		}
	}
	return x;
}

bool strfind(const char* A, const char* B) {
	bool found = true;
	for (int i=0; i < (lstrlenA(A) - lstrlenA(B)); i++){
		found = true;
		for (int j=0; j < lstrlenA(B); j++)
			if (A[i+j] != B[j])
				found = false;
		if (found) return found;
	}
	return false;
}

std::string EyeCrawl::disassemble(UINT_PTR addr, int count, info_mode extra_info) {
	std::string str = "";
	if (proc == INVALID_HANDLE_VALUE) return str;

	for (int n=0,s=0; n<count; n++) {
		EyeCrawl::pinstruction i = EyeCrawl::disassemble(addr+s);
		s += i->size;
		str += i->data;
		if (extra_info == show_offsets || extra_info == show_ioffsets){
			if (i->offset != 0) {
				char spaces[44];
				spaces[0] = '\0';
				for (int j=lstrlenA(i->data); j<44; j++) strcat_s(spaces, " ");

				char c[4];
				if (extra_info == show_offsets)
					sprintf_s(c,"%02X",(UCHAR)i->offset);
				else if (extra_info == show_ioffsets)
					sprintf_s(c,"%i",(UCHAR)i->offset);
				str += spaces;
				str += " // ";
				str += c;
			}
		} else if (extra_info == show_int32){
			char spaces[44];
			spaces[0] = '\0';
			for (int j=lstrlenA(i->data); j<44; j++) strcat_s(spaces, " ");

			char c[16];
			sprintf_s(c,"%i",i->v32);
			str += spaces;
			str += " // ";
			str += c;
		} else if (extra_info == show_args){
			if (strfind(i->data, "ebp+")){
				char spaces[44],c[8];
				spaces[0] = '\0';
				for (int j=lstrlenA(i->data); j<44; j++) strcat_s(spaces," ");

				sprintf_s(c,"arg_%i",(UCHAR)((i->offset-0x8)/0x4));
				str = replaceex(str,"ebp+??","....xx",c);
				str += spaces;
				str += " // ";
				str += c;
			}
		} else if (extra_info == show_vars){
			if (strfind(i->data, "ebp-")){
				char spaces[44],c[8];
				spaces[0] = '\0';
				for (int j=lstrlenA(i->data); j<44; j++) strcat_s(spaces," ");
				
				sprintf_s(c,"var_%i",(UCHAR)((UCHAR_MAX-i->offset-1)/0x4));
				str = replaceex(str,"ebp-??","....xx",c);
				str += spaces;
				str += " // ";
				str += c;
			}
		} else if (extra_info == show_args_and_vars){
			bool found_var = strfind(i->data, "ebp-");
			bool found_arg = strfind(i->data, "ebp+");
			if (found_var || found_arg){
				char spaces[44],c[8];
				spaces[0] = '\0';
				for (int j=lstrlenA(i->data); j<44; j++) strcat_s(spaces," ");
				
				if (found_var){
					sprintf_s(c,"var_%i",(UCHAR)((UCHAR_MAX-i->offset-1)/0x4));
					str = replaceex(str,"ebp-??","....xx",c);
				} else if (found_arg){
					sprintf_s(c,"arg_%i",(UCHAR)((i->offset-0x8)/0x4));
					str = replaceex(str,"ebp+??","....xx",c);
				}

				str += spaces;
				str += " // ";
				str += c;
			}
		} else if (extra_info == show_non_aslr){
			if (strfind(i->data, "call") ||
				strfind(i->data, "jmp")){
				char spaces[44],c[16];
				spaces[0] = '\0';
				for (int j=lstrlenA(i->data); j<44; j++) strcat_s(spaces," ");

				sprintf_s(c,"%08X",non_aslr(i->v32));
				str += spaces;
				str += " // ";
				str += c;
			}
		}
		str += "\n";
		delete i;
	}
	return str;
}

// -----------------------------------------------------------
// ---------------- EyeCrawl Memory Utility ------------------
// -----------------------------------------------------------
UINT_PTR EyeCrawl::util::valloc(ULONG_PTR size) {
	return (UINT_PTR)VirtualAllocEx(proc,reinterpret_cast<void*>(0),size,0x1000|0x2000,0x40);
}

bool EyeCrawl::util::vfree(UINT_PTR address, ULONG_PTR size) {
	return VirtualFreeEx(proc,reinterpret_cast<void*>(address),size,MEM_RELEASE);
}

UCHAR EyeCrawl::util::to_byte(char* x) {
	if (lstrlenA(x)<2) return 0;
	if (x[0]=='?' && x[1]=='?') return 0;
	UCHAR b=0;
	for (int i=0;i<16;i++){
		if (x[0]==c_ref1[i]) b+=c_ref2[i]*16;
		if (x[1]==c_ref1[i]) b+=i;
	}
	return b;
}

std::string EyeCrawl::util::to_str(UCHAR b) {
	std::string x="";
	x += c_ref1[b/16];
	x += c_ref1[b%16];
	return x;
}

results EyeCrawl::util::scan(UINT_PTR begin, UINT_PTR end, const char* aob, const char* mask) {
	HANDLE self				= GetCurrentProcess();
	int oldpriority			= GetThreadPriority(self);
	SetThreadPriority(self, THREAD_PRIORITY_HIGHEST);

	results	 results_list	= results();
	UINT_PTR start			= begin,
			 size			= lstrlenA(mask),
			 at				= 0;
	UCHAR* data				= new UCHAR[size];
	UCHAR* buffer			= new UCHAR[buffersize];
	
	for (int i=0,j=0;i<size;i++,j+=2){
		char c[2];
		c[0]=aob[j];
		c[1]=aob[j+1];
		data[i]=to_byte(c);
	}

	while (start < end){
		if (PMREAD(proc,reinterpret_cast<void*>(start),buffer,buffersize,0)){
			__asm push edi
			__asm mov edi,0
			__asm jmp L2
		L1:	__asm inc edi
			__asm mov at,edi
			unsigned char match=1;
			for (UINT_PTR x=0; x<size; x++)
				if (buffer[at+x]!=data[x] && mask[x]!='x')
					match=0;
			if (match) results_list.push_back(start+at);
		L2:	__asm cmp edi,buffersize
			__asm jb L1
			__asm pop edi
		}
		start += (buffersize-size)+1;
	}
	delete[] buffer;
	delete[] data;

	SetThreadPriority(self, oldpriority);
	return results_list;
}

results EyeCrawl::util::scan(UINT_PTR begin, UINT_PTR end, const char* aob){
	int size = (lstrlenA(aob)/2);
	char* c = new char[size+1];
	for (int i=0;i<size;i++) c[i]='.';
	c[size]='\0';
	results results_list = scan(begin, end, aob, c);
	delete[] c;
	return results_list;
}

// Simply identifies 3 different function
// prologues that are most commonly used.
// 
// Obviously will not work on a naked
// function
// 
bool EyeCrawl::util::isprologue(UINT_PTR address) {
	UCHAR	b1=readb(address),
			b2=readb(address+1),
			b3=readb(address+2);
	return	(b1==0x55 && b2==0x8B && b3==0xEC) ||
			(b1==0x56 && b2==0x8B && b3==0xF1) ||
			(b1==0x53 && b2==0x8B && b3==0xDC);
}

// This doesnt even require disassembly
// Lol
bool EyeCrawl::util::isepilogue(UINT_PTR address) {
	UCHAR b1 = readb(address);
	UCHAR b2 = readb(address+1);
	return ((b1==0x5D || b1==0x5E) && // pop ebp, or pop esi,
			(b2==0xC2 || b2==0xC3));  // with a retn or ret XX
}

UINT_PTR EyeCrawl::util::nextprologue(UINT_PTR address, dir direction, bool aligned){
	UINT_PTR at=address,count=0;
	while (!isprologue(at)){
		if (count++ > 0xFFFF) break;
		if (direction == dir::ahead)  if (!aligned) at++; else at += 16;
		if (direction == dir::behind) if (!aligned) at--; else at -= 16;
	}
	return at;
}

UINT_PTR EyeCrawl::util::nextepilogue(UINT_PTR address, dir direction){
	UINT_PTR at=address,count=0;
	while (!isepilogue(at)){
		if (count++ > 0xFFFF) break;
		if (direction == dir::ahead)  at++;
		if (direction == dir::behind) at--;
	}
	return at+1; // Return the functions retn address
}

// go forward to the next function, then
// go backwards from there till we reach
// the last epilogue of the current function
UINT_PTR EyeCrawl::util::getepilogue(UINT_PTR func) {
	return nextepilogue(nextprologue(func+16,dir::ahead,true), dir::behind);
}

short EyeCrawl::util::fretn(UINT_PTR func) {
	for (UINT_PTR addr : getepilogues(func)) {
		if (readb(addr) == 0xC2) {
			pinstruction i = disassemble(addr);
			short v = i->v16;
			delete i;
			return v;
		}
	}
	return 0;
}

int EyeCrawl::util::fsize(UINT_PTR func) {
	UINT_PTR eof = getepilogue(func);
	if (readb(eof) == 0xC2) eof += 3;
	if (readb(eof) == 0xC3) eof += 1;
	int funcSz = (int)(eof-func);
	if (funcSz < 0) return 0;
	return funcSz;
}

results EyeCrawl::util::getepilogues(UINT_PTR func) {
	results r = results();
	UINT_PTR start=func, end=(start+fsize(func));
	while (start < end) {
		start++;
		if (isepilogue(start)){
			r.push_back(start+1);
		}
	}
	return r;
}

// This doesn't necessarily need any
// disassembling
results EyeCrawl::util::getcalls(UINT_PTR func) {
	results r = results();
	UINT_PTR start=func, end=(start+fsize(func));
	while (start < end) {
		start++;
		if (readb(start) == 0xE8){
			UINT_PTR o = readui(start+1);
			if (o%16==0 && o>base_start() && o<base_end()){
				r.push_back((UINT_PTR)o);
			}
		}
	}
	return r;
}

UINT_PTR EyeCrawl::util::nextcall(UINT_PTR func,dir d,bool loc){
	UINT_PTR start=func;
	while (readb(start) != 0xE8){
		if (d==dir::ahead)  start++;
		if (d==dir::behind) start--;
	}
	UINT_PTR o = readui(start+1);
	if (o%16==0 && o>base_start() && o<base_end())
		if (loc)
			return (start+o+5);
		else
			return start;
	return 0;
}

EyeCrawl::util::MEM_PROTECT EyeCrawl::util::vprotect(UINT_PTR location, ULONG_PTR size) {
	MEM_PROTECT mp = MEM_PROTECT();
	MEMORY_BASIC_INFORMATION mbi = { 0 };
	VirtualQueryEx(proc,reinterpret_cast<void*>(location),&mbi,sizeof(mbi));
	VirtualProtectEx(proc,mbi.BaseAddress,size,PAGE_READWRITE,&mbi.Protect);
	mp.address = location;
	mp.size = size;
	mp.protection_data = mbi;
	return mp;
}

void EyeCrawl::util::vrestore(MEM_PROTECT protection) {
	ULONG_PTR oldProtect;
	VirtualProtectEx(proc,protection.protection_data.BaseAddress,protection.size,protection.protection_data.Protect,&oldProtect);
}

UINT_PTR EyeCrawl::util::debug32(UINT_PTR address, UCHAR r32, int offset) {
	ULONG_PTR size=5,nop=0,isize=0,d=0;
	UINT_PTR value=0,at=0,mask=0xABCDEF,
			 code_loc=valloc(32),
			 trace_loc=valloc(4);

	// Figure out how many left over bytes
	// from an instruction we might overwrite
	// 
	pinstruction i;
	i = disassemble(address);
	while (i->address<(address+size)){
		isize += i->size;
		nop = ((i->address+i->size)-(address+size));
		free(i);
		i = disassemble(address+isize);
	}
	free(i);

	// Get current bytes + bytes from
	// instruction we might overwrite
	UCHAR* old_bytes = new UCHAR[size+nop];
	PMREAD(proc,reinterpret_cast<void*>(address),old_bytes,size+nop,0);

	// Make up our JMP from the address
	// to our own code
	UCHAR* inject = new UCHAR[5];
	memcpy(inject,"\xE9",1);
	*(UINT_PTR*)(inject+1)=(code_loc-(address+5));

	if (offset == 0){
		// simply place one instruction to capture 
		// the value of the register to our readout location
		write(code_loc+at++,(UCHAR)0x50+r32); // push (r32)
		switch (r32) {
			case R_EAX:
				write(code_loc+at++,(UCHAR)0xA3);
				break;
			default:
				write(code_loc+at++,(UCHAR)0x89); // ecx-edi (0xD,0x15,0x1D,0x25,0x2D . . .)
				write(code_loc+at++,(UCHAR)0x5+(r32*8));
			break;
		}
		// Trace register to our trace location
		write(code_loc+at,(int)trace_loc);
		at += 4;
		write(code_loc+at++,(UCHAR)0x58+r32); // pop (r32)
	} else {
		// or, if we want an offset of a register ...
		// move the offset into EAX and show the value of EAX
		// at our readout location
		write(code_loc+at++,(UCHAR)0x50); // push eax
		write(code_loc+at++,(UCHAR)0x8B);
		if (r32 != R_ESP)
			write(code_loc+at++,(UCHAR)(0x40+r32));
		else {
			write(code_loc+at++,(UCHAR)(0x44));
			write(code_loc+at++,(UCHAR)(0x24));
		}
		write(code_loc+at++,(UCHAR)offset);
		// Trace register to our trace location
		write(code_loc+at++,(UCHAR)0xA3);
		write(code_loc+at,(int)trace_loc);
		at += 4;
		write(code_loc+at++,(UCHAR)0x58); // pop eax
	}

	// Put overwritten bytes back (full instruction(s))
	PMWRITE(proc,reinterpret_cast<void*>(code_loc+at),old_bytes,size+nop,0);
	at += (size+nop);

	// Place our JMP back
	write(code_loc+at++,(UCHAR)0xE9);
	write(code_loc+at,(address+5)-(code_loc+at+4));
	at += 4;


	// Inject the JMP to our own code
	PMWRITE(proc,reinterpret_cast<void*>(address),inject,size,0);
	for (int i=0; i<nop; i++) write(address+size+i,(UCHAR)0x90);
	delete[] inject;

	// Uncommenting this will cause debugging
	// to delay until it reads a value from
	// the register, but this is left out
	// due to memcheck.
	// (thankfully, alot of functions are used
	// rapidly, so a fast hook can still work)
	// 
	write(trace_loc,mask);

	// Wait for our masked value to be modified
	// This means something wrote to our location
	bool modified = false;
	while (modified == false){
		Sleep(10);
		value = readui(trace_loc);
		if (value != mask)
			modified = true;
		if (d++>2000) break; // dont debug for eternity
	}

	PMWRITE(proc,reinterpret_cast<void*>(address),old_bytes,size+nop,0);
	delete[] old_bytes;
	vfree(code_loc,32);
	vfree(trace_loc,4);
	return (value==mask)?0:value;
}

std::string EyeCrawl::util::to_str(UINT_PTR address) {
	std::string str = "";
	char c[16];
	sprintf_s(c, "%08X", address);
	str += c;
	return str;
}

std::string EyeCrawl::util::calltype(UINT_PTR func) {
	std::string			convention = "unknown";
	UINT_PTR			at = func,
						eof = (func+fsize(func)),
						cleanup = fretn(func),
						cur_instr = 0;
	bool				local_stack = false,
						ecx = false,
						ecx_abused = false;
	if (cleanup==0)		convention = "cdecl";
	else if (cleanup>0)	convention = "stdcall";

	std::vector<UCHAR>vars;
	while (at < eof){
		cur_instr++;
		pinstruction i = disassemble(at);
		if (strfind(i->data,"push ecx")) ecx=true;
		//if (i->r32[1] == R_ECX && !ecx)	 ecx_abused=true;
		if (strfind(i->data,",ecx") || strfind(i->data,",[ecx"))
			if (!ecx)
				ecx_abused = true;

		if (cur_instr == 3)
			if (strfind(i->data,"sub esp"))
				if (i->v8 == 0x10)
					local_stack = true;

		if (local_stack){
			if (i->r32[0] == R_EBP || i->r32[1] == R_EBP){
				UCHAR var = (UCHAR)(256-i->offset);
				if (i->offset >= 0x80 && var >= 0x8){
					bool found = false;
					for (UCHAR x : vars){
						found = (x == var);
						if (found) break;
					}
					if (!found) vars.push_back(var);
				}
			}
		}

		at += i->size;
		delete i;
	}

	if (vars.size()>0 && !ecx_abused){
		if (vars[0] == 0x8 || vars[1] == 0x8)
			convention = "thiscall";
		if (vars[0] == 0x10 || vars[1] == 0x10){
			convention = "fastcall";
			return convention;
		}
	}

	return convention;
}




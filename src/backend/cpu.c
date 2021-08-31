#include"cpu.h"
#include<stdbool.h>

#define PANIC assert(false)
#define TICK_M(cycles) cpu->t_cycles+=4*cycles;
#define SET_Z(state) cpu->f.z = (state) != 0
#define SET_N(state) cpu->f.n = (state) != 0
#define SET_H(state) cpu->f.h = (state) != 0
#define SET_C(state) cpu->f.c = (state) != 0

typedef enum{
    _V00h = 0x00,
    _V08h = 0x08,
    _V10h = 0x10,
    _V18h = 0x18,
    _V20h = 0x20,
    _V28h = 0x28,
    _V30h = 0x30,
    _V38h = 0x38
} RESET_VEC;

static uint8_t fetch(CPU* cpu){
    TICK_M(1);
    return read(cpu, cpu->pc++);
}

static uint8_t fetch16(CPU* cpu){
    uint16_t val = fetch(cpu);
    return val |= fetch(cpu)<<8;
}

static uint8_t cpuRead(CPU* cpu, uint16_t adr){
    TICK_M(1);
    return read(&cpu->memory, adr);
}

static void cpuWrite(CPU* cpu, uint16_t adr, uint8_t val){
    TICK_M(1);
    write(&cpu->memory, adr, val);
}

//  LOAD INSTRUCTIONS

static void LD_r8_r8(CPU* cpu, uint8_t* dest, uint8_t src){
    *dest = src;
}

static void LD_r8_u8(CPU* cpu, uint8_t* dest){
    *dest = fetch(cpu);
}

static void LD_r16_u16(CPU* cpu, uint16_t* dest){
    *dest = fetch16(cpu);
}

static void LD_pHL_r8(CPU* cpu, uint8_t src){
    cpuWrite(cpu, cpu->hl, src);
}

static void LD_pHL_u8(CPU* cpu){
    cpuWrite(cpu, cpu->hl, fetch(cpu));
}

static void LD_pu16_A(CPU* cpu){
    cpu->a = cpuRead(cpu, fetch16(cpu));
}

static void LDH_pu8_A(CPU* cpu){
    cpuWrite(cpu, 0xFF00+fetch(cpu), cpu->a);
}

static void LDH_pC_A(CPU* cpu){
    cpuWrite(cpu, 0xFF00+cpu->c, cpu->a);
}

static void LD_A_pr16(CPU* cpu, uint16_t adr){
    cpu->a = cpuRead(cpu, adr);
}

static void LD_pr16_A(CPU* cpu, uint16_t adr){
    cpuWrite(cpu, adr, cpu->a);
}


static void LD_A_pu16(CPU* cpu){
    cpu->a = cpuRead(cpu, fetch16(cpu));
}

static void LDH_A_pu8(CPU* cpu){
    cpu->a = cpuRead(cpu, 0xFF00+fetch(cpu));
}

static void LDH_A_pC(CPU* cpu){
    cpu->a = cpuRead(cpu, 0xFF00+cpu->c);
}

static void LD_pHLi_A(CPU* cpu){
    cpuWrite(cpu, cpu->hl++, cpu->a);
}

static void LD_pHLd_A(CPU* cpu){
    cpuWrite(cpu, cpu->hl--, cpu->a);
}

static void LD_A_pHLi(CPU* cpu){
    cpu->a = cpuRead(cpu, cpu->hl++);
}

static void LD_A_pHLd(CPU* cpu){
    cpu->a = cpuRead(cpu, cpu->hl--);
}

static void LD_SP_u16(CPU* cpu){
    cpu->sp = fetch16(cpu);
}

static void LD_pu16_SP(CPU* cpu){
    uint16_t adr = fetch16(cpu);
    cpuWrite(cpu, adr, cpu->sp);
    cpuWrite(cpu, adr+1, cpu->sp>>8);
}

static void LD_HL_SPi8(CPU* cpu){
    uint8_t imm = fetch(cpu);
    SET_Z(false);
    SET_N(false);
    SET_H((cpu->sp&0x0F) + (imm&0x0F) > 0x0F);
    SET_C((cpu->sp&0xFF) + imm > 0xFF);
    cpu->hl = cpu->sp+(int8_t)imm;
    TICK_M(1);
}

static void LD_SP_HL(CPU* cpu){
    cpu->sp = cpu->hl;
    TICK_M(1);
}

//  MISC INSTRUCTIONS

static void NOP(CPU* cpu){}

static void DAA(CPU* cpu){}

static void HALT(CPU* cpu){}

static void CB(CPU* cpu){}

static void CCF(CPU* cpu){}

static void DI(CPU* cpu){}

static void EI(CPU* cpu){}

static void SCF(CPU* cpu){}

//  ALU INSTRUCTIONS

/* ADC */
__always_inline void __ADC(CPU* cpu, uint8_t src, bool carry){
    SET_N(false);
    SET_H((cpu->a&0x0F) + (src&0x0F) + carry > 0x0F);
    SET_C(cpu->a + src + carry > 0xFF);
    cpu->a += src + carry;
    SET_Z(cpu->a == 0);
}

static void ADC_r8(CPU* cpu, uint8_t src){
    __ADC(cpu, src, cpu->f.c);
}

static void ADC_pHL(CPU* cpu){
    __ADC(cpu, cpuRead(cpu, cpu->hl), cpu->f.c);
}

static void ADC_u8(CPU* cpu){
    __ADC(cpu, fetch(cpu), cpu->f.c);
}

/* ADD */

static void ADD_r8(CPU* cpu, uint8_t src){
    __ADC(cpu, src, 0);
}

static void ADD_pHL(CPU* cpu){
    __ADC(cpu, cpuRead(cpu, cpu->hl), 0);
}

static void ADD_u8(CPU* cpu){
    __ADC(cpu, fetch(cpu), 0);
}

/* CP */

__always_inline void __CP(CPU* cpu, uint8_t src){
    SET_Z(cpu->a - src == 0);
    SET_N(true);
    SET_H((cpu->a&0x0F) < (src&0x0F));
    SET_C(cpu->a < src);
}

static void CP_r8(CPU* cpu, uint8_t src){
    __CP(cpu, src);
}

static void CP_pHL(CPU* cpu){
    __CP(cpu, cpuRead(cpu, cpu->hl));
}

static void CP_u8(CPU* cpu){
    __CP(cpu, fetch(cpu));
}

/* CPL */

static void CPL(CPU* cpu){
    cpu->a = ~cpu->a;
    set_N(true);
    SET_H(true);
}

/* DEC */

__always_inline void __DEC(CPU* cpu, uint8_t* dest){
    SET_N(true);
    SET_H((*dest&0x0F) < 1);
    --(*dest);
    SET_Z(*dest == 0);
}

static void DEC_r8(CPU* cpu, uint8_t* dest){
    __DEC(cpu, dest);
}

static void DEC_pHL(CPU* cpu){
    uint8_t val = cpuRead(cpu, cpu->hl);
    __DEC(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

static void DEC_r16(CPU* cpu, uint16_t* dest){
    --(*dest);
}

/* INC */

__always_inline void __INC(CPU* cpu, uint8_t* dest){
    SET_N(false);
    SET_H((*dest&0x0F) + 1 > 0x0F);
    ++(*dest);
    SET_Z(*dest == 0);
}

static void INC_r8(CPU* cpu, uint8_t* dest){
    __INC(cpu, dest);
}

static void INC_pHL(CPU* cpu){
    uint8_t val = cpuRead(cpu, cpu->hl);
    __INC(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

static void INC_r16(CPU* cpu, uint16_t* dest){
    ++(*dest);
}

/* SBC */

__always_inline __SBC(CPU* cpu, uint8_t src, bool carry){
    SET_N(true);
    SET_H((cpu->a&0x0F) < (src&0x0F) + carry);
    SET_C(cpu->a < src + carry);
    cpu->a -= src + carry;
    SET_Z(cpu->a == 0);
}

static void SBC_r8(CPU* cpu, uint8_t src){
    __SBC(cpu, src, cpu->f.c);
}

static void SBC_pHL(CPU* cpu){
    uint8_t val = cpuRead(cpu, cpu->hl);
    __SBC(cpu, val, cpu->f.c);
}

static void SBC_u8(CPU* cpu){
    __SBC(cpu, fetch(cpu), cpu->f.c);
}

/* SUB */

static void SUB_r8(CPU* cpu, uint8_t src){
    __SBC(cpu, src, 0);
}

static void SUB_pHL(CPU* cpu){
    uint8_t val = cpuRead(cpu, cpu->hl);
    __SBC(cpu, val, 0);
}

static void SUB_u8(CPU* cpu){
    __SBC(cpu, fetch(cpu), 0);
}

/* ADD */

static void ADD_HL_r16(CPU* cpu, uint16_t src){
    SET_N(false);
    SET_H((cpu->hl&0x0FFF) + (src&0x0FFF) > 0x0FFF);
    SET_C(cpu->hl + src > 0xFFFF); 
    cpu->hl += src;
}

static void ADD_SP_i8(CPU* cpu){
    uint8_t imm = fetch(cpu);
    SET_Z(false);
    SET_N(false);
    SET_H(cpu->sp&0x0F < imm&0x0F);
    SET_C(cpu->sp&0xFF < imm);
    cpu->sp += (int8_t)imm;
}

//  BIT OPERATIONS

/* BIT */

__always_inline void __BIT(CPU* cpu, uint8_t bit, uint8_t val){
    SET_Z(val&bit == 0);
    SET_N(false);
    SET_H(true);
}

static void BIT_u3_r8(CPU* cpu, uint8_t bit, uint8_t src){
    __BIT(cpu, bit, src);
}

static void BIT_u3_pHL(CPU* cpu, uint8_t bit){
    __BIT(cpu, bit, cpuRead(cpu, cpu->hl));
}

/* AND */

__always_inline void __AND(CPU* cpu, uint8_t src){
    SET_N(false);
    SET_H(true);
    SET_C(false);
    cpu->a &= src;
    SET_Z(cpu->a == 0);
}

static void AND_r8(CPU* cpu, uint8_t src){
    __AND(cpu, src);
}

static void AND_pHL(CPU* cpu){
    __AND(cpu, cpuRead(cpu, cpu->hl));
}

static void AND_u8(CPU* cpu){
    __AND(cpu, fetch(cpu));
}

/* OR */

__always_inline void __OR(CPU* cpu, uint8_t src){
    SET_N(false);
    SET_H(false);
    SET_C(false);
    cpu->a |= src;
    SET_Z(cpu->a == 0);
}

static void OR_r8(CPU* cpu, uint8_t src){
    
}

static void OR_pHL(CPU* cpu){}

static void OR_u8(CPU* cpu){}

static void RES_u3_r8(CPU* cpu){}

static void RES_u3_pHL(CPU* cpu){}

static void RL_r8(CPU* cpu){}

static void RL_pHL(CPU* cpu){}

static void RLA(CPU* cpu){}

static void RLC_r8(CPU* cpu){}

static void RLC_pHL(CPU* cpu){}

static void RLCA(CPU* cpu){}

static void RR_r8(CPU* cpu){}

static void RR_pHL(CPU* cpu){}

static void RRA(CPU* cpu){}

static void RRC_r8(CPU* cpu){}

static void RRC_pHL(CPU* cpu){}

static void RRCA(CPU* cpu){}

static void SET_u3_r8(CPU* cpu){}

static void SET_u3_pHL(CPU* cpu){}

static void SLA_r8(CPU* cpu){}

static void SLA_pHL(CPU* cpu){}

static void SRA_r8(CPU* cpu){}

static void SRA_pHL(CPU* cpu){}

static void SRL_r8(CPU* cpu){}

static void SRL_pHL(CPU* cpu){}

static void STOP(CPU* cpu){}

static void SWAP_r8(CPU* cpu){}

static void SWAP_pHL(CPU* cpu){}

static void XOR_r8(CPU* cpu, uint8_t src){}

static void XOR_pHL(CPU* cpu){}

static void XOR_u8(CPU* cpu){}

//  CONTROL OPERATIONS

static void CALL_u16(CPU* cpu){}

static void CALL_cc_u16(CPU* cpu, bool cond){}

static void JP_u16(CPU* cpu){}

static void JP_cc_u16(CPU* cpu, bool cond){}

static void JP_HL(CPU* cpu){}

static void JR_i8(CPU* cpu){}

static void JR_cc_i8(CPU* cpu, bool cond){}

static void RET(CPU* cpu){}

static void RET_cc(CPU* cpu, bool cond){}

static void RETI(CPU* cpu){}

static void RST(CPU* cpu, RESET_VEC vec){}

//  STACK OPERATIONS

static void POP_r16(CPU* cpu, uint16_t* dest){}

static void POP_AF(CPU* cpu){}

static void PUSH_r16(CPU* cpu, uint16_t src){}

uint8_t determineRegister(CPU* cpu, uint8_t opc){
    switch(opc%8)
    {
    case 0: return cpu->b;
    case 1: return cpu->c;
    case 2: return cpu->d;
    case 3: return cpu->e;
    case 4: return cpu->h;
    case 5: return cpu->l;
    case 6: return read(&cpu->memory, cpu->hl);
    case 7: return cpu->a;
    }
}

typedef void(*InstrFunc)(CPU*);
static InstrFunc fetchInstruction(CPU* cpu, uint8_t opcode){
    switch(opcode)
    {
    case 0x00: NOP(cpu);                            break;
    case 0x01: LD_r16_u16(cpu, &cpu->bc);           break;
    case 0x02: LD_pr16_A(cpu, cpu->bc);             break;
    case 0x03: INC_r16(cpu, &cpu->bc);              break;
    case 0x04: INC_r8(cpu, &cpu->b);                break;
    case 0x05: DEC_r8(cpu, &cpu->b);                break;
    case 0x06: LD_r8_u8(cpu, &cpu->b);              break;
    case 0x07: RLCA(cpu);                           break;
    case 0x08: LD_pu16_SP(cpu);                     break;
    case 0x09: ADD_HL_r16(cpu, cpu->bc);            break;
    case 0x0A: LD_A_pr16(cpu, cpu->bc);             break;
    case 0x0B: DEC_r16(cpu, &cpu->bc);              break;
    case 0x0C: INC_r8(cpu, &cpu->c);                break;
    case 0x0D: DEC_r8(cpu, &cpu->c);                break;
    case 0x0E: LD_r8_u8(cpu, &cpu->c);              break;
    case 0x0F: RRCA(cpu);                           break;

    case 0x10: STOP(cpu);                           break;
    case 0x11: LD_r16_u16(cpu, &cpu->de);           break;
    case 0x12: LD_pr16_A(cpu, cpu->de);             break;
    case 0x13: INC_r16(cpu, &cpu->de);              break;
    case 0x14: INC_r8(cpu, &cpu->d);                break;
    case 0x15: DEC_r8(cpu, &cpu->d);                break;
    case 0x16: LD_r8_u8(cpu, &cpu->d);              break;
    case 0x17: RLA(cpu);                            break;
    case 0x18: JR_i8(cpu);                          break;
    case 0x19: ADD_HL_r16(cpu, cpu->de);            break;
    case 0x1A: LD_A_pr16(cpu, cpu->de);             break;
    case 0x1B: DEC_r16(cpu, &cpu->de);              break;
    case 0x1C: INC_r8(cpu, &cpu->e);                break;
    case 0x1D: DEC_r8(cpu, &cpu->e);                break;
    case 0x1E: LD_r8_u8(cpu, &cpu->e);              break;
    case 0x1F: RRA(cpu);                            break;

    case 0x20: JR_cc_i8(cpu, !cpu->f.z);            break;
    case 0x21: LD_r16_u16(cpu, &cpu->hl);           break;
    case 0x22: LD_pHLi_A(cpu);                      break;
    case 0x23: INC_r16(cpu, &cpu->hl);              break;
    case 0x24: INC_r8(cpu, &cpu->h);                break;
    case 0x25: DEC_r8(cpu, &cpu->h);                break;
    case 0x26: LD_r8_u8(cpu, &cpu->h);              break;
    case 0x27: DAA(cpu);                            break;
    case 0x28: JR_cc_i8(cpu, cpu->f.z);             break;
    case 0x29: ADD_HL_r16(cpu, cpu->hl);            break;
    case 0x2A: LD_A_pHLi(cpu);                      break;
    case 0x2B: DEC_r16(cpu, &cpu->hl);              break;
    case 0x2C: INC_r8(cpu, &cpu->l);                break;
    case 0x2D: DEC_r8(cpu, &cpu->l);                break;
    case 0x2E: LD_r8_u8(cpu, &cpu->l);              break;
    case 0x2F: CPL(cpu);                            break;

    case 0x30: JR_cc_i8(cpu, !cpu->f.c);            break;
    case 0x31: LD_SP_u16(cpu);                      break;
    case 0x32: LD_pHLd_A(cpu);                      break;
    case 0x33: INC_r16(cpu, &cpu->sp);              break;
    case 0x34: INC_pHL(cpu);                        break;
    case 0x35: DEC_pHL(cpu);                        break;
    case 0x36: LD_pHL_u8(cpu);                      break;
    case 0x37: SCF(cpu);                            break;
    case 0x38: JR_cc_i8(cpu, cpu->f.c);             break;
    case 0x39: ADD_HL_r16(cpu, cpu->sp);            break;
    case 0x3A: LD_A_pHLd(cpu);                      break;
    case 0x3B: DEC_r16(cpu, &cpu->sp);              break;
    case 0x3C: INC_r8(cpu, &cpu->a);                break;
    case 0x3D: DEC_r8(cpu, &cpu->a);                break;
    case 0x3E: LD_r8_u8(cpu, &cpu->a);              break;
    case 0x3F: CCF(cpu);                            break;

    case 0x40 ... 0x47:
        LD_r8_r8(cpu, &cpu->b, determineRegister(cpu, opcode));
        break;
    case 0x48 ... 0x4F:
        LD_r8_r8(cpu, &cpu->c, determineRegister(cpu, opcode));
        break;
    case 0x50 ... 0x57:
        LD_r8_r8(cpu, &cpu->d, determineRegister(cpu, opcode));
        break;
    case 0x58 ... 0x5F:
        LD_r8_r8(cpu, &cpu->e, determineRegister(cpu, opcode));
        break;
    case 0x60 ... 0x67:
        LD_r8_r8(cpu, &cpu->h, determineRegister(cpu, opcode));
        break;
    case 0x68 ... 0x6F:
        LD_r8_r8(cpu, &cpu->l, determineRegister(cpu, opcode));
        break;
    case 0x70 ... 0x77:
        if(opcode%8 == 6)
            HALT(cpu);
        else 
            LD_pHL_r8(cpu, determineRegister(cpu, opcode));
        break;
    case 0x78 ... 0x7F:
        LD_r8_r8(cpu, &cpu->a, determineRegister(cpu, opcode));
        break;

    case 0x80 ... 0x87:
        ADD_r8(cpu, determineRegister(cpu, opcode));break;
    case 0x88 ... 0x8F:
        ADC_r8(cpu, determineRegister(cpu, opcode));break;
    case 0x90 ... 0x97:
        SUB_r8(cpu, determineRegister(cpu, opcode));break;
    case 0x98 ... 0x9F:
        SBC_r8(cpu, determineRegister(cpu, opcode));break;
    case 0xA0 ... 0xA7:
        AND_r8(cpu, determineRegister(cpu, opcode));break;
    case 0xA8 ... 0xAF:
        XOR_r8(cpu, determineRegister(cpu, opcode));break;
    case 0xB0 ... 0xB7:
        OR_r8(cpu, determineRegister(cpu, opcode)); break;
    case 0xB8 ... 0xBF:
        CP_r8(cpu, determineRegister(cpu, opcode)); break; 

    case 0xC0: RET_cc(cpu, !cpu->f.z);              break;
    case 0xC1: POP_r16(cpu, &cpu->bc);              break;
    case 0xC2: JP_cc_u16(cpu, !cpu->f.z);           break;
    case 0xC3: JP_u16(cpu);                         break;
    case 0xC4: CALL_cc_u16(cpu, !cpu->f.z);         break;
    case 0xC5: PUSH_r16(cpu, cpu->bc);              break;
    case 0xC6: ADD_u8(cpu);                         break;
    case 0xC7: RST(cpu, _V00h);                     break;
    case 0xC8: RET_cc(cpu, cpu->f.z);               break;
    case 0xC9: RET(cpu);                            break;
    case 0xCA: JP_cc_u16(cpu, cpu->f.z);            break;
    case 0xCB: CB(cpu);                             break; 
    case 0xCC: CALL_cc_u16(cpu, cpu->f.z);          break;
    case 0xCD: CALL_u16(cpu);                       break;
    case 0xCE: ADC_u8(cpu);                         break;
    case 0xCF: RST(cpu, _V08h);                     break;

    case 0xD0: RET_cc(cpu, !cpu->f.c);              break;
    case 0xD1: POP_r16(cpu, &cpu->de);              break;
    case 0xD2: JP_cc_u16(cpu, !cpu->f.c);           break;
    case 0xD3: PANIC;                               break;
    case 0xD4: CALL_cc_u16(cpu, !cpu->f.c);         break;
    case 0xD5: PUSH_r16(cpu, cpu->de);              break;
    case 0xD6: SUB_u8(cpu);                         break;
    case 0xD7: RST(cpu, _V10h);                     break;
    case 0xD8: RET_cc(cpu, cpu->f.c);               break;
    case 0xD9: RETI(cpu);                           break;
    case 0xDA: JP_cc_u16(cpu, cpu->f.c);            break;
    case 0xDB: PANIC;                               break;
    case 0xDC: CALL_cc_u16(cpu, cpu->f.c);          break;
    case 0xDD: PANIC;                               break;
    case 0xDE: SBC_u8(cpu);                         break;
    case 0xDF: RST(cpu, _V18h);                     break;

    case 0xE0: LDH_pu8_A(cpu);                      break;
    case 0xE1: POP_r16(cpu, &cpu->hl);              break;
    case 0xE2: LDH_pC_A(cpu);                       break;
    case 0xE3: PANIC;                               break;
    case 0xE4: PANIC;                               break;
    case 0xE5: PUSH_r16(cpu, cpu->hl);              break;
    case 0xE6: AND_u8(cpu);                         break;
    case 0xE7: RST(cpu, _V20h);                     break;
    case 0xE8: ADD_SP_i8(cpu);                      break;
    case 0xE9: JP_HL(cpu);                          break;
    case 0xEA: LD_pu16_A(cpu);                      break;
    case 0xEB: PANIC;                               break;
    case 0xEC: PANIC;                               break;
    case 0xED: PANIC;                               break;
    case 0xEE: XOR_u8(cpu);                         break;
    case 0xEF: RST(cpu, _V28h);                     break;

    case 0xF0: LDH_A_pu8(cpu);                      break;
    case 0xF1: POP_AF(cpu);                         break;
    case 0xF2: LDH_A_pC(cpu);                       break;
    case 0xF3: DI(cpu);                             break;
    case 0xF4: PANIC;                               break;
    case 0xF5: PUSH_r16(cpu, cpu->af);              break;
    case 0xF6: OR_u8(cpu);                          break;
    case 0xF7: RST(cpu, _V30h);                     break;
    case 0xF8: LD_HL_SPi8(cpu);                     break;
    case 0xF9: LD_SP_HL(cpu);                       break;
    case 0xFA: LD_A_pu16(cpu);                      break;
    case 0xFB: EI(cpu);                             break;
    case 0xFC: PANIC;                               break;
    case 0xFD: PANIC;                               break;
    case 0xFE: CP_u8(cpu);                          break;
    case 0xFF: RST(cpu, _V38h);                     break;

    default: 
        PANIC;
    }
}
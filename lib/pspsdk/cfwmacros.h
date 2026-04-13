/*
 * This file is part of PRO CFW.

 * PRO CFW is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PRO CFW is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PRO CFW. If not, see <http://www.gnu.org/licenses/ .
 */

#ifndef _CFW_MACROS_H_
#define _CFW_MACROS_H_

// magic numbers
#define PBP_MAGIC 0x50425000
#define ELF_MAGIC 0x464C457F
#define PSP_MAGIC 0x5053507E
#define BTCNF_MAGIC 0x0F803001
#define SFO_MAGIC 0x46535000

// Different firmware versions
#define FW_661 0x06060110
#define FW_660 0x06060010
#define FW_150 0x01050001

#define FLASH0_PATH "flash0:/"
#define FLASH1_PATH "flash1:/"
#define USER_BASE 0x08800000 // user partition (p2)
#define KERNEL_BASE 0x88000000 // kernel partition (p1)
#define GAME_TEXT (USER_BASE + 0x4000) // game's main elf load address
#define SYSMEM_TEXT KERNEL_BASE // sysmem.prx load address (start of kernel ram)
#define EXTRA_RAM 0x8A000000 // extra RAM (on 2k+ or vita)
#define VITA_FLASH_SONY 0x8B000000 // flash ramfs on vita
#define BOOT_KEY_BUFFER (KERNEL_BASE + 0xFB0000) // controller input in payloadex (cIPL)
#define USER_SIZE (24 * 1024 * 1024) // user partition size
#define KERNEL_SIZE (4 * 1024 * 1024) // kernel partition size
#define VITA_FLASH_SIZE 0x01000000 // vita flash ramfs size
#define EXTRA_RAM_SIZE (32 * 1024 * 1024) // size of extra ram (2k+)
#define MAX_HIGH_MEMSIZE 55 // max ram that can be given to user

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#define PTR_ALIGN_64(p) ((void*) ALIGN((u32)p, 64))

// Kernelify Address
#define KERNELIFY(f) (0x80000000 | ((unsigned int)(f)))
#define IS_KERNEL_ADDR(a) ((0x80000000&((unsigned int)(a))) != 0)

// j addr
#define JUMP(f) (0x08000000 | (((unsigned int)(f) >> 2) & 0x03ffffff))

// j addr getter (for kernel range, use in combination with KERNELIFY, works with j & jal)
#define JUMP_TARGET(i) (((unsigned int)(i) & 0x03ffffff) << 2)

// syscall number
#define SYSCALL(n) ((n<<6)|12)

// jal addr
#define JAL(f) (0x0C000000 | (((unsigned int)(f) >> 2) & 0x03ffffff))

// jal checker
#define IS_JAL(i) ((((unsigned int)i) & 0xFC000000) == 0x0C000000)

// nop
#define NOP 0

// Move from `zr` zero register to `v0` register (`move v0, zr`) instruction value
#define MOVE_V0_ZR 0x00001021

// jr ra
#define JR_RA 0x03E00008

// v0 result setter
#define LI_V0(n) ((0x2402 << 16) | ((n) & 0xFFFF))

#define MAKE_JUMP(a, f) _sw(JUMP(f), a);
#define MAKE_CALL(a, f) _sw(JAL(f), a);
#define MAKE_SYSCALL(a, n) _sw(SYSCALL(n), (u32)(a));

// Insert a instruction optcode `i` at the address `a`
#define MAKE_INSTRUCTION(a, i) _sw(i, a);

// Insert a no operation (`nop`) instruction at the address `a`
#define MAKE_NOP(a) _sw(NOP, a);

// Volatile read a u8 value from the address `addr`
#define VREAD8(addr) _lb(addr)
// Volatile read a u16 value from the address `addr`
#define VREAD16(addr) _lh(addr)
// Volatile read a u32 value from the address `addr`
#define VREAD32(addr) _lw(addr)
// Volatile read a u64 value from the address `addr`
#define VREAD64(addr) _ld(addr)

// Volatile write to `addr` the byte value of `val`
#define VWRITE8(addr, val) _sb(val, addr);
// Volatile write to `addr` the u16 value of `val`
#define VWRITE16(addr, val) _sh(val, addr);
// Volatile write to `addr` the u32 value of `val`
#define VWRITE32(addr, val) _sw(val, addr);
// Volatile write to `addr` the u64 value of `val`
#define VWRITE64(addr, val) _sd(val, addr);

#define MAKE_DUMMY_FUNCTION_RETURN_0(a) \
    _sw(JR_RA, a);\
    _sw(LI_V0(0), a + 4);\

#define MAKE_DUMMY_FUNCTION_RETURN_1(a) \
    _sw(JR_RA, a);\
    _sw(LI_V0(1), a + 4);\

// Array Element Counter
#define NELEMS(n) ((sizeof(n)) / sizeof(n[0]))

// is UID
#define IsUID(uid) ((uid > 0 && uid < 0x05000000) && ((uid & 1) == 1))

// Min & Max Macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#undef UNUSED
#define UNUSED(arg) ((void)(arg))

//by Bubbletune
#define U_EXTRACT_IMPORT(x) ((((u32)_lw((u32)x)) & ~0x08000000) << 2)
#define K_EXTRACT_IMPORT(x) (((((u32)_lw((u32)x)) & ~0x08000000) << 2) | 0x80000000)
#define U_EXTRACT_CALL(x) ((((u32)_lw((u32)x)) & ~0x0C000000) << 2)
#define K_EXTRACT_CALL(x) (((((u32)_lw((u32)x)) & ~0x0C000000) << 2) | 0x80000000)
#define K_EXTRACT_BRANCH(x) ((((((u32)_lw((u32)x)) & 0x0000FFFF) << 2) + x + 4) | 0x80000000)

// by Acid_Snake
// the opcode is filled with two 0's to the right and shifted to make it a byte long
#define GET_OPCODE(x) ((_lw(x) & 0xFC000000)>>24)
#define GET_FUNCTION_OPCODE(x) (_lw(x) & 0x3F)

#define MAKE_JUMP_PATCH(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a);

//by Davee
#define HIJACK_FUNCTION(a, f, p) \
{ \
    static u32 _pb_[5]; \
    _sw(_lw((u32)(a)), (u32)_pb_); \
    _sw(_lw((u32)(a) + 4), (u32)_pb_ + 4);\
    _sw(NOP, (u32)_pb_ + 8);\
    _sw(NOP, (u32)_pb_ + 16);\
    MAKE_JUMP_PATCH((u32)_pb_ + 12, (u32)(a) + 8); \
    _sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), (u32)(a)); \
    _sw(0, (u32)(a) + 4); \
    p = (void *)_pb_; \
}

#define REDIRECT_SYSCALL(a, f) \
    _sw(JR_RA, a); \
    _sw(SYSCALL(sceKernelQuerySystemCall(f)), a + 4);

#define MAKE_DUMMY_FUNCTION(a, r) \
{ \
    u32 func = a; \
    if(r == 0) \
    { \
        _sw(JR_RA, func); \
        _sw(0x00001021, func + 4); \
    } \
    else \
    { \
        _sw(JR_RA, func); \
        _sw(0x24020000 | r, func + 4); \
    } \
}

#define REDIRECT_FUNCTION(a, f) \
{ \
    _sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), (u32)(a)); \
    _sw(0, (u32)(a) + 4); \
}

// from adrenaline
#define FW_TO_FIRMWARE(f) ((((f >> 8) & 0xF) << 24) | (((f >> 4) & 0xF) << 16) | ((f & 0xF) << 8) | 0x10)
#define FIRMWARE_TO_FW(f) ((((f >> 24) & 0xF) << 8) | (((f >> 16) & 0xF) << 4) | ((f >> 8) & 0xF))

#define MAKE_SYSCALL_FUNCTION(a, n) \
{ \
    _sw(JR_RA, (u32)(a)); \
    _sw(SYSCALL(n), (u32)(a)+4); \
}


#define K_HIJACK_CALL(a, f, ptr) \
{ \
    ptr = (void *)K_EXTRACT_CALL(a); \
    MAKE_CALL(a, f); \
}

#endif /* _CFW_MACROS_H_ */


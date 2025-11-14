#pragma once

// Sets read/write/execute permission on memory page of addr, copy data to addr, then restore permissions
void SafeWriteBuf(uintptr_t addr, void * data, size_t len); // Copy data of size len
void SafeWrite8(uintptr_t addr, UInt8 data);                // Copy a byte
void SafeWrite16(uintptr_t addr, UInt16 data);              // Copy 16 bits
void SafeWrite32(uintptr_t addr, UInt32 data);              // Copy 32 bits
void SafeWrite64(uintptr_t addr, UInt64 data);              // Copy 64 bits

// ### warning: if you try to branch more than +/- 2GB with these, they will fail and return false
// ### this is a limitation of the 'jmp' instruction and more generally the x64 ISA
// 5 bytes written to src
bool SafeWriteJump(uintptr_t src, uintptr_t dst); // Writes JMP at src to dest, returns whether successful
bool SafeWriteCall(uintptr_t src, uintptr_t dst); // Writes CALL at src to dest, returns whether successful

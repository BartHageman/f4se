#include "SafeWrite.h"

void SafeWriteBuf(uintptr_t addr, void *data, size_t len) {
  UInt32 oldProtect;

  VirtualProtect((void *)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
  memcpy((void *)addr, data, len);
  VirtualProtect((void *)addr, len, oldProtect, &oldProtect);
}

void SafeWrite8(uintptr_t addr, UInt8 data) {
  SafeWriteBuf(addr, &data, sizeof(data));
}

void SafeWrite16(uintptr_t addr, UInt16 data) {
  SafeWriteBuf(addr, &data, sizeof(data));
}

void SafeWrite32(uintptr_t addr, UInt32 data) {
  SafeWriteBuf(addr, &data, sizeof(data));
}

void SafeWrite64(uintptr_t addr, UInt64 data) {
  SafeWriteBuf(addr, &data, sizeof(data));
}

// Attempts to write a JMP or CALL (depending on op), at address src towards
// address dst.
//
// Returns false if the destination is more than 2GB away. (x86_64 architectural
// limitation)
//
// Returns true if successful.
//
// Internal function: not exposed externally.
static bool SafeWriteJump_Internal(uintptr_t src, uintptr_t dst, UInt8 op) {
// Don't add any padding between fields for alignment
// We need the bytes to be *exactly* these bytes, because we're going to write
// the struct as asm instructions
#pragma pack(push, 1)
  struct Code {
    UInt8 op;     // Operation, which is either JMP (E9) or CALL (E8)
    SInt32 displ; // Displacement or offset. (How far we jump forwards or
                  // backwards. JMPs are relative!)
  };
#pragma pack(pop)

  // If this is not exactly 5 bytes, we have done something wrong, and we
  // should stop right here, before we muck things up even further.
  STATIC_ASSERT(sizeof(Code) == 5);

  // Since JMPs are relative, we need to figure out the offset from the src
  // and dest addresses. Due to a limitation in x86_64, we can't jump more
  // than 2GB in either direction. If the distance is larger, we abort, and
  // return false
  ptrdiff_t delta = dst - (src + sizeof(Code));
  if ((delta < INT_MIN) || (delta > INT_MAX))
    return false;

  Code code;

  code.op = op;
  code.displ = delta;

  // If all is well, set up the struct and write the instructions to the source.
  SafeWriteBuf(src, &code, sizeof(code));

  return true;
}

bool SafeWriteJump(uintptr_t src, uintptr_t dst) {
  return SafeWriteJump_Internal(src, dst, 0xE9);
}

bool SafeWriteCall(uintptr_t src, uintptr_t dst) {
  return SafeWriteJump_Internal(src, dst, 0xE8);
}

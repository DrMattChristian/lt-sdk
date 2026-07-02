# Complex `#if 0` Blocks — Deferred for Roku Review

These two `#if 0` blocks were identified during the code audit but require
input from Roku before removal.

## 1. LTNetX509Types.h:160 — Struct field

```c
typedef struct LTX509Cert {
    // ... other fields ...
    #if 0
    LTAsn1Sequence           certificatePolicies;
    #endif
} LTX509Cert;
```

Removing this changes the memory layout of `LTX509Cert`. This may affect
serialization/deserialization of certificate objects. Roku needs to confirm
whether this field can safely be removed.

## 2. LTKArchArmCortexM_Main.h:80 — Scheduler Lock/Unlock

```c
#if 0
LT_INLINE u32 LockScheduler(void) {
    extern u8 _LTK_nInterruptPriMaskLow;
    asm volatile (
        "msr basepri, %0\n"
        "isb"
            : : "r" (_LTK_nInterruptPriMaskLow) : "memory"
    );
    return 0;
}

LT_INLINE void UnlockScheduler(u32 nMask) {
    asm volatile (
        "msr basepri, %0\n"
        "isb"
            : : "r" (nMask) : "memory"
    );
}
#endif
```

This ARM Cortex-M inline assembly may be needed for scheduler functionality.
Roku needs to determine whether these functions are dead code or should be
kept for future use.

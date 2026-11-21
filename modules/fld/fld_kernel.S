/* FLD kernel linked into the program as read-only data. shared.mk builds this file with FLD_KERNEL set to the
   kernel binary (SRL_FLD_KERNEL) when SRL_USE_FLD = 1; SRL::Fld::ExecGame copies it to 0x00200000. */
    .section .rodata
    .align 4
    .global _FldKernel
    .global _FldKernelEnd
_FldKernel:
    .incbin FLD_KERNEL
_FldKernelEnd:

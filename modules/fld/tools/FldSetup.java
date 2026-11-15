// Ghidra pre-analysis script for SEGA's FLD kernel (fld_knl.bin loaded raw at 0x00200000, SuperH:BE:32:SH-2).
// Marks the entry pointer and FLD_1stRead, creates a function at every call target found in the literal pools, and
// names the fixed addresses the kernel uses.
// @category SRL

import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.SourceType;

public class FldSetup extends GhidraScript {
    private void label(long offset, String name) throws Exception {
        createLabel(toAddr(offset), name, true, SourceType.USER_DEFINED);
    }

    private void function(Address address) throws Exception {
        if (getFunctionAt(address) == null) {
            disassemble(address);
            new CreateFunctionCmd(address).applyTo(currentProgram, monitor);
        }
    }

    @Override
    public void run() throws Exception {
        Memory memory = currentProgram.getMemory();
        MemoryBlock image = memory.getBlocks()[0];

        // The image is never written: the decompiler then reads literal pools as constants, so calls through them
        // show the called function
        image.setWrite(false);
        long end = image.getEnd().getOffset() + 1;

        // Variables right after the image
        MemoryBlock vars = memory.createUninitializedBlock("vars", toAddr(end), 0x100, false);
        vars.setWrite(true);

        Address base = toAddr(0x00200000L);
        createDWord(base);
        label(0x00200000L, "fld_entry_table");
        Address entry = toAddr(getInt(base) & 0xffffffffL);
        disassemble(entry);
        createFunction(entry, "FLD_1stRead");

        // Every pointer into the image found in a literal pool is a call target (data is copied, never pointed to)
        for (long a = entry.getOffset() & ~3L; a + 4 <= end; a += 4) {
            long value = getInt(toAddr(a)) & 0xffffffffL;
            if (value >= entry.getOffset() && value < end && (value & 1) == 0 && getInstructionAt(toAddr(a)) == null) {
                function(toAddr(value));
            }
        }

        // Fixed addresses outside the image
        label(0x0600026CL, "BIOS_CD_PLAYER_VECTOR");
        label(0x060002B0L, "BIOS_1ST_READ_ADDRESS");
        label(0x06000300L, "BIOS_SYS_SETUINT");
        label(0x06000CCCL, "DDS_ID");
        label(0x06000CCDL, "DDS_TNO");
        label(0x06000CCEL, "DDS_FID");
        label(0x06002000L, "IP_HEADER");
        label(0x25890008L, "CD_HIRQ");
        label(0x2589000CL, "CD_HIRQ_MASK");
        label(0x25890018L, "CD_CR1");
        label(0x2589001CL, "CD_CR2");
        label(0x25890020L, "CD_CR3");
        label(0x25890024L, "CD_CR4");
        label(0x25898000L, "CD_DATA");
    }
}

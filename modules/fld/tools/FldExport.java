// Ghidra post-analysis script: writes every function of the program, decompiled, to the file given as argument.
// @category SRL

import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;

public class FldExport extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);

        try (PrintWriter out = new PrintWriter(args[0])) {
            for (Function function : currentProgram.getFunctionManager().getFunctions(true)) {
                DecompileResults result = decompiler.decompileFunction(function, 60, monitor);
                out.printf("// %s @ %s, %d bytes%n", function.getName(), function.getEntryPoint(), function.getBody().getNumAddresses());
                out.println(result.decompileCompleted() ? result.getDecompiledFunction().getC() : "// decompilation failed: " + result.getErrorMessage());
            }
        }
    }
}

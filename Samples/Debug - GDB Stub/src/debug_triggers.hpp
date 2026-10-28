#pragma once

#include <srl.hpp>

/**
 * @brief Things to debug: one function per SH-2 exception vector, a function to
 * single-step, a variable to watch, and the `monitor <text>` dispatcher that
 * fires them from GDB.
 */

/**
 * @brief Variable to watch and write from GDB.
 * @details Try `watch g_testVariable`, `continue`, then press D-Pad Down
 * (or `monitor touch`). Or `set variable g_testVariable = 99`.
 */
extern volatile int32_t g_testVariable;

/** @brief Vector 4, Illegal Instruction: executes opcode 0xFFFF. */
[[noreturn]] void CrashProgram();

/** @brief Vector 5, Reserved Instruction: executes reserved opcode 0xFFFD. */
void ReservedInstruction();

/** @brief Vector 6, Slot Illegal Instruction: an illegal opcode in a branch delay slot. */
void SlotIllegalInstruction();

/** @brief Vector 7, General Illegal Instruction: executes undefined opcode 0xFFFC. */
void GeneralIllegalInstruction();

/** @brief Vector 8, Slot Reserved Instruction: a reserved opcode in a branch delay slot. */
void SlotReservedInstruction();

/**
 * @brief Vector 9, CPU Address Error: a 32-bit read from an address not aligned to 4.
 * @note Does not fault on the test hardware (the exception thunk count stays at 0).
 */
void CPUAddressError();

/** @brief Vector 10, DMA Address Error: starts DMAC channel 0 with a misaligned source. */
void DMAAddressError();

/** @brief Vector 12, User Break: sets UBC channel A to break where this function returns to. */
void UserBreakController();

/** @brief Vector 35, TRAPA #35: GDB reports SIGTRAP at the instruction after the trapa. */
void Trapa35();

/** @brief Straight-line code with no branches: an easy target for `step` and `next`. */
void SteppableFunction();

/**
 * @brief Runs every SH-2 branch kind once, for the test suite to `stepi` through.
 * @details Written in assembly; every instruction a step lands on has an sb_* label.
 */
extern "C" void StepBranches();

/** @brief Runs the trigger named by a `monitor <text>` command, e.g. `monitor crash illegal`. */
void HandleMonitorCommand(const char* cmd);

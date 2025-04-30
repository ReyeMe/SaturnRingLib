#pragma once
/**
 * @file srl_hardware.hpp
 * @brief Saturn Hardware Access - Convenience Header
 * 
 * This is a convenience header that includes all the component-specific hardware headers.
 * Each hardware component (VDP1, VDP2, SCU, etc.) is now defined in its own dedicated
 * header file in the hardware/ directory.
 *
 * The hardware access is organized into the following namespaces:
 * - SRL::Hardware::VDP1 - Video Display Processor 1
 * - SRL::Hardware::VDP2 - Video Display Processor 2
 * - SRL::Hardware::SCU - System Control Unit
 * - SRL::Hardware::CDBlock - CD Block
 * - SRL::Hardware::SMPC - System Manager & Peripheral Control
 * 
 * Common definitions like the Reg<T> template and memory region base addresses
 * are defined in srl_hardware_common.hpp.
 * 
 * @note For more targeted includes, you can directly include the specific component
 * headers instead of this convenience header.
 */

// Include the component-specific headers
#include "hardware/srl_hardware_common.hpp"
#include "hardware/srl_cpu.hpp"
#include "hardware/srl_vdp1.hpp"
#include "hardware/srl_vdp2.hpp"
#include "hardware/srl_scu.hpp"
#include "hardware/srl_cd_block.hpp"
#include "hardware/srl_smpc.hpp"
#include "hardware/srl_ram_cartridge.hpp"

/**
 * @brief Usage Examples
 * 
 * The hardware components can be accessed through their respective namespaces:
 * 
 * ```cpp
 * // VDP1 register access
 * SRL::Hardware::VDP1::TvMode = 0x0001;
 * 
 * // VDP2 register access
 * auto bgMode = SRL::Hardware::VDP2::BgEnable;
 * 
 * // SCU register access
 * SRL::Hardware::SCU::InterruptMask = 0x0000;
 * 
 * // CD Block register access
 * SRL::Hardware::CDBlock::Command = 0x00;
 * 
 * // SMPC register access
 * auto status = SRL::Hardware::SMPC::Status;
 * ```
 */

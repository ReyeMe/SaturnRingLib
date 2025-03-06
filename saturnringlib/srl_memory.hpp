#pragma once

#include "srl_base.hpp"

extern "C" {
    extern char _heap_start;
    extern char _heap_end;
}

#include <tlsf.h>
#include <stdlib.h>

namespace SRL
{
    /** @brief Dynamic memory management
     */
    class Memory
    {
    public:
    
        /** @brief Contains report of the state
         */
        struct Report
        {
            /** @brief Total size of memory used for allocation headers
             */
            size_t AllocationHeaders;

            /** @brief Number of available free blocks
             */
            size_t FreeBlocks;

            /** @brief Total size of free available memory
             */
            size_t FreeSize;

            /** @brief Total memory zone size
             */
            size_t TotalSize;
            
            /** @brief Number of allocated blocks
             */
            size_t UsedBlocks;
        };

    private:
        /** @brief Memory zone definition
         */
        struct MemoryZone
        {
            /** @brief Memory zone start address
             * @note Address must be 4 byte aligned
             */
            void* Address;

            /** @brief Memory zone size in number of bytes
             * @note Size must be 4 byte aligned, if unaligned size will be specified, it will get rounded down to nearest alignment
             */
            size_t Size;
        };

        /** @brief Check if pointer is inside this memory zone
         * @param ptr Pointer to check
         * @return true if is inside the memory zone
         */
        inline static constexpr bool InZone(const MemoryZone& zone, void* ptr)
        {
            return (ptr >= (void*)zone.Address && ptr <= (char*)zone.Address + zone.Size);
        }

        /** @brief Reye's simple malloc
         */
        class SimpleMalloc
        {
        private:

            /** @brief State of the block
             */
            enum class BlockState : size_t
            {
                /** @brief Block is not used
                 */
                Free = 0,

                /** @brief Block is in use
                 */
                Used = 1
            };

            /** @brief Block header
             */
            struct Header
            {
                /** @brief Block state (0=Free, 1=Allocated)
                 */
                BlockState State : 1;

                /** @brief Block size (without header)
                 */
                size_t Size : 31;
            };

            /** @brief Get location of the next block in memory
             * @param zone Memory zone settings
             * @param currentBlock Current block location
             * @return Location of the next block in memory
             */
            inline static constexpr size_t GetNextBlockLocation(const Memory::MemoryZone& zone, size_t currentBlock)
            {
                return currentBlock + sizeof(SimpleMalloc::Header) + ((SimpleMalloc::Header*)&((uint8_t*)zone.Address)[currentBlock])->Size;
            }

            /** @brief Merges all free memory blocks until allocated block
             * @param zone Memory zone settings
             * @param startBlock Starting free block
             */
            inline static void MergeFreeMemoryBlocks(const Memory::MemoryZone& zone, size_t startBlock)
            {
                size_t next = startBlock;

                // Check if we are inside the array
                if (startBlock < zone.Size)
                {
                    // Check if current block is free
                    SimpleMalloc::Header* currentBlockHead = ((SimpleMalloc::Header*)&((uint8_t*)zone.Address)[startBlock]);

                    // Check if the block is free
                    if (currentBlockHead->State == SimpleMalloc::BlockState::Free)
                    {
                        // Search memory for free blocks
                        size_t location = SimpleMalloc::GetNextBlockLocation(zone, startBlock);
                        size_t freeMem = 0;

                        while (location < zone.Size)
                        {
                            // Gets header of the current block
                            SimpleMalloc::Header* header = ((SimpleMalloc::Header*)&((uint8_t*)zone.Address)[location]);

                            // Check if the block is free
                            if (header->State == SimpleMalloc::BlockState::Free)
                            {
                                freeMem += header->Size + sizeof(SimpleMalloc::Header);
                            }
                            else
                            {
                                break;
                            }

                            // Move to next block
                            location = SimpleMalloc::GetNextBlockLocation(zone, location);
                        }

                        currentBlockHead->Size += freeMem;
                    }
                }
            }
            
            /** @brief Try to mark block as allocated and split it if not allocated fully
             * @param zone Memory zone settings
             * @param location Block location
             * @param size New block size
             * @return True if block was successfully allocated
             */
            inline static bool SetBlockAllocation(const Memory::MemoryZone& zone, size_t location, size_t size)
            {
                // Gets block header
                SimpleMalloc::Header* header = ((SimpleMalloc::Header*)&((uint8_t*)zone.Address)[location]);

                // Store size of new block
                size_t newBlock = size;

                // If next block size is smaller than four bytes
                if (header->Size - newBlock <= (sizeof(SimpleMalloc::Header) << 1) &&
                    header->Size >= newBlock)
                {
                    newBlock = header->Size;
                }

                // Can we fit in this block?
                if (header->Size == newBlock)
                {
                    header->State = SimpleMalloc::BlockState::Used;
                }
                else if (header->Size > newBlock)
                {
                    // Save old block size for later use
                    size_t oldBlockSize = header->Size;

                    // Set as allocated
                    header->State = SimpleMalloc::BlockState::Used;
                    header->Size = newBlock;

                    // Set next block as free, this splits current block into two
                    // First part is allocated memory, second part is left over free memory
                    if (oldBlockSize - newBlock > 0)
                    {
                        size_t nextBlock = SimpleMalloc::GetNextBlockLocation(zone, location);
                        ((SimpleMalloc::Header*)&((uint8_t*)zone.Address)[nextBlock])->State = SimpleMalloc::BlockState::Free;
                        ((SimpleMalloc::Header*)&((uint8_t*)zone.Address)[nextBlock])->Size = oldBlockSize - newBlock - sizeof(SimpleMalloc::Header);
                    }

                    return true;
                }

                return false;
            }
        public:

            /** @brief Free memory
             * @param zone Memory zone settings
             * @param ptr Allocated memory
             */
            inline static void Free(const MemoryZone& zone, void* ptr)
            {
                // Validate pointer to not be null and be in zone
                if (ptr != nullptr && Memory::InZone(zone, ptr))
                {
                    // Gets offset to memory array
                    size_t location = reinterpret_cast<uint32_t>(ptr) - reinterpret_cast<uint32_t>(zone.Address);

                    // Check if offset is valid, we do not need to check whether location is 0, since first 4 bytes are always header
                    if (location > 0 && location < zone.Size && (location & 3) == 0)
                    {
                        // Flag area as free
                        ((SimpleMalloc::Header*)&((uint8_t*)zone.Address)[location - sizeof(SimpleMalloc::Header)])->State = SimpleMalloc::BlockState::Free;

                        // Merge areas
                        SimpleMalloc::MergeFreeMemoryBlocks(zone, location - sizeof(SimpleMalloc::Header));
                    }
                }
            }

            /** @brief Get report on the allocator in specified memory zone
             * @param zone Memory zone
             * @return State report
             */
            inline static const Report GetReport(const MemoryZone& zone)
            {
                size_t location = 0;
                auto report = Report { 0, 0, 0, zone.Size, 0 };

                while (location < zone.Size)
                {
                    SimpleMalloc::Header* header = ((SimpleMalloc::Header*)&((uint8_t*)zone.Address)[location]);
                    report.AllocationHeaders += sizeof(SimpleMalloc::Header);

                    if (header->State == SimpleMalloc::BlockState::Free)
                    {
                        report.FreeBlocks++;
                        report.FreeSize += header->Size;
                    }
                    else
                    {
                        report.UsedBlocks++;
                    }

                    location = SimpleMalloc::GetNextBlockLocation(zone, location);
                }

                return report;
            }

            /** @brief Initializes a new memory zone and returns its starting address
             * @param zone Memory zone
             * @return Zone start address
             */
            inline static void* InitializeZone(void* start, const size_t size)
            {
                SimpleMalloc::Header* header = reinterpret_cast<SimpleMalloc::Header*>(start);
                header->Size = size - sizeof(SimpleMalloc::Header);
                header->State = BlockState::Free;
                return start;
            }

            /** @brief Allocate memory
             * @param zone Memory zone settings
             * @param size Number of bytes to allocate
             * @return Pointer to allocated space
             */
            inline static void* Malloc(const MemoryZone& zone, size_t size)
            {
                // Align to 4
                size_t length = size;
                size_t align = length & 3;

                if (align != 0)
                {
                    length += 4 - align;
                }

                // Find free space
                size_t location = 0;
                size_t newBlock = length & 0x7fffffff;

                // We try until we reach end of available space
                while (location < zone.Size)
                {
                    // Gets header of the current block
                    SimpleMalloc::Header* header = ((SimpleMalloc::Header*)&((uint8_t*)zone.Address)[location]);

                    // Check if the block is free
                    if (header->State == SimpleMalloc::BlockState::Free)
                    {
                        // Merge free areas to create one big free space from many small neighboring ones
                        SimpleMalloc::MergeFreeMemoryBlocks(zone, location);

                        // Can we fit in this block?
                        if (SimpleMalloc::SetBlockAllocation(zone, location, newBlock))
                        {
                            // Return pointer to allocated block
                            return (void*)&((uint8_t*)zone.Address)[location + sizeof(SimpleMalloc::Header)];
                        }
                    }

                    // Move to next block
                    location = SimpleMalloc::GetNextBlockLocation(zone, location);
                }

                // We could not allocate anything
                return nullptr;
            }

            /** @brief Reallocate memory (can either shrink, enlarge or move)
             * @param zone Memory zone settings
             * @param ptr Allocated memory to resize
             * @param size New size of the allocated block
             * @return void* Pointer to resized or moved block
             */
            inline static void* Realloc(const MemoryZone& zone, void* ptr, size_t size)
            {
                // Validate pointer to not be null and be in zone
                if (ptr != nullptr && Memory::InZone(zone, ptr))
                {
                    // This one is located in the free() function as well, but I do not want to repeat its insides here again 
                    size_t location = reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(zone.Address);
                    size_t headerLocation = location - sizeof(SimpleMalloc::Header);

                    // We free the block first, this will also join it will all free blocks right after it
                    SimpleMalloc::Free(zone, ptr);

                    // Now we check if we can fit inside the new created space
                    if (SimpleMalloc::SetBlockAllocation(zone, headerLocation, size))
                    {
                        // We fit inside
                        return (void*)&((uint8_t*)zone.Address)[location];
                    }
                    else
                    {
                        // We do not fit, try to find space elsewhere
                        void* newSpace = SimpleMalloc::Malloc(zone, size);

                        // Copy data to the new location
                        slDMACopy(ptr, newSpace, ((SimpleMalloc::Header*)&((uint8_t*)zone.Address)[headerLocation])->Size);

                        // Return address to new thing
                        return newSpace;
                    }
                }

                return nullptr;
            }
        };
        
    public:

        /** @brief Memory zone codes
         */
        enum Zone
        {
            /** @brief High (main) system RAM
             */
            HWRam = 0,

            /** @brief Low system RAM
             */
            LWRam = 1,

            /** @brief Expansion cart RAM
             */
            CartRam = 2
        };

        /** @brief Malloc for main system RAM
         */
        class HighWorkRam
        {
        private:
        
            /** @brief Memory class needs to be able to see private members to initialize zones
             */
            friend class Memory;

            /** @brief Main system memory zone
             * @note This is assigned on system initialization
             */
            inline static MemoryZone zone;

            /** @brief Full main system memory zone
             */
            inline static const MemoryZone fullZone = { (void*)0x06000000, 0x07FFFFFF - 0x06000000};

            /** @brief Initialize memory zone
             */
            inline static void Initialize()
            {
                auto address = reinterpret_cast<void*>(&_heap_start);
                auto size = reinterpret_cast<size_t>(&_heap_end) - reinterpret_cast<size_t>(&_heap_start);

                #if defined(USE_TLSF_ALLOCATOR)
                HighWorkRam::zone = Memory::MemoryZone
                {
                    tlsf_create_with_pool(address, size),
                    size
                };
                #else
                HighWorkRam::zone = Memory::MemoryZone
                {
                    Memory::SimpleMalloc::InitializeZone(address, size),
                    size
                };
                #endif
            }
            
        public:

            /** @brief Check whether pointer is in range of the memory zone
             * @param ptr Pointer to check
             * @return true if pointer belongs to the current memory zone
             */
            static bool InRange(void* ptr)
            {
                return Memory::InZone(HighWorkRam::fullZone, ptr);
            }

            /** @brief Check whether pointer is in range of the memory zone
             * @param ptr Pointer to check
             * @return true if pointer belongs to the current memory zone
             */
            static bool InRange(uint32_t ptr)
            {
                return Memory::InZone(HighWorkRam::fullZone, (void*)ptr);
            }

            /** @brief Free allocated memory
             * @param ptr Pointer to allocated memory
             */
            static void Free(void* ptr)
            {
                #if defined(USE_TLSF_ALLOCATOR)
                tlsf_free(Memory::mainWorkRam.Address, ptr);
                #else
                Memory::SimpleMalloc::Free(HighWorkRam::zone, ptr);
                #endif
            }

            /** @brief Allocate some memory
             * @param size Number of bytes to allocate
             * @return Pointer to the allocated space in memory
             */
            static void* Malloc(size_t size)
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return tlsf_malloc(Memory::mainWorkRam.Address, size);
                #else
                return Memory::SimpleMalloc::Malloc(HighWorkRam::zone, size);
                #endif
            }

            /** @brief Reallocate existing memory
             * @param ptr Pointer to the existing allocated memory
             * @param size New size in number of bytes that should be allocated
             * @return Pointer to the allocated space in memory
             */
            static void* Realloc(void* ptr, size_t size)
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return tlsf_realloc(Memory::mainWorkRam.Address, size);
                #else
                return Memory::SimpleMalloc::Realloc(HighWorkRam::zone, ptr, size);
                #endif
            }

            /** @brief Gets total size of the free space in the memory zone
             * @return Number of bytes
             */
            static size_t GetFreeSpace()
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return 0;
                #else
                return Memory::SimpleMalloc::GetReport(HighWorkRam::zone).FreeSize;
                #endif
            }

            /** @brief Gets report on the allocator state
             * @return Current state of the allocator
             */
            static const Report GetReport()
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return Report { 0, 0, 0, Memory::mainWorkRam.Size, 0};
                #else
                return Memory::SimpleMalloc::GetReport(HighWorkRam::zone);
                #endif
            }

            /** @brief Gets total size of the memory zone
             * @return Number of bytes
             */
            static size_t GetSize()
            {
                return HighWorkRam::zone.Size;
            }
            
            /** @brief Gets total size of the used space in the memory zone
             * @return Number of bytes
             */
            static size_t GetUsedSpace()
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return 0;
                #else
                auto report = Memory::SimpleMalloc::GetReport(HighWorkRam::zone);
                return report.TotalSize - report.FreeSize;
                #endif
            }

        };

        /** @brief Malloc for slower system RAM
         */
        class LowWorkRam
        {
        private:

            /** @brief Memory class needs to be able to see private members to initialize zones
             */
            friend class Memory;

            /** @brief Memory zone
             */
            inline static Memory::MemoryZone zone;

            /** @brief Initialize memory zone
             */
            inline static void Initialize()
            {
                const volatile void* address = (void*)0x00200000;
                const uint32_t size = 0x100000;

                #if defined(USE_TLSF_ALLOCATOR)
                LowWorkRam::mainWorkRam = Memory::MemoryZone
                {
                    tlsf_create_with_pool((void*)address, size),
                    size
                };
                #else
                LowWorkRam::zone = Memory::MemoryZone
                {
                    Memory::SimpleMalloc::InitializeZone((void*)address, size),
                    size
                };
                #endif
            }
            
        public:

            /** @brief Check whether pointer is in range of the memory zone
             * @param ptr Pointer to check
             * @return true if pointer belongs to the current memory zone
             */
            inline static bool InRange(void* ptr)
            {
                return Memory::InZone(LowWorkRam::zone, ptr);
            }

            /** @brief Check whether pointer is in range of the memory zone
             * @param zoneAddress Zone address to check
             * @return true if pointer belongs to the current memory zone
             */
            inline static bool InRange(uint32_t zoneAddress)
            {
                return Memory::InZone(LowWorkRam::zone, (void*)zoneAddress);
            }

            /** @brief Free allocated memory
             * @param ptr Pointer to allocated memory
             */
            inline static void Free(void* ptr)
            {
                #if defined(USE_TLSF_ALLOCATOR)
                tlsf_free(LowWorkRam::Zone.Address, ptr);
                #else
                Memory::SimpleMalloc::Free(LowWorkRam::zone, ptr);
                #endif
            }

            /** @brief Allocate some memory
             * @param size Number of bytes to allocate
             * @return Pointer to the allocated space in memory
             */
            inline static void* Malloc(size_t size)
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return tlsf_malloc(LowWorkRam::Zone.Address, size);
                #else
                return Memory::SimpleMalloc::Malloc(LowWorkRam::zone, size);
                #endif
            }

           /** @brief Reallocate existing memory
            * @param ptr Pointer to the existing allocated memory
            * @param size New size in number of bytes that should be allocated
            * @return Pointer to the allocated space in memory
            */
            inline static void* Realloc(void* ptr, size_t size)
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return tlsf_realloc(LowWorkRam::Zone.Address, ptr, size);
                #else
                return Memory::SimpleMalloc::Realloc(LowWorkRam::zone, ptr, size);
                #endif
            }

            /** @brief Gets total size of the free space in the memory zone
             * @return Number of bytes
             */
            static size_t GetFreeSpace()
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return 0;
                #else
                return Memory::SimpleMalloc::GetReport(LowWorkRam::zone).FreeSize;
                #endif
            }

            /** @brief Gets report on the allocator state
             * @return Current state of the allocator
             */
            static const Report GetReport()
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return Report { 0, 0, 0, LowWorkRam::Zone.Size, 0};
                #else
                return Memory::SimpleMalloc::GetReport(LowWorkRam::zone);
                #endif
            }

            /** @brief Gets total size of the memory zone
             * @return Number of bytes
             */
            inline static size_t GetSize()
            {
                return LowWorkRam::zone.Size;
            }
            
            /** @brief Gets total size of the used space in the memory zone
             * @return Number of bytes
             */
            static size_t GetUsedSpace()
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return 0;
                #else
                auto report = Memory::SimpleMalloc::GetReport(LowWorkRam::zone);
                return report.TotalSize - report.FreeSize;
                #endif
            }
        };

        /** @brief Malloc for expansion cart RAM
         */
        class CartRam
        {
        private:

            /** @brief Memory class needs to be able to see private members to initialize zones
             */
            friend class Memory;

            /** @brief Memory zone
             */
            inline static Memory::MemoryZone zone;

            /** @brief Initialize memory zone
             */
            inline static void Initialize()
            {
                // TODO: Implement
            }
            
        public:

            /** @brief Check whether pointer is in range of the memory zone
             * @param ptr Pointer to check
             * @return true if pointer belongs to the current memory zone
             */
            inline static bool InRange(void* ptr)
            {
                return CartRam::InRange(reinterpret_cast<uint32_t>(ptr));
            }

            /** @brief Check whether pointer is in range of the memory zone
             * @param zoneAddress Address in the memory zone where object should be allocated
             * @return true if pointer belongs to the current memory zone
             */
            inline static bool InRange(uint32_t zoneAddress)
            {
                return false;
            }

            /** @brief Free allocated memory
             * @param ptr Pointer to allocated memory
             */
            inline static void Free(void* ptr)
            {
                // TODO implement
            }

            /** @brief Allocate some memory
             * @param size Number of bytes to allocate
             * @return Pointer to the allocated space in memory
             */
            inline static void* Malloc(size_t size)
            {
                return nullptr;
            }

            /** @brief Reallocate existing memory
             * @param ptr Pointer to the existing allocated memory
             * @param size New size in number of bytes that should be allocated
             * @return Pointer to the allocated space in memory
             */
            inline static void* Realloc(void* ptr, size_t size)
            {
                return nullptr;
            }

            /** @brief Gets total size of the free space in the memory zone
             * @return Number of bytes
             */
            inline static size_t GetFreeSpace()
            {
                return 0;
            }

            /** @brief Gets report on the allocator state
             * @return Current state of the allocator
             */
            static const Report GetReport()
            {
                #if defined(USE_TLSF_ALLOCATOR)
                return Report { 0, 0, 0, 0, 0};
                #else
                return Report { 0, 0, 0, 0, 0};
                #endif
            }

            /** @brief Gets total size of the memory zone
             * @return Number of bytes
             */
            inline static size_t GetSize()
            {
                return 0;
            }
            
            /** @brief Gets total size of the used space in the memory zone
             * @return Number of bytes
             */
            inline static size_t GetUsedSpace()
            {
                return 0;
            }
        };

        /** @brief Det memory to some value
         * @param destination Destination to set
         * @param value Value to set
         * @param length Data length to set
         */
        inline static void MemSet(void* destination, const uint8_t value, const size_t length)
        {
            for (size_t i = 0; i < length; i++)
            {
                *(((uint8_t*)destination) + i) = value;
            }
        }

        /** @brief Initialize memory
         */
        inline static void Initialize()
        {
            // Memset SGL workarea until the DMA transfer list location, if we go over it, it will corrupt the DMA transfer list
            Memory::MemSet(&_heap_end, 0, reinterpret_cast<uint32_t>(TransList) - reinterpret_cast<uint32_t>(&_heap_end));

            // Initialize memory zones
            Memory::HighWorkRam::Initialize();
            Memory::LowWorkRam::Initialize();
            Memory::CartRam::Initialize();
        }

        /** @brief Gets total size of the used space in the memory zone
         * @param zone Memory zone
         * @return Number of bytes
         */
        inline static size_t GetUsedSpace(const Zone zone)
        {
            switch (zone)
            {
            case Zone::HWRam:
                return HighWorkRam::GetUsedSpace();

            case Zone::LWRam:
                return LowWorkRam::GetUsedSpace();

            case Zone::CartRam:
                return CartRam::GetUsedSpace();

            default:
                return 0;
            }
        }

        /** @brief Gets total size of the free space in the memory zone
         * @param zone Memory zone
         * @return Number of bytes
         */
        inline static size_t GetFreeSpace(const Zone zone)
        {
            switch (zone)
            {
            case Zone::HWRam:
                return HighWorkRam::GetFreeSpace();

            case Zone::LWRam:
                return LowWorkRam::GetFreeSpace();

            case Zone::CartRam:
                return CartRam::GetFreeSpace();

            default:
                return 0;
            }
        }

        /** @brief Gets total size of the memory zone
         * @param zone Memory zone
         * @return Number of bytes
         */
        inline static size_t GetSize(const Zone zone)
        {
            switch (zone)
            {
            case Zone::HWRam:
                return HighWorkRam::GetSize();

            case Zone::LWRam:
                return LowWorkRam::GetSize();

            case Zone::CartRam:
                return CartRam::GetSize();

            default:
                return 0;
            }
        }

        /** @brief Free allocated memory from any memory zone
         * @param ptr Pointer to allocated memory
         */
        inline static void Free(void* ptr)
        {
            if (HighWorkRam::InRange(ptr))
            {
                HighWorkRam::Free(ptr);
            }
            else if (LowWorkRam::InRange(ptr))
            {
                LowWorkRam::Free(ptr);
            }
            else if (CartRam::InRange(ptr))
            {
                CartRam::Free(ptr);
            }
        }

        /** @brief Allocate some memory in specified zone
         * @param size Number of bytes to allocate
         * @param zone Memory zone
         * @return Pointer to the allocated space in memory
         */
        inline static void* Malloc(size_t size, const SRL::Memory::Zone zone)
        {
            switch (zone)
            {
            case SRL::Memory::Zone::CartRam:
                return SRL::Memory::CartRam::Malloc(size);

            case SRL::Memory::Zone::LWRam:
                return SRL::Memory::LowWorkRam::Malloc(size);

            default:
                return SRL::Memory::HighWorkRam::Malloc(size);
            }
        }

        /** @brief Allocate some memory in zone containing specified address
         * @param size Number of bytes to allocate
         * @param address Address in the memory where object should be allocated
         * @return Pointer to the allocated space in memory
         */
        inline static void* PlacementMalloc(size_t size, uint32_t address)
        {
            // Figure out what malloc we have to use
            if (SRL::Memory::HighWorkRam::InRange(address))
            {
                return SRL::Memory::HighWorkRam::Malloc(size);
            }
            else if (SRL::Memory::LowWorkRam::InRange(address))
            {
                return SRL::Memory::LowWorkRam::Malloc(size);
            }
            else if (SRL::Memory::CartRam::InRange(address))
            {
                return SRL::Memory::CartRam::Malloc(size);
            }

            return NULL;
        }

        /** @brief Allocate some memory in zone containing specified address
         * @param size Number of bytes to allocate
         * @param address Address in the memory where object should be allocated
         * @return Pointer to the allocated space in memory
         */
        inline static void* PlacementMalloc(size_t size, void* address)
        {
            // Figure out what malloc we have to use
            if (SRL::Memory::HighWorkRam::InRange(address))
            {
                return SRL::Memory::HighWorkRam::Malloc(size);
            }
            else if (SRL::Memory::LowWorkRam::InRange(address))
            {
                return SRL::Memory::LowWorkRam::Malloc(size);
            }
            else if (SRL::Memory::CartRam::InRange(address))
            {
                return SRL::Memory::CartRam::Malloc(size);
            }

            return NULL;
        }
    };
}

/** @relates SRL::Memory
 * @brief @c new keyword for expansion cartridge RAM
 * @code {.cpp}
 * class MyFirstThing { }
 * 
 * void main()
 * {
 *      // Allocates MyFirstThing in the cart RAM
 *      MyFirstThing* second = cartnew MyFirstThing();
 * }
 * @endcode
 */
#define cartnew new (SRL::Memory::Zone::CartRam)

/** @relates SRL::Memory
 * @brief @c new keyword for low work RAM
 * @code {.cpp}
 * class MyFirstThing { }
 * 
 * void main()
 * {
 *      // Allocates MyFirstThing in the low RAM
 *      MyFirstThing* second = lwnew MyFirstThing();
 * }
 * @endcode
 */
#define lwnew new (SRL::Memory::Zone::LWRam)

/** @relates SRL::Memory
 * @brief @c new keyword for high work RAM 
 * @code {.cpp}
 * class MyFirstThing { }
 * 
 * void main()
 * {
 *      // Allocates MyFirstThing in the high RAM
 *      MyFirstThing* second = hwnew MyFirstThing();
 * 
 *      // For High RAM, keywords new or hwnew can be both used
 *      MyFirstThing* second = new MyFirstThing();
 * }
 * @endcode
 */
#define hwnew new

/** @relates SRL::Memory
 * @brief Allocates memory in the same zone as current context
 * @warning <b>Extreme cation is required</b>.<br/> This will allocate new object in the same zone as the @c this keyword is present in.
 * @code {.cpp}
 * class MyFirstThing { }
 * 
 * class MySecondThing
 * {
 *      MyFirstThing * thing;
 * 
 *      MySecondThing() {
 *          // Allocate object MyFirstThing in the same memory zone as MySecondThing
 *          this->thing = autonew MyFirstThing();
 *      }
 * }
 * 
 * void main()
 * {
 *      // Allocates MySecondThing object in the Low RAM, which in term in its constructor allocates
 *      // MyFirstThing object in the same zone (Low RAM)
 *      MySecondThing* first = lwnew MySecondThing();
 * 
 *      // Allocates MySecondThing object in the High RAM, which in term in its constructor allocates
 *      // MyFirstThing object in the same zone (High RAM)
 *      // For High RAM, keywords new or hwnew can be both used
 *      MySecondThing* second = new MySecondThing();
 * }
 * @endcode
 */
#define autonew new(reinterpret_cast<uint32_t>(this))

/** @brief Allocate some memory
 * @param size Number of bytes to allocate
 * @param zoneAddress Address in the memory zone where object should be allocated
 * @return Pointer to the allocated space in memory
 */
inline void* operator new(size_t size, uint32_t zoneAddress)
{
    return SRL::Memory::PlacementMalloc(size, zoneAddress);
}

/** @brief Allocate some memory
 * @param size Number of bytes to allocate
 * @param zoneAddress Address in the memory zone where object should be allocated
 * @return Pointer to the allocated space in memory
 */
inline void* operator new[](size_t size, uint32_t zoneAddress)
{
    return SRL::Memory::PlacementMalloc(size, zoneAddress);
}

/** @brief Allocate some memory
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated space in memory
 */
inline void* operator new(size_t size)
{
    return SRL::Memory::HighWorkRam::Malloc(size);
}

/** @brief Allocate some memory
 * @param size Number of bytes to allocate
 * @param zone Memory zone
 * @return Pointer to the allocated space in memory
 */
inline void* operator new(size_t size, const SRL::Memory::Zone zone)
{
    switch (zone)
    {
    case SRL::Memory::Zone::CartRam:
        return SRL::Memory::CartRam::Malloc(size);

    case SRL::Memory::Zone::LWRam:
        return SRL::Memory::LowWorkRam::Malloc(size);

    default:
        return SRL::Memory::HighWorkRam::Malloc(size);
    }
}

/** @brief Free allocated memory
 * @param ptr Pointer to allocated memory
 */
inline void operator delete(void* ptr)
{
    SRL::Memory::Free(ptr);
}

/** @brief Free allocated memory
 * @param ptr Pointer to allocated memory
 * @param size Number of bytes to free (Not used)
 */
inline void operator delete(void* ptr, size_t size)
{
    SRL::Memory::Free(ptr);
}

/** @brief Allocate some memory from array
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated space in memory
 */
inline void* operator new[](size_t size)
{
    return SRL::Memory::HighWorkRam::Malloc(size);
}

/** @brief Allocate some memory
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated space in memory
 */
inline void* operator new[](size_t size, const SRL::Memory::Zone zone)
{
    switch (zone)
    {
    case SRL::Memory::Zone::CartRam:
        return SRL::Memory::CartRam::Malloc(size);

    case SRL::Memory::Zone::LWRam:
        return SRL::Memory::LowWorkRam::Malloc(size);

    default:
        return SRL::Memory::HighWorkRam::Malloc(size);
    }
}

/** @brief Free allocated array from memory
 * @param ptr Pointer to allocated memory
 */
inline void operator delete[](void* ptr)
{
    SRL::Memory::Free(ptr);
}

/** @brief Free allocated array from memory
 * @param ptr Pointer to allocated memory
 * @param size Number of bytes to free (Not used)
 */
inline void operator delete[](void* ptr, size_t size)
{
    SRL::Memory::Free(ptr);
}
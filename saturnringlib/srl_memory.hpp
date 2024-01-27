#pragma once

#include "srl_base.hpp"
#include "srl_utils.hpp"

namespace SRL
{
	/** @brief Dynamic memory management
	 */
	class Memory
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
			BlockState State:1;

			/** @brief Block size (without header)
			 */
			size_t Size:31;
		};

		/** @brief Memory heap
		 */
		inline static Uint8 MemoryHeap[SRL_MALLOC_MEMORY];

		/** @brief Get location of the next block in memory
		 * @param currentBlock Current block location
		 * @return Location of the next block in memory 
		 */
		inline static size_t GetNextBlockLocation(size_t currentBlock)
		{
			return currentBlock + sizeof(Memory::Header) + ((Memory::Header*)&Memory::MemoryHeap[currentBlock])->Size;
		}

		/** @brief Merges all free memory blocks until allocated block
		 * @param startBlock Starting free block
		 */
		inline static void MergeFreeMemoryBlocks(size_t startBlock)
		{
			size_t next = startBlock;

			// Check if we are inside the array
			if (startBlock < SRL_MALLOC_MEMORY)
			{
				// Check if current block is free
				Memory::Header* currentBlockHead = ((Memory::Header*)&Memory::MemoryHeap[startBlock]);

				// Check if the block is free
				if (currentBlockHead->State == Memory::BlockState::Free)
				{
					// Search memory for free blocks
					size_t location = Memory::GetNextBlockLocation(startBlock);
					size_t freeMem = 0;

					while (location < SRL_MALLOC_MEMORY)
					{
						// Gets header of the current block
						Memory::Header* header = ((Memory::Header*)&Memory::MemoryHeap[location]);
						
						// Check if the block is free
						if (header->State == Memory::BlockState::Free)
						{
							freeMem += header->Size + sizeof(Memory::Header);
						}
						else
						{
							break;
						}
						
						// Move to next block
						location = Memory::GetNextBlockLocation(location);
					}

					currentBlockHead->Size += freeMem; 
				}
			}
		}

		/** @brief Try to mark block as allocated and split it if not allocated fully
		 * @param location Block location
		 * @param size New block size
		 * @return True if block was succesfully allocated 
		 */
		inline static bool SetBlockAllocation(size_t location, size_t size)
		{
			// Gets block header
			Memory::Header* header = ((Memory::Header*)&Memory::MemoryHeap[location]);

			// Store size of new block
			size_t newBlock = size;

			// If next block size is smaller than four bytes
			if (header->Size - newBlock <= sizeof(Memory::Header) &&
				header->Size >= newBlock)
			{
				newBlock = header->Size;
			}

			// Can we fit in this block?
			if (header->Size >= newBlock)
			{
				// Save old block size for later use
				size_t oldBlockSize = header->Size;

				// Set as allocated
				header->State = Memory::BlockState::Used;
				header->Size = newBlock;

				// Set next block as free, this splits current bolc into two
				// First part is allocated meory, second part is left over free memory
				if (oldBlockSize - newBlock > 0)
				{
					size_t nextBlock = Memory::GetNextBlockLocation(location);
					((Memory::Header*)&Memory::MemoryHeap[nextBlock])->State = Memory::BlockState::Free; 
					((Memory::Header*)&Memory::MemoryHeap[nextBlock])->Size = oldBlockSize - newBlock - sizeof(Memory::Header);
				}

				return true;
			}

			return false;
		}

	public:

		/** @brief Free memory
		 * @param address Allocated memory
		 */
		inline static void Free(void* address)
		{
			// Validate pointer to not be null
			if (address != nullptr)
			{
				// Gets offset to memory array
				size_t location = reinterpret_cast<Uint32>(address) - reinterpret_cast<Uint32>(Memory::MemoryHeap);

				// Check of offset is valid, we do not need to check whether location is 0, since first 4 bytes are always header
				if (location > 0 && location < SRL_MALLOC_MEMORY && (location & 3) == 0)
				{
					// Flag area as free
					((Memory::Header*)&Memory::MemoryHeap[location - sizeof(Memory::Header)])->State = Memory::BlockState::Free;

					// Merge areas
					Memory::MergeFreeMemoryBlocks(location - sizeof(Memory::Header));
				}
			}
		}

		/** @brief Gets all available free memory
		 * @return Number of bytes free 
		 */
		inline static size_t GetAvailableMemory()
		{
			size_t freeMemory = 0;

			// Search memory for free blocks
			size_t location = 0;

			while (location < SRL_MALLOC_MEMORY)
			{
				// Gets header of the current block
				Memory::Header* header = ((Memory::Header*)&Memory::MemoryHeap[location]);
				
				// Check if the block is free
				if (header->State == Memory::BlockState::Free)
				{
					freeMemory += header->Size;
				}
				
				// Move to next block
				location = Memory::GetNextBlockLocation(location);
			}

			return freeMemory;
		}

		/** @brief Gets all blocks created by allocation
		 * @return Number of blocks
		 */
		inline static size_t GetNumberOfBlocks()
		{
			// Search memory for free blocks
			size_t blocks = 0;
			size_t location = 0;

			while (location < SRL_MALLOC_MEMORY)
			{
				blocks++;
				
				// Move to next block
				location = Memory::GetNextBlockLocation(location);
			}

			return blocks;
		}

		/** @brief Gets all available free blocks (when used in tandum with GetNumberOfBlocks() this number can indicate fragmentation of the memory)
		 * @return Number of blocks that can fit stuff
		 */
		inline static size_t GetNumberOfFreeBlocks()
		{
			// Search memory for free blocks
			size_t freeBlocks = 0;
			size_t location = 0;

			while (location < SRL_MALLOC_MEMORY)
			{
				// Gets header of the current block
				Memory::Header* header = ((Memory::Header*)&Memory::MemoryHeap[location]);
				
				// Check if the block is free
				if (header->State == Memory::BlockState::Free)
				{
					freeBlocks++;
				}
				
				// Move to next block
				location = Memory::GetNextBlockLocation(location);
			}

			return freeBlocks;
		}

		/** @brief Initialize memory
		 */
		inline static void Initialize()
		{
			((Memory::Header*)Memory::MemoryHeap)->Size = SRL_MALLOC_MEMORY - sizeof(Memory::Header);
			((Memory::Header*)Memory::MemoryHeap)->State = Memory::BlockState::Free;
		}

		/** @brief Allocate memory
		 * @param size Number of bytes to allocate
		 * @return Pointer to allocated space
		 */
		inline static void* Malloc(size_t size)
		{
			// Allign to 4
			size_t length = size;
			size_t allign = length & 3;

			if (allign != 0)
			{
				length += 4 - allign;
			}

			// Find free space
			size_t location = 0;
			size_t newBlock = length & 0x7fffffff;

			// We try until we reach end of available space
			while (location < SRL_MALLOC_MEMORY)
			{
				// Gets header of the current block
				Memory::Header* header = ((Memory::Header*)&Memory::MemoryHeap[location]);
				
				// Check if the block is free
				if (header->State == Memory::BlockState::Free)
				{
					// Merge free areas to create one big free space from many small neighbouring ones
					Memory::MergeFreeMemoryBlocks(location);

					// Can we fit in this block?
					if (Memory::SetBlockAllocation(location, newBlock))
					{
						// Return pointer to allocated block
						return (void*)&Memory::MemoryHeap[location + sizeof(Memory::Header)];
					}
				}
				
				// Move to next block
				location = Memory::GetNextBlockLocation(location);
			}

			// We could not allocate anything
			return nullptr;
		}

		/** @brief Reallocate memory (can either shrik, enlarge or move)
		 * @param address Allocated memory to resize
		 * @param size New size of the allocated block
		 * @return void* Pointer to resized or moved block
		 */
		inline static void * Realloc(void* address, size_t size)
		{
			// Validate pointer to not be null
			if (address != nullptr)
			{
				// This one is located in the free() function as well, but I do not want to repeat its insides here again 
				size_t location = reinterpret_cast<size_t>(address) - reinterpret_cast<size_t>(Memory::MemoryHeap);
				size_t headerLocation = location - sizeof(Memory::Header);

				// We free the block first, this will also join it will all free blocks right after it
				Memory::Free(address);
			
				// Now we check if we can fit inside the new created space
				if (Memory::SetBlockAllocation(headerLocation, size))
				{
					// We fit inside
					return (void*)&Memory::MemoryHeap[location];
				}
				else
				{
					// We do not fit, try to find space elsewhere
					void * newSpace = Memory::Malloc(size);

					// Copy data to the new location
					slDMACopy(address, newSpace, ((Memory::Header*)&Memory::MemoryHeap[headerLocation])->Size);

					// Return address to new thing
					return newSpace;
				}
			}

			return nullptr;
		}
	};
}

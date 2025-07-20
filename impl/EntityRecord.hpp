#pragma once

/**
 * @file EntityRecord.hpp
 * @brief Core entity record management for the SECS library
 * @author Roberto Duarte
 * @date 2025
 * @copyright MIT License
 * 
 * @details
 * This file contains the EntityRecord class which manages the lifecycle and
 * metadata of entities in the SECS Entity Component System. EntityRecord
 * provides efficient entity storage with automatic memory management,
 * version tracking for safe entity references, and optimized allocation
 * patterns suitable for resource-constrained environments.
 * 
 * Key features:
 * - **Memory efficient**: Uses uint16_t indices to minimize memory footprint
 * - **Version tracking**: Prevents access to destroyed entities
 * - **Free list recycling**: Reuses entity slots to minimize fragmentation
 * - **Exception-free**: Safe allocation failure handling for retro platforms
 * - **Cache-friendly**: Contiguous storage for optimal memory access patterns
 * 
 * @par Design Philosophy:
 * EntityRecord follows a pool-based allocation strategy where entity records
 * are stored in a contiguous array. When entities are destroyed, their slots
 * are added to a free list for efficient reuse. This approach minimizes
 * memory fragmentation and provides predictable performance characteristics
 * essential for real-time gaming applications.
 * 
 * @par Memory Management:
 * - Records are stored in a dynamically growing array
 * - Destroyed entities are recycled via a free list
 * - Growth strategy: capacity = (capacity * 2) - (capacity / 2)
 * - Allocation failures are handled gracefully without exceptions
 * 
 * @par Thread Safety:
 * This class is NOT thread-safe. External synchronization is required
 * for multi-threaded access.
 * 
 * @par Compiler Requirements:
 * - C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
 * - Standard library support for std::stack and basic containers
 */

#include <cstdint>
#include <cstddef>
#include <vector>
#include <stack>

namespace SECS
{
    /**
     * @brief Type alias for entity and component indices
     * 
     * @details
     * Uses uint16_t to minimize memory usage while supporting up to 65,535
     * entities. This is sufficient for most retro gaming scenarios while
     * keeping memory overhead minimal on systems with limited RAM.
     */
    using Index = uint16_t;
    
    /**
     * @brief Sentinel value representing an invalid or uninitialized index
     * 
     * @details
     * This value (65535 for uint16_t) is used to indicate invalid entities,
     * uninitialized components, or error conditions throughout the ECS.
     */
    static constexpr Index InvalidIndex = ~(Index(0));

    /**
     * @brief Core entity record for tracking entity metadata and lifecycle
     * 
     * @details
     * EntityRecord manages the fundamental data associated with each entity
     * in the ECS system. Each record contains:
     * - Archetype index: Which component combination this entity has
     * - Row index: Position within the archetype's component arrays
     * - Version: Incremented on destruction to invalidate old references
     * 
     * The class uses static storage to maintain a global pool of entity
     * records, with automatic growth and efficient recycling of destroyed
     * entity slots.
     * 
     * @par Memory Layout:
     * Records are stored in a contiguous array for cache efficiency.
     * The layout minimizes padding and uses the smallest viable data types:
     * ```
     * struct EntityRecord {
     *     uint16_t archetype;  // 2 bytes
     *     uint16_t row;        // 2 bytes  
     *     uint16_t version;    // 2 bytes
     * };                      // Total: 6 bytes per entity
     * ```
     * 
     * @par Lifecycle Management:
     * 1. **Creation**: Reserve() allocates a new record or reuses a freed one
     * 2. **Usage**: Record tracks entity's archetype and component row
     * 3. **Destruction**: Release() increments version and adds to free list
     * 4. **Recycling**: Freed records are reused for new entities
     * 
     * @par Version Tracking:
     * Each entity has a version number that increments when the entity is
     * destroyed. This allows EntityReference objects to detect when they
     * reference a destroyed entity, preventing access to invalid data.
     * 
     * @par Example Usage:
     * ```cpp
     * // Internal usage (typically called by World class)
     * EntityRecord& record = EntityRecord::Reserve();
     * record.archetype = archetypeIndex;
     * record.row = componentRow;
     * 
     * // Later, when entity is destroyed
     * record.Release();
     * ```
     * 
     * @warning This class is primarily for internal use by the ECS system.
     *          User code should interact with entities through EntityReference.
     * 
     * @see EntityReference For safe entity access and manipulation
     * @see World For entity creation and management
     * @see ArchetypeManager For component storage details
     */
    class EntityRecord
    {
        friend class EntityReference;
        friend class World;
        friend class ArchetypeManager;

        static inline size_t capacity = 0;
        static inline size_t last = 0;
        static inline std::stack<size_t> freeList;  // Free list for recycling indices
        static inline EntityRecord* records = nullptr;

        /**
         * @brief Reserves an entity record for a new entity
         * 
         * @details
         * This method allocates a new EntityRecord for entity creation. It follows
         * an efficient allocation strategy:
         * 1. First, attempts to reuse a record from the free list (recycled entities)
         * 2. If no free records exist, expands the storage array if needed
         * 3. Returns a reference to the allocated record
         * 
         * The growth strategy uses a modified doubling approach:
         * `new_capacity = (old_capacity * 2) - (old_capacity / 2)`
         * This provides ~1.5x growth to balance memory usage and allocation frequency.
         * 
         * @return Reference to a valid EntityRecord ready for initialization
         * @retval EntityRecord& Valid record when allocation succeeds
         * @retval EntityRecord& Invalid record (archetype=InvalidIndex) on allocation failure
         * 
         * @par Memory Safety:
         * - Handles allocation failures gracefully without exceptions
         * - Returns an invalid record on failure rather than throwing
         * - Preserves existing data during array expansion
         * 
         * @par Performance Notes:
         * - O(1) when reusing from free list
         * - O(n) when expanding array (rare, amortized O(1))
         * - Memory usage: 6 bytes per entity record
         * 
         * @par Saturn Optimization:
         * For Saturn development, consider pre-allocating a fixed pool:
         * ```cpp
         * // Pre-allocate for 1000 entities at startup
         * for (int i = 0; i < 1000; ++i) {
         *     EntityRecord::Reserve().Release();
         * }
         * ```
         * 
         * @warning This method is for internal ECS use only.
         *          User code should use World::CreateEntity() instead.
         * 
         * @see Release() For returning records to the free list
         * @see World::CreateEntity() For user-facing entity creation
         */
        static EntityRecord& Reserve()
        {
            size_t index;
            if (!freeList.empty()) {
                // Reuse an index from the free list
                index = freeList.top();
                freeList.pop();
            } else {
                // No free indices, need to expand storage if necessary
                if (last >= capacity) {
                    size_t originalCapacity = capacity;
                    capacity = (capacity == 0) ? 2 : (capacity * 2) - (capacity / 2);

                    EntityRecord* newArray = new EntityRecord[capacity];
                    if (!newArray) {
                        // Handle allocation failure - return invalid record for Saturn compatibility
                        // In production, consider pre-allocating a fixed pool
                        capacity = originalCapacity; // Restore original capacity
                        static EntityRecord invalidRecord;
                        invalidRecord.archetype = InvalidIndex;
                        invalidRecord.row = InvalidIndex;
                        invalidRecord.version = InvalidIndex;
                        return invalidRecord;
                    }

                    // Move existing records to the new array
                    for (Index i = 0; i < originalCapacity; ++i) {
                        newArray[i] = std::move(records[i]);
                    }

                    delete[] records;
                    records = newArray;
                }
                
                // Use the next available index
                index = last++;
            }
            return records[index];
        }

        Index archetype = InvalidIndex;
        Index row = InvalidIndex;
        Index version = 0;

        /**
         * @brief Calculate this record's index within the global records array
         * 
         * @details
         * Computes the index of this EntityRecord within the static records array
         * using pointer arithmetic. This index serves as the entity's unique
         * identifier and is used by EntityReference for entity lookup.
         * 
         * @return The zero-based index of this record in the records array
         * @retval Index Valid index (0 to 65534) for records in the array
         * @retval InvalidIndex Theoretical return for invalid records (shouldn't occur)
         * 
         * @par Implementation Details:
         * Uses pointer arithmetic: `this - records` to calculate the offset.
         * This is safe because all EntityRecord objects are allocated from
         * the contiguous records array.
         * 
         * @par Performance:
         * - O(1) constant time operation
         * - Single pointer subtraction and cast
         * - No memory access beyond the current object
         * 
         * @warning Only valid for records allocated from the records array.
         *          Calling this on stack-allocated records causes undefined behavior.
         * 
         * @see EntityReference For entity identification usage
         * @see Reserve() For record allocation details
         */
        Index GetIndex() const { return static_cast<Index>(this - records); }

        /**
         * @brief Release this entity record and make it available for reuse
         * 
         * @details
         * Marks this EntityRecord as destroyed and adds it to the recycling system.
         * The method performs several cleanup operations:
         * 1. Clears entity data (archetype, row set to InvalidIndex)
         * 2. Increments version to invalidate existing EntityReference objects
         * 3. Adds the record to the free list for efficient reuse
         * 4. Optimizes storage by shrinking the active range when possible
         * 
         * @par Version Tracking:
         * The version number is incremented to ensure that any existing
         * EntityReference objects pointing to this entity will detect the
         * destruction and refuse to access the entity's components.
         * 
         * @par Memory Optimization:
         * When the last entity in the array is destroyed, the method shrinks
         * the active range (`last` counter) and removes any trailing free
         * entries. This keeps memory usage minimal and improves cache locality.
         * 
         * @par Free List Management:
         * Records not at the end of the array are added to a free list (stack)
         * for O(1) reuse during future entity creation. This minimizes memory
         * fragmentation and provides predictable allocation performance.
         * 
         * @par Performance Characteristics:
         * - O(1) for most releases
         * - O(k) when optimizing trailing free entries (k = consecutive free entries)
         * - Memory overhead: One stack entry per fragmented free record
         * 
         * @par Example Lifecycle:
         * ```cpp
         * // Entity creation
         * EntityRecord& record = EntityRecord::Reserve();
         * record.archetype = myArchetype;
         * record.row = myRow;
         * 
         * // Entity destruction
         * record.Release(); // Version incremented, added to free list
         * 
         * // Future entity creation may reuse this record
         * EntityRecord& newRecord = EntityRecord::Reserve(); // May return same record
         * ```
         * 
         * @warning After calling Release(), this record should not be accessed
         *          until it's returned by a future Reserve() call.
         * 
         * @see Reserve() For record allocation and reuse
         * @see EntityReference::Access() For version-based validity checking
         * @see World::CreateEntity() For high-level entity management
         */
        void Release()
        {
            // Clear the record's data
            archetype = InvalidIndex;
            row = InvalidIndex;
            version++;

            // Get the index of this record
            Index index = GetIndex();
            
            // If this is the last record in the array, just decrement last
            if (index == last - 1) {
                last--;
                
                // Also release any indices at the end that are in the free list
                while (last > 0 && !freeList.empty() && freeList.top() == last - 1) {
                    freeList.pop();
                    last--;
                }
            } else {
                // Add the index to the free list for reuse
                freeList.push(index);
            }
        }
    };
}

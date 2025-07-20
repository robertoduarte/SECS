#pragma once

/**
 * @file Archetype.hpp
 * @brief Archetype management and component storage for the SECS library
 * @author Roberto Duarte
 * @date 2025
 * @copyright MIT License
 * 
 * @details
 * This file contains the ArchetypeManager class which manages component storage
 * and entity organization in the SECS Entity Component System. The archetype
 * system groups entities with identical component combinations for optimal
 * cache performance and efficient iteration.
 * 
 * Key features:
 * - **Cache-optimized storage**: Components stored in separate arrays
 * - **Archetype-based organization**: Entities grouped by component combination
 * - **Efficient iteration**: Linear memory access patterns for systems
 * - **Type-safe operations**: Compile-time component type validation
 * - **Memory efficient**: Minimal overhead for component access
 * 
 * @par Design Philosophy:
 * The archetype system is based on the principle that entities with the same
 * component types should be stored together in memory. This approach provides
 * excellent cache locality when iterating over entities in systems, which is
 * crucial for performance on resource-constrained platforms like the Sega Saturn.
 * 
 * @par Archetype Organization:
 * Each unique combination of component types forms an "archetype". For example:
 * - Archetype A: [Position, Velocity]
 * - Archetype B: [Position, Velocity, Health]
 * - Archetype C: [Position, Sprite]
 * 
 * Entities are automatically moved between archetypes when components are
 * added or removed, maintaining the optimal storage layout.
 * 
 * @par Memory Layout:
 * Within each archetype, components are stored in separate arrays:
 * ```
 * Archetype [Position, Velocity]:
 * Position Array: [pos0, pos1, pos2, ...]
 * Velocity Array: [vel0, vel1, vel2, ...]
 * Entity Records:  [ent0, ent1, ent2, ...]
 * ```
 * 
 * This Structure of Arrays (SoA) layout maximizes cache efficiency during
 * system iteration compared to Array of Structures (AoS) approaches.
 * 
 * @par Thread Safety:
 * This class is NOT thread-safe. External synchronization is required
 * for multi-threaded access.
 * 
 * @par Compiler Requirements:
 * - C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
 * - Support for concepts (C++20) or SFINAE-based validation
 * - Template template parameters and variadic templates
 * 
 * @see Component For component type management and ID generation
 * @see EntityRecord For entity metadata and lifecycle
 * @see World For high-level archetype operations
 */

#include <stdint.h>
#include <stddef.h>
#include <vector>

#include "EntityRecord.hpp"
#include "Component.hpp"
#include "Utils.hpp"

namespace SECS
{
    /**
     * @brief Concept defining valid component types for the ECS system
     * 
     * @details
     * This concept ensures that component types meet the basic requirements
     * for use in the SECS Entity Component System. Currently, the only
     * requirement is that component types must not be empty classes.
     * 
     * @tparam T The component type to validate
     * 
     * @par Requirements:
     * - Must not be an empty class (sizeof(T) > 0)
     * - Should be trivially copyable for optimal performance
     * - Should avoid dynamic allocations for Saturn compatibility
     * 
     * @par Valid Component Examples:
     * ```cpp
     * struct Position { float x, y; };           // Valid: has data members
     * struct Velocity { float dx, dy; };         // Valid: has data members
     * struct Health { uint16_t current, max; };  // Valid: has data members
     * ```
     * 
     * @par Invalid Component Examples:
     * ```cpp
     * struct EmptyTag {};                        // Invalid: empty class
     * class AbstractComponent { virtual ~AbstractComponent() = 0; }; // Avoid: virtual
     * ```
     * 
     * @note For Saturn development, keep components under 32 bytes and use
     *       fixed-size types (uint16_t instead of int) when possible.
     * 
     * @see Component For component type registration and ID assignment
     * @see ArchetypeManager For component storage and management
     */
    template <typename T>
    concept ComponentType = !std::is_empty_v<T>;

    /**
     * @brief Manages archetype-based component storage and entity organization
     * 
     * @details
     * ArchetypeManager is the core class responsible for organizing entities
     * by their component combinations (archetypes) and providing efficient
     * storage and access patterns for component data. Each unique combination
     * of component types gets its own ArchetypeManager instance.
     * 
     * The class provides:
     * - Automatic archetype creation and management
     * - Efficient component array storage (Structure of Arrays)
     * - Fast entity iteration within archetypes
     * - Type-safe component access and manipulation
     * - Memory-efficient entity record management
     * 
     * @par Archetype Lifecycle:
     * 1. **Creation**: Automatically created when first entity with component combination is made
     * 2. **Population**: Entities are added as they're created with matching components
     * 3. **Iteration**: Systems iterate over entities within each relevant archetype
     * 4. **Migration**: Entities move between archetypes when components are added/removed
     * 
     * @par Storage Strategy:
     * Uses Structure of Arrays (SoA) for optimal cache performance:
     * - Each component type has its own contiguous array
     * - Entity data at index N corresponds across all component arrays
     * - Linear iteration provides excellent cache locality
     * 
     * @par Memory Management:
     * - Component arrays grow dynamically as entities are added
     * - Removed entities leave gaps that are filled by moving the last entity
     * - No memory fragmentation within component arrays
     * - Automatic cleanup when archetypes become empty
     * 
     * @par Performance Characteristics:
     * - Entity creation: O(1) amortized
     * - Entity removal: O(1) with potential data movement
     * - Component access: O(1) direct array indexing
     * - Archetype iteration: O(n) linear, cache-friendly
     * 
     * @warning This class is primarily for internal ECS use. User code should
     *          interact with entities through World and EntityReference classes.
     * 
     * @see World For user-facing entity and component operations
     * @see EntityReference For safe entity access and manipulation
     * @see Component For component type management
     */
    class ArchetypeManager
    {
        friend class EntityReference;
        friend class World;

        static inline std::vector<ArchetypeManager> managers;

        /**
         * @brief Finds the index of the archetype manager associated with a specific component binary identifier.
         * @param id The binary identifier of the components.
         * @return The index of the archetype manager.
         */
        static size_t Find(Component::BinaryId id)
        {
            size_t size = managers.size();

            for (size_t i = 0; i < size; ++i)
            {
                if (managers[i].id == id) { return i; }
            }

            managers.push_back(std::move(ArchetypeManager(id)));

            return size;
        }

        /**
         * @brief Helper struct for managing components.
         * @tparam T The component types.
         */
        template <typename... T>
        struct HelperImplementation
        {
            static inline Component::BinaryId id = (Component::IdBinary<T> | ...);

            /**
             * @brief Get the instance of the archetype manager.
             * @return The instance.
             */
            static ArchetypeManager& GetInstance()
            {
                static const size_t index = ArchetypeManager::Find(id);
                return ArchetypeManager::managers[index];
            }

            /**
             * @brief Add component to the binary identifier.
             * @param sourceID The binary identifier to add to.
             * @return The updated binary identifier.
             */
            static Component::BinaryId AddTo(Component::BinaryId sourceID)
            {
                return sourceID | id;
            }

            /**
             * @brief Remove component from the binary identifier.
             * @param sourceID The binary identifier to remove from.
             * @return The updated binary identifier.
             */
            static Component::BinaryId RemoveFrom(Component::BinaryId sourceID)
            {
                return sourceID & ~id;
            }
        };

        /**
         * @brief Template alias for HelperImplementation.
         * @tparam Ts The component types.
         */
        template <class... Ts>
        using Helper = Instantiate<HelperImplementation, SortedList<list<Ts...>>>;

        /**
         * @brief Struct for caching lookup results.
         * @tparam T The component types.
         */
        template <typename... T>
        struct LookupCacheImplementation
        {
            static inline size_t lastIndexChecked = 0;
            static inline std::vector<uint16_t> matchedIndices;

            /**
             * @brief Update the cache.
             */
            static void Update()
            {
                if (lastIndexChecked < ArchetypeManager::managers.size())
                {
                    auto cacheIterator = ArchetypeManager::managers.begin() + lastIndexChecked;
                    while (cacheIterator != ArchetypeManager::managers.end())
                    {
                        ArchetypeManager& manager = *cacheIterator;
                        if (manager.Contains(Helper<T...>::id))
                        {
                            matchedIndices.push_back(lastIndexChecked);
                        }
                        ++cacheIterator;
                        ++lastIndexChecked;
                    }
                }
            }
        };

        /**
         * @brief Template alias for LookupCacheImplementation.
         * @tparam Ts The component types.
         */
        template <class... Ts>
        using LookupCache = Instantiate<LookupCacheImplementation, SortedList<list<Ts...>>>;

        Index GetIndex() { return static_cast<Index>(this - &(*managers.begin())); }

        Component::BinaryId id;

        using InternalIndex = uint8_t;
        static inline constexpr InternalIndex Unused = ~(InternalIndex(0));

        Index* recordIndices = nullptr;
        void** componentArrays = nullptr;
        InternalIndex internalIndex[Component::MaxComponentTypes] = { Unused };
        Index capacity = 0;
        Index size = 0;

        /**
         * @brief Iterate over each component in a binary identifier.
         * @param id The binary identifier.
         * @param lambda The lambda function to apply to each component.
         */
        template <typename Lambda>
        static void EachComponent(Component::BinaryId id, Lambda lambda)
        {
            size_t componentId = 0;
            do
            {
                if (1 & id) { lambda(componentId); }
                componentId++;
            } while (id >>= 1);
        }

        /**
         * @brief Iterate over common components in two binary identifiers.
         * @param idA The first binary identifier.
         * @param idB The second binary identifier.
         * @param lambda The lambda function to apply to common components.
         */
        template <typename Lambda>
        static void EachCommonComponent(Component::BinaryId idA, Component::BinaryId idB, Lambda lambda)
        {
            size_t componentId = 0;
            do
            {
                if (1 & idA & idB) { lambda(componentId); }
                componentId++;
            } while ((idA >>= 1) && (idB >>= 1));
        }

        /**
         * @brief Check if the archetype contains a set of components.
         * @param expected The binary identifier of expected components.
         * @return true if the archetype contains the expected components, false otherwise.
         */
        bool Contains(Component::BinaryId expected) { return (id & expected) == expected; }

        /**
         * @brief Get a strongly-typed pointer to a component array.
         * @tparam T The component type.
         * @return A pointer to the component array.
         */
        template <typename T>
        T* GetComponentArray() const
        {
            return static_cast<T*>(componentArrays[internalIndex[Component::Id<T>]]);
        }

        /**
         * @brief Get a strongly-typed pointer to a component within a row.
         * @tparam T The component type.
         * @param row The row index.
         * @return A pointer to the component.
         */
        template <typename T>
        T* GetComponent(Index row) const
        {
            auto index = internalIndex[Component::Id<T>];
            return (index == Unused) ? nullptr :
                &((static_cast<T*>(componentArrays[index]))[row]);
        }

    public:
        /**
         * @brief Default constructor.
         */
        ArchetypeManager() = default;

        /**
         * @brief Move constructor.
         * @param other The other ArchetypeManager to move.
         */
        ArchetypeManager(ArchetypeManager&& other) noexcept
            : id(std::move(other.id)),
            recordIndices(std::move(other.recordIndices)),
            componentArrays(std::move(other.componentArrays)),
            capacity(std::move(other.capacity)),
            size(std::move(other.size))
        {
            for (size_t i = 0; i < Component::MaxComponentTypes; ++i)
            {
                internalIndex[i] = std::move(other.internalIndex[i]);
                other.internalIndex[i] = Unused;
            }

            // Reset the source object
            other.id = 0;
            other.recordIndices = nullptr;
            other.componentArrays = nullptr;
            other.capacity = 0;
            other.size = 0;
        }

        /**
         * @brief Move assignment operator.
         * @param other The other ArchetypeManager to move.
         * @return Reference to this ArchetypeManager after the move.
         */
        ArchetypeManager& operator=(ArchetypeManager&& other) noexcept
        {
            if (this != &other)
            {
                id = std::move(other.id);
                recordIndices = std::move(other.recordIndices);
                componentArrays = std::move(other.componentArrays);
                capacity = std::move(other.capacity);
                size = std::move(other.size);

                for (size_t i = 0; i < Component::MaxComponentTypes; ++i)
                {
                    internalIndex[i] = std::move(other.internalIndex[i]);
                    other.internalIndex[i] = Unused;
                }

                // Reset the source object
                other.id = 0;
                other.recordIndices = nullptr;
                other.componentArrays = nullptr;
                other.capacity = 0;
                other.size = 0;
            }

            return *this;
        }

    private:
        /**
         * @brief Construct an ArchetypeManager with a given binary identifier.
         * @param newId The binary identifier.
         */
        ArchetypeManager(Component::BinaryId newId) : id(newId)
        {
            uint8_t localComponentCount = 0;
            EachComponent(newId, [this, &localComponentCount](size_t componentId)
            {
                internalIndex[componentId] = localComponentCount++;
            });

            componentArrays = new void* [localComponentCount]();
        }

        /**
         * @brief Reserve an EntityRecord within the archetype.
         * @return The reserved EntityRecord.
         */
        EntityRecord& ReserveRecord()
        {
            EntityRecord& entityRecord = EntityRecord::Reserve();

            if (size >= capacity)
            {
                capacity = (capacity == 0) ? 2 : (capacity * 2) - (capacity / 2);
                recordIndices = static_cast<Index*>(realloc(recordIndices, sizeof(Index) * capacity));

                EachComponent(id, [this](const size_t& componentId)
                {
                    Component::ResizeArray(componentId, &componentArrays[internalIndex[componentId]], capacity, size);
                });
            }
            recordIndices[size] = entityRecord.GetIndex();

            entityRecord.archetype = static_cast<Index>(GetIndex());
            entityRecord.row = size++;

            return entityRecord;
        }

        /**
         * @brief Remove a row from the archetype.
         * @param row The row index to remove.
         */
        void RemoveRow(Index row)
        {
            if (size) size--;
            Index lastRow = size;
            if (row != lastRow)
            {
                EachComponent(id, [this, row, lastRow](size_t componentId)
                {
                    void* arrayPtr = componentArrays[internalIndex[componentId]];
                    Component::MoveElement(componentId, arrayPtr, row, arrayPtr, lastRow);
                });

                EntityRecord::records[recordIndices[row]].Release();
                EntityRecord::records[recordIndices[lastRow]].row = row;
                recordIndices[row] = recordIndices[lastRow];
            }
        }

        /**
         * @brief Move an entity from another archetype into this archetype.
         * @param sourceArchetype The source archetype.
         * @param sourceRow The source row within the source archetype.
         * @return The EntityRecord for the moved entity.
         */
        EntityRecord& MoveEntity(ArchetypeManager* sourceArchetype, size_t sourceRow)
        {
            EntityRecord& record = ReserveRecord();
            EachCommonComponent(id, sourceArchetype->id, [this, &record, sourceArchetype, sourceRow](size_t componentId)
            {
                void* arrayPtr = componentArrays[internalIndex[componentId]];
                void* srcArrayPtr = sourceArchetype->componentArrays[sourceArchetype->internalIndex[componentId]];
                Component::MoveElement(componentId, arrayPtr, record.row, srcArrayPtr, sourceRow);
            });
            sourceArchetype->RemoveRow(sourceRow);
            return record;
        }
    };
}

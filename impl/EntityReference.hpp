#pragma once

/**
 * @file EntityReference.hpp
 * @brief Entity reference and safe component access for the SECS library
 * @author Roberto Duarte
 * @date 2025
 * @copyright MIT License
 * 
 * @details
 * This file contains the EntityReference class which provides safe, versioned
 * handles to entities in the SECS Entity Component System. EntityReference
 * acts as a smart pointer that automatically validates entity existence and
 * prevents access to destroyed entities through version tracking.
 * 
 * Key features:
 * - **Automatic validation**: Access attempts return false for invalid entities
 * - **Version tracking**: Prevents access to destroyed entities
 * - **Lightweight design**: Only 4 bytes per reference (2x uint16_t)
 * - **Exception-free**: All operations use return codes instead of exceptions
 * - **Type-safe access**: Component access validated at compile-time
 * 
 * @par Design Philosophy:
 * EntityReference uses a generation counter approach where each entity slot
 * has a version number that increments when the entity is destroyed. This
 * allows references to detect when they point to a destroyed entity whose
 * slot has been reused, preventing dangerous access to invalid data.
 * 
 * @par Memory Efficiency:
 * Designed for memory-constrained environments like the Sega Saturn:
 * - Uses uint16_t indices (supports up to 65,535 entities)
 * - Minimal memory footprint (4 bytes per reference)
 * - No dynamic allocation required
 * - Cache-friendly access patterns
 * 
 * @par Thread Safety:
 * This class is NOT thread-safe. External synchronization is required
 * for multi-threaded access.
 * 
 * @par Compiler Requirements:
 * - C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
 * - Lambda expression support
 * - Template template parameters
 * 
 * @see World For entity creation and management
 * @see EntityRecord For entity metadata and lifecycle
 * @see ArchetypeManager For component storage details
 */

#include "Archetype.hpp"

namespace SECS
{
    /**
     * @brief Lightweight, versioned handle to an entity in the ECS system
     * 
     * @details
     * EntityReference provides a safe, versioned handle to an entity within
     * the ECS. It acts as a smart pointer that automatically validates entity
     * existence and prevents access to destroyed entities. The reference uses
     * a generation counter to detect when an entity has been destroyed and
     * its memory slot reused.
     * 
     * @par Key Features:
     * - **Automatic validation**: Access attempts return false for invalid entities
     * - **Version tracking**: Prevents access to destroyed entities
     * - **Lightweight**: Only 4 bytes (2x uint16_t) per reference
     * - **Exception-free**: All operations use return codes instead of exceptions
     * - **Type-safe access**: Component access is validated at compile-time
     * 
     * @par Memory Efficiency:
     * EntityReference is designed for memory-constrained environments:
     * - Uses uint16_t for indices (supports up to 65,535 entities)
     * - Minimal memory footprint (4 bytes per reference)
     * - No dynamic allocation
     * - Cache-friendly access patterns
     * 
     * @par Lifetime Management:
     * - References remain valid until the entity is explicitly destroyed
     * - Destroyed entity references become invalid but safe to use
     * - No manual memory management required
     * - Automatic cleanup when entity is destroyed
     * 
     * @par Thread Safety:
     * EntityReference is NOT thread-safe. External synchronization is required
     * for concurrent access from multiple threads.
     * 
     * @par Usage Examples:
     * @code{.cpp}
     * // Create entity and get reference
     * auto entity = World::CreateEntity<Position, Velocity>();
     * 
     * // Safe component access
     * bool success = entity.Access([](Position& pos, Velocity& vel) {
     *     pos.x += vel.dx;
     *     pos.y += vel.dy;
     * });
     * 
     * if (!success) {
     *     // Entity was destroyed or invalid
     *     // Handle gracefully without exceptions
     * }
     * 
     * // Destroy entity
     * entity.Destroy();
     * 
     * // Further access attempts will return false
     * bool stillValid = entity.Access([](Position& pos) {
     *     // This lambda will not execute
     * }); // stillValid will be false
     * @endcode
     * 
     * @see World::CreateEntity() For entity creation
     * @see World::GetIterator() For entity iteration
     * @see ArchetypeManager For internal storage details
     */
    class EntityReference
    {
    private:
        friend class World;

        Index recordIndex = InvalidIndex;
        Index version = InvalidIndex;

        /**
         * @brief Private constructor for creating an EntityReference from an EntityRecord.
         * @param record The EntityRecord to reference.
         */
        EntityReference(const EntityRecord& record) : recordIndex(record.GetIndex()), version(record.version) {}

    public:
        /**
         * @brief Default constructor for creating an empty EntityReference.
         */
        EntityReference() = default;

        /**
         * @brief Safely access the entity's components through a lambda function
         * 
         * @details
         * This method provides safe, validated access to an entity's components.
         * It automatically deduces the required component types from the lambda
         * signature and verifies that the entity is still valid before executing
         * the lambda. If the entity has been destroyed or is invalid, the lambda
         * will not execute and the method returns false.
         * 
         * @tparam Lambda A callable type (lambda, function, or functor) that accepts
         *                references to component types as parameters. The component
         *                types are automatically deduced from the lambda signature.
         * 
         * @param lambda A callable object that receives references to the entity's
         *               components. The lambda signature determines which components
         *               are accessed. All specified components must exist on the entity.
         * 
         * @return bool Returns true if the entity is valid and the lambda was executed
         *              successfully. Returns false if the entity is invalid, destroyed,
         *              or doesn't have the required components.
         * 
         * @par Performance Notes:
         * - Component types are validated at compile-time
         * - Entity validity check is performed at runtime (minimal overhead)
         * - Direct memory access to component data (no indirection)
         * - Cache-friendly access pattern within archetypes
         * 
         * @par Error Handling:
         * This method never throws exceptions. Invalid access attempts simply
         * return false, allowing for graceful error handling in resource-constrained
         * environments.
         * 
         * @par Component Type Deduction:
         * The method automatically determines which components to access based on
         * the lambda parameter types. All parameters must be references to component
         * types that exist on the entity.
         * 
         * @par Example Usage:
         * @code{.cpp}
         * // Access single component
         * bool success = entity.Access([](Position& pos) {
         *     pos.x += 10.0f;
         *     pos.y += 5.0f;
         * });
         * 
         * // Access multiple components
         * bool moved = entity.Access([](Position& pos, Velocity& vel) {
         *     pos.x += vel.dx;
         *     pos.y += vel.dy;
         *     
         *     // Boundary checking
         *     if (pos.x < 0) { pos.x = 0; vel.dx = -vel.dx; }
         *     if (pos.y < 0) { pos.y = 0; vel.dy = -vel.dy; }
         * });
         * 
         * // Handle access failure
         * if (!moved) {
         *     // Entity was destroyed or doesn't have required components
         *     // Handle gracefully (e.g., remove from active list)
         * }
         * 
         * // Read-only access (const references)
         * bool valid = entity.Access([](const Position& pos, const Health& hp) {
         *     if (hp.current <= 0) {
         *         // Entity is dead, handle accordingly
         *     }
         * });
         * @endcode
         * 
         * @par Best Practices:
         * - Always check the return value for critical operations
         * - Use const references for read-only access
         * - Keep lambda bodies lightweight for optimal performance
         * - Avoid capturing large objects in the lambda
         * 
         * @warning The lambda parameters must be references to component types.
         *          Value parameters or pointers will cause compilation errors.
         * 
         * @see World::CreateEntity() For entity creation
         * @see Destroy() For entity destruction
         * @see LambdaUtil For internal type deduction details
         */
        template <typename Lambda>
        bool Access(Lambda lambda)
        {
            bool status = false;
            if (recordIndex != InvalidIndex)
            {
                const EntityRecord& record = EntityRecord::records[recordIndex];
                if (version == record.version)
                {
                    ArchetypeManager& archetype = ArchetypeManager::managers[record.archetype];
                    using LambdaTraits = LambdaUtil<decltype(&Lambda::operator())>;
                    LambdaTraits::CallWithTypes([lambda, &archetype, &record]<typename ...Components>()
                    {
                        lambda(archetype.GetComponent<Components>(record.row)...);
                    });
                    status = true;
                }
            }
            return status;
        }

        /**
         * @brief Safely destroy the referenced entity and free its resources
         * 
         * @details
         * This method destroys the entity referenced by this EntityReference,
         * immediately freeing its component memory and making the entity slot
         * available for reuse. After destruction, this EntityReference becomes
         * invalid and all future Access() calls will return false.
         * 
         * @par Memory Management:
         * - Component memory is immediately freed
         * - Entity slot is marked for reuse
         * - Version counter is incremented to invalidate existing references
         * - No memory leaks or dangling pointers
         * 
         * @par Performance Notes:
         * - O(1) destruction time
         * - Minimal memory overhead
         * - Efficient memory reuse through slot recycling
         * - No dynamic allocation/deallocation
         * 
         * @par Safety Guarantees:
         * - Safe to call multiple times (subsequent calls are no-ops)
         * - Safe to call on invalid entities (no effect)
         * - No exceptions thrown under any circumstances
         * - Automatic invalidation of all references to this entity
         * 
         * @par Archetype Management:
         * When an entity is destroyed, its components are removed from the
         * archetype storage. If this was the last entity in an archetype,
         * the archetype remains allocated for future entities with the same
         * component combination.
         * 
         * @par Example Usage:
         * @code{.cpp}
         * // Create and use entity
         * auto entity = World::CreateEntity<Position, Health>();
         * entity.Access([](Position& pos, Health& hp) {
         *     pos.x = 100.0f;
         *     hp.current = 0; // Entity died
         * });
         * 
         * // Destroy entity when no longer needed
         * entity.Destroy();
         * 
         * // Further access attempts will fail safely
         * bool stillExists = entity.Access([](Position& pos) {
         *     // This lambda will never execute
         * }); // stillExists will be false
         * 
         * // Safe to call destroy again (no effect)
         * entity.Destroy(); // No-op
         * @endcode
         * 
         * @par Best Practices:
         * - Call Destroy() when entities are no longer needed
         * - Don't store EntityReferences after calling Destroy()
         * - Use Access() return value to detect destroyed entities
         * - Consider entity pooling for frequently created/destroyed entities
         * 
         * @warning After calling Destroy(), this EntityReference becomes invalid.
         *          Storing and later using destroyed EntityReferences will result
         *          in Access() calls returning false.
         * 
         * @see Access() For component access and validity checking
         * @see World::CreateEntity() For entity creation
         * @see ArchetypeManager::RemoveRow() For internal destruction details
         */
        void Destroy()
        {
            if (recordIndex != InvalidIndex)
            {
                const EntityRecord& record = EntityRecord::records[recordIndex];
                recordIndex = InvalidIndex;
                if (version == record.version)
                {
                    ArchetypeManager::managers[record.archetype].RemoveRow(record.row);
                }
            }
        }
    };
}

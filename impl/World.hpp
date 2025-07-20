#pragma once

/**
 * @file World.hpp
 * @brief Main ECS interface and entity management for the SECS library
 * @author Roberto Duarte
 * @date 2025
 * @copyright MIT License
 * 
 * @details
 * This file contains the World class which provides the primary interface
 * for entity creation, component access, and system iteration in the SECS
 * Entity Component System. The World class uses an archetype-based design
 * for optimal cache performance and efficient entity management.
 * 
 * Key features:
 * - **Static interface**: No instantiation required, global entity management
 * - **Type-safe operations**: Compile-time component type validation
 * - **Archetype-based storage**: Entities grouped by component combinations
 * - **Lambda-based initialization**: Efficient component setup patterns
 * - **Iterator support**: System-friendly entity iteration
 * 
 * @par Design Philosophy:
 * The World class follows a static design pattern where all operations are
 * performed through static methods. This approach eliminates the need for
 * World instance management and provides a clean, global interface for
 * entity operations throughout the application.
 * 
 * @par Archetype Management:
 * Entities are automatically organized into archetypes based on their
 * component combinations. This provides excellent cache locality during
 * system iteration and minimizes memory fragmentation.
 * 
 * @par Thread Safety:
 * This class is NOT thread-safe. External synchronization is required
 * for multi-threaded access.
 * 
 * @par Compiler Requirements:
 * - C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
 * - Template template parameters and variadic templates
 * - Lambda expression support
 * 
 * @see EntityReference For entity access and manipulation
 * @see ArchetypeManager For component storage details
 * @see Component For component type management
 */

#include "EntityReference.hpp"

namespace SECS
{
    /**
     * @brief Main interface for Entity Component System operations
     * 
     * @details
     * The World class provides the primary interface for creating entities,
     * accessing components, and iterating over entities in the ECS. It uses
     * an archetype-based design where entities with the same component
     * combination are stored together for optimal cache performance.
     * 
     * @par Key Features:
     * - Static interface (no instantiation required)
     * - Type-safe entity creation with compile-time validation
     * - Efficient archetype-based storage
     * - Lambda-based component initialization
     * - Iterator pattern for system processing
     * 
     * @par Memory Management:
     * The World manages memory automatically through archetype managers.
     * Each unique component combination creates a separate archetype
     * with its own memory pool. This design provides:
     * - Excellent cache locality for component access
     * - Efficient iteration over entities with specific components
     * - Minimal memory fragmentation
     * 
     * @par Thread Safety:
     * This class is NOT thread-safe. External synchronization is required
     * for concurrent access from multiple threads.
     * 
     * @par Usage Examples:
     * @code{.cpp}
     * // Create entity with lambda initialization
     * auto player = World::CreateEntity([](Position* pos, Velocity* vel) {
     *     pos->x = 100.0f; pos->y = 200.0f;
     *     vel->dx = 1.0f; vel->dy = 0.0f;
     * });
     * 
     * // Create empty entity and access later
     * auto enemy = World::CreateEntity<Position, Health>();
     * enemy.Access([](Position& pos, Health& hp) {
     *     pos.x = 50.0f; pos.y = 75.0f;
     *     hp.current = hp.max = 100;
     * });
     * 
     * // Iterate over entities with specific components
     * for (auto it = World::GetIterator<Position, Velocity>(); it; ++it) {
     *     it.Access([](Position& pos, Velocity& vel) {
     *         pos.x += vel.dx;
     *         pos.y += vel.dy;
     *     });
     * }
     * @endcode
     * 
     * @see EntityReference For entity manipulation
     * @see ArchetypeManager For internal storage details
     */
    struct World
    {
        /**
         * @brief Create a new entity with components using a lambda function for initialization
         * 
         * @details
         * This method creates a new entity and automatically deduces the required
         * component types from the lambda function signature. The lambda receives
         * pointers to uninitialized component data, allowing for efficient
         * in-place construction.
         * 
         * @tparam Lambda A callable type (lambda, function, or functor) that accepts
         *                pointers to component types as parameters
         * 
         * @param lambda A callable object that initializes the entity's components.
         *               The lambda signature determines which components the entity will have.
         *               Each parameter must be a pointer to a component type.
         * 
         * @return EntityReference A handle to the newly created entity. The reference
         *                        will be valid until the entity is destroyed.
         * 
         * @par Performance Notes:
         * - Component types are deduced at compile-time
         * - Memory is allocated from the appropriate archetype pool
         * - No dynamic allocation for component storage
         * - Initialization happens in-place for optimal performance
         * 
         * @par Error Handling:
         * If memory allocation fails, an invalid EntityReference is returned.
         * Always check the validity using EntityReference::Access().
         * 
         * @par Example Usage:
         * @code{.cpp}
         * // Create entity with Position and Velocity components
         * auto entity = World::CreateEntity([](Position* pos, Velocity* vel) {
         *     pos->x = 100.0f;
         *     pos->y = 200.0f;
         *     vel->dx = 1.0f;
         *     vel->dy = 0.0f;
         * });
         * 
         * // Create entity with single component
         * auto health_entity = World::CreateEntity([](Health* hp) {
         *     hp->current = hp->max = 100;
         * });
         * @endcode
         * 
         * @warning The lambda parameters must be pointers to component types.
         *          References or value parameters will cause compilation errors.
         * 
         * @see CreateEntity() For creating entities without initialization
         * @see EntityReference::Access() For accessing components after creation
         */
        template <typename Lambda>
        static EntityReference CreateEntity(Lambda lambda)
        {
            using LambdaTraits = LambdaUtil<decltype(&Lambda::operator())>;
            return LambdaTraits::CallWithTypes([&lambda]<typename ...Ts>()
            {
                auto& manager = ArchetypeManager::Helper<Ts...>::GetInstance();
                const EntityRecord& record = manager.ReserveRecord();
                lambda(&manager.template GetComponentArray<Ts>()[record.row] ...);
                return EntityReference(record);
            });
        }

        /**
         * @brief Create a new entity with explicitly specified component types
         * 
         * @details
         * This method creates a new entity with the specified component types
         * without initializing the component data. The components will contain
         * uninitialized memory and must be properly initialized before use
         * through EntityReference::Access().
         * 
         * @tparam Ts Variadic template parameter pack specifying the component
         *            types to include in the entity. Each type must satisfy
         *            the ComponentType concept (non-empty types).
         * 
         * @return EntityReference A handle to the newly created entity with
         *                        uninitialized components. The reference will
         *                        be valid until the entity is destroyed.
         * 
         * @par Performance Notes:
         * - Component types are validated at compile-time
         * - Memory is allocated from the appropriate archetype pool
         * - No component initialization overhead
         * - Fastest entity creation method
         * 
         * @par Memory Layout:
         * Components are stored in separate arrays within the archetype,
         * providing excellent cache locality when iterating over entities
         * with the same component combination.
         * 
         * @par Error Handling:
         * If memory allocation fails, an invalid EntityReference is returned.
         * Always verify entity validity before accessing components.
         * 
         * @par Example Usage:
         * @code{.cpp}
         * // Create entity with Position and Velocity (uninitialized)
         * auto entity = World::CreateEntity<Position, Velocity>();
         * 
         * // Initialize components after creation
         * bool success = entity.Access([](Position& pos, Velocity& vel) {
         *     pos.x = pos.y = 0.0f;
         *     vel.dx = vel.dy = 1.0f;
         * });
         * 
         * if (!success) {
         *     // Handle entity creation failure
         * }
         * 
         * // Create entity with single component
         * auto health_entity = World::CreateEntity<Health>();
         * health_entity.Access([](Health& hp) {
         *     hp.current = hp.max = 100;
         * });
         * @endcode
         * 
         * @par Best Practices:
         * - Always initialize components after creation
         * - Check EntityReference validity before use
         * - Prefer lambda initialization method for complex setup
         * - Use this method for performance-critical entity creation
         * 
         * @warning Components contain uninitialized memory after creation.
         *          Accessing uninitialized components leads to undefined behavior.
         * 
         * @see CreateEntity(Lambda) For entity creation with initialization
         * @see EntityReference::Access() For component initialization and access
         * @see ComponentType For component type requirements
         */
        template <typename... Ts>
        static EntityReference CreateEntity()
        {
            ArchetypeManager& manager = ArchetypeManager::Helper<Ts...>::GetInstance();
            return EntityReference(manager.ReserveRecord());
        }

        /**
         * @brief Represents an iterator for entities in the ECS world.
         */
        class EntityIterator
        {
            ArchetypeManager* currentManager = nullptr;
            Index currentRow = InvalidIndex;
            bool stop = false;
        public:
            /**
             * @brief Stops the current iteration.
             */
            void StopIteration() { stop = true; };

            /**
             * @brief Get a reference to the current entity in the iteration.
             * @return An EntityReference to the current entity, if available, or an empty one.
             */
            EntityReference GetCurrentEntity()
            {
                return (currentRow != InvalidIndex) ?
                    EntityReference(EntityRecord::records[currentManager->recordIndices[currentRow]]) :
                    EntityReference();
            }

            /**
             * @brief Iterate over entities with specified component types and execute a lambda function.
             * @tparam Lambda The lambda function to execute for each entity.
             * @param lambda The lambda function to execute for each entity, providing access to entity components.
             */
            template <typename Lambda>
            void Iterate(Lambda lambda)
            {
                stop = false;
                using LambdaTraits = LambdaUtil<decltype(&Lambda::operator())>;
                LambdaTraits::CallWithTypes([this, lambda]<typename ...Components>()
                {
                    using LookupCache = ArchetypeManager::LookupCache<Components...>;
                    LookupCache::Update();

                    for (size_t managerIndex : LookupCache::matchedIndices)
                    {
                        if (stop) break;

                        currentManager = &ArchetypeManager::managers[managerIndex];

                        [this, lambda](Components* ...componentArray)
                        {
                            for (currentRow = 0; !stop && currentRow < currentManager->size; currentRow++)
                            {
                                lambda(componentArray++...);
                            }
                        }(currentManager->template GetComponentArray<Components>() ...);
                    }
                });
                currentRow = InvalidIndex;
            }
        };
    };
};

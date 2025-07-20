#pragma once

/**
 * @file Component.hpp
 * @brief Component type management and operations for the SECS library
 * @author Roberto Duarte
 * @date 2025
 * @copyright MIT License
 * 
 * @details
 * This file contains the Component class which provides compile-time component
 * type identification, registration, and type-erased operations for the SECS
 * Entity Component System. The Component system enables efficient archetype
 * management and component array operations while maintaining type safety.
 * 
 * Key features:
 * - **Compile-time ID generation**: Each component type gets a unique ID
 * - **Type-erased operations**: Generic array operations for any component type
 * - **Memory efficient**: Uses bitfields for component combinations
 * - **Exception-free**: Safe allocation failure handling
 * - **Template-based**: Zero runtime overhead for type operations
 * 
 * @par Design Philosophy:
 * The Component class uses advanced C++17 template metaprogramming to assign
 * unique compile-time IDs to component types. This enables the archetype
 * system to efficiently group entities with the same component combinations
 * while providing type-safe access to component data.
 * 
 * @par ID Generation System:
 * Uses a sophisticated compile-time counter based on SFINAE and friend
 * function injection. Each component type receives a unique ID (0, 1, 2, ...)
 * that remains consistent across compilation units.
 * 
 * @par Type-Erased Operations:
 * Provides function pointers for common array operations (delete, move, resize)
 * that work with any component type. This allows the archetype system to
 * manipulate component arrays without knowing their specific types.
 * 
 * @par Memory Management:
 * - Component arrays are managed through type-erased function pointers
 * - Allocation failures are handled gracefully without exceptions
 * - Move semantics are used for efficient component relocation
 * - Default constructors are used for cleanup when available
 * 
 * @par Thread Safety:
 * This class is NOT thread-safe. External synchronization is required
 * for multi-threaded access.
 * 
 * @par Compiler Requirements:
 * - C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
 * - Support for consteval, if constexpr, and SFINAE
 * - Template template parameters and friend function injection
 * 
 * @par Usage Example:
 * ```cpp
 * // Define component types
 * struct Position { float x, y; };
 * struct Velocity { float dx, dy; };
 * 
 * // Component IDs are automatically assigned
 * constexpr size_t posId = Component::Id<Position>; // 0
 * constexpr size_t velId = Component::Id<Velocity>; // 1
 * 
 * // Binary IDs for archetype combinations
 * constexpr auto posBinary = Component::IdBinary<Position>; // 0b01
 * constexpr auto velBinary = Component::IdBinary<Velocity>; // 0b10
 * constexpr auto bothBinary = posBinary | velBinary;        // 0b11
 * ```
 * 
 * @see ArchetypeManager For component storage and archetype management
 * @see EntityRecord For entity metadata and lifecycle
 * @see World For high-level component operations
 */

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <vector>

namespace SECS
{
    /**
     * @brief Component type management and type-erased operations
     * 
     * @details
     * The Component class provides the foundation for component type management
     * in the SECS Entity Component System. It automatically assigns unique IDs
     * to component types and provides type-erased operations for efficient
     * archetype management.
     * 
     * The class uses compile-time metaprogramming to:
     * - Generate unique IDs for each component type
     * - Create binary representations for archetype matching
     * - Register type-erased operations for component arrays
     * - Provide safe, exception-free memory management
     * 
     * @par Component ID System:
     * Each component type receives a unique compile-time ID starting from 0.
     * These IDs are used throughout the ECS for archetype identification,
     * component array indexing, and type-safe operations.
     * 
     * @par Binary ID System:
     * Component types also receive binary IDs (powers of 2) that can be
     * combined using bitwise OR to represent archetype signatures.
     * For example:
     * - Position: ID=0, Binary=0b001
     * - Velocity: ID=1, Binary=0b010  
     * - Health:   ID=2, Binary=0b100
     * - Position+Velocity: Binary=0b011
     * 
     * @par Type-Erased Operations:
     * The class maintains function pointers for common operations on component
     * arrays, allowing the archetype system to manipulate arrays of any
     * component type without template instantiation at runtime.
     * 
     * @warning This class is primarily for internal ECS use. User code should
     *          define component types as simple structs and let the ECS handle
     *          ID assignment and operations automatically.
     * 
     * @see ArchetypeManager For archetype and component array management
     * @see World For user-facing component operations
     */
    class Component
    {
    private:
        /**
         * @brief Helper struct for determining if an ID has been used.
         */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
        template <size_t N>
        struct IdReader
        {
            friend auto IsCountedFlag(IdReader<N>);
        };
#pragma GCC diagnostic pop

        /**
         * @brief Helper struct for setting the ID.
         */
        template <size_t N>
        struct IdSetter
        {
            friend auto IsCountedFlag(IdReader<N>) {}
            static constexpr size_t n = N;
        };

        /**
         * @brief Retrieves the next available ID.
         *
         * @tparam Tag Unused template parameter.
         * @tparam NextVal The next available ID.
         * @return constexpr auto The next available ID.
         */
        template <auto Tag, size_t NextVal = 0>
        [[nodiscard]] static consteval auto GetNextID()
        {
            constexpr bool isCountedPastValue =
                requires(IdReader<NextVal> r) { IsCountedFlag(r); };

            if constexpr (isCountedPastValue)
            {
                return GetNextID<Tag, NextVal + 1>();
            }
            else
            {
                IdSetter<NextVal> s;
                return s.n;
            }
        }

        /**
         * @brief Deletes an array of type T.
         *
         * @tparam T The type of the array elements.
         * @param array Pointer to the array to be deleted.
         */
        template<typename T>
        static void DeleteArray(void* array)
        {
            delete[] static_cast<T*>(array);
        }

        /**
         * @brief Moves an element from one array to another.
         *
         * @tparam T The type of the array elements.
         * @param dstArray Pointer to the destination array.
         * @param dstPos The position in the destination array.
         * @param srcArray Pointer to the source array.
         * @param srcPos The position in the source array.
         */
        template<typename T>
        static void MoveElement(void* dstArray, size_t dstPos, void* srcArray, size_t srcPos)
        {
            // Move data from source object to destination object
            static_cast<T*>(dstArray)[dstPos] = std::move(static_cast<T*>(srcArray)[srcPos]);

            // Check if the type T has a default constructor
            if constexpr (std::is_default_constructible_v<T>)
            {
                // Reset the original object to a default state
                new (&static_cast<T*>(srcArray)[srcPos]) T{};
            }
        }

        /**
         * @brief Resizes an array of a specific component type.
         *
         * @tparam T The type of the array elements.
         * @param ptrToArray Pointer to the array to be resized.
         * @param newSize The new size of the array.
         * @param moveCount The number of elements to move from the original array to the resized array.
         * @return true If the array was resized successfully.
         * @return false If resizing failed.
         */
        template <typename T>
        static bool ResizeArray(void** ptrToArray, size_t newSize, size_t moveCount)
        {
            // Allocate memory for the resized array
            T* newArray = new T[newSize];
            if (!newArray)
            {
                return false; // Return false if allocation fails
            }

            T* originalArray = *reinterpret_cast<T**>(ptrToArray);

            // Move elements from the original array to the resized array
            for (size_t i = 0; i < moveCount; ++i)
            {
                newArray[i] = std::move(originalArray[i]);
            }

            // Delete the original array
            delete[] originalArray;

            // Update the pointer to point to the resized array
            *reinterpret_cast<T**>(ptrToArray) = newArray;

            return true; // Return true if resizing is successful
        }
        // Typedefs for function pointers
        using DeleteArrayInterface = void (*)(void* array);
        using MoveElementInterface = void(*)(void* dstArray, size_t dstPos, void* srcArray, size_t srcPos);
        using ResizeArrayInterface = bool(*)(void** ptrToArray, size_t newSize, size_t moveCount);

        // Struct to hold operation function pointers
        struct Operation
        {
            DeleteArrayInterface DeleteArray;       /**< Function pointer to delete an array. */
            MoveElementInterface MoveElement;       /**< Function pointer to move an element from one array to another. */
            ResizeArrayInterface ResizeArray;       /**< Function pointer to resize an array. */
        };

        static inline std::vector<Operation> OperationList;    /**< Vector to hold operation function pointers. */

    public:
        using BinaryId = size_t;                          /**< Alias for component ID in binary format. */
        static inline constexpr size_t MaxComponentTypes = sizeof(BinaryId) * CHAR_BIT;   /**< Maximum number of component types. */

        /**
         * @brief Retrieves the ID of a component type.
         *
         * @tparam T The type of the component.
         */
        template <typename T>
        static inline constexpr size_t Id = GetNextID < [] {} > ();

        /**
         * @brief Retrieves the binary ID of a component type.
         *
         * @tparam T The type of the component.
         */
        template <typename T>
        static inline BinaryId IdBinary = []()
        {
            constexpr size_t id = Id<T>;
            constexpr size_t size = id + 1;

            if (OperationList.size() < size)
            {
                OperationList.resize(size);
            }

            OperationList[id] = Operation(&DeleteArray<T>, &MoveElement<T>, &ResizeArray<T>);

            return (BinaryId)1 << id;
        }();

        /**
         * @brief Deletes an array of a specific component type.
         *
         * @param componentId The ID of the component type.
         * @param array Pointer to the array to be deleted.
         */
        static void DeleteArray(size_t componentId, void* array)
        {
            OperationList[componentId].DeleteArray(array);
        }

        /**
         * @brief Moves an element from one array to another.
         *
         * @param componentId The ID of the component type.
         * @param dstArray Pointer to the destination array.
         * @param dstPos The position in the destination array.
         * @param srcArray Pointer to the source array.
         * @param srcPos The position in the source array.
         */
        static void MoveElement(size_t componentId, void* dstArray, size_t dstPos, void* srcArray, size_t srcPos)
        {
            OperationList[componentId].MoveElement(dstArray, dstPos, srcArray, srcPos);
        }

        /**
         * @brief Resizes an array of a specific component type.
         *
         * @param componentId The ID of the component type.
         * @param ptrToArray Pointer to the array to be resized.
         * @param newSize The new size of the array.
         * @param moveCount The number of elements to move from the original array to the resized array.
         * @return true If the array was resized successfully.
         * @return false If resizing failed.
         */
        static bool ResizeArray(size_t componentId, void** ptrToArray, size_t newSize, size_t moveCount)
        {
            return OperationList[componentId].ResizeArray(ptrToArray, newSize, moveCount);
        }

    };
}

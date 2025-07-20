#pragma once

/**
 * @file secs.hpp
 * @brief Main include file for the SECS (Small Entity Component System) library
 * @author Roberto Duarte
 * @date 2025
 * @copyright MIT License
 * 
 * @details
 * SECS is a lightweight, high-performance Entity Component System designed for
 * resource-constrained environments, particularly retro gaming consoles like the
 * Sega Saturn. The library provides:
 * 
 * - **Header-only design**: No compilation required, just include and use
 * - **Exception-free**: Safe for environments without exception support
 * - **Memory efficient**: Optimized for systems with limited RAM (2-16MB)
 * - **Cache-friendly**: Archetype-based storage for optimal data locality
 * - **Type-safe**: Modern C++17 with compile-time type checking
 * - **Cross-platform**: Works on any system with C++17 support
 * 
 * @par Basic Usage Example:
 * @code{.cpp}
 * #include <secs.hpp>
 * 
 * // Define components
 * struct Position { float x, y; };
 * struct Velocity { float dx, dy; };
 * 
 * int main() {
 *     // Create entity with components
 *     auto entity = SECS::World::CreateEntity([](Position* pos, Velocity* vel) {
 *         pos->x = pos->y = 0.0f;
 *         vel->dx = vel->dy = 1.0f;
 *     });
 *     
 *     // Access and modify components
 *     entity.Access([](Position& pos, Velocity& vel) {
 *         pos.x += vel.dx;
 *         pos.y += vel.dy;
 *     });
 *     
 *     return 0;
 * }
 * @endcode
 * 
 * @par Memory Considerations:
 * For optimal performance on resource-constrained systems:
 * - Keep components under 32 bytes each
 * - Use fixed-size types (uint16_t instead of uint32_t where possible)
 * - Avoid dynamic allocations in components
 * - Limit to ~32 different component combinations
 * 
 * @par Thread Safety:
 * This library is NOT thread-safe. External synchronization is required
 * for multi-threaded access.
 * 
 * @par Compiler Requirements:
 * - C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
 * - Concepts support (via C++17 SFINAE techniques)
 * - Template template parameters
 * 
 * @see World For the main ECS interface
 * @see EntityReference For entity manipulation
 * @see Component For component type requirements
 */

// Ensure C++17 compatibility
#if __cplusplus < 201703L
    #error "SECS requires C++17 or later. Please use a compatible compiler."
#endif

// Core ECS types
#include "impl/Archetype.hpp"
#include "impl/Component.hpp"
#include "impl/EntityRecord.hpp"
#include "impl/EntityReference.hpp"
#include "impl/World.hpp"

/**
 * @namespace SECS
 * @brief Main namespace for the Small Entity Component System library
 * 
 * @details
 * The SECS namespace contains all public API elements for the Entity Component System.
 * All user code should interact with the library through this namespace.
 * 
 * Key classes:
 * - SECS::World: Main ECS interface for entity creation and system iteration
 * - SECS::EntityReference: Handle to an entity for component access and manipulation
 * - SECS::Component: Base functionality for component type management
 * 
 * @par Design Philosophy:
 * SECS follows the archetype-based ECS pattern where entities with the same
 * component combination are stored together in memory for optimal cache performance.
 * This design is particularly beneficial for systems with limited memory bandwidth.
 */
namespace SECS {
    // Main types are defined in the included headers
    // No additional aliases needed - use classes directly
}

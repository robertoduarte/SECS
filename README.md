# SECS - Small Entity Component System

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A lightweight, header-only Entity Component System (ECS) library designed specifically for retro game consoles and resource-constrained environments. Optimized for the **Sega Saturn** and other classic gaming platforms with severe memory limitations.

## 🎮 Why SECS?

Retro consoles like the Sega Saturn have unique constraints:
- **Limited RAM**: Saturn has ~2MB total system RAM
- **No Exception Handling**: Exception support adds overhead
- **Cache Efficiency**: SH-2 processor benefits from data locality
- **Code Size**: Template instantiation must be controlled

**Thinking about adding SECS to your game?**
- Will having SECS on my game be good? Absolutely – it boosts performance and simplifies entity management!
- Is SECS easy to integrate? Yes, it's header-only and straightforward for any project.
- Can SECS handle complex game logic? Definitely, with its archetype-based system for efficient updates.

**Why choose SECS?**
- SECS will definitely make you happier with your game performance by optimizing RAM and cache usage.
- Experience smoother gameplay and faster development – SECS is designed to make your life easier and your games shine.

## ✨ Features

- **🏗️ Header-only**: Just include and use, no building required
- **⚡ Ultra-lightweight**: Minimal overhead for 2MB RAM systems
- **🚀 Cache-optimized**: Components stored in separate arrays for SH-2 efficiency
- **🛡️ Exception-free**: No exception handling for maximum compatibility
- **🔧 Type-safe**: Modern C++17 design with compile-time type checking
- **📦 Zero dependencies**: Uses only standard C++17 features
- **🎯 Archetype-based**: Efficient iteration over entities with same components

## 🎯 Target Platforms

| Platform | RAM | Status | Notes |
|----------|-----|--------|---------|
| **Sega Saturn** | 2MB | ✅ Primary | SH-2 optimized |
| PlayStation 1 | 2MB | ✅ Supported | R3000 compatible |
| Nintendo 64 | 4-8MB | ✅ Supported | MIPS compatible |
| Dreamcast | 16MB | ✅ Supported | SH-4 compatible |
| Modern systems | Any | ✅ Supported | Development/testing |

## 🚀 Getting Started

### Prerequisites

- **C++17 compatible compiler** (GCC 7+, Clang 5+, MSVC 2017+, or any cross-compiler with C++17 support)
- **Doxygen** (optional, for documentation generation)

> **Note**: No platform-specific SDKs required. SECS works on any platform with a C++17 compliant compiler.

### Installation

Simply copy the `secs.hpp` header file to your project's include path, or add this repository as a submodule.

```bash
# As a submodule
git submodule add https://github.com/robertoduarte/SECS.git
```

## 📖 Basic Usage

### Simple Entity Creation

```cpp
#include <secs.hpp>

// Define lightweight components (keep under 32 bytes for Saturn)
struct Position { 
    float x, y; 
};

struct Velocity { 
    float dx, dy; 
};

struct Health { 
    uint16_t current, max;  // Use uint16_t to save memory
};

int main() {
    // Method 1: Create entity with lambda initialization (recommended)
    auto player = SECS::World::CreateEntity([](Position* pos, Velocity* vel, Health* hp) {
        pos->x = 160.0f;  // Center of Saturn screen (320x224)
        pos->y = 112.0f;
        vel->dx = 0.0f;
        vel->dy = 0.0f;
        hp->current = hp->max = 100;
    });
    
    // Method 2: Create empty entity and access later
    auto enemy = SECS::World::CreateEntity<Position, Velocity>();
    enemy.Access([](Position& pos, Velocity& vel) {
        pos.x = 50.0f;
        pos.y = 50.0f;
        vel.dx = 1.0f;
        vel.dy = 0.5f;
    });
    
    // Game loop
    while (true) {
        // Update all entities with Position and Velocity
        SECS::World::EntityIterator iterator;
        iterator.Iterate([](Position& pos, Velocity& vel) {
            pos.x += vel.dx;
            pos.y += vel.dy;
        });
        
        // Destroy entity when done
        enemy.Destroy();
        break;
    }
    
    return 0;
}
```

### Advanced Usage

```cpp
// Entity iteration over specific component combinations
SECS::World::EntityIterator iterator;
iterator.Iterate([](Position& pos, Health& hp) {
    if (hp.current <= 0) {
        // Handle entity death
        // Can call iterator.StopIteration() to break early if needed
    }
});

// Safe component access with error handling
bool success = entity.Access([](Position& pos) {
    pos.x = 100.0f;
});

if (!success) {
    // Entity was destroyed or invalid - handle gracefully
}
```

## 📚 Documentation

- **[Usage Examples](USAGE_EXAMPLES.md)** - Comprehensive code examples
- **API Documentation** - See header files for detailed Doxygen comments

## 🎯 Saturn-Specific Guidelines

### Memory Management
- Keep components under 32 bytes each
- Use `uint16_t` instead of `uint32_t` where possible
- Avoid `std::string` and dynamic containers
- Pre-allocate entity pools when possible

### Performance Tips
- Limit to ~32 different component combinations
- Group related components together
- Use archetype iteration for cache efficiency
- Avoid random entity access patterns

### Error Handling
- No exceptions - check return values and entity validity
- Use debug assertions for development
- Handle allocation failures gracefully

## 🤝 Contributing

Contributions are welcome! Please ensure your code follows the existing style and includes appropriate documentation.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by various ECS implementations
- Special thanks to the retro gaming community

### Project Origins

This work was derived from [HyperionEngine](https://github.com/robertoduarte/HyperionEngine), which served as the foundational base for this project and several other Saturn development libraries including:

- [SaturnRingLib](https://github.com/ReyeMe/SaturnRingLib) - Saturn development utilities
- [SaturnMathPP](https://github.com/robertoduarte/SaturnMathPP) - Mathematical operations for Saturn

As HyperionEngine evolved to support multiple Saturn development projects, the ECS features became increasingly specialized. To maintain focus and enable dedicated development, the Entity Component System functionality was extracted and moved to this dedicated SECS library, allowing for more targeted optimization and feature development specifically for ECS use cases.

# SECS Usage Examples

## Basic Entity Management

### Creating Entities

```cpp
#include <secs.hpp>

// Define components
struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Sprite { uint32_t textureId; };  // Changed to uint32_t for broader compatibility

// Method 1: Create entity with lambda initialization
auto player = SECS::World::CreateEntity([](Position* pos, Velocity* vel) {
    pos->x = 100.0f;
    pos->y = 200.0f;
    vel->dx = 0.0f;
    vel->dy = 0.0f;
});

// Method 2: Create empty entity and access components later
auto enemy = SECS::World::CreateEntity<Position, Velocity, Sprite>();
enemy.Access([](Position& pos, Velocity& vel, Sprite& spr) {
    pos = {50.0f, 100.0f};
    vel = {-10.0f, 0.0f};
    spr.textureId = 42;
});
```

### Accessing Entity Components

```cpp
// Safe component access with validation
bool success = player.Access([](Position& pos, Velocity& vel) {
    pos.x += vel.dx;
    pos.y += vel.dy;
});

if (!success) {
    // Entity was destroyed or invalid
    // Handle gracefully without exceptions
}
```

### Entity Destruction

```cpp
// Destroy entity - automatically handles cleanup
player.Destroy();

// Accessing destroyed entity returns false
bool stillExists = player.Access([](Position& pos) {
    // This won't execute
});
```

## System Patterns

### Movement System

```cpp
void UpdateMovement(float screenWidth, float screenHeight) {
    // Iterate over all entities with Position and Velocity
    SECS::World::EntityIterator iterator;
    iterator.Iterate([=](Position& pos, Velocity& vel) {
        pos.x += vel.dx;
        pos.y += vel.dy;
        
        // Screen bounds checking
        if (pos.x < 0 || pos.x > screenWidth) vel.dx = -vel.dx;
        if (pos.y < 0 || pos.y > screenHeight) vel.dy = -vel.dy;
    });
}
```

### Rendering System

```cpp
struct RenderData {
    uint32_t textureId;    // Generic texture identifier
    uint8_t renderLayer;   // Rendering layer/priority
};

void RenderSprites() {
    // Collect all renderable entities
    SECS::World::EntityIterator iterator;
    iterator.Iterate([](const Position& pos, const RenderData& render) {
        // Generic rendering call
        RenderTexture(render.textureId, pos.x, pos.y, render.renderLayer);
    });
}
```

## Memory-Efficient Patterns

### Component Design

```cpp
// Good: Compact components
struct Transform2D {
    float x, y;           // Position
    float rotation;        // Rotation in radians
};

// Good: Bitfield flags
struct EntityFlags {
    uint8_t active : 1;
    uint8_t visible : 1;
    uint8_t collidable : 1;
    uint8_t reserved : 5;  // Future use
};

// Avoid: Large components with dynamic allocation
struct BadComponent {
    std::string name;      // Dynamic allocation
    float matrix[16];      // Large fixed-size array
    std::vector<int> data; // Dynamic allocation
};
```

### Entity Pooling Pattern

```cpp
class GameEntityManager {
private:
    static constexpr size_t MAX_BULLETS = 100;  // Increased pool size
    std::array<SECS::EntityReference, MAX_BULLETS> bulletPool;
    size_t nextBullet = 0;
    
public:
    SECS::EntityReference CreateBullet(float x, float y, float dx, float dy) {
        // Reuse entity from pool
        auto& bullet = bulletPool[nextBullet];
        nextBullet = (nextBullet + 1) % MAX_BULLETS;
        
        // If entity exists, reuse it; otherwise create new
        bool reused = bullet.Access([x, y, dx, dy](Position& pos, Velocity& vel) {
            pos = {x, y};
            vel = {dx, dy};
        });
        
        if (!reused) {
            bullet = SECS::World::CreateEntity<Position, Velocity>();
            bullet.Access([x, y, dx, dy](Position& pos, Velocity& vel) {
                pos = {x, y};
                vel = {dx, dy};
            });
        }
        
        return bullet;
    }
};
```

## Error Handling Without Exceptions

### Safe Entity Operations

```cpp
// Check if entity creation succeeded
auto entity = SECS::World::CreateEntity<Position>();
bool isValid = entity.Access([](Position& pos) {
    pos.x = 0;
    pos.y = 0;
    return true;  // Dummy return for validation
});

if (!isValid) {
    // Handle entity creation failure
    // Maybe skip this frame or use fallback behavior
    return;
}
```

### Memory Allocation Monitoring

```cpp
#ifdef DEBUG
void CheckMemoryUsage() {
    // Add custom memory tracking
    size_t entityCount = 0;
    size_t componentCount = 0;
    
    // Count entities and components
    SECS::World::EntityIterator iterator;
    iterator.Iterate([&](auto&... components) {
        entityCount++;
        componentCount += sizeof...(components);
    });
    
    // Log memory usage for debugging
    printf("Entities: %zu, Total Components: %zu\n", entityCount, componentCount);
}
#endif
```

## Game Loop Integration

### Basic Game Loop Example

```cpp
void GameLoop() {
    // Main game loop
    while (true) {
        // Process input
        if (!ProcessInput()) {
            break;  // Exit if input indicates game should end
        }
        
        // Update game state
        UpdateMovement();
        UpdateCollision();
        UpdateAnimation();
        
        // Render frame
        ClearScreen();
        RenderSprites();
        RenderUI();
        SwapBuffers();
        
        // Debug information
        #ifdef DEBUG
        CheckMemoryUsage();
        #endif
    }
}
```

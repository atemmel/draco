#include "systems/entity.hpp"

#include "array.hpp"

constexpr s64 max_entities = 1024;

static Array<Entity> entity_list;

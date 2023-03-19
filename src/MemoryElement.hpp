#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Structure for storing a memory element's label and value.
 */
class MemoryElement
{
public:
    std::string label;
    int32_t     value;

    static int32_t sort_memory(const MemoryElement&,
                               const MemoryElement&);
};

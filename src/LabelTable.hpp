#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Struct ure for storing a label and its address.
 */
struct LabelTable
{
    std::string label;
    int32_t     address;
};
#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Struct ure for storing a label and its address.
 */
class LabelTable
{
public:
    std::string label;
    int32_t     address;

    static int32_t sort_table(const LabelTable &a, const LabelTable &b);
};
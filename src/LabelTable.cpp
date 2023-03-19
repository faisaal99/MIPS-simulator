#include <LabelTable.hpp>

/**
 * @brief TODO:
 * @param a
 * @param b
 * @return
*/
int32_t LabelTable::sort_table(const LabelTable &a, const LabelTable &b)
{
    return a.label < b.label;
}
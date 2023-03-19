#include <MemoryElement.hpp>

int32_t MemoryElement::sort_memory(
	const MemoryElement &a,
	const MemoryElement &b
)
{
	return a.label < b.label;
}
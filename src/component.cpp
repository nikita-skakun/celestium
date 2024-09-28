#include "component.h"

template <>
struct magic_enum::customize::enum_range<PowerConnectorComponent::IO>
{
    static constexpr bool is_flags = true;
};
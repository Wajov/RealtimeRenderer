#ifndef QUEUE_FAMILY_INDICES_HPP
#define QUEUE_FAMILY_INDICES_HPP

#include <cstdint>

struct QueueFamilyIndices {
    int32_t graphicsFamilyIndex = -1;
    int32_t presentFamilyIndex = -1;

    bool IsComplete() const
    {
        return graphicsFamilyIndex >= 0 && presentFamilyIndex >= 0;
    }
};

#endif
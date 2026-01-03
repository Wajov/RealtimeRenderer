#ifndef QUEUE_FAMILY_INDICES_HPP
#define QUEUE_FAMILY_INDICES_HPP

#include <cstdint>

class QueueFamilyIndices {
public:
    QueueFamilyIndices();
    ~QueueFamilyIndices() = default;
    void SetGraphicsFamilyIndex(int32_t graphicsFamilyIndex);
    int32_t GetGraphicsFamilyIndex() const;
    void SetPresentFamilyIndex(int32_t presentFamilyIndex);
    int32_t GetPresentFamilyIndex() const;
    bool IsComplete() const;

private:
    int32_t graphicsFamilyIndex_;
    int32_t presentFamilyIndex_;
};

#endif
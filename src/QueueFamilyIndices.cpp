#include "QueueFamilyIndices.hpp"

QueueFamilyIndices::QueueFamilyIndices()
    : graphicsFamilyIndex_(-1), presentFamilyIndex_(-1) {}

void QueueFamilyIndices::SetGraphicsFamilyIndex(int32_t graphicsFamilyIndex)
{
    graphicsFamilyIndex_ = graphicsFamilyIndex;
}

int32_t QueueFamilyIndices::GetGraphicsFamilyIndex() const
{
    return graphicsFamilyIndex_;
}

void QueueFamilyIndices::SetPresentFamilyIndex(int32_t presentFamilyIndex)
{
    presentFamilyIndex_ = presentFamilyIndex;
}

int32_t QueueFamilyIndices::GetPresentFamilyIndex() const
{
    return presentFamilyIndex_;
}

bool QueueFamilyIndices::IsComplete() const
{
    return graphicsFamilyIndex_ >= 0 && presentFamilyIndex_ >= 0;
}
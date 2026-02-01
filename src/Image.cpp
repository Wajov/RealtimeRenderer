#include <stb_image.h>

#include "Image.hpp"

Image::Image(const std::string& path)
{
    pixels_ = stbi_load(path.c_str(), &width_, &height_, &channels_, STBI_rgb_alpha);
    isValid_ = (pixels_ != nullptr);
}

Image::~Image()
{
    stbi_image_free(pixels_);
}

bool Image::IsValid() const
{
    return isValid_;
}

int32_t Image::GetWidth() const
{
    return width_;
}

int32_t Image::GetHeight() const
{
    return height_;
}

int32_t Image::GetChannels() const
{
    return channels_;
}

unsigned char* Image::GetPixels() const
{
    return pixels_;
}
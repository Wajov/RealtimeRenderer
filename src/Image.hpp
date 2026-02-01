#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <string>

class Image {
public:
    Image(const std::string& path);
    ~Image();
    bool IsValid() const;
    int32_t GetWidth() const;
    int32_t GetHeight() const;
    int32_t GetChannels() const;
    unsigned char* GetPixels() const;

private:
    bool isValid_;
    int32_t width_, height_, channels_;
    unsigned char* pixels_;
};

#endif
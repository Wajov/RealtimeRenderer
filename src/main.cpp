#include <iostream>
#include <cstdlib>

#include "Renderer.hpp"

constexpr uint32_t WIDTH = 1920;
constexpr uint32_t HEIGHT = 1080;

int main()
{
    Renderer renderer(WIDTH, HEIGHT);
    renderer.Run();

    return EXIT_SUCCESS;
}
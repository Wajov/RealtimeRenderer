#include <iostream>
#include <cstdlib>

#include "Renderer.hpp"

constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;

int main() {
    Renderer renderer(WIDTH, HEIGHT);
    renderer.Run();

    return EXIT_SUCCESS;
}
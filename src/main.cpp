#include <iostream>
#include <cstdlib>

#include "Renderer.hpp"

constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;

int main() {
    Renderer renderer(WIDTH, HEIGHT);

    try {
        renderer.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
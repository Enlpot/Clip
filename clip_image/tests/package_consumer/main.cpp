#include <clip/image/service.h>
#include <clip/image/version.h>

#include <iostream>

int main() {
    clip::image::Service service;
    if (service.formats().empty() || CLIP_IMAGE_VERSION_MAJOR != 1) {
        return 1;
    }
    std::cout << service.formats().size() << " formats\n";
    return 0;
}

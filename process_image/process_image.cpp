/**
 * @file process_image.cpp
 * @author Mauricio David Correa Hernández.
 * @brief:  Implements of the Image class - Class that provides image processing operations such as scaling and rotation.
 * @version 1.1
 * @date 2025-05-01
 * @copyright Copyright (c) 2025
 */


#include "process_image.h" // Include the header file


/**
 * @brief Constructor to initialize an image.
 * @param img Pointer to an Image object.
 */
ProcessImage::ProcessImage(Image *img) : image(img) {}


/**
 * @brief Scales an image using bilinear interpolation.
 * @param image Pointer to the input image data.
 * @param width Width of the input image.
 * @param high Height of the input image.
 * @param channels Number of color channels in the image.
 * @param scalar Scaling factor.
 * @param buddy Flag to use the Buddy Allocator.
 * @param buddyAllocator Pointer to a BuddyAllocator (default is nullptr).
 * @return Image_info structure containing the scaled image and metadata.
 */

Image_info ProcessImage::image_scaling(const unsigned char* image, int width, int high, int channels, float scalar, bool buddy, BuddyAllocator* buddyAllocator) {
    Image_info scaled_info;
    MeasureMemoryTime measureMemoryTime;

    int new_width, new_high;
    float scala_x, scala_y;
    
    // Calculate new dimensions
    new_width = scalar * width;
    new_high = scalar * high;
    scala_x = (float)width / new_width;
    scala_y = (float)high / new_high;
    
    scaled_info.new_width = new_width;
    scaled_info.new_high = new_high;
    
    cout << "| [INFO] Scaling factor on the x-axis: " << scala_x << endl;
    cout << "| [INFO] Scaling factor on the y-axis: " << scala_y << endl;
    
    // Create new image buffer using BuddyAllocator if needed
    size_t image_size = new_width * new_high * channels;
    unsigned char* scaled_image = nullptr;
    cout << "| [INFO] Scaled image size: " << (image_size / (1024 * 1024)) << " MB" << endl;

    if (buddy) {
        buddyAllocator = new BuddyAllocator(image_size);
        scaled_image = static_cast<unsigned char*>(buddyAllocator->alloc(image_size));
    
        if (!scaled_image) {
            cerr << "Error: Could not allocate memory with BuddyAllocator for Scaled Image.\n";
            exit(1);
        }
    } else {
        scaled_image = new unsigned char[image_size];
    }
    
    long memory_for_scaling = measureMemoryTime.get_memory_usage();
    long memory_for_loop = 0;

    #pragma omp parallel for collapse(2) schedule(static)
    for (int y = 0; y < new_high; y++) {
        for (int x = 0; x < new_width; x++) {
            float src_x = x * scala_x;
            float src_y = y * scala_y;

            int x0 = (int)src_x;
            int y0 = (int)src_y;
            int x1 = std::min(x0 + 1, width - 1);
            int y1 = std::min(y0 + 1, high - 1);

            float delta_x = src_x - x0;
            float delta_y = src_y - y0;

            for (int channel = 0; channel < channels; channel++) {
                unsigned char neighboring_pixel_00 = image[(y0 * width + x0) * channels + channel];
                unsigned char neighboring_pixel_10 = image[(y0 * width + x1) * channels + channel];
                unsigned char neighboring_pixel_01 = image[(y1 * width + x0) * channels + channel];
                unsigned char neighboring_pixel_11 = image[(y1 * width + x1) * channels + channel];

                float value =   (1 - delta_x) * (1 - delta_y) * neighboring_pixel_00 +
                                delta_x * (1 - delta_y) * neighboring_pixel_10 +
                                (1 - delta_x) * delta_y * neighboring_pixel_01 +
                                delta_x * delta_y * neighboring_pixel_11;

                scaled_image[(y * new_width + x) * channels + channel] = (unsigned char)value;
            }
        }
    }

    // Memory usage in each iteration
    memory_for_loop = measureMemoryTime.get_memory_usage();
    if (memory_for_scaling < memory_for_loop){
        memory_for_scaling = memory_for_loop;
    }
    
    scaled_info.memory_used_for_process = memory_for_scaling;
    scaled_info.new_data = scaled_image;
    
    return scaled_info;
}


/**
 * @brief Rotates an image by a given angle using bilinear interpolation.
 * @param image Pointer to the input image data.
 * @param width Width of the input image.
 * @param high Height of the input image.
 * @param channels Number of color channels in the image.
 * @param theta Rotation angle in degrees.
 * @param buddy Flag to use the Buddy Allocator.
 * @param buddyAllocator Pointer to a BuddyAllocator (default is nullptr).
 * @return Image_info structure containing the rotated image and metadata.
 */
Image_info ProcessImage::image_rotation(const unsigned char* image, int width, int high, int channels, float theta, bool buddy, BuddyAllocator* buddyAllocator) {
    Image_info rotated_info;
    MeasureMemoryTime measureMemoryTime;

    const float radians = theta * M_PI / 180.0f;
    const float cos_theta = std::cos(radians);
    const float sin_theta = std::sin(radians);

    const int x_center = width / 2;
    const int y_center = high / 2;

    const int new_width = std::abs(width * cos_theta) + std::abs(high * sin_theta);
    const int new_high = std::abs(width * sin_theta) + std::abs(high * cos_theta);
    rotated_info.new_width = new_width;
    rotated_info.new_high = new_high;

    const int new_x_center = new_width / 2;
    const int new_y_center = new_high / 2;

    const size_t image_size = static_cast<size_t>(new_width) * new_high * channels;
    unsigned char* rotated_image = nullptr;
    std::cout << "| [INFO] Rotated image size: " << (image_size / (1024 * 1024)) << " MB\n";

    if (buddy) {
        buddyAllocator = new BuddyAllocator(image_size);
        rotated_image = static_cast<unsigned char*>(buddyAllocator->alloc(image_size));
        if (!rotated_image) {
            std::cerr << "Error: Could not allocate memory with BuddyAllocator.\n";
            exit(1);
        }
    } else {
        rotated_image = new unsigned char[image_size];
    }

    long memory_for_rotation = measureMemoryTime.get_memory_usage();

    int max_threads = omp_get_max_threads();
    omp_set_num_threads(max_threads);
    std::cout << "| [INFO] Using " << max_threads << " threads\n";

    #pragma omp parallel for collapse(2) schedule(static)
    for (int y = 0; y < new_high; y++) {
        for (int x = 0; x < new_width; x++) {
            const float x_orig = (x - new_x_center) * cos_theta - (y - new_y_center) * sin_theta + x_center;
            const float y_orig = (x - new_x_center) * sin_theta + (y - new_y_center) * cos_theta + y_center;

            const int x0 = static_cast<int>(x_orig);
            const int y0 = static_cast<int>(y_orig);

            if (x0 >= 0 && x0 < width - 1 && y0 >= 0 && y0 < high - 1) {
                const int x1 = std::min(x0 + 1, width - 1);
                const int y1 = std::min(y0 + 1, high - 1);
                const float dx = x_orig - x0;
                const float dy = y_orig - y0;

                for (int c = 0; c < channels; ++c) {
                    const unsigned char p00 = image[(y0 * width + x0) * channels + c];
                    const unsigned char p10 = image[(y0 * width + x1) * channels + c];
                    const unsigned char p01 = image[(y1 * width + x0) * channels + c];
                    const unsigned char p11 = image[(y1 * width + x1) * channels + c];

                    const float val = (1 - dx) * (1 - dy) * p00 +
                                      dx * (1 - dy) * p10 +
                                      (1 - dx) * dy * p01 +
                                      dx * dy * p11;

                    rotated_image[(y * new_width + x) * channels + c] = static_cast<unsigned char>(val);
                }
            } else {
                for (int c = 0; c < channels; ++c) {
                    rotated_image[(y * new_width + x) * channels + c] = 0x00;
                }
            }
        }
    }

    rotated_info.memory_used_for_process = memory_for_rotation;
    rotated_info.new_data = rotated_image;

    return rotated_info;
}

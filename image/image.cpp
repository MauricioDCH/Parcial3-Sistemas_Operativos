/**
 * @file image.h
 * @author Mauricio David Correa Hernández.
 * @brief:  Implements of the Image class - Class representing an image and allowing its manipulation.
 * @version 1.1
 * @date 2025-05-01
 * @copyright Copyright (c) 2025
 */


#include "image.h" // Include the header file


/**
 * @brief Constructor that loads an image from a file.
 * @param file_name Name of the image file to load.
 * @param allocator Optional pointer to a BuddyAllocator for memory management.
 */
Image::Image(const std::string &file_name, BuddyAllocator *allocator)
    : allocator(allocator) {

    unsigned char* buffer = stbi_load(file_name.c_str(), &width, &high, &channels, 0);
    if (!buffer) {
        cerr << "Error: Could not load image '" << file_name << "'.\n";
        exit(1);
    }

    convert_buffer_to_matrix(buffer);
    stbi_image_free(buffer);
}


/**
 * @brief Constructor that loads an image from a memory buffer.
 * @param data Pointer to the image data.
 * @param high Image height.
 * @param width Image width.
 * @param channels Number of color channels.
 * @param allocator Optional pointer to a BuddyAllocator for memory management.
 */
Image::Image(unsigned char* data, int high, int width, int channels, BuddyAllocator* allocator)
    : high(high), width(width), channels(channels), allocator(allocator) {
    
    if(!data) {
        cerr << "Error: Could not load image from buffer.\n";
        exit(1);
    }
    
    convert_buffer_to_matrix(data);
    stbi_image_free(data);
}


/**
 * @brief Destructor that releases memory used by the image.
 */
Image::~Image() {
    if (!allocator) {
        for (int position_in_y = 0; position_in_y < high; position_in_y++) {
            for (int position_in_x = 0; position_in_x < width; position_in_x++) {
                delete[] pixels[position_in_y][position_in_x];
            }
            delete[] pixels[position_in_y];
        }
        delete[] pixels;
    }
}


// Getters implementation
int Image::get_high() const { return high; } ///< Gets the image height.


int Image::get_width() const { return width; } ///< Gets the image width.


int Image::get_channels() const { return channels; } ///< Gets the number of color channels in the image.


unsigned char*** Image::get_pixels() const { return pixels; } ///< Gets the image pixels in a matrix format.


///< Gets the image data in a linear buffer.
unsigned char* Image::get_data() const {
    // Calculate the total buffer size
    int total_size = high * width * channels;
    
    // Allocate memory for the output buffer
    unsigned char* data_buffer = new unsigned char[total_size];

    int index = 0;
    for (int position_in_y = 0; position_in_y < high; position_in_y++) {
        for (int position_in_x = 0; position_in_x < width; position_in_x++) {
            for (int channel = 0; channel < channels; channel++) {
                data_buffer[index++] = pixels[position_in_y][position_in_x][channel];
            }
        }
    }

    return data_buffer;
}


/**
 * @brief Converts a linear image buffer into a 3D pixel matrix [high][width][channels].
 * @param buffer Pointer to the flat image data.
 */
void Image::convert_buffer_to_matrix(unsigned char* buffer) {
    pixels = new unsigned char**[high];

    int max_threads = omp_get_max_threads();
    omp_set_num_threads(max_threads);
    std::cout << "| [INFO] Using " << max_threads << " threads\n";

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < high; y++) {
        pixels[y] = new unsigned char*[width];
        for (int x = 0; x < width; x++) {
            pixels[y][x] = new unsigned char[channels];
            int base_index = (y * width + x) * channels;
            for (int c = 0; c < channels; c++) {
                pixels[y][x][c] = buffer[base_index + c];
            }
        }
    }
}


/**
 * @brief Saves the image to a file.
 * @param file_name Name of the output file.
 * @param buddy Boolean Indicates whether to use BuddyAllocator for memory.
 * @param allocator Optional pointer to a BuddyAllocator.
 */
void Image::save_image(const std::string &file_name, bool buddy, BuddyAllocator* allocator) const {
    size_t buffer_size = static_cast<size_t>(high) * width * channels;
    unsigned char* buffer = nullptr;

    if (buddy) {
        if (!allocator) {
            std::cerr << "Error: BuddyAllocator not initialized.\n";
            exit(1);
        }

        buffer = static_cast<unsigned char*>(allocator->alloc(buffer_size));
        if (!buffer) {
            std::cerr << "Error: Memory allocation with BuddyAllocator failed.\n";
            exit(1);
        }
    } else {
        buffer = new unsigned char[buffer_size];
    }


    int max_threads = omp_get_max_threads();
    omp_set_num_threads(max_threads);
    std::cout << "| [INFO] Using " << max_threads << " threads\n";

    #pragma omp parallel for collapse(2) schedule(static)
    for (int y = 0; y < high; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                size_t index = static_cast<size_t>(y) * width * channels + x * channels + c;
                buffer[index] = pixels[y][x][c];
            }
        }
    }

    if (!stbi_write_png(file_name.c_str(), width, high, channels, buffer, width * channels)) {
        std::cerr << "Error: Could not save image '" << file_name << "'.\n";
        if (buddy) allocator->free(buffer, buffer_size);
        else delete[] buffer;
        exit(1);
    }

    std::cout << "| [INFO] Image successfully saved.\n";

    if (buddy) {
        allocator->free(buffer, buffer_size);
    } else {
        delete[] buffer;
    }
}

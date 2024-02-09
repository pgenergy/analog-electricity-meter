//
// Created by SlepiK on 29.01.24.
//

#ifndef ENERGYLEAF_STREAM_V1_TYPES_IMAGE_HPP
#define ENERGYLEAF_STREAM_V1_TYPES_IMAGE_HPP

#include <cstdint>
#include <utility>
#include "ImageFormat.hpp"
#include <Arduino.h>

namespace Energyleaf::Stream::V1::Types {

    class Image {
    public:
        Image()
            : vWidth(0), vHeight(0), vBytesPerPixel(0), vFormat(ImageFormat::FB_RGB888), vData(nullptr){
        };

        Image(int width, int height, int bytesPerPixel, ImageFormat format, std::uint8_t* data)
            : vWidth(width), vHeight(height), vBytesPerPixel(bytesPerPixel), vFormat(format) {
            size_t arraySize = this->vWidth * this->vHeight * this->vBytesPerPixel;
        #ifdef ENERGYLEAF_ESP
            this->vData = static_cast<std::uint8_t*>(ps_malloc(arraySize));
            if (!vData) {
                throw std::bad_alloc();
            }
        #else
            this->vData = new std::uint8_t[arraySize];   
        #endif
            std::copy(data, data + arraySize, vData);
        }

         Image(int width, int height, int bytesPerPixel, ImageFormat format)
            : vWidth(width), vHeight(height), vBytesPerPixel(bytesPerPixel), vFormat(format) {
            size_t arraySize = this->vWidth * this->vHeight * this->vBytesPerPixel;
        #ifdef ENERGYLEAF_ESP
            this->vData = static_cast<std::uint8_t*>(ps_malloc(arraySize));
            if (!vData) {
                throw std::bad_alloc();
            }
        #else
            this->vData = new std::uint8_t[arraySize];   
        #endif
        }

        Image(Image &&other)
        noexcept: vWidth(other.vWidth), vHeight(other.vHeight), vBytesPerPixel(other.vBytesPerPixel), vFormat(other.vFormat), vData(other.vData){
            other.vWidth = 0;
            other.vHeight = 0;
            other.vBytesPerPixel = 0;
            other.vFormat = ImageFormat::FB_RGB888;
            other.vData = nullptr;
        }

        Image(const Image& other) {
            this->vWidth = other.vWidth;
            this->vHeight = other.vHeight;
            this->vBytesPerPixel = other.vBytesPerPixel;
            this->vFormat = other.vFormat;
            size_t arraySize = this->vWidth * this->vHeight * this->vBytesPerPixel;
        #ifdef ENERGYLEAF_ESP
            this->vData = static_cast<std::uint8_t*>(ps_malloc(arraySize));
            if (!vData) {
                throw std::bad_alloc();
            }
        #else
            this->vData = new std::uint8_t[arraySize];   
        #endif
            std::copy(other.vData, other.vData + arraySize, vData);
        }

        Image& operator=(Image&& other) noexcept {
            if (this != &other) {
            #ifdef ENERGYLEAF_ESP
                free(vData);
            #else
                delete[] vData;
            #endif
                this->vWidth = other.vWidth;
                this->vHeight = other.vHeight;
                this->vBytesPerPixel = other.vBytesPerPixel;
                this->vFormat = other.vFormat;
                this->vData = other.vData;
                other.vWidth = 0;
                other.vHeight = 0;
                other.vBytesPerPixel = 0;
                other.vFormat = ImageFormat::FB_RGB888;
                other.vData = nullptr;
            }
            return *this;
        }

        Image& operator=(const Image& other) {
            this->vWidth = other.vWidth;
            this->vHeight = other.vHeight;
            this->vBytesPerPixel = other.vBytesPerPixel;
            this->vFormat = other.vFormat;
            size_t arraySize = this->vWidth * this->vHeight * this->vBytesPerPixel;
        #ifdef ENERGYLEAF_ESP
            this->vData = static_cast<std::uint8_t*>(ps_malloc(arraySize));
            if (!vData) {
                throw std::bad_alloc();
            }
        #else
            this->vData = new std::uint8_t[arraySize];   
        #endif
            std::copy(other.vData, other.vData + arraySize, vData);
            return *this;
        }

        virtual ~Image() {
            if(this->vData) {
            #ifdef ENERGYLEAF_ESP
                free(vData);
            #else
                delete[] vData;
            #endif
                this->vData = nullptr;
            }
        }

        [[nodiscard]] int getWidth() const {
            return this->vWidth;
        }

        [[nodiscard]] int getHeight() const {
            return this->vHeight;
        }

        [[nodiscard]] int getBytesPerPixel() const {
            return this->vBytesPerPixel;
        }

        [[nodiscard]] ImageFormat getFormat() const {
            return this->vFormat;
        }

        std::uint8_t* getData() {
            return this->vData;
        }

        void setWidth(int width) {
            this->vWidth = width;
        }

        void setHeight(int height) {
            this->vHeight = height;
        }

        void setBytesPerPixel(int bytesPerPixel) {
            this->vBytesPerPixel = bytesPerPixel;
        }

        void setFormat(ImageFormat format) {
            this->vFormat = format;
        }

        void initData() {
            if(this->vData != nullptr) {
                delete[] this->vData;
            } 
            this->vData = new std::uint8_t[this->vWidth * this->vHeight * this->vBytesPerPixel];
        }

    private:
        int vWidth;
        int vHeight;
        int vBytesPerPixel;
        ImageFormat vFormat;
        std::uint8_t* vData;
    };
}
#endif //ENERGYLEAF_STREAM_V1_TYPES_IMAGE_HPP
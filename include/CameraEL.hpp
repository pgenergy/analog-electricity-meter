#ifndef ENERGYLEAF_V1_SOURCE_CAMERAEL_HPP
#define ENERGYLEAF_V1_SOURCE_CAMERAEL_HPP

#include <Extras/Vision/Camera/AbstractCamera.hpp>
#include <esp32-hal-psram.h>
#include <esp_camera.h>
#include "Arduino.h"
#include <LED/CameraLED.hpp>

class CameraEL : public Energyleaf::Stream::V1::Extras::Vision::AbstractCamera<camera_config_t> {
    public:
        using CameraConfig = camera_config_t;

        CameraEL() : AbstractCamera(), led(4, 2, 15) {
        }

        virtual ~CameraEL() {
            internalStop();
        };

         virtual const camera_config_t& getConfig() const {
            return vConfig;
         }

        virtual void setConfig(camera_config_t&& config) {
            vConfig = config;
        }

        virtual void setConfig(camera_config_t& config) {
            vConfig = config;
        }
    private:
        camera_config_t vConfig;
        LED::CameraLED led;
    protected:
        void internalStart() override{
            log_d("Grab_Mode: %d", (&this->vConfig)->grab_mode);
            esp_err_t err = esp_camera_init(&this->vConfig);
            if (err != ESP_OK) {
                throw std::runtime_error("Could not initialize the Camera!");
            }

            sensor_t* vSensor = esp_camera_sensor_get();
            if (vSensor == nullptr) {
                throw std::runtime_error("Problem reading camera sensor settings!");
            }

            vSensor->set_framesize(vSensor, this->vConfig.frame_size);
            vSensor->set_gain_ctrl(vSensor, 1);  // auto gain on
            vSensor->set_exposure_ctrl(vSensor, 1);  // auto exposure on
            vSensor->set_awb_gain(vSensor, 1);  // Auto White Balance enable (0 or 1)
            vSensor->set_brightness(vSensor, 0);  // (-2 to 2) - set brightness

            this->led.enable();
        }
        void internalStop() override{
            this->led.disable();
        }
        Energyleaf::Stream::V1::Types::Image getInternalImage() const override {
            camera_fb_t* framebuffer = esp_camera_fb_get();
             if (!framebuffer) {
                throw std::runtime_error("Camera capture failed!");
            } else {
                bool validJpeg = false;
                if (framebuffer->len >= 2 && framebuffer->buf[0] == 0xFF && framebuffer->buf[1] == 0xD8) {
                    if (framebuffer->buf[framebuffer->len - 2] == 0xFF &&
                        framebuffer->buf[framebuffer->len - 1] == 0xD9) {
                        validJpeg = true;
                    } else {
                        log_d("JPEG EOI marker not found (End of Image)");
                        validJpeg = false;
                    }
                } else {
                    log_d("JPEG SOI marker not found (Start of Image)");
                    validJpeg = false;
                }
                if (!validJpeg) {
                    esp_camera_fb_return(framebuffer);
                    framebuffer = nullptr;
                    framebuffer = esp_camera_fb_get();
                    log_d("Camera capture failed in first try. Using second try!");
                    if (!framebuffer) {
                        throw std::runtime_error(
                            "Camera capture failed in second try!");
                    }
                }
            }
            Energyleaf::Stream::V1::Types::Image img;
            img.setWidth(framebuffer->width);
            img.setHeight(framebuffer->height);
            img.setBytesPerPixel(3);
            img.setFormat(Energyleaf::Stream::V1::Types::ImageFormat::FB_BGR888);
            img.initData();
            bool s = fmt2rgb888(framebuffer->buf, framebuffer->len, framebuffer->format, img.getData());
            if (framebuffer) {
                esp_camera_fb_return(framebuffer);
                framebuffer = nullptr;
            }
            if (!s) {
                throw std::runtime_error("Could not convert frame to rgb888!");
            }
            return img;
        }
};

#endif // ENERGYLEAF_V1_SOURCE_CAMERAEL_HPP
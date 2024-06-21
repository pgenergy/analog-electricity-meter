#ifndef ENERGYLEAF_SENSOR_LED_CAMERALED_HPP
#define ENERGYLEAF_SENSOR_LED_CAMERALED_HPP

#include "AbstractLED.hpp"

namespace LED {
    class CameraLED : public AbstractLED {
        public:
        CameraLED(std::uint8_t pin, std::uint8_t channel, std::uint32_t duty)
                : AbstractLED(pin), vChannel(channel), vDuty(duty) {
            ledcSetup(this->vChannel, 5000, 8);
            ledcAttachPin(this->vPin, this->vChannel);
        }

        ~CameraLED() override {
            this->disable();
        }

        void setDuty(std::uint32_t&& duty) {
            this->vDuty = duty;
            this->update();
        }

        [[nodiscard]] std::uint32_t getDuty() const {
            return this->vDuty;
        }

        private:
        const std::uint8_t vChannel;
        std::uint32_t vDuty;
        static constexpr int CONFIG_LED_MAX_INTENSITY = 255;

    protected:
        void update() override {
            int duty = this->vEnable ? this->vDuty : 0;
            if (this->vEnable && (this->vDuty > CONFIG_LED_MAX_INTENSITY)) {
                duty = CONFIG_LED_MAX_INTENSITY;
            }
            ledcWrite(this->vChannel, duty);
        }
    };
}

#endif //ENERGYLEAF_SENSOR_LED_CAMERALED_HPP
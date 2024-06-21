#ifndef ENERGYLEAF_SENSOR_LED_ABSTRACTLED_HPP
#define ENERGYLEAF_SENSOR_LED_ABSTRACTLED_HPP

#include <cstdint>
#include <esp32-hal-gpio.h>

namespace LED {
    class AbstractLED {
        public:
        explicit AbstractLED(std::uint8_t vPin) : vPin(vPin), vEnable(false) {
        }

        virtual ~AbstractLED() = default;

        void enable() {
            this->vEnable = true;
            this->update();
        }

        void disable() {
            this->vEnable = false;
            this->update();
        }

        [[nodiscard]] bool isEnabled() const { return this->vEnable; }

        [[nodiscard]] std::uint8_t getPin() const {
            return this->vPin;
        }

        protected:
        const std::uint8_t vPin;
        bool vEnable;

        virtual void update() = 0;
    };
}

#endif //ENERGYLEAF_SENSOR_LED_ABSTRACTLED_HPP

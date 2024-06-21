#ifndef ENERGYLEAF_SENSOR_LED_STATUSLED_HPP
#define ENERGYLEAF_SENSOR_LED_STATUSLED_HPP

#include "AbstractLED.hpp"

namespace LED {
    class StatusLED : public AbstractLED {
        public:
        explicit StatusLED(std::uint8_t pin) : AbstractLED(pin) {
            pinMode(this->vPin, OUTPUT);
        }

        ~StatusLED() override {
            this->disable();
        }

        protected:
        void update() override{
            digitalWrite(this->vPin, this->vEnable ? LOW : HIGH);
        }
    };
}

#endif //ENERGYLEAF_SENSOR_LED_STATUSLED_HPP
//
// Created by SlepiK on 30.01.24.
//

#ifndef ENERGYLEAF_STREAM_V1_EXTRAS_DEVICE_ESP_LED_STATUSLED_HPP
#define ENERGYLEAF_STREAM_V1_EXTRAS_DEVICE_ESP_LED_STATUSLED_HPP

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

    private:
    protected:
        void update() override{
            digitalWrite(this->vPin, this->vEnable ? LOW : HIGH);
        }
    };
}

#endif //ENERGYLEAF_STREAM_V1_EXTRAS_DEVICE_ESP_LED_STATUSLED_HPP
//
// Created by SlepiK on 02.02.2024.
//

#ifndef SENSOR_SENDER_POWER_HPP
#define SENSOR_SENDER_POWER_HPP

#include <Core/Operator/SinkOperator/SenderSinkOperator/AbstractSender.hpp>
#include <Extras/Network/WebSender/AbstractWebSender.hpp>

#include <WebSender/WebSender.hpp>

#include <Types/Datatype/DtFloat.hpp>

namespace Sensor::Sender {
    class Power :   public Energyleaf::Stream::V1::Core::Operator::SinkOperator::AbstractSender,
                    public Energyleaf::Stream::V1::Extras::Network::AbstractWebSender<Sensor::WebSender::WebSender> {
    public:
        explicit Power() : AbstractSender(), AbstractWebSender() {
        }

        explicit Power(Sensor::WebSender::WebSender *client) : AbstractSender(), AbstractWebSender(client) {
        }

        explicit Power(Sensor::Sender::Power& other) :  AbstractSender(), AbstractWebSender(other.getSender()) {
        }

        explicit Power(Sensor::Sender::Power&& other) noexcept :    AbstractSender(), AbstractWebSender(std::move(other.getSender())) {
        }

        ~Power() = default;

        bool work(Energyleaf::Stream::V1::Tuple::Tuple &inputTuple) override {
            auto items = inputTuple.getItems();
            if(items.find("Power") != items.end()) {
                auto power = inputTuple.getItem<Energyleaf::Stream::V1::Types::Datatype::DtFloat>("Power");

                log_i("Debug Power-Value: %f",power.toFloat());
                return this->getSender()->send(power.toFloat());
            } else {
                return false;
            }
        }
    private:
    protected:
    };
}

#endif
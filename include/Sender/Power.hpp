#ifndef ENERGYLEAF_SENSOR_POWER_HPP
#define ENERGYLEAF_SENSOR_POWER_HPP

#include "Operator/SinkOperator/SenderSinkOperator/AbstractSender.hpp"
#include "Extras/Network/WebSender/AbstractWebSender.hpp"
#include "WebSender/WebSender.hpp"
#include "Core/Type/Datatype/DtFloat.hpp"

namespace Sender {
    class Power :   public Apalinea::Operator::SinkOperator::AbstractSender,
                    public Apalinea::Extras::Network::AbstractWebSender<WebSender::WebSender> {
    public:
        explicit Power() : AbstractSender(), AbstractWebSender() {
        }

        explicit Power(WebSender::WebSender *client) : AbstractSender(), AbstractWebSender(client) {
        }

        explicit Power(Sender::Power& other) :  AbstractSender(), AbstractWebSender(other.getSender()) {
        }

        explicit Power(Sender::Power&& other) noexcept :    AbstractSender(), AbstractWebSender(std::move(other.getSender())) {
        }

        ~Power() = default;

        bool work(Apalinea::Core::Tuple::Tuple &inputTuple) override {
            auto items = inputTuple.getItems();
            if(items.find("Power") != items.end()) {
                auto power = inputTuple.getItem<Apalinea::Core::Type::Datatype::DtFloat>("Power");

                log_i("Debug Power-Value: %f",power.toFloat());
                return this->getSender()->send(power.toFloat());
            } else {
                return false;
            }
        }
    };
}

#endif //ENERGYLEAF_SENSOR_POWER_HPP
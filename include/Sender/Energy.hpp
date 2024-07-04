#ifndef ENERGYLEAF_SENSOR_POWER_HPP
#define ENERGYLEAF_SENSOR_POWER_HPP

#include "Operator/SinkOperator/SenderSinkOperator/AbstractSender.hpp"
#include "Extras/Network/WebSender/AbstractWebSender.hpp"
#include "WebSender/WebSender.hpp"
#include "Core/Type/Datatype/DtFloat.hpp"

namespace Sender {
    class Energy :   public Apalinea::Operator::SinkOperator::AbstractSender,
                    public Apalinea::Extras::Network::AbstractWebSender<WebSender::WebSender> {
    public:
        explicit Energy() : AbstractSender(), AbstractWebSender() {
        }

        explicit Energy(WebSender::WebSender *client) : AbstractSender(), AbstractWebSender(client) {
        }

        explicit Energy(Sender::Energy& other) :  AbstractSender(), AbstractWebSender(other.getSender()) {
        }

        explicit Energy(Sender::Energy&& other) noexcept :    AbstractSender(), AbstractWebSender(std::move(other.getSender())) {
        }

        ~Energy() = default;

        bool work(Apalinea::Core::Tuple::Tuple &inputTuple) override {
            auto items = inputTuple.getItems();
            if(items.find("energy") != items.end()) {
                auto power = inputTuple.getItem<Apalinea::Core::Type::Datatype::DtFloat>("energy");

                log_i("Debug energy-Value: %f",power.toFloat());
                return this->getSender()->send(power.toFloat());
            } else {
                return false;
            }
        }
    };
}

#endif //ENERGYLEAF_SENSOR_POWER_HPP
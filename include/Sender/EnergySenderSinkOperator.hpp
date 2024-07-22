#ifndef ENERGYLEAF_SENSOR_SENDER_ENERGYSENDERSINKOPERATOR_HPP
#define ENERGYLEAF_SENSOR_SENDER_ENERGYSENDERSINKOPERATOR_HPP

#include "Operator/SinkOperator/SenderSinkOperator/SenderSinkOperator.hpp"
#include "Energy.hpp"

namespace Sender {
    class EnergySenderSinkOperator : public Apalinea::Operator::SinkOperator::SenderSinkOperator<Energy> {
    public:
        [[nodiscard]] Apalinea::Core::Operator::OperatorMode getOperatorMode() const override {
            return Apalinea::Core::Operator::OperatorMode::MAIN;
        }

        void handleHeartbeat(std::optional<std::chrono::steady_clock::time_point> hbTP, Apalinea::Core::Tuple::Tuple &tuple) override {
            if(tuple.getItems().size() > 0) {
                this->process(tuple);
                return;
            } else {
                vProcessState = Apalinea::Core::Operator::OperatorProcessState::BREAK;
                return;
            }
        }
    };
}

#endif //ENERGYLEAF_SENSOR_SENDER_ENERGYSENDERSINKOPERATOR_HPP
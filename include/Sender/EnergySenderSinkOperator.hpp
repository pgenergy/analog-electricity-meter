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
    };
}

#endif //ENERGYLEAF_SENSOR_SENDER_ENERGYSENDERSINKOPERATOR_HPP
//
// Created by SlepiK on 28.01.2024.
//

#ifndef STREAM_V1_CORE_OPERATOR_SINKOPERATOR_SERIALSINKOPERATOR_HPP
#define STREAM_V1_CORE_OPERATOR_SINKOPERATOR_SERIALSINKOPERATOR_HPP

#include "Operator/SinkOperator/AbstractSinkOperator.hpp"
#include "Tuple/Tuple.hpp"
#include "Types/Power/Power.hpp"

#ifdef ENERGYLEAF_ESP
#include <HardwareSerial.h>
#endif

namespace Energyleaf::Stream::V1::Core::Operator::SinkOperator {

    class SerialSinkOperator
            : public Energyleaf::Stream::V1::Operator::AbstractSinkOperator<Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Power>> {
    public:
    private:
    protected:
        void work(Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Power> &inputTuple) override {
#ifdef ENERGYLEAF_ESP
            Serial.print("New Tuple: [");
            Serial.print(inputTuple.getItem<Energyleaf::Stream::V1::Types::Power>(0).getName().data());
            Serial.print("|");
            Serial.print(inputTuple.getItem<Energyleaf::Stream::V1::Types::Power>(0).getData().getPower());
            Serial.println("]");
#endif
        }
    };

} // Energyleaf::Stream::V1::Core::Operator::SinkOperator

#endif //STREAM_V1_CORE_OPERATOR_SINKOPERATOR_SERIALSINKOPERATOR_HPP

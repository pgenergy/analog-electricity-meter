//
// Created by SlepiK on 18.06.2024.
//

#ifndef SENSOR_ENRICHER_TOKENENRICHER_HPP
#define SENSOR_ENRICHER_TOKENENRICHER_HPP

#include "Token.hpp"
#include "Core/Operator/PipeOperator/EnrichPipeOperator/EnrichPipeOperator.hpp"

namespace Sensor::Enricher {
    class TokenEnrichPipeOperator : public Energyleaf::Stream::V1::Core::Operator::PipeOperator::EnrichPipeOperator<Token> {
    public:
        [[nodiscard]] Energyleaf::Stream::V1::Operator::OperatorMode getOperatorMode() const override {
            return Energyleaf::Stream::V1::Operator::OperatorMode::MAIN;
        }
    };
}

#endif
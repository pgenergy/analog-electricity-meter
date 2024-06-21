//
// Created by SlepiK on 18.06.2024.
//

#ifndef SENSOR_ENRICHER_TOKENENRICHER_HPP
#define SENSOR_ENRICHER_TOKENENRICHER_HPP

#include "Token.hpp"
#include "Operator/PipeOperator/EnrichPipeOperator/EnrichPipeOperator.hpp"

namespace Sensor::Enricher {
    class TokenEnrichPipeOperator : public Apalinea::Operator::PipeOperator::EnrichPipeOperator<Token> {
    public:
        [[nodiscard]] Apalinea::Core::Operator::OperatorMode getOperatorMode() const override {
            return Apalinea::Core::Operator::OperatorMode::MAIN;
        }
    };
}

#endif
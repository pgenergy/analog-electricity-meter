#ifndef ENERGYLEAF_SENSOR_ENRICHER_TOKENENRICHER_HPP
#define ENERGYLEAF_SENSOR_ENRICHER_TOKENENRICHER_HPP

#include "Token.hpp"
#include "Operator/PipeOperator/EnrichPipeOperator/EnrichPipeOperator.hpp"

namespace Enricher {
    class TokenEnrichPipeOperator : public Apalinea::Operator::PipeOperator::EnrichPipeOperator<Token> {
    public:
        [[nodiscard]] Apalinea::Core::Operator::OperatorMode getOperatorMode() const override {
            return Apalinea::Core::Operator::OperatorMode::MAIN;
        }
    };
}

#endif //ENERGYLEAF_SENSOR_ENRICHER_TOKENENRICHER_HPP
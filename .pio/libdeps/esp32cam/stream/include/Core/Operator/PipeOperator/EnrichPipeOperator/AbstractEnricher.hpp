//
// Created by SlepiK on 30.01.24.
//

#ifndef STREAM_V1_CORE_OPERATOR_PIPEOPERATOR_ABSTRACTENRICHER_HPP
#define STREAM_V1_CORE_OPERATOR_PIPEOPERATOR_ABSTRACTENRICHER_HPP

#include <Operator/PipeOperator/AbstractPipeOperator.hpp>

namespace Energyleaf::Stream::V1::Core::Operator::PipeOperator {
    template<typename TupleInput, typename TupleOutput>
    class AbstractEnricher {
        public:
            AbstractEnricher() = default;
            ~AbstractEnricher() = default;
        private:
        protected:
            virtual bool work(TupleInput &inputTuple, TupleOutput& outputTuple) = 0;
    };
} // STREAM_V1_CORE_OPERATOR_PIPEOPERATOR_ABSTRACTENRICHER_HPP

#endif
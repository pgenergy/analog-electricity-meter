//
// Created by SlepiK on 02.02.2024.
//

#ifndef SENSOR_ENRICHER_TOKEN_HPP
#define SENSOR_ENRICHER_TOKEN_HPP

#include <Core/Operator/PipeOperator/EnrichPipeOperator/AbstractEnricher.hpp>
#include <Extras/Network/WebSender/AbstractWebSender.hpp>

#include <WebSender/WebSender.hpp>

namespace Sensor::Enricher {
    class Token :   public Energyleaf::Stream::V1::Core::Operator::PipeOperator::AbstractEnricher, 
                    public Energyleaf::Stream::V1::Extras::Network::AbstractWebSender<Sensor::WebSender::WebSender> {
    public:
        explicit Token() : AbstractEnricher(), AbstractWebSender() {
        }

        explicit Token(Sensor::WebSender::WebSender *client) : AbstractEnricher(), AbstractWebSender(client) {
        }

        explicit Token(Sensor::Enricher::Token &other) :  AbstractEnricher(), AbstractWebSender(other.getSender()) {
        }

        explicit Token(Sensor::Enricher::Token &&other) noexcept :    AbstractEnricher(), AbstractWebSender(std::move(other.getSender())) {
        }

        ~Token() = default;

        bool work(Energyleaf::Stream::V1::Tuple::Tuple &inputTuple, Energyleaf::Stream::V1::Tuple::Tuple& outputTuple) override {
            if(this->getSender()->request()) {
                //If we got a new token or the old one is not expired from our side, we continue
                outputTuple = inputTuple;
                return true;
            } else {
                //If we cant get a new token because the old one is expired from our side, we break
                return false;
            }
        }
    private:
    protected:
    };
}

#endif
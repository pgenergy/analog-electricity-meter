//
// Created by SlepiK on 02.02.2024.
//

#ifndef SENSOR_ENRICHER_TOKEN_HPP
#define SENSOR_ENRICHER_TOKEN_HPP

#include "Operator/PipeOperator/EnrichPipeOperator/AbstractEnricher.hpp"
#include "Extras/Network/WebSender/AbstractWebSender.hpp"
#include "WebSender/WebSender.hpp"

namespace Sensor::Enricher {
    class Token :   public Apalinea::Operator::PipeOperator::AbstractEnricher, 
                    public Apalinea::Extras::Network::AbstractWebSender<Sensor::WebSender::WebSender> {
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

        bool work(Apalinea::Core::Tuple::Tuple &inputTuple, Apalinea::Core::Tuple::Tuple& outputTuple) override {
            if(this->getSender()->request()) {
                //If we got a new token or the old one is not expired from our side, we continue
                outputTuple = inputTuple;
                outputTuple.addItem(std::string("RotationKWH"),Apalinea::Core::Type::Datatype::DtInt(this->getSender()->getRotation()));
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
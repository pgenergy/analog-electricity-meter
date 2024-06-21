#ifndef ENERGYLEAF_SENSOR_ENRICHER_TOKEN_HPP
#define ENERGYLEAF_SENSOR_ENRICHER_TOKEN_HPP

#include "Operator/PipeOperator/EnrichPipeOperator/AbstractEnricher.hpp"
#include "Extras/Network/WebSender/AbstractWebSender.hpp"
#include "WebSender/WebSender.hpp"

namespace Enricher {
    class Token :   public Apalinea::Operator::PipeOperator::AbstractEnricher, 
                    public Apalinea::Extras::Network::AbstractWebSender<WebSender::WebSender> {
    public:
        explicit Token() : AbstractEnricher(), AbstractWebSender() {
        }

        explicit Token(WebSender::WebSender *client) : AbstractEnricher(), AbstractWebSender(client) {
        }

        explicit Token(Enricher::Token &other) :  AbstractEnricher(), AbstractWebSender(other.getSender()) {
        }

        explicit Token(Enricher::Token &&other) noexcept :    AbstractEnricher(), AbstractWebSender(std::move(other.getSender())) {
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

#endif //ENERGYLEAF_SENSOR_ENRICHER_TOKEN_HPP
//
// Created by SlepiK on 30.01.24.
//

#ifndef STREAM_V1_CORE_OPERATOR_PIPEOPERATOR_WEBREQUESTENRICHOPERATOR_HPP
#define STREAM_V1_CORE_OPERATOR_PIPEOPERATOR_WEBREQUESTENRICHOPERATOR_HPP

#include <utility>
#include <cstddef>
#include <cstring>
#include <locale>
#include <codecvt>

#include "Operator/PipeOperator/AbstractPipeOperator.hpp"
#include "Tuple/Tuple.hpp"
#include "Types/Power/Power.hpp"
#include "Extras/Network/ESP/WebSender/WebSender.hpp"
#include "Types/Buffer/ELData.pb.h"
#include <Arduino.h>
#include "Types/Image/Image.hpp"
#include <pb_decode.h>
#include "Auth.pb.h"

namespace Energyleaf::Stream::V1::Core::Operator::PipeOperator {
    class WebRequestEnrichOperator : public Energyleaf::Stream::V1::Operator::AbstractPipeOperator<Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Image>,Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Image,std::string>>, 
    public Energyleaf::Stream::V1::Extras::Network::ESP::WebSender<std::string> {
    public:
        void setEndpoint(std::string endpoint) { this->vEndpoint = std::move(endpoint); }
    private:
        std::string vEndpoint;
        std::string vToken;
        bool gotToken = false;
        uint32_t vTokenExpires = 0;
#ifdef ENERGYLEAF_ESP
#endif
    protected:
        void work(Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Image> &inputTuple,
                  Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Image,std::string> &outputTuple) {
            outputTuple.clear();
            if(!gotToken) {
                send(std::string(WiFi.macAddress().c_str()));
            }
            outputTuple.addItem(std::string("TOKEN"),this->vToken);
            outputTuple.addItem(inputTuple.getItem<Energyleaf::Stream::V1::Types::Image>(0));
            inputTuple.clear();
        }

        void send(const std::string& data,const std::string& token) override {
        }

        void send(const std::string& data) override {
            if (this->vHost.empty() && this->vEndpoint.empty() && !this->vPortSet) {
                throw std::runtime_error("Endpointdata not set!");
            }

#ifdef ENERGYLEAF_ESP
            log_d("Connecting to %s:%d for authorization",vHost.c_str(),vPort);
            vWifiClientSecure.connect(vHost.c_str(), vPort);
            if (vWifiClientSecure.connected()) {

                TokenRequest request = TokenRequest_init_default;
                TokenResponse response;
                uint8_t vBufferReq[TokenRequest_size];
                uint8_t vBufferRes[TokenResponse_size];
                
                std::copy(std::begin(data), std::end(data), request.client_id);

                pb_ostream_t stream_out = pb_ostream_from_buffer(vBufferReq, sizeof(vBufferReq));
                if (!pb_encode(&stream_out, TokenRequest_fields, &request)) {
                    throw std::runtime_error("Failed to encode ProtoBuf request");
                }

                vWifiClientSecure.print("POST ");
                vWifiClientSecure.print(this->vEndpoint.c_str());
                vWifiClientSecure.print(" HTTP/1.1\r\n");
                vWifiClientSecure.print("Host: ");
                vWifiClientSecure.print(vHost.c_str());
                vWifiClientSecure.print("\r\n");
                vWifiClientSecure.print("Content-Type: application/x-protobuf\r\n");
                vWifiClientSecure.print("Content-Length: ");
                vWifiClientSecure.print(stream_out.bytes_written);
                vWifiClientSecure.print("\r\n");
                vWifiClientSecure.print("\r\n");
                vWifiClientSecure.write(vBufferReq, stream_out.bytes_written);

                //Header
                String headers = "";
                while (vWifiClientSecure.connected() || vWifiClientSecure.available()) {
                    String line = vWifiClientSecure.readStringUntil('\r');
                    headers += line + "\r";
                    if (line == "\n") {
                        break;
                    }
                }

                //ToDO: Maybe check the header for an error code or so ?

                //Body
                std::size_t bytesRead = vWifiClientSecure.readBytes(vBufferRes, sizeof(vBufferRes));
                pb_istream_t stream_in = pb_istream_from_buffer(vBufferRes, bytesRead);

                if (!pb_decode(&stream_in, TokenResponse_fields, &response)) {
                    throw std::runtime_error("Failed to decode ProtoBuf response");
                }
                
                this->vToken = response.access_token;
                this->vTokenExpires = response.expires_in;
                this->gotToken = true;
                vProcessState = Energyleaf::Stream::V1::Operator::OperatorProcessState::CONTINUE;
                vWifiClientSecure.stop();
            } else {
                log_e("Connection for token failed!");
                vProcessState = Energyleaf::Stream::V1::Operator::OperatorProcessState::STOP;
            }
#endif
        }
    };
}

#endif //STREAM_V1_CORE_OPERATOR_PIPEOPERATOR_WEBREQUESTENRICHOPERATOR_HPP
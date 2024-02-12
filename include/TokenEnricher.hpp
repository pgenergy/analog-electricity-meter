//
// Created by SlepiK on 02.02.2024.
//

#ifndef SENSOR_ENRICHER_TOKENENRICHER_HPP
#define SENSOR_ENRICHER_TOKENENRICHER_HPP

#include <Core/Operator/PipeOperator/EnrichPipeOperator/AbstractEnricher.hpp>
#include <Extras/Network/WebSender/AbstractWebSender.hpp>
#include <WiFiClientSecure.h>
#include <Tuple/Tuple.hpp>
#include <Types/Image/Image.hpp>
#include <pb_decode.h>
#include <pb_encode.h>
#include <Auth.pb.h>

extern const std::uint8_t rootca_crt_bundle_start[] asm(
"_binary_data_cert_x509_crt_bundle_bin_start");

class TokenEnricher : public Energyleaf::Stream::V1::Core::Operator::PipeOperator::AbstractEnricher<
Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Image>,
Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Image,std::string>>, 
public Energyleaf::Stream::V1::Extras::Network::AbstractWebSender<WiFiClientSecure> {
    public:
        explicit TokenEnricher() : AbstractEnricher(), AbstractWebSender() {
            if (rootca_crt_bundle_start == nullptr) {
                throw std::runtime_error("Cant load CACertBundle!");
            }
            this->getSender().setCACertBundle(rootca_crt_bundle_start);
            vCertSet = true;
        }

        explicit TokenEnricher(WiFiClientSecure& client, bool loadCert = true) : AbstractEnricher(), AbstractWebSender(client), vCertSet(!loadCert) {
            if(loadCert) {
                if (rootca_crt_bundle_start == nullptr) {
                    throw std::runtime_error("Cant load CACertBundle!");
                }
                this->getSender().setCACertBundle(rootca_crt_bundle_start);
            }
        }

        explicit TokenEnricher(TokenEnricher& other) :  AbstractEnricher(), AbstractWebSender(other.getSender()),vCertSet(other.vCertSet), 
        vPort(other.vPort), vPortSet(other.vPortSet), vHost(other.vHost) {
            if(!other.isCertSet()) {
                throw std::runtime_error("CACertBundle was not loaded in the given TokenEnricher");
            }
        }

        explicit TokenEnricher(TokenEnricher&& other) noexcept
            : AbstractEnricher(), AbstractWebSender(std::move(other)),
            vHost(std::move(other.vHost)), vEndpoint(std::move(other.vEndpoint)),
            vPort(other.vPort), vPortSet(other.vPortSet), vCertSet(other.vCertSet),
            vTokenExpires(other.vTokenExpires), vToken(std::move(other.vToken)),
            vTokenAvailable(other.vTokenAvailable) {
        }

        ~TokenEnricher() = default;
        
        void setHost(std::string&& host) { this->vHost = std::move(host); }

        void setHost(const std::string& host) { this->vHost = host; }

        void setEndpoint(std::string&& endpoint) { this->vEndpoint = std::move(endpoint); }

        void setEndpoint(const std::string& endpoint) { this->vEndpoint = endpoint; }

        std::string_view getHost() { return this->vHost; }

        void setPort(int port) { 
            this->vPort = port; 
            this->vPortSet = true;
            }

        const int& getPort() { return this->vPort; }

        const bool& isCertSet() { return this->vCertSet; }

        const bool& isPortSet() { return this->vCertSet; }

        bool work(Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Image> &inputTuple, Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Image,std::string>& outputTuple) override {
            if (this->vHost.empty() && this->vEndpoint.empty() && !this->vPortSet) {
                throw std::runtime_error("Endpointdata not set!");
            }

            log_d("Connecting to %s:%d for authorization",vHost.c_str(),vPort);
            this->getSender().connect(vHost.c_str(), vPort);
            if (this->getSender().connected()) {

                TokenRequest request = TokenRequest_init_default;
                TokenResponse response;
                uint8_t vBufferReq[TokenRequest_size];
                uint8_t vBufferRes[TokenResponse_size];
                String mac = WiFi.macAddress();
                std::copy(std::begin(mac), std::end(mac), request.client_id);

                pb_ostream_t stream_out = pb_ostream_from_buffer(vBufferReq, sizeof(vBufferReq));
                if (!pb_encode(&stream_out, TokenRequest_fields, &request)) {
                    throw std::runtime_error("Failed to encode ProtoBuf request");
                }

                this->getSender().print("POST ");
                this->getSender().print(this->vEndpoint.c_str());
                this->getSender().print(" HTTP/1.1\r\n");
                this->getSender().print("Host: ");
                this->getSender().print(vHost.c_str());
                this->getSender().print("\r\n");
                this->getSender().print("Content-Type: application/x-protobuf\r\n");
                this->getSender().print("Content-Length: ");
                this->getSender().print(stream_out.bytes_written);
                this->getSender().print("\r\n");
                this->getSender().print("\r\n");
                this->getSender().write(vBufferReq, stream_out.bytes_written);

                //Header
                String headers = "";
                while (this->getSender().connected() || this->getSender().available()) {
                    String line = this->getSender().readStringUntil('\r');
                    headers += line + "\r";
                    if (line == "\n") {
                        break;
                    }
                }

                //ToDO: Maybe check the header for an error code or so ?

                //Body
                std::size_t bytesRead = this->getSender().readBytes(vBufferRes, sizeof(vBufferRes));
                pb_istream_t stream_in = pb_istream_from_buffer(vBufferRes, bytesRead);

                if (!pb_decode(&stream_in, TokenResponse_fields, &response)) {
                    throw std::runtime_error("Failed to decode ProtoBuf response");
                }
                
                this->vToken = response.access_token;
                this->vTokenExpires = response.expires_in;
                this->vTokenAvailable = true;
                this->getSender().stop();
                return true;
            } else {
                log_e("Connection for token failed!");
                return false;
            }
        }
    private:
        std::string vHost;
        std::string vEndpoint;
        int vPort;
        bool vPortSet;
        bool vCertSet;
        uint32_t vTokenExpires;
        std::string vToken;
        bool vTokenAvailable = false;
};

#endif // SENSOR_ENRICHER_TOKENENRICHER_HPP
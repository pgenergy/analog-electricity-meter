//
// Created by SlepiK on 02.02.2024.
//

#ifndef SENSOR_SENDER_POWERSENDER_HPP
#define SENSOR_SENDER_POWERSENDER_HPP

#include <Core/Operator/SinkOperator/SenderSinkOperator/AbstractSender.hpp>
#include <Tuple/Tuple.hpp>
#include <Types/Power/Power.hpp>
#include <string>
#include <pb_encode.h>
#include <ELData.pb.h>
#include <Extras/Network/WebSender/AbstractWebSender.hpp>
#include <WiFiClientSecure.h>

extern const std::uint8_t rootca_crt_bundle_start[] asm(
"_binary_data_cert_x509_crt_bundle_bin_start");

class PowerSender : 
    public Energyleaf::Stream::V1::Core::Operator::SinkOperator::AbstractSender<Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Power,std::string>>, 
    public Energyleaf::Stream::V1::Extras::Network::AbstractWebSender<WiFiClientSecure> {
    public:
        explicit PowerSender() : AbstractSender(), AbstractWebSender() {
            if (rootca_crt_bundle_start == nullptr) {
                throw std::runtime_error("Cant load CACertBundle!");
            }
            this->getSender().setCACertBundle(rootca_crt_bundle_start);
            vCertSet = true;
        }

        explicit PowerSender(WiFiClientSecure& client, bool loadCert = true) : AbstractSender(), AbstractWebSender(client), vCertSet(!loadCert) {
            if(loadCert) {
                if (rootca_crt_bundle_start == nullptr) {
                    throw std::runtime_error("Cant load CACertBundle!");
                }
                this->getSender().setCACertBundle(rootca_crt_bundle_start);
            }
        }

        explicit PowerSender(PowerSender& other) :  AbstractSender(), AbstractWebSender(other.getSender()),vCertSet(other.vCertSet), 
        vPort(other.vPort), vPortSet(other.vPortSet), vHost(other.vHost) {
            if(!other.isCertSet()) {
                throw std::runtime_error("CACertBundle was not loaded in the given TokenEnricher");
            }
        }

        explicit PowerSender(PowerSender&& other) noexcept
            : AbstractSender(), AbstractWebSender(std::move(other)),
            vHost(std::move(other.vHost)), vEndpoint(std::move(other.vEndpoint)),
            vPort(other.vPort), vPortSet(other.vPortSet), vCertSet(other.vCertSet) {
        }

        ~PowerSender() = default;

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

        bool work(Energyleaf::Stream::V1::Tuple::Tuple<Energyleaf::Stream::V1::Types::Power,std::string> &inputTuple) override {
            if (this->vHost.empty() && this->vEndpoint.empty() && !this->vPortSet) {
                throw std::runtime_error("Endpointdata not set!");
            }

            std::string token = inputTuple.getItem<std::string>(0).getData();

            log_d("Connecting to %s:%d for authorization",vHost.c_str(),vPort);
            this->getSender().connect(vHost.c_str(), vPort);
            if (this->getSender().connected()) {
                ELData msg = ELData_init_default;
                uint8_t vBuffer[ELData_size];
                std::copy(std::begin(token), std::end(token), msg.sensorId);
                msg.sensorValue = inputTuple.getItem<Energyleaf::Stream::V1::Types::Power>(1).getData().getPower();

                pb_ostream_t stream = pb_ostream_from_buffer(vBuffer, sizeof(vBuffer));
                if (!pb_encode(&stream, ELData_fields, &msg)) {
                    throw std::runtime_error("Failed to encode ProtoBuf ELData for Power");
                }

                this->getSender().print("POST ");
                this->getSender().print(this->vEndpoint.c_str());
                this->getSender().println(" HTTP/1.1");
                this->getSender().print("Host: ");
                this->getSender().println(vHost.c_str());
                this->getSender().println("Content-Type: application/x-protobuf");
                this->getSender().println("Content-Length: " + String(stream.bytes_written));
                this->getSender().print("\r\n");
                this->getSender().print("\r\n");
                this->getSender().write(vBuffer, stream.bytes_written);

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
    protected:
};

#endif
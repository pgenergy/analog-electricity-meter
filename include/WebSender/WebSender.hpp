//
// Created by SlepiK on 02.02.2024.
//

#ifndef SENSOR_WEBSENDER_WEBSENDER_HPP
#define SENSOR_WEBSENDER_WEBSENDER_HPP

#ifndef USE_ENERGYLEAF
#define USE_ENERGYLEAF
#endif

#ifndef USE_ENERGYLEAFCERT
#define USE_ENERGYLEAFCERT
#endif

#ifndef ENERGYLEAF_MANUALCOUNTER
#define ENERGYLEAF_MANUALCOUNTER 5
#endif

#include <WiFiClientSecure.h>

#include <Energyleaf/Energyleaf.pb.h>
#include <Energyleaf/Energyleaf.error.h>
#include <Energyleaf/Energyleaf.cert.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <cstring>

namespace Sensor::WebSender {
    class WebSender {
        public:

            explicit WebSender() : client(new WiFiClientSecure()) {            
                log_d("Creation of an WebSender-Instance!");    
                #ifdef USE_ENERGYLEAFCERT
                if(this->certificate == nullptr) {
                    delete[] this->certificate;
                }
                this->certificate = new char[strlen(LE_ENERGYLEAF_CERTIFICATE) + 1];
                strcpy(this->certificate, LE_ENERGYLEAF_CERTIFICATE);
                this->client->setCACert(this->certificate);
                #endif
                expiresIn = 0;
            }

            explicit WebSender(WiFiClientSecure *pWiFiClientSecure,const char *pCertificate) : client(pWiFiClientSecure) {
                if(this->certificate == nullptr) {
                    delete[] this->certificate;
                }
                this->certificate = new char[strlen(pCertificate) + 1];
                strcpy(this->certificate, pCertificate);
                this->client->setCACert(this->certificate);
                log_d("Creation of an WebSender-Instance with given WCS!");    
                expiresIn = 0;
            }

            explicit WebSender(const Sensor::WebSender::WebSender &other) : WebSender(other.client, other.certificate) {
                this->host = other.host;
                this->port = other.port;
                this->accessToken = other.accessToken;       
                log_d("Creation of an WebSender-Instance with given other WebSender-Instance!");    
            }

            explicit WebSender(Sensor::WebSender::WebSender &&other) : WebSender(std::move(other.client), std::move(other.certificate)) {
                this->host = std::move(other.host);
                this->port = std::move(other.port);
                this->accessToken = std::move(other.accessToken);       
                log_d("Creation of an WebSender-Instance with given WebSender-Instance!");    
            }

            void setCertificate(const char* pCertificate) {
                if(this->certificate == nullptr) {
                    delete[] this->certificate;
                }
                this->certificate = new char[strlen(pCertificate) + 1];
                strcpy(this->certificate, pCertificate);
                this->client->setCACert(this->certificate);
            }

            const char *getCertificate() const {
                return this->certificate;
            }

            const WiFiClientSecure *getClient() const {
                return this->client;
            }

            void setHost(const char *pHost) {
                if(this->host == nullptr) {
                    delete[] this->host;
                }
                this->host = new char[strlen(pHost) + 1];
                strcpy(this->host, pHost);
            }

            const char *getHost() const {
                return this->host;
            }

            void setPort(const uint16_t pPort) {
                this->port = pPort;
            }

            uint16_t getPort() const {
                return this->port;
            }

            bool send(float pValue) {
                this->value = pValue;
                if(this->retryCounter == 5){
                    return false;
                }
                ENERGYLEAF_ERROR state = this->sendIntern();
                if(state != ENERGYLEAF_ERROR::NO_ERROR) {
                    if(this->expiresIn <= 0 || state == ENERGYLEAF_ERROR::TOKEN_EXPIRED) {
                        bool req = this->requestIntern();
                        if(req) {
                            state = this->sendIntern();
                            return true;
                        } else {
                            return false;
                        }
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            }

            bool request() {
                if(this->expiresIn <= 0) {
                    if(this->retryCounter == 5){
                        return false;
                    }
                    log_i("Current used MAC for request: %s\n",WiFi.macAddress().c_str());
                    if(!(this->requestIntern() == ENERGYLEAF_ERROR::NO_ERROR ? true : false)) {
                        ++this->retryCounter;
                        return false;
                    } else {
                        this->retryCounter = 0;
                        return true;
                    }
                } else {
                    --this->expiresIn;
                    return true;//ToDo: Maybe own enum for ret.-type(?)
                }
            }

            const char *getAccessToken() const {
                return this->accessToken;
            }
        private:
            char *host;
            uint16_t port; 
            char *accessToken; 
            uint32_t expiresIn = 0;
            const energyleaf_SensorType type = energyleaf_SensorType::energyleaf_SensorType_ANALOG_ELECTRICITY;    
            uint8_t retryCounter = 0;
            uint8_t manualMaxCounter = ENERGYLEAF_MANUALCOUNTER;
            uint8_t manualCurrentCounter = ENERGYLEAF_MANUALCOUNTER;
            WiFiClientSecure *client;
            float value = 0.f;
            bool active;
            char* certificate;


            ENERGYLEAF_ERROR sendIntern() {
                if(this->client != nullptr) {
                    if(WiFi.isConnected()) {
                        bool state = false;
                        bool chunked = false;
                        uint16_t bodySize = 0;
                        {
                            //Prepare SensorDataRequest
                            uint8_t bufferSensorDataRequest[energyleaf_SensorDataRequest_size];
                            pb_ostream_t streamSensorDataRequestOut;
                            {
                                energyleaf_SensorDataRequest sensorDataRequest = energyleaf_SensorDataRequest_init_default;
                                memcpy(sensorDataRequest.access_token, this->accessToken, sizeof(this->accessToken));
                                sensorDataRequest.type = this->type;

                                sensorDataRequest.value = this->value;

                                streamSensorDataRequestOut = pb_ostream_from_buffer(bufferSensorDataRequest, sizeof(bufferSensorDataRequest));

                                state = pb_encode(&streamSensorDataRequestOut,energyleaf_SensorDataRequest_fields, &sensorDataRequest);
                            }

                            if(!state) {
                                return ENERGYLEAF_ERROR::ERROR;
                            }

                            //Send SensorDataRequest and process received header
                            {
                                state = this->client->connect(this->host,this->port);
                                if(!state) {
                                    if(this->client->connected()){
                                        this->client->stop(); 
                                    }
                                    return ENERGYLEAF_ERROR::ERROR;
                                }

                                this->client->print(PSTR("POST /api/v1/sensor_input HTTP/1.1\r\n"));
                                this->client->print(PSTR("Host: "));
                                this->client->print(this->host);
                                this->client->print(PSTR("\r\n"));
                                this->client->print(PSTR("Content-Type: application/x-protobuf\r\n"));
                                this->client->print(PSTR("Content-Length: "));
                                this->client->print(streamSensorDataRequestOut.bytes_written);
                                this->client->print(PSTR("\r\n\r\n"));

                                this->client->write(bufferSensorDataRequest, streamSensorDataRequestOut.bytes_written);

                                char header[128];
                                char headerStatus[4];
                                state = false;

                                while(true) {
                                    int l = this->client->readBytesUntil('\n',header,sizeof(header));
                                    if(l<=1) {
                                        break;
                                    }

                                    header[l-1] = 0;

                                    if(strstr_P(header,PSTR("HTTP/1.1"))) {
                                        strncpy(headerStatus,&header[9],3);
                                        headerStatus[3] = '\0';
                                        uint16_t headerStatusCode = atoi(headerStatus);
                                        if(headerStatusCode >= 200 && headerStatusCode <= 299) {
                                            log_d("ENERGYLEAF_DRIVER_DATA_REQUEST: GOT A %d STATUS",headerStatusCode);
                                        } else {
                                            log_d("ENERGYLEAF_DRIVER_DATA_REQUEST: UNSUCCESSFUL - GOT A %d STATUS",headerStatusCode);
                                            //return ENERGYLEAF_ERROR::ERROR; //If a special status is known that results in no body here we can return direct
                                        }
                                        continue;
                                    }

                                    if(strstr_P(header,PSTR("Content-Length:"))){
                                        char contentLength[4];
                                        strncpy(contentLength,&header[16],3);
                                        contentLength[3] = '\0';
                                        bodySize = atoi(contentLength);
                                        chunked = false;
                                        continue;
                                    }

                                    if(strstr_P(header,PSTR("Transfer-Encoding: chunked"))) {
                                        chunked = true;
                                        continue;
                                    }

                                    if(strstr_P(header,PSTR("Content-Type: application/x-protobuf"))) {
                                        state = true;
                                        continue;
                                    }
                                }
                            }

                            if(!state) {
                                this->client->stop(); 
                                return ENERGYLEAF_ERROR::ERROR;
                            }
                        }

                        {
                            //Process received body and generate SensorDataResponse from it
                            energyleaf_SensorDataResponse sensorDataResponse = energyleaf_SensorDataResponse_init_default;
                            {
                                uint8_t bufferSensorDataResponse[energyleaf_SensorDataResponse_size];
                                int currentSize = 0;
                                {
                                    if(chunked) {
                                        while(true) {
                                            char chunkSize[16];
                                            int l = this->client->readBytesUntil('\n',chunkSize,sizeof(chunkSize));
                                            if(l<=0) {
                                                break;
                                            }

                                            chunkSize[l-1] = 0;
                                            int chunkSizeI = strtol(chunkSize,NULL,16);

                                            if(chunkSizeI == 0) {
                                                break;
                                            }

                                            char chunkData[chunkSizeI];
                                            l = this->client->readBytes(chunkData,chunkSizeI);
                                            if(l<=0) {
                                                break;
                                            }

                                            if(currentSize + l <= energyleaf_SensorDataResponse_size) {
                                                memcpy(bufferSensorDataResponse + currentSize, chunkData, l);
                                                currentSize += l;
                                            } else {
                                                this->client->stop(); 
                                                return ENERGYLEAF_ERROR::ERROR;
                                            }
                                        }
                                    } else {
                                        //currently not tested, therefore better use chunked data
                                        currentSize = bodySize;
                                        while(true) {
                                            int l = this->client->readBytesUntil('\n',bufferSensorDataResponse,currentSize);
                                            if(l==1) {
                                                break;
                                            }
                                        }
                                    }

                                    state = currentSize > 0;
                                }

                                if(!state) {
                                    this->client->stop(); 
                                    return ENERGYLEAF_ERROR::ERROR;
                                }

                                {
                                    pb_istream_t streamSensorDataResponseIn = pb_istream_from_buffer(bufferSensorDataResponse,currentSize);
                                    state = pb_decode(&streamSensorDataResponseIn,energyleaf_SensorDataResponse_fields, &sensorDataResponse);
                                }

                                if(!state) {
                                    this->client->stop(); 
                                    return ENERGYLEAF_ERROR::ERROR;
                                }
                            }

                            state = sensorDataResponse.status >= 200 && sensorDataResponse.status <= 299;
                            if(!state) {
                                this->client->stop(); 
                                if(sensorDataResponse.has_status_message) {
                                            log_d("ENERGYLEAF_DRIVER_DATA_REQUEST: UNSUCCESSFUL - ERROR WITH %d STATUS [%s]",sensorDataResponse.status, sensorDataResponse.status_message);
                                } else {
                                            log_d("ENERGYLEAF_DRIVER_DATA_REQUEST: UNSUCCESSFUL - ERROR WITH %d STATUS",sensorDataResponse.status);
                                }
                                if(sensorDataResponse.status == ENERGYLEAF_TOKEN_EXPIRED_CODE) {
                                    return ENERGYLEAF_ERROR::TOKEN_EXPIRED;
                                } else {
                                    return ENERGYLEAF_ERROR::ERROR;
                                }
                            } 
                        }
                        this->client->stop();
                        return ENERGYLEAF_ERROR::NO_ERROR;
                    } else {
                        return ENERGYLEAF_ERROR::ERROR;
                    }
                } else {
                    return ENERGYLEAF_ERROR::ERROR;
                }
            }

            //Requesting only token on this sensors currently!
            ENERGYLEAF_ERROR requestIntern() {
                if(this->client != nullptr) {
                    if(WiFi.isConnected()) {
                        bool state = false;
                        bool chunked = false;
                        uint16_t bodySize = 0;
                        {
                            //Prepare TokenRequest
                            uint8_t bufferTokenRequest[energyleaf_TokenRequest_size];
                            pb_ostream_t streamTokenRequestOut;
                            {
                                energyleaf_TokenRequest tokenRequest = energyleaf_TokenRequest_init_default;
                                //collect the MAC of this sensor
                                memcpy(tokenRequest.client_id, WiFi.macAddress().c_str(), sizeof(tokenRequest.client_id));
                                //set the type of this sensor
                                tokenRequest.type = this->type;

                                tokenRequest.need_script = false;

                                streamTokenRequestOut = pb_ostream_from_buffer(bufferTokenRequest, sizeof(bufferTokenRequest));

                                state = pb_encode(&streamTokenRequestOut,energyleaf_TokenRequest_fields, &tokenRequest);
                            }

                            if(!state) {
                                return ENERGYLEAF_ERROR::ERROR;
                            }

                            //Send TokenRequest and process received header
                            {
                                state = this->client->connect(this->host,this->port);
                                if(!state) {
                                    if(this->client->connected()){
                                        this->client->stop(); 
                                    }
                                    return ENERGYLEAF_ERROR::ERROR;
                                }

                                this->client->print(PSTR("POST /api/v1/token HTTP/1.1\r\n"));
                                this->client->print(PSTR("Host: "));
                                this->client->print(this->host);
                                this->client->print(PSTR("\r\n"));
                                this->client->print(PSTR("Content-Type: application/x-protobuf\r\n"));
                                this->client->print(PSTR("Content-Length: "));
                                this->client->print(streamTokenRequestOut.bytes_written);
                                this->client->print(PSTR("\r\n\r\n"));

                                this->client->write(bufferTokenRequest, streamTokenRequestOut.bytes_written);

                                char header[128];
                                char headerStatus[4];
                                state = false;

                                while(true) {
                                    int l = this->client->readBytesUntil('\n',header,sizeof(header));
                                    if(l<=1) {
                                        break;
                                    }

                                    header[l-1] = 0;

                                    if(strstr_P(header,PSTR("HTTP/1.1"))) {
                                        strncpy(headerStatus,&header[9],3);
                                        headerStatus[3] = '\0';
                                        uint16_t headerStatusCode = atoi(headerStatus);
                                        if(headerStatusCode >= 200 && headerStatusCode <= 299) {
                                            log_d("ENERGYLEAF_DRIVER_TOKEN_REQUEST: GOT A %d STATUS",headerStatusCode);
                                        } else {
                                            log_d("ENERGYLEAF_DRIVER_TOKEN_REQUEST: UNSUCCESSFUL - GOT A %d STATUS",headerStatusCode);
                                            //return ENERGYLEAF_ERROR::ERROR; //If a special status is known that results in no body here we can return direct
                                        }
                                        continue;
                                    }

                                    if(strstr_P(header,PSTR("Content-Length:"))){
                                        char contentLength[4];
                                        strncpy(contentLength,&header[16],3);
                                        contentLength[3] = '\0';
                                        bodySize = atoi(contentLength);
                                        chunked = false;
                                        continue;
                                    }

                                    if(strstr_P(header,PSTR("Transfer-Encoding: chunked"))) {
                                        chunked = true;
                                        continue;
                                    }

                                    if(strstr_P(header,PSTR("Content-Type: application/x-protobuf"))) {
                                        state = true;
                                        continue;
                                    }
                                }
                            }     

                            if(!state) {
                                this->client->stop(); 
                                return ENERGYLEAF_ERROR::ERROR;
                            }       
                        }

                        uint8_t bufferScriptAcceptedRequest[energyleaf_ScriptAcceptedRequest_size];
                        pb_ostream_t streamScriptAcceptedRequestOut;

                        {
                            //Process received body and generate TokenResponse from it
                            energyleaf_TokenResponse tokenResponse = energyleaf_TokenResponse_init_default;
                            {
                                uint8_t bufferTokenResponse[energyleaf_TokenResponse_size];
                                int currentSize = 0;
                                {
                                    if(chunked) {
                                        while(true) {
                                            char chunkSize[16];
                                            int l = this->client->readBytesUntil('\n',chunkSize,sizeof(chunkSize));
                                            if(l<=0) {
                                                break;
                                            }

                                            chunkSize[l-1] = 0;
                                            int chunkSizeI = strtol(chunkSize,NULL,16);

                                            if(chunkSizeI == 0) {
                                                break;
                                            }

                                            char chunkData[chunkSizeI];
                                            l = this->client->readBytes(chunkData,chunkSizeI);
                                            if(l<=0) {
                                                break;
                                            }

                                            if(currentSize + l <= energyleaf_TokenResponse_size) {
                                                memcpy(bufferTokenResponse + currentSize, chunkData, l);
                                                currentSize += l;
                                            } else {
                                                this->client->stop(); 
                                                return ENERGYLEAF_ERROR::ERROR;
                                            }
                                        }
                                    } else {
                                        //currently not tested, therefore better use chunked data
                                        currentSize = bodySize;
                                        while(true) {
                                            int l = this->client->readBytesUntil('\n',bufferTokenResponse,currentSize);
                                            if(l==1) {
                                                break;
                                            }
                                        }
                                    }
                                    state = currentSize > 0;
                                }

                                if(!state) {
                                    this->client->stop(); 
                                    return ENERGYLEAF_ERROR::ERROR;
                                }

                                {
                                    pb_istream_t streamTokenResponseIn = pb_istream_from_buffer(bufferTokenResponse,currentSize);
                                    state = pb_decode(&streamTokenResponseIn,energyleaf_TokenResponse_fields, &tokenResponse);
                                }

                                if(!state) {
                                    this->client->stop(); 
                                    return ENERGYLEAF_ERROR::ERROR;
                                }
                            }

                            log_d("TokenResponse.Status: %d", tokenResponse.status);
                            state = tokenResponse.status >= 200 && tokenResponse.status <= 299 && tokenResponse.has_access_token;
                            if(!state) {
                                this->client->stop(); 
                                if(tokenResponse.has_status_message) {
                                    log_d("ENERGYLEAF_DRIVER_TOKEN_REQUEST: UNSUCCESSFUL - ERROR WITH %d STATUS [%s]",tokenResponse.status, tokenResponse.status_message);
                                } else {
                                    log_d("ENERGYLEAF_DRIVER_TOKEN_REQUEST: UNSUCCESSFUL - ERROR WITH %d STATUS",tokenResponse.status);
                                }
                                if(tokenResponse.status == ENERGYLEAF_TOKEN_EXPIRED_CODE) {
                                    return ENERGYLEAF_ERROR::TOKEN_EXPIRED;
                                } else {
                                    return ENERGYLEAF_ERROR::ERROR;
                                }
                            }

                            //Store the Parameter from the TokenResponse
                            {
                                if(this->accessToken != nullptr) {
                                    delete[] this->accessToken;
                                }
                                this->accessToken = new char[strlen(tokenResponse.access_token) + 1];
                                strcpy(this->accessToken, tokenResponse.access_token);
                                this->expiresIn = tokenResponse.expires_in;

                                this->client->stop(); 
                                return ENERGYLEAF_ERROR::NO_ERROR;
                            }
                        }
                    } else {
                        return ENERGYLEAF_ERROR::ERROR;
                    }
                } else {
                    return ENERGYLEAF_ERROR::ERROR;
                }
            }
        protected:
    };
}

#endif
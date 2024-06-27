#ifndef ENERGYLEAF_SENSOR_SERIALLOG_HPP
#define ENERGYLEAF_SENSOR_SERIALLOG_HPP

#include <Arduino.h>
#include "Core/Log/ILog.hpp"

namespace Log {
    class SerialLog : public Apalinea::Core::Log::ILog {
    public:
        SerialLog() = default;

        ~SerialLog() override = default;

        void open() final {
        }

        void close() final {
        }

        void flush() final {
            Serial.flush();
        }

        void log(Apalinea::Core::Log::LogLevel level, std::tm* time, std::string_view file, int line, std::string_view message) final {
            if(line >= 0) {
                Serial.print("APALINEA_LOG]["); 
                Serial.print(time->tm_mday);
                Serial.print(".");
                Serial.print((time->tm_mon + 1));
                Serial.print(".");
                Serial.print((time->tm_year + 1900));
                Serial.print("-");
                Serial.print(time->tm_hour);
                Serial.print(":");
                Serial.print(time->tm_min); 
                Serial.print(":");
                Serial.print(time->tm_sec);
                Serial.print("][");
                Serial.print(static_cast<int>(level.getLogLevelCategory()));
                Serial.print("][");
                Serial.print(file.data());
                Serial.print(":");
                Serial.print(line);
                Serial.print("]:");
                Serial.print(message.data());
                Serial.print("\n");
            } else {
                Serial.print("APALINEA_LOG]["); 
                Serial.print(time->tm_mday);
                Serial.print(".");
                Serial.print((time->tm_mon + 1));
                Serial.print(".");
                Serial.print((time->tm_year + 1900));
                Serial.print("-");
                Serial.print(time->tm_hour);
                Serial.print(":");
                Serial.print(time->tm_min); 
                Serial.print(":");
                Serial.print(time->tm_sec);
                Serial.print("][");
                Serial.print(static_cast<int>(level.getLogLevelCategory()));
                Serial.print("][");
                Serial.print(file.data());
                Serial.print("]:");
                Serial.print(message.data());
                Serial.print("\n");
            }
        }
    };
}

#endif

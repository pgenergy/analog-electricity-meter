#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <fb_gfx.h>

#include <WiFiManager.h> 

#include <cstring>
#include <deque>

#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "soc/soc.h"           //disable brownout problems
#include "time.h"

#include <Core/Plan/Plan.hpp>
#include <Core/Operator/SourceOperator/CameraSourceOperator/CameraSourceOperator.hpp>
#include <Core/Operator/PipeOperator/CropPipeOperator/CropPipeOperator.hpp>
#include "Core/Operator/PipeOperator/DetectorPipeOperator/DetectorPipeOperator.hpp"
#include "Core/Operator/PipeOperator/StatePipeOperator/StatePipeOperator.hpp"
#include "Core/Operator/PipeOperator/SelectPipeOperator/SelectPipeOperator.hpp"
#include "Core/Operator/PipeOperator/CalculatorPipeOperator/CalculatorPipeOperator.hpp"
#include "Core/Operator/PipeOperator/EnrichPipeOperator/EnrichPipeOperator.hpp"
#include "Core/Operator/SinkOperator/SenderSinkOperator/SenderSinkOperator.hpp"
#include "Enricher/Token.hpp"
#include "Sender/Power.hpp"
#include "CameraEL.hpp"
#include <Core/Constants/Settings.hpp>
#include "PSRAMCreator.hpp"
#include <Expression/Datatype/DtSizeTExpression.hpp>
#include <Expression/ToExpression/ToDtBoolExpression.hpp>
#include <Expression/Compare/CompareExpression.hpp>
#include "Executor/FRExecutor.hpp"

SET_LOOP_TASK_STACK_SIZE(16 * 1024);  // 16KB

constexpr bool USE_WEBSERVER = false;

static const uint32_t UPDATE_INTERVAL = 50;
const char *otaPassword = "energyleaf";

const char *ntpServer = "pool.ntp.org";  // Recode to use TZ
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;

Energyleaf::Stream::V1::Core::Plan::Plan plan(std::make_shared<Sensor::Executor::FRExecutor>(2));

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // disable brownout detector

    Serial.begin(115200);
    Serial.setDebugOutput(true);

    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());
    log_i("Current MAC: %s", WiFi.macAddress().c_str());

    WiFiManager wifiManager;
    wifiManager.autoConnect("Energyleaf_Sensor");

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else  // U_SPIFFS
                type = "filesystem";

            Serial.println("Start updating " + type);
            esp_camera_deinit();
        })
        .onEnd([]() { Serial.println("\nEnd"); })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
                Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR)
                Serial.println("End Failed");
        });

    ArduinoOTA.setPassword(otaPassword);
    ArduinoOTA.begin();

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    Energyleaf::Stream::Constants::Settings::uint8_tCreator.setCreator(std::make_unique<PSRAMCreator<std::uint8_t>>());

    auto camerasourcelink = plan.createSource<Energyleaf::Stream::V1::Core::Operator::SourceOperator::CameraSourceOperator<CameraEL>>();
    
    camera_config_t vConfig;
    vConfig.ledc_channel = LEDC_CHANNEL_0;
    vConfig.ledc_timer = LEDC_TIMER_0;
    vConfig.pin_d0 = 5;
    vConfig.pin_d1 = 18;
    vConfig.pin_d2 = 19;
    vConfig.pin_d3 = 21;
    vConfig.pin_d4 = 36;
    vConfig.pin_d5 = 39;
    vConfig.pin_d6 = 34;
    vConfig.pin_d7 = 35;
    vConfig.pin_xclk = 0;
    vConfig.pin_pclk = 22;
    vConfig.pin_vsync = 25;
    vConfig.pin_href = 23;
    vConfig.pin_sccb_sda = 26;
    vConfig.pin_sccb_scl = 27;
    vConfig.pin_pwdn = 32;
    vConfig.pin_reset = -1;
    vConfig.xclk_freq_hz = 10000000;
    vConfig.pixel_format = PIXFORMAT_JPEG;
    vConfig.grab_mode = CAMERA_GRAB_LATEST;

        if (psramFound()) {
            vConfig.frame_size = FRAMESIZE_QVGA;
            vConfig.jpeg_quality = 10;
            vConfig.fb_count = 2;
            vConfig.fb_location = CAMERA_FB_IN_PSRAM;
        } else {
            vConfig.frame_size = FRAMESIZE_QVGA;
            vConfig.jpeg_quality = 12;
            vConfig.fb_count = 1;
            vConfig.fb_location = CAMERA_FB_IN_DRAM;
        }

    camerasourcelink->getOperator().setCameraConfig(vConfig);
    camerasourcelink->getOperator().start();
    auto enrichRequest = plan.createPipe<Energyleaf::Stream::V1::Core::Operator::PipeOperator::EnrichPipeOperator<Sensor::Enricher::Token>>();
    enrichRequest->getOperator().getEnricher().getSender()->setHost("admin.energyleaf.de");
    enrichRequest->getOperator().getEnricher().getSender()->setPort(443);
    auto pipelink2 = plan.createPipe<Energyleaf::Stream::V1::Core::Operator::PipeOperator::CropPipeOperator>();
    pipelink2->getOperator().setSize(120, 60, 0, 240);
    auto pipelink3 = plan.createPipe<Energyleaf::Stream::V1::Core::Operator::PipeOperator::DetectorPipeOperator>();
    pipelink3->getOperator().setLowerBorder(Energyleaf::Stream::V1::Types::Pixel::HSV(90.f,50.f,70.f));
    pipelink3->getOperator().setHigherBorder(Energyleaf::Stream::V1::Types::Pixel::HSV(128.f,255.f,255.f));
    auto pipelink4 = plan.createPipe<Energyleaf::Stream::V1::Core::Operator::PipeOperator::SelectPipeOperator>();
    //pipelink4->getOperator().setThreshold(300);//Expression
    auto *ti = new Energyleaf::Stream::V1::Expression::DataType::DtSizeTExpression("FOUNDPIXEL");
    auto *cv = new Energyleaf::Stream::V1::Expression::DataType::DtSizeTExpression(300);
    auto *comp = new Energyleaf::Stream::V1::Expression::Compare::CompareExpression();
    comp->add(ti);
    comp->add(cv);
    comp->setCompareType(Energyleaf::Stream::V1::Expression::Compare::CompareTypes::GREATER_THAN);
    pipelink4->getOperator().setExpression(comp);
    auto pipelink5 = plan.createPipe<Energyleaf::Stream::V1::Core::Operator::PipeOperator::StatePipeOperator>();
    pipelink5->getOperator().setState(false);
    auto pipelink6 = plan.createPipe<Energyleaf::Stream::V1::Core::Operator::PipeOperator::CalculatorPipeOperator>();
    pipelink6->getOperator().setRotationPerKWh(375);
    auto websink = plan.createSink<Energyleaf::Stream::V1::Core::Operator::SinkOperator::SenderSinkOperator<Sensor::Sender::Power>>();
    websink.get()->getOperator().getSender().setSender(enrichRequest.get()->getOperator().getEnricher());
    
    plan.connect(camerasourcelink,enrichRequest);
    plan.connect(enrichRequest,pipelink2);
    plan.connect(pipelink2,pipelink3);
    plan.connect(pipelink3,pipelink4);
    plan.connect(pipelink4,pipelink5);
    plan.connect(pipelink5,pipelink6);
    plan.connect(pipelink6,websink);
}

void loop() {
    ArduinoOTA.handle();
    try {
        plan.process();
    } catch (std::runtime_error &error) {
        log_d("Error: %s", error.what());
    }
}

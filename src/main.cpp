#include <Arduino.h>
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
#include "Core/Plan/Plan.hpp"
#include "Operator/SourceOperator/CameraSourceOperator/CameraSourceOperator.hpp"
#include "Operator/PipeOperator/CropPipeOperator/CropPipeOperator.hpp"
#include "Operator/PipeOperator/DetectorPipeOperator/DetectorPipeOperator.hpp"
#include "Operator/PipeOperator/StatePipeOperator/StatePipeOperator.hpp"
#include "Operator/PipeOperator/SelectPipeOperator/SelectPipeOperator.hpp"
#include "Operator/PipeOperator/EnergyCalculatorPipeOperator/EnergyCalculatorPipeOperator.hpp"
#include "Operator/PipeOperator/EnrichPipeOperator/EnrichPipeOperator.hpp"
#include "Operator/SinkOperator/SenderSinkOperator/SenderSinkOperator.hpp"
#include "Enricher/TokenEnrichOperator.hpp"
#include "Enricher/Token.hpp"
#include "Sender/Energy.hpp"
#include "CameraEL.hpp"
#include "Core/Constants/Settings.hpp"
#include "PSRAMCreator.hpp"
#include "Expression/Datatype/DtSizeTExpression.hpp"
#include "Expression/ToExpression/ToDtBoolExpression.hpp"
#include "Expression/Compare/CompareExpression.hpp"
#include "Executor/FRExecutor.hpp"
#include "Core/Executor/STLExecutor.hpp"
#include "Log/SerialLog.hpp"
#include "Sender/EnergySenderSinkOperator.hpp"

SET_LOOP_TASK_STACK_SIZE(16 * 1024);

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;

Apalinea::Core::Plan::Plan* plan;

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // disable brownout detector

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println("Test");
    Serial.flush();

    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());
    log_i("Current MAC: %s", WiFi.macAddress().c_str());

    WebSender::WebSender ws;
    Apalinea::Core::Log::LogManager::addLog(std::make_unique<Log::SerialLog>());
    plan = new Apalinea::Core::Plan::Plan(std::make_shared<Apalinea::Core::Executor::STLExecutor>(2));

    WiFiManager wifiManager;
    wifiManager.autoConnect("Energyleaf_Sensor");

    wifi_ps_type_t theType;

    esp_err_t set_ps = esp_wifi_set_ps(WIFI_PS_NONE);

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    Apalinea::Core::Constants::Settings::uint8_tCreator.setCreator(std::make_unique<PSRAMCreator<std::uint8_t>>());

    auto camerasourcelink = plan->createSource<Apalinea::Operator::SourceOperator::CameraSourceOperator<CameraEL>>();
    
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
    vConfig.xclk_freq_hz = 16500000;//10000000;
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
    auto enrichRequest = plan->createPipe<Enricher::TokenEnrichPipeOperator>();
    enrichRequest->getOperator().getEnricher().getSender()->setHost("admin.energyleaf.de");
    enrichRequest->getOperator().getEnricher().getSender()->setPort(443);
    auto pipelink2 = plan->createPipe<Apalinea::Operator::PipeOperator::CropPipeOperator>();
    pipelink2->getOperator().setSize(120, 60, 0, 240);
    auto pipelink3 = plan->createPipe<Apalinea::Operator::PipeOperator::DetectorPipeOperator>();
    pipelink3->getOperator().setLowerBorder(Apalinea::Core::Type::Pixel::HSV(90.f,50.f,70.f));
    pipelink3->getOperator().setHigherBorder(Apalinea::Core::Type::Pixel::HSV(128.f,255.f,255.f));
    auto pipelink4 = plan->createPipe<Apalinea::Operator::PipeOperator::SelectPipeOperator>();
    auto *ti = new Apalinea::Expression::DataType::DtSizeTExpression("FOUNDPIXEL");
    auto *cv = new Apalinea::Expression::DataType::DtSizeTExpression(300);
    auto *comp = new Apalinea::Expression::Compare::CompareExpression();
    comp->add(ti);
    comp->add(cv);
    comp->setCompareType(Apalinea::Expression::Compare::CompareTypes::GREATER_THAN);
    pipelink4->getOperator().setExpression(comp);
    auto pipelink5 = plan->createPipe<Apalinea::Operator::PipeOperator::StatePipeOperator>();
    pipelink5->getOperator().setState(false);
    auto pipelink6 = plan->createPipe<Apalinea::Operator::PipeOperator::EnergyCalculatorPipeOperator>();
    auto websink = plan->createSink<Sender::EnergySenderSinkOperator>();
    websink.get()->getOperator().getSender().setSender(enrichRequest.get()->getOperator().getEnricher().getSender());
    
    plan->connect(camerasourcelink,enrichRequest);
    plan->connect(enrichRequest,pipelink2);
    plan->connect(pipelink2,pipelink3);
    plan->connect(pipelink3,pipelink4);
    plan->connect(pipelink4,pipelink5);
    plan->connect(pipelink5,pipelink6);
    plan->connect(pipelink6,websink);
    plan->order();
}

void loop() {
    try {
        plan->processOrdered();
        plan->join();
    } catch (std::runtime_error &error) {
        log_d("Error: %s", error.what());
    }
}

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <FS.h>  // SD Card ESP32
#include <SPIFFS.h>
#include <WiFi.h>
#include <fb_gfx.h>

#include <cstring>
#include <deque>

#include "Calculator.hpp"
#include "Detector.hpp"
#include "State.hpp"
#include "Window.hpp"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "img_converters.h"
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "soc/soc.h"           //disable brownout problems
#include "time.h"

#include <Core/Plan/Plan.hpp>
#include <Core/Operator/SinkOperator/CoutSinkOperator/CoutSinkOperator.hpp>
#include <Core/Operator/SourceOperator/StringDemoSourceOperator/StringDemoSourceOperator.hpp>
#include "CameraEL.hpp"
#include <Core/Operator/SourceOperator/CameraSourceOperator/CameraSourceOperator.hpp>
#include <Core/Operator/PipeOperator/CropPipeOperator/CropPipeOperator.hpp>
#include "Core/Operator/PipeOperator/DetectorPipeOperator/DetectorPipeOperator.hpp"
#include "Core/Operator/PipeOperator/StatePipeOperator/StatePipeOperator.hpp"
#include "Core/Operator/PipeOperator/SelectPipeOperator/SelectPipeOperator.hpp"
#include "Core/Operator/PipeOperator/CalculatorPipeOperator/CalculatorPipeOperator.hpp"
#include "Core/Operator/SinkOperator/WebSenderSinkOperator/WebSenderSinkOperator.hpp"
#include "Core/Operator/SinkOperator/SerialSinkOperator/SerialSinkOperator.hpp"
#include "CameraTest.hpp"

SET_LOOP_TASK_STACK_SIZE(16 * 1024);  // 16KB

constexpr bool USE_WEBSERVER = false;

const char *ssid = "FRITZ!Box 7590 OT";
const char *password = "44090592638152189621";
const char *otaPassword = "energyleaf";  // For Testing only actual

static const uint32_t UPDATE_INTERVAL = 50;

const char *ntpServer = "pool.ntp.org";  // Recode to use TZ
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;

//Energyleaf::V1::Detector detector;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t *req) {
    char *part_buf[64];
    esp_err_t res = ESP_OK;
    //detector.initialize();

    Serial.println("Stream requested");
    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        Serial.println("STREAM: failed to set HTTP response type");
        return res;
    }
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY,
                                    strlen(_STREAM_BOUNDARY));
    }

    while (true) {
        try {
            //detector.push();
        } catch (std::runtime_error &err) {
            Serial.println(err.what());
            return ESP_FAIL;
        }

        if (res == ESP_OK) {
           // size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART,
                                   //detector.getJpgBufLen());
           // res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK) {
           //res = httpd_resp_send_chunk(req, (const char *)detector.getJpgBuf(),
             //                           detector.getJpgBufLen());
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY,
                                        strlen(_STREAM_BOUNDARY));
        }
        //detector.afterPush();

        if (res != ESP_OK) {
            break;
        }
        vTaskDelay(UPDATE_INTERVAL / portTICK_PERIOD_MS);
    }

    //detector.deinitialize();

    return res;
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_uri_t index_uri = {.uri = "/",
                             .method = HTTP_GET,
                             .handler = stream_handler,
                             .user_ctx = NULL};

    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
    }
}

    Energyleaf::Stream::V1::Core::Plan::Plan plan;
void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // disable brownout detector

    Serial.begin(115200);
    Serial.setDebugOutput(true);

  log_d("Total heap: %d", ESP.getHeapSize());
  log_d("Free heap: %d", ESP.getFreeHeap());
  log_d("Total PSRAM: %d", ESP.getPsramSize());
  log_d("Free PSRAM: %d", ESP.getFreePsram());

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
    Serial.println("Connected to WiFi!");

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else  // U_SPIFFS
                type = "filesystem";

            // NOTE: if updating SPIFFS this would be the place to unmount
            Serial.println("Start updating " + type);
            SPIFFS.end();
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

    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (USE_WEBSERVER) {
        //detector.setUseWithWebServer(true);
        //startCameraServer();
        //detector.setGenerateJpegCall(true);
    } else {
        //detector.initialize();
        //detector.setAfterPushCall(true);
    }

    //auto sourcelink = plan.createLink(Energyleaf::Stream::V1::Link::make_SourceLinkUPtr<Energyleaf::Stream::V1::Core::Operator::SourceOperator::StringDemoSourceOperator>());
    auto camerasourcelink = plan.createLink(Energyleaf::Stream::V1::Link::make_SourceLinkUPtr<Energyleaf::Stream::V1::Core::Operator::SourceOperator::CameraSourceOperator<CameraEL>>());
    //auto testr = Energyleaf::Stream::V1::Link::make_SourceLinkUPtr<Energyleaf::Stream::V1::Core::Operator::SourceOperator::CameraSourceOperator<CameraEL>>();    
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

   camerasourcelink.get()->getOperator().setCameraConfig(vConfig);
   camerasourcelink.get()->getOperator().start();
    auto pipelink2 = plan.createLink(Energyleaf::Stream::V1::Link::make_PipeLinkUPtr<Energyleaf::Stream::V1::Core::Operator::PipeOperator::CropPipeOperator>());
    pipelink2.get()->getOperator().setSize(120, 60, 0, 240);
    auto pipelink3 = plan.createLink(Energyleaf::Stream::V1::Link::make_PipeLinkUPtr<Energyleaf::Stream::V1::Core::Operator::PipeOperator::DetectorPipeOperator>());
    pipelink3->getOperator().setLowerBorder(Energyleaf::Stream::V1::Types::Pixel::HSV(90.f,50.f,70.f));
    pipelink3->getOperator().setHigherBorder(Energyleaf::Stream::V1::Types::Pixel::HSV(128.f,255.f,255.f));
    auto pipelink4 = plan.createLink(Energyleaf::Stream::V1::Link::make_PipeLinkUPtr<Energyleaf::Stream::V1::Core::Operator::PipeOperator::SelectPipeOperator>());
    pipelink4->getOperator().setThreshold(300);
    auto pipelink5 = plan.createLink(Energyleaf::Stream::V1::Link::make_PipeLinkUPtr<Energyleaf::Stream::V1::Core::Operator::PipeOperator::StatePipeOperator>());
    pipelink5->getOperator().setState(false);
    auto pipelink6 = plan.createLink(Energyleaf::Stream::V1::Link::make_PipeLinkUPtr<Energyleaf::Stream::V1::Core::Operator::PipeOperator::CalculatorPipeOperator>());
    pipelink6->getOperator().setRotationPerKWh(375);

    /*auto websink = plan.createLink(Energyleaf::Stream::V1::Link::make_SinkLinkUPtr<Energyleaf::Stream::V1::Core::Operator::SinkOperator::WebSenderSinkOperator>());
    websink->getOperator().setSensorId(std::string("TestSensor").c_str());
    websink->getOperator().setHost("google.de");
    websink->getOperator().setPort(443);
    websink->getOperator().setEndpoint("demo");*/
    auto sinklink = plan.createLink(Energyleaf::Stream::V1::Link::make_SinkLinkUPtr<Energyleaf::Stream::V1::Core::Operator::SinkOperator::SerialSinkOperator>());
    //plan.connect(sourcelink,sinklink);
    plan.connect(camerasourcelink,pipelink2);
    plan.connect(pipelink2,pipelink3);
    plan.connect(pipelink3,pipelink4);
    plan.connect(pipelink4,pipelink5);
    plan.connect(pipelink5,pipelink6);
    //plan.connect(pipelink5,websink);
    plan.connect(pipelink6,sinklink);
}

void loop() {
    ArduinoOTA.handle();
    plan.process();

    if (!USE_WEBSERVER) {
        try {
            //detector.push();
        } catch (std::runtime_error &err) {
            log_d("Error: %s", err.what());
        }
    }
}

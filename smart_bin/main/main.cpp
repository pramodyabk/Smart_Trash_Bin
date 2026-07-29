#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "mqtt_client.h"
#include "driver/ledc.h"

// TensorFlow Lite Micro
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/system_setup.h"

// model header
#include "g_model.h"

static const char* TAG = "SMART_BIN";

// --- Configuration ---
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD

#define MQTT_BROKER_URI "mqtts://20c5892a531843cca01ec894c278f1de.s1.eu.hivemq.cloud:8883" 
#define MQTT_USERNAME CONFIG_HIVEMQ_USERNAME
#define MQTT_PASSWORD CONFIG_HIVEMQ_PASSWORD
#define MQTT_TOPIC "smartbin/detections/my_unique_bin_001"

// Encrypt ISRG Root X1
static const char *hivemq_root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRnXubJIVczcwDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHp0D6KEPIU6FjzC\n" \
"tg1+UcVh//eXk+uB9Tf/H6JvMHT1P8sQO4169/89BItEw5Uo8w80FzVzE/tW9f3E\n" \
"m42m+n1D/p/sZt0f9rU6z1y7qNfP1E4/zI/2bX8O3A5H6J9g3T1KkK/6P6X7uX8g\n" \
"lVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/\n" \
"6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A\n" \
"4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1\n" \
"X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5w\n" \
"Y7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V\n" \
"4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7\n" \
"p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0\n" \
"R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8\n" \
"glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1Kk\n" \
"K/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ\n" \
"2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+\n" \
"t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5\n" \
"wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3\n" \
"V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x\n" \
"7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j\n" \
"0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8\n" \
"glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1Kk\n" \
"K/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ\n" \
"2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+\n" \
"t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5\n" \
"wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3\n" \
"V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x\n" \
"7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j\n" \
"0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8\n" \
"glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1Kk\n" \
"K/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ\n" \
"2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5wY7U0y4X+\n" \
"t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3V4A5Dq3y5\n" \
"wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x7p6c4D1W3\n" \
"V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4LzT1KkK/6P6X7uX8glVvA2j7j0R1kLqJ9x\n" \
"7p6c4D1W3V4A5Dq3y5wY7U0y4X+t1X3X+YxQ2A4Lw==\n" \
"-----END CERTIFICATE-----\n";

// ESP32-CAM Pins
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27
#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22


#define SERVO_PIN 13 

// classes
const char* class_labels[] = {"cardboard", "glass", "metal", "paper", "plastic", "trash"};
const int NUM_CLASSES = 6;

// TFLite Globals
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
constexpr int kTensorArenaSize = 80 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

esp_mqtt_client_handle_t mqtt_client;

// --- Initialization Functions ---
void init_wifi() {
    // NVS Initialization
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASS);
    
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
    ESP_LOGI(TAG, "Connecting to Wi-Fi...");
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Successfully connected to HiveMQ Cloud Securely!");
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Disconnected from HiveMQ Cloud");
    }
}

void init_secure_mqtt() {
    esp_mqtt_client_config_t mqtt_cfg = {};
    
    // Configure secure connection and authentication
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URI;
    mqtt_cfg.broker.verification.certificate = hivemq_root_ca;
    mqtt_cfg.credentials.username = MQTT_USERNAME;
    mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    
   
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void init_servo() {
    ledc_timer_config_t timer_conf = {};
    timer_conf.duty_resolution = LEDC_TIMER_13_BIT;
    timer_conf.freq_hz = 50; 
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.timer_num = LEDC_TIMER_1;
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ch_conf = {};
    ch_conf.channel = LEDC_CHANNEL_1;
    ch_conf.duty = 400; // start center
    ch_conf.gpio_num = SERVO_PIN;
    ch_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    ch_conf.timer_sel = LEDC_TIMER_1;
    ledc_channel_config(&ch_conf);
}

esp_err_t init_camera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAM_PIN_D0; config.pin_d1 = CAM_PIN_D1; config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3; config.pin_d4 = CAM_PIN_D4; config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6; config.pin_d7 = CAM_PIN_D7; config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK; config.pin_vsync = CAM_PIN_VSYNC; config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD; config.pin_sccb_scl = CAM_PIN_SIOC; config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET; config.xclk_freq_hz = 20000000;
    
    config.pixel_format = PIXFORMAT_GRAYSCALE; 
    config.frame_size = FRAMESIZE_96X96;      
    config.jpeg_quality = 12;
    config.fb_count = 1;

    return esp_camera_init(&config);
}

void sort_trash(int class_index) {
    int target_duty = 400; 

    switch (class_index) {
        case 0: // Cardboard
        case 3: // Paper
            ESP_LOGW(TAG, "Routing to PAPER BIN");
            target_duty = 250; // Rotate left
            break;
        case 1: // Glass
        case 2: // Metal
        case 4: // Plastic
            ESP_LOGW(TAG, "Routing to RECYCLING BIN");
            target_duty = 550; // Rotate right
            break;
        case 5: // Trash
            ESP_LOGW(TAG, "Routing to LANDFILL BIN");
            target_duty = 400; // Stay center
            break;
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, target_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    
    vTaskDelay(pdMS_TO_TICKS(1500));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 400);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting Smart Bin Firmware");

    init_wifi();
    vTaskDelay(pdMS_TO_TICKS(5000)); // Wait for connection
    init_secure_mqtt();
    init_servo();
    
    if (init_camera() != ESP_OK) return;

    // --- Init TFLite ---
    tflite::InitializeTarget();
    model = tflite::GetModel(g_model);
    static tflite::MicroMutableOpResolver<5> micro_op_resolver;
    micro_op_resolver.AddConv2D();
    micro_op_resolver.AddMaxPool2D();
    micro_op_resolver.AddReshape();        
    micro_op_resolver.AddFullyConnected(); 
    micro_op_resolver.AddSoftmax();

    static tflite::MicroInterpreter static_interpreter(model, micro_op_resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;
    interpreter->AllocateTensors();
    input = interpreter->input(0);
    output = interpreter->output(0);

    // --- Main Loop ---
    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Load image into tensor (uint8 to int8)
        for (int i = 0; i < fb->len; i++) {
            input->data.int8[i] = (int8_t)((int)fb->buf[i] - 128); 
        }

        if (interpreter->Invoke() == kTfLiteOk) {
            int max_index = 0;
            int8_t max_score = -128; 

            for (int i = 0; i < NUM_CLASSES; i++) {
                if (output->data.int8[i] > max_score) {
                    max_score = output->data.int8[i];
                    max_index = i;
                }
            }

            // Dequantize
            float confidence = (max_score - output->params.zero_point) * output->params.scale;
            ESP_LOGI(TAG, "Detected: %s (%.2f%%)", class_labels[max_index], confidence * 100.0);

            // Publish if confident
            if (confidence > 0.70) {
                sort_trash(max_index);
                
                char payload[128];
                snprintf(payload, sizeof(payload), "{\"item\": \"%s\", \"confidence\": %.2f}", class_labels[max_index], confidence);
                esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, payload, 0, 1, 0);
            }
        }
        
        
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(500));
        
    } 
} 
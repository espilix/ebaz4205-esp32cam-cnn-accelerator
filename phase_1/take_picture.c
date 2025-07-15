/**
 * This example takes a picture and sends it to EBAZ4205 via UART
 */

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

// support IDF 5.x
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#include "esp_camera.h"

#define BOARD_ESP32CAM_AITHINKER 1

// UART Configuration for EBAZ4205 communication (avoid monitor conflict)
#define UART_NUM UART_NUM_1
#define UART_TX_PIN 12   // GPIO12 (safe pin)
#define UART_RX_PIN 13   // GPIO13 (safe pin)
#define UART_BAUD_RATE 921600  // High baud rate for image data
#define UART_BUF_SIZE 2048

// Image transmission protocol
#define IMG_HEADER_START 0xAA55
#define IMG_HEADER_END 0x55AA
#define CHUNK_SIZE 1024  // Send image in chunks

// ESP32Cam (AiThinker) PIN Map
#ifdef BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
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

#endif

static const char *TAG = "camera_uart";

// Image header structure
typedef struct {
    uint16_t start_marker;    // 0xAA55
    uint16_t width;
    uint16_t height;
    uint32_t data_length;
    uint16_t pixel_format;    // 0=RGB565, 1=JPEG, 2=GRAYSCALE
    uint16_t image_id;        // Sequential image counter
    uint32_t checksum;        // Simple checksum
    uint16_t end_marker;      // 0x55AA
} __attribute__((packed)) image_header_t;

#if ESP_CAMERA_SUPPORTED
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565,
    .frame_size = FRAMESIZE_96X96,    // Smaller size for faster transfer

    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

static esp_err_t init_camera(void)
{
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }
    ESP_LOGI(TAG, "Camera initialized successfully");
    return ESP_OK;
}
#endif

static esp_err_t init_uart(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver
    esp_err_t err = uart_driver_install(UART_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed");
        return err;
    }

    // Configure UART parameters
    err = uart_param_config(UART_NUM, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed");
        return err;
    }

    // Set UART pins
    err = uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed");
        return err;
    }

    ESP_LOGI(TAG, "UART initialized: TX=%d, RX=%d, Baud=%d", UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);
    return ESP_OK;
}

static uint32_t calculate_checksum(const uint8_t *data, size_t length)
{
    uint32_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

static esp_err_t send_image_header(camera_fb_t *fb, uint16_t image_id)
{
    image_header_t header = {0};
    
    header.start_marker = IMG_HEADER_START;
    header.width = fb->width;
    header.height = fb->height;
    header.data_length = fb->len;
    header.pixel_format = (fb->format == PIXFORMAT_RGB565) ? 0 : 
                         (fb->format == PIXFORMAT_JPEG) ? 1 : 2;
    header.image_id = image_id;
    header.checksum = calculate_checksum(fb->buf, fb->len);
    header.end_marker = IMG_HEADER_END;
    
    ESP_LOGI(TAG, "Sending header: ID=%d, Size=%dx%d, Length=%lu, Format=%d", 
             header.image_id, header.width, header.height, header.data_length, header.pixel_format);
    
    int bytes_sent = uart_write_bytes(UART_NUM, (const char*)&header, sizeof(header));
    if (bytes_sent != sizeof(header)) {
        ESP_LOGE(TAG, "Failed to send complete header");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

static esp_err_t send_image_data(camera_fb_t *fb)
{
    size_t total_sent = 0;
    size_t remaining = fb->len;
    
    ESP_LOGI(TAG, "Sending image data: %zu bytes", fb->len);
    
    while (remaining > 0) {
        size_t chunk_size = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        
        int bytes_sent = uart_write_bytes(UART_NUM, (const char*)(fb->buf + total_sent), chunk_size);
        if (bytes_sent < 0) {
            ESP_LOGE(TAG, "UART write error");
            return ESP_FAIL;
        }
        
        total_sent += bytes_sent;
        remaining -= bytes_sent;
        
        // Wait for UART buffer to clear
        uart_wait_tx_done(UART_NUM, pdMS_TO_TICKS(50));
        
        // Progress indicator (less frequent)
        if (total_sent % (CHUNK_SIZE * 5) == 0) {
            ESP_LOGI(TAG, "Progress: %.1f%%", (float)total_sent * 100.0 / fb->len);
        }
    }
    
    ESP_LOGI(TAG, "Image data sent: %zu bytes", total_sent);
    return ESP_OK;
}

static esp_err_t send_image_via_uart(camera_fb_t *fb, uint16_t image_id)
{
    esp_err_t err;
    
    // Send header first
    err = send_image_header(fb, image_id);
    if (err != ESP_OK) {
        return err;
    }
    
    // Small delay between header and data
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Send image data
    err = send_image_data(fb);
    if (err != ESP_OK) {
        return err;
    }
    
    // Send end marker
    uint16_t end_marker = 0xDEAD;
    uart_write_bytes(UART_NUM, (const char*)&end_marker, sizeof(end_marker));
    uart_wait_tx_done(UART_NUM, pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Image transmission completed");
    return ESP_OK;
}

void app_main(void)
{
#if ESP_CAMERA_SUPPORTED
    // Initialize UART first
    if (init_uart() != ESP_OK) {
        ESP_LOGE(TAG, "UART initialization failed");
        return;
    }
    
    // Initialize camera
    if (init_camera() != ESP_OK) {
        ESP_LOGE(TAG, "Camera initialization failed");
        return;
    }
    
    ESP_LOGI(TAG, "System ready - Camera and UART initialized");
    
    uint16_t image_counter = 0;
    
    while (1) {
        ESP_LOGI(TAG, "Taking picture #%d...", image_counter + 1);
        camera_fb_t *pic = esp_camera_fb_get();
        
        if (pic) {
            ESP_LOGI(TAG, "Picture captured: %dx%d, %zu bytes", 
                     pic->width, pic->height, pic->len);
            
            // Send image via UART
            esp_err_t err = send_image_via_uart(pic, image_counter);
            if (err == ESP_OK) {
                image_counter++;
                ESP_LOGI(TAG, "Image #%d sent successfully", image_counter);
            } else {
                ESP_LOGE(TAG, "Failed to send image #%d", image_counter + 1);
            }
            
            esp_camera_fb_return(pic);
        } else {
            ESP_LOGE(TAG, "Failed to capture image");
        }
        
        // Wait before next capture
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
#else
    ESP_LOGE(TAG, "Camera support is not available for this chip");
#endif
}

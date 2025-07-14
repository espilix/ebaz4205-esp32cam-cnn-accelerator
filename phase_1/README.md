# Phase 1 Checklist: ESP32-CAM with Zynq PS Integration

---

## **A. Grayscale Image Pipeline (UART)**

### ESP32-CAM
- [ ] Configure camera for 240x240 **grayscale** mode.
- [ ] Capture grayscale image frame.
- [ ] Transmit grayscale image data via **UART** to Zynq PS.

### Zynq PS (ARM Cortex-A9)
- [ ] Receive 240x240 grayscale image data via UART.
- [ ] Buffer/store the grayscale image.
- [ ] Send grayscale image data (as-is) to 240x240 SPI LCD (ST7789V).
- [ ] Validate: LCD displays the correct grayscale image.

---

## **B. RGB565 Image Pipeline (UART)**

### ESP32-CAM
- [ ] Change camera capture mode to 240x240 **RGB565**.
- [ ] Capture RGB565 image frame.
- [ ] Transmit RGB565 image data via **UART** to Zynq PS.

### Zynq PS (ARM Cortex-A9)
- [ ] Receive 240x240 RGB565 image data via UART.
- [ ] Buffer/store the RGB565 image.
- [ ] Send RGB565 image data to 240x240 SPI LCD (ST7789V).
- [ ] Validate: LCD displays the correct color (RGB565) image.

---

## **C. Upgrade to SPI for Real-Time Performance**

### ESP32-CAM
- [ ] Reconfigure to transmit image data via **SPI** (as master).
- [ ] Ensure SPI protocol and framing matches Zynq PS expectations.

### Zynq PS (ARM Cortex-A9)
- [ ] Configure as **SPI slave** to receive image data from ESP32-CAM.
- [ ] Buffer/store received image (grayscale or RGB565).
- [ ] Send image data to 240x240 SPI LCD (ST7789V).
- [ ] Validate: LCD displays correct image at higher frame rate.
- [ ] Measure and document improved frame rate and latency.

---

## **D. General Validation**
- [ ] For each mode and interface, verify LCD output matches ESP32-CAM capture.
- [ ] Document any issues or artifacts observed on the LCD.
- [ ] Confirm reliable, repeatable operation for both UART and SPI modes.

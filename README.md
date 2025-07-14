### Description

**Real-time Edge AI Video Pipeline: ESP32-CAM to EBAZ4205 FPGA with Systolic Array CNN Accelerator**

This project implements a complete real-time AI video processing pipeline using an ESP32-CAM as the image source and an EBAZ4205 (Zynq-7010) FPGA board as the processing platform. It features:
- Custom camera capture (240x240, grayscale/RGB565)
- High-speed UART/SPI image transfer
- LCD display output
- FPGA-based CNN accelerator IP core using a systolic array architecture for object detection
- Integration with Zynq PS (ARM) for control, preprocessing, and postprocessing

The project includes Verilog HDL, C drivers, Python/TensorFlow model scripts, and detailed documentation for reproducibility and learning.

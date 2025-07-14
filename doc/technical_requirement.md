# Technical Project Requirements & Implementation Plan  
**ESP32-CAM to EBAZ4205 AI Video Pipeline with Systolic Array CNN Accelerator**

---

## 1. System Overview

- **Goal:** Real-time object detection on 240x240 RGB565 images from ESP32-CAM, processed by a Zynq-7010 FPGA (EBAZ4205) using a custom CNN IP core with a systolic array.
- **Target:** ≥10 FPS inference, <100ms latency, <7W system power.

---

## 2. Hardware Requirements

- **ESP32-CAM Module:** OV2640, custom 240x240 frame, UART/SPI interface.
- **EBAZ4205 Board:** Zynq-7010 (ARM Cortex-A9 + Artix-7 FPGA), 256MB DDR3, ST7789V 240x240 LCD, USB-UART, SPI, GPIOs.
- **Optional:** PC for development, logic analyzer, oscilloscope.

---

## 3. Software/Firmware Requirements

- **ESP32-CAM:**  
  - Camera init (240x240, grayscale/RGB565)
  - UART/SPI image transfer with framing protocol
  - SPI master mode for high-speed transfer
- **Zynq PS (ARM):**  
  - UART/SPI receiver, frame buffer in DDR
  - Grayscale to RGB565 conversion (if needed)
  - SPI LCD driver (ST7789V)
  - CNN accelerator driver (DMA, AXI)
  - Application: preprocessing, postprocessing, LCD overlay
- **FPGA (PL):**  
  - Systolic array CNN IP core (8x8/16x16 PEs, INT8)
  - AXI4/AXI4-Stream interfaces
  - LCD SPI controller (if not using PS SPI)
  - CNN controller FSM

---

## 4. Communication Protocols

- **UART (initial):**  
  - 2–4 Mbps, start marker, size, data, end marker, error handling.
- **SPI (high-speed):**  
  - ≥10 Mbps, same frame structure, ESP32 master, Zynq slave.

---

## 5. Image Processing Pipeline

1. **Grayscale Mode:**  
   - ESP32 captures grayscale (240x240), sends via UART/SPI, Zynq buffers and expands to RGB565 for LCD.
2. **RGB565 Mode:**  
   - ESP32 captures and sends RGB565 directly.
3. **CNN Inference:**  
   - Zynq sends image to CNN IP core, receives detection results, overlays on LCD.

---

## 6. CNN Accelerator IP Core (Systolic Array)

- **Array Size:** 8x8 or 16x16 PEs (resource-dependent)
- **Data Precision:** 8-bit INT
- **Clock:** 100–200 MHz
- **Memory:** AXI4 for DDR3, on-chip buffers for weights/activations
- **PE Structure:** MAC, local data reuse, input/weight/psum registers
- **Control:** FSM for layer sequencing, instruction memory
- **Integration:** AXI4/AXI4-Stream, DMA for data transfer

---

## 7. Performance Specifications

| Parameter      | Specification           |
|----------------|------------------------|
| Throughput     | 10–20 FPS (240x240)    |
| Latency        | <100 ms/inference      |
| Power          | <7W total system       |
| Accuracy       | >85% on dataset        |
| Resource Usage | <80% of Zynq-7010      |

---

## 8. Implementation Steps

### **Phase 1: System Bring-up**

1. **ESP32-CAM Setup:**  
   - Configure camera for 240x240, grayscale → RGB565.
   - Implement UART/SPI image transfer with framing.
2. **Zynq PS Setup:**  
   - UART/SPI receiver, buffer image in DDR.
   - LCD SPI driver: display test patterns, then images.
   - Validate end-to-end image capture and display.

### **Phase 2: CNN Model Preparation**

1. **Model Selection:**  
   - Lightweight CNN (MobileNet-style or custom) for 240x240 input.
2. **Quantization:**  
   - Train with quantization-aware methods, export INT8 weights/activations.

### **Phase 3: Systolic Array IP Core Development**

1. **PE RTL Implementation:**  
   - Verilog module for MAC, data registers, local interconnect.
2. **Array Construction:**  
   - 2D grid of PEs, interconnect for feature maps, weights, partial sums.
3. **Buffering:**  
   - Input, weight, output, ping-pong buffers.
4. **AXI Interface:**  
   - AXI4 for DDR3, AXI4-Stream for data flow, DMA for transfers.
5. **Controller FSM:**  
   - Layer sequencing, data scheduling, pipeline management.
6. **Simulation:**  
   - Testbench for PE, array, and full IP core (ModelSim/Vivado).

### **Phase 4: System Integration**

1. **IP Packaging:**  
   - Package CNN IP as Vivado IP block.
2. **Zynq Integration:**  
   - Connect AXI interfaces, DMA, and interrupts.
3. **Software Driver:**  
   - C API for inference: DMA setup, start, poll, result fetch.
4. **Application Integration:**  
   - Preprocessing, postprocessing, LCD overlay, performance monitoring.

### **Phase 5: Optimization & Testing**

1. **Performance Tuning:**  
   - Data reuse, parallelism, pipelining, clock gating.
2. **Power Analysis:**  
   - Measure and optimize system power.
3. **Stress Testing:**  
   - Validate under various conditions, long runs.
4. **Accuracy Validation:**  
   - Test on target dataset, measure accuracy and FPS.

### **Phase 6: Documentation & Deployment**

1. **Documentation:**  
   - Schematics, pinouts, source code, protocol specs, build instructions, user guides.
2. **Deliverables:**  
   - All code (HDL, drivers, apps), test reports, demo videos.

---

## 9. Tools & Resources

- **Vivado Design Suite:** FPGA design/implementation.
- **Vitis HLS:** Optional for high-level synthesis.
- **ModelSim:** Simulation.
- **Python/TensorFlow:** Model training/quantization.
- **OpenCV:** Image processing/testing.
- **Hardware:** EBAZ4205, ESP32-CAM, ST7789V LCD, logic analyzer, oscilloscope.

---

## 10. Optimization Strategies

- **Memory:** Weight compression, activation reuse, tiling, buffering.
- **Computation:** Parallelization, pipelining, dataflow, precision tuning.

---

## 11. Example Code Snippets

**Processing Element (PE) Verilog:**
```verilog:src/pe.v
module processing_element (
    input clk,
    input rst_n,
    input [7:0] weight_in,
    input [7:0] ifmap_in,
    input [15:0] psum_in,
    output [7:0] weight_out,
    output [7:0] ifmap_out,
    output [15:0] psum_out
);
    reg [7:0] weight_reg;
    reg [7:0] ifmap_reg;
    reg [15:0] psum_reg;

    always @(posedge clk) begin
        if (!rst_n) begin
            weight_reg <= 8'b0;
            ifmap_reg <= 8'b0;
            psum_reg <= 16'b0;
        end else begin
            weight_reg <= weight_in;
            ifmap_reg <= ifmap_in;
            psum_reg <= psum_in + (weight_reg * ifmap_reg);
        end
    end

    assign weight_out = weight_reg;
    assign ifmap_out = ifmap_reg;
    assign psum_out = psum_reg;
endmodule
```

**Systolic Array Top Module:**
```verilog:src/systolic_array_8x8.v
module systolic_array_8x8 (
    input clk,
    input rst_n,
    input [7:0] weights_in [7:0],
    input [7:0] ifmap_in [7:0],
    output [15:0] results_out [7:0]
);
    // PE instantiation and interconnection
    genvar i, j;
    generate
        for (i = 0; i < 8; i = i + 1) begin : row
            for (j = 0; j < 8; j = j + 1) begin : col
                processing_element pe_inst (
                    .clk(clk),
                    .rst_n(rst_n),
                    .weight_in(/* interconnect */),
                    .ifmap_in(/* interconnect */),
                    .psum_in(/* interconnect */),
                    .weight_out(/* interconnect */),
                    .ifmap_out(/* interconnect */),
                    .psum_out(/* interconnect */)
                );
            end
        end
    endgenerate
endmodule
```

**AXI Interface Skeleton:**
```verilog:src/axi_interface.v
module axi_interface (
    // AXI4 Master interface
    input axi_clk,
    input axi_rst_n,
    // AXI Write/Read channels
    output [31:0] axi_awaddr,
    output [7:0] axi_awlen,
    // CNN IP interface
    input [7:0] cnn_data_in,
    output [7:0] cnn_data_out,
    // Control signals
    input start_inference,
    output inference_done
);
    // AXI transaction management
    // DMA operations for weight loading
    // Result writeback to memory
endmodule
```

**C Driver Skeleton:**
```c:src/cnn_driver.c
int cnn_inference(uint8_t *input_image, detection_result_t *results) {
    // Configure DMA for input transfer
    dma_transfer_to_pl(input_image, CNN_INPUT_ADDR, IMAGE_SIZE);

    // Start CNN inference
    Xil_Out32(CNN_CTRL_REG, CNN_START_BIT);

    // Wait for completion
    while(!(Xil_In32(CNN_STATUS_REG) & CNN_DONE_BIT));

    // Read results
    dma_transfer_from_pl(CNN_OUTPUT_ADDR, results, RESULT_SIZE);

    return 0;
}
```

---

## 12. References

- [Systolic Array CNN Accelerator](https://github.com/PSCLab-ASU/Systolic-CNN)
- [ZynqNet: An FPGA-Accelerated Embedded CNN](https://paperswithcode.com/paper/zynqnet-an-fpga-accelerated-embedded)
- [Edge AI Applications](https://xailient.com/blog/a-comprehensive-guide-to-edge-ai/)
- [YOLO on FPGA](https://www.slideshare.net/HirokiNakahara1/fpga2018-a-lightweight-yolov2-a-binarized-cnn-with-a-parallel-support-vector-regression-for-an-fpga)
- [Vivado, Vitis HLS, ModelSim, TensorFlow, OpenCV]

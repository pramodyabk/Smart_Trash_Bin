# AI-Powered Smart Sorting Bin ♻️

An end-to-end IoT smart bin that uses on-device Machine Learning (Edge AI) to automatically classify and sort waste. Built with an ESP32-CAM running TensorFlow Lite Micro, the system captures images, runs a quantized neural network to identify the waste type, physically sorts the item using a servo motor, and securely streams the detection data to a real-time Node.js web dashboard via MQTT.

## 🚀 Key Features

* **On-Device Edge AI:** Runs a quantized TensorFlow Lite model entirely on the ESP32-CAM's SRAM. No cloud processing required for inference.
* **Hardware-Accelerated Vision:** Utilizes the RHYX M21-45 (GC2145) camera module capturing raw 96x96 grayscale frames for optimized memory footprint.
* **Secure Cloud Telemetry:** Telemetry data is streamed using MQTT over TLS (Port 8883) with Root CA validation via HiveMQ Cloud Serverless.
* **Real-Time Web Dashboard:** A Node.js/Express backend subscribes to the MQTT broker and updates a frontend interface with the latest bin detections and confidence scores.
* **Secure Credentials:** Wi-Fi and MQTT secrets are completely decoupled from the codebase using ESP-IDF `menuconfig` and Node.js `.env` variables.

## 🛠️ Tech Stack

**Hardware:**

* ESP32-CAM Development Board
* RHYX M21-45 Camera Module
* Standard Servo Motor (SG90/MG996R)
* FTDI PL2303 USB-to-TTL Converter

**Firmware (C++):**

* ESP-IDF framework (v6.0+)
* TensorFlow Lite for Microcontrollers
* FreeRTOS (Task scheduling and delays)
* ESP-MQTT (Secure MQTTS implementation)

**Backend & Dashboard:**

* Node.js & Express.js
* MQTT.js

## 🧠 Machine Learning Pipeline

The system is trained on a 6-class dataset to identify common waste items:

1. `cardboard`
2. `glass`
3. `metal`
4. `paper`
5. `plastic`
6. `trash`

Frames are captured natively in grayscale, shifted to `int8` formatting (`-128` to `127`), and passed to the TFLite interpreter. The output tensor is dequantized back into a float confidence score. If the confidence exceeds 70%, the physical sorting sequence triggers and the data payload is published to the cloud.

## ⚙️ Setup & Installation

### 1. Firmware (ESP-IDF)

1. Clone the repository and navigate to the `firmware` folder.
2. Run `idf.py menuconfig`.
3. Navigate to **Smart Bin Configuration** and enter your local Wi-Fi and HiveMQ Cloud credentials.
4. Navigate to **Partition Table** and ensure `Single factory app (large), no OTA` is selected to accommodate the ML model.
5. Connect your ESP32-CAM in Download Mode (GPIO 0 to GND) and run:
```bash
idf.py build flash monitor

```



### 2. Web Dashboard (Node.js)

1. Navigate to the `dashboard` folder.
2. Install dependencies:
```bash
npm install

```


3. Create a `.env` file in the root of the dashboard directory:
```env
HIVEMQ_URL=mqtts://<your_cluster>.s1.eu.hivemq.cloud:8883
HIVEMQ_USERNAME=your_username
HIVEMQ_PASSWORD=your_password

```


4. Start the server:
```bash
node server.js

```


5. Open your browser and navigate to `http://localhost:3000`.

---

**Author:** Pramodya Karunasekara

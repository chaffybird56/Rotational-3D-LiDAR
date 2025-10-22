## 🛰️ 3D LIDAR Module — MSP432 + VL53L1X + Stepper Scanning & Point Clouds

<!-- Front-load a compelling result -->
<p align="center">
  <img width="709" height="447" alt="SCR-20250929-lasl" src="https://github.com/user-attachments/assets/fbff079c-c1da-4971-be08-fb57a24acae5" />
  <br/>
  <sub><b>Fig 1 — Hallway scan:</b> representative point cloud produced by the module.</sub>
</p>

> This project builds a **rotating lidar-like scanner** using an **MSP432E401Y microcontroller**, a **VL53L1X time‑of‑flight (ToF) sensor**, and a **28BYJ‑48 stepper motor**.  
> As the stepper rotates the sensor through 360°, the MCU collects distance samples at each step, streams them over UART, and a host computer reconstructs the readings into a **point cloud**.  
> In plain terms: the module “sweeps” its surroundings, recording how far away walls and objects are, and then stitches those measurements into a 2D/3D map. The goal is to demonstrate how embedded hardware, sensors, and host visualization come together to form a **low‑cost scanning system**.

---

## ⚡ System Architecture

<!-- Flowchart placeholder: acquisition → control → streaming → visualization -->
<p align="center">
  <img width="788" height="713" alt="SCR-20250929-laml" src="https://github.com/user-attachments/assets/d7c073ae-2829-4019-b1a6-26166af98c5b" />
  <br/>
  <sub><b>Fig 2 — Data flow:</b> initialization → step/measure → pack → UART → host visualization.</sub>
</p>

1. **Initialization.** Configure I²C for VL53L1X, stepper pins, and UART.  
2. **Sweep loop.** Rotate stepper → trigger ToF measurement → read distance.  
3. **Telemetry.** Send angle–distance pair via UART.  
4. **Host processing.** Convert polar readings to Cartesian coordinates and render as a point cloud.

---

## 🛠 Hardware Details

- **MSP432E401Y (ARM Cortex‑M4F)** handles motor stepping, sensor polling, and serial streaming.  
- **VL53L1X ToF sensor** (up to ~4 m range, ~50 Hz measurement rate, ±20 mm typical accuracy).  
- **28BYJ‑48 stepper + ULN2003 driver**: ~4096 microsteps per revolution for fine angular resolution.  
- **Buttons**: active‑low inputs for start/stop control.  
- **Interfaces**: I²C @ 100 kHz (sensor), UART ~128 kbps effective throughput (PC link).  

<!-- Circuit schematic placeholder -->
<p align="center">
  <img width="1460" height="751" alt="SCR-20250929-kzld" src="https://github.com/user-attachments/assets/32dd998f-aa16-4a4d-9869-aa191e9aa436" />
  <br/>
  <sub><b>Fig 3 — Circuit schematic:</b> MSP432 ↔ VL53L1X (I²C), MSP432 ↔ ULN2003 (GPIO), control buttons.</sub>
</p>


---

## 🧮 Measurement Model

At step index \(i\) the module has a motor angle \(\theta_i\) and a measured distance \(d_i\). With the sensor pointing vertically (module translating along the **x‑axis** between sweeps), one polar slice maps as:

$$
y_i = d_i \cos(\theta_i), \qquad z_i = d_i \sin(\theta_i)
$$

Stacking slices along \(x\) yields a volumetric reconstruction (if the module is shifted incrementally and multiple revolutions are recorded).

---

## 📈 Results

- **Hallway scans** reconstruct long straight surfaces (walls) and features beyond ~0.5 m.  
- **Object scans** produce dense clusters corresponding to target geometry.  
- Data accuracy is constrained by ToF latency and stepper jitter.  

<!-- Secondary result placeholder -->
<p align="center">
 <img width="808" height="415" alt="SCR-20250929-layj" src="https://github.com/user-attachments/assets/08b90cc3-b911-4f81-8b2e-c16571253770" />
  <br/>
  <sub><b>Fig 4 — Secondary scan view:</b> additional rendering of the scanned environment.</sub>
</p>

---

## ⚖️ Performance & Limitations

- **Sensor latency** is the bottleneck; dwell must allow VL53L1X to finish ranging.  
- **Serial link** limits throughput; UART bandwidth constrains density of streamed data.  
- **Motor precision**: missed steps or insufficient delays cause jitter.  
- **MCU math**: single‑precision FPU; heavier processing deferred to host side.  

---

## 🧠 Glossary

- **MCU** — Microcontroller Unit (MSP432E401Y).  
- **ToF** — Time‑of‑Flight ranging (VL53L1X).  
- **SPAD** — Single‑Photon Avalanche Diode (VL53L1X receiver).  
- **UART** — Universal Asynchronous Receiver/Transmitter.  
- **I²C** — Inter‑Integrated Circuit (sensor bus).  
- **FPU** — Floating‑Point Unit (single precision on MSP432).  
- **FOV** — Field of View.  
- **XYZ** — Cartesian coordinates of point‑cloud samples.  
- **Microstep** — Partial step of a stepper motor for finer resolution.  

---

## 📜 License

MIT — see `LICENSE`.

# B. System Architecture

[⬅️ Back to Main README](../README.md)

---

## 1. High-Level Block Diagram
The machine is built around a modular architecture separating the high-voltage power stage supply, low-voltage power stage supply, the sensing and comparator feedback loops, the central processing unit, and the user interface.

<!-- Espacio para tu diagrama de bloques -->
<p align="center">
  <img src="..images/diagrams/general_block_diag.jpeg" alt="System Block Diagram" width="600px" style="border-radius: 6px; border: 1px solid #ddd;">
</p>
<p align="center"><sub><i>Figure 1: High-level system block diagram.</i></sub></p>

---

## 2. Power Electronics Topology & Spark Extinction
For this build, two competing topological approaches were evaluated regarding how energy is stored, injected into the gap, and extinguished:

* **Why the Second Configuration Was Chosen:**  
  While the first configuration relies on standard switching, the **second configuration** was selected because it provides a **significantly faster spark extinction rate**. 
* **Operating Principle:**  
  When a spark completes or an unwanted condition occurs, the voltage across the inductor rises automatically, because of the opening of igbt 3. This voltage spike forward-biases a discharge path, enabling a fast and efficient discharge back into the main storage capacitor. This rapid quenching prevents excessive thermal buildup and gives cleaner control over the ~60 µs pulse width managed by the LM311 comparators.

<!-- Espacio para el diagrama de las dos configuraciones (Topología y secuencia de inductor/chispa) -->
<p align="center">
  <img src="..images/diagrams/idea.jpg" alt="Topologies Comparison and Spark Extinction" width="650px" style="border-radius: 6px; border: 1px solid #ddd;">
</p>
<p align="center"><sub><i>Figure 2: Topological configurations showing inductor charging, spark presence/absence, and the extinction mechanism.</i></sub></p>

---

## 3. Pre-Charge and Discharge Networks
To ensure safe handling of the high-voltage rail (up to 150VDC supplied by the series telecom power supplies):
* **Slow-Charge Network:** Employs high-power resistors to gradually charge the main pulse capacitors, preventing massive inrush currents upon startup.
* **Slow-Discharge Network:** Safely bleeds residual energy from the storage capacitors when the system is powered down, protecting the operator during bench testing. This is the default state, but more of this and else in later descriptions of control logic.

---

## 4. User Interface & Finite State Machine (FSM) Feedback
The user interface is kept minimal yet robust, relying on hardware-level simplicity rather than complex displays:

### Input Controls (3 Buttons)
* **GO Button:** Primary function selector.
* **Button A:** Secondary function selector (mode switching or manual jog routines depending on FSM state).
* **Button B:** Tertiary function / parameter adjustment / reset trigger.

### State Indication (3 LEDs)
Because the system lacks an LCD screen, the current state of the **Finite State Machine (FSM)** is communicated visually through a combination of **3 status LEDs**:
* **LED 1 & LED 2:** Located directly on the main PCB.
* **LED 3:** Located onboard the STM32 "Blue Pill" module.

By observing the binary/combinational lighting states of these 3 LEDs, the operator can instantly identify whether the machine is in *Idle, Pre-charge, Cutting, or Retract* mode, etc.

---

[⬅️ Back to Main README](../README.md)

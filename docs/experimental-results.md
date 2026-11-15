# E. Experimental Results & Oscilloscope Captures

[⬅️ Back to Main](..)

---

## E. Experimental Results & Oscilloscope Captures

### 1. Cutting Tests & Material Removal Rate (MRR)

To evaluate the machining performance of the sinker EDM prototype, three cutting trials were conducted across different workpiece materials (aluminum and structural steel) and step-control algorithms. Cavity surface areas were optically measured using **ImageJ** calibrated against a known dimensional reference, and total removed volume was calculated from the resulting ticknes of the material.

#### Cutting Parameters & Results Summary

| test                                 | voltage (V) | t_on (us) | t_off (us) | algorithm       | short current (A) | current spark (A) | time (mins) | MRR (mm3/min) |
|:------------------------------------:|:-----------:|:---------:|:----------:|:---------------:|:-----------------:|:-----------------:|:-----------:|:-------------:|
| aluminum                             | 156.7       | 8         | 52         | HIGH Duty Cycle | 30                | 20                | 5           | 4.31          |
| structural steel (direct polarity)   | 156.7       | 8         | 52         | LOW Duty Cycle  | 30                | 20                | 180         | 0.03          |
| structural steel (inverted polarity) | 156.7       | 12        | 48         | LOW Duty Cycle  | 40                | 34                | 180         | 0.12          |


---

#### Impact of Step-Control Strategy & Workpiece Material on MRR

A drastic drop in Material Removal Rate—from **4.30 $\text{mm}^3/\text{min}$** down to **0.12 $\text{mm}^3/\text{min}$**—was observed between the initial and subsequent trials. This reduction is driven by two compounding factors:

1. **Mechanical Step-Control Overhead (Firmware Sequence):**
   *(Stepper resolution: $12,800\text{ microsteps} = 2\text{ mm} \rightarrow 0.15625\ \mu\text{m/step}$)*
   * **Initial Algorithm (HIGH Duty Cycle):** The electrode advanced 1 microstep and triggered a 4-pulse burst. If a short circuit was detected, it retracted by just **1 microstep**, fired another 4-pulse burst, and evaluated the spark-to-short ratio. It continued stepping back 1 microstep at a time only until valid sparks outnumbered short circuits. This kept the electrode hovering tightly within the ionization gap, maximizing active spark time.
   * **Modified Algorithm (LOW Duty Cycle):** Upon detecting a short circuit, the firmware was modified to execute an immediate **50-microstep retraction (~7.8 $\mu\text{m}$)** without pulsing, followed by a single-step, single-pulse probing advance until valid sparks outnumbered shorts (at which point an 8-pulse machining burst was enabled). While this effectively cleared debris and prevented prolonged arcing, traversing 50 microsteps back and forth at single-pulse intervals introduced massive mechanical dead time, starving the gap of discharge energy.

2. **Workpiece Thermal & Physical Properties:**
   * **Aluminum vs. Structural Steel:** Aluminum has a significantly lower melting point ($\approx 660\ ^\circ\text{C}$) and lower latent heat of fusion compared to structural steel ($\approx 1,425 - 1,540\ ^\circ\text{C}$). Steel requires substantially higher thermal energy density per spark to melt and eject material, naturally yielding a much lower MRR even under optimal gap control.

---

#### Optical Area Measurement Methodology (ImageJ)

To accurately quantify the eroded cavity volume without relying solely on tool-tip geometry, top-down macro photographs of the machined craters were processed in **ImageJ**:

1. **Spatial Calibration:** A physical reference (machined feature or millimeter scale in the focal plane) was used to set the pixel-to-millimeter ratio (`Analyze > Set Scale`).
2. **Contour & Thresholding:** The eroded boundary was isolated using polygon selection / color thresholding to measure the exact projected area ($\text{mm}^2$).
3. **Volume Estimation:** $\text{Volume} = \text{Measured Area} \times \text{thickness of piece}$.

![ImageJ Cavity Area Measurement](../images/measurment_imageJ/area_piece_front.png)
*Figure 1: Example of cavity surface area measurement in ImageJ showing scale calibration and eroded boundary selection.*

---

### 2. Oscilloscope Captures & Waveform Analysis *(Coming Soon)*

* **Short-Circuit vs. Open-Circuit & Valid Spark Waveforms:** Gap voltage ($V_{gap}$) and discharge current ($I_{gap}$) behavior across spark states.
* **Gate Drive Signals:** High-side/low-side MOSFET switching transitions and dead-time verification.
* **Current Noise Analysis:** Switching transients, comparator (LM311) false-triggering mitigation, and parasitic inductance effects.
---

[⬅️ Back to Main](..)

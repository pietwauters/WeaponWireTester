# WeaponWireTester

**ESP32-based weapon & wire tester for fencing** — zero-button UX, fast fault finding, and accurate resistance measurement.  
Supports **body cords** and **weapons** (foil, sabre, épée). Lamé continuity is **WIP**.

---

## Overview

![image](https://github.com/user-attachments/assets/49b0fbee-1a46-4eb2-9e2e-ef7cf60c51a1)

- **Auto mode detection:** No buttons. The tester watches the connectors and switches between **wire test** and **weapon test** automatically.  
- **Intermittent fault capture:** Brief dropouts are latched and displayed for a few seconds.  
- **Clear LED UI:** 5×5 WS2812B matrix with intuitive color/status cues.  
- **Power:** USB-C (power bank or USB adapter).

**Connections:**  
- Plug the **enrouleur-side** body-cord plug into the **right** side of the tester, and the **weapon-side** plug into the **left** side.  
- A small hole accommodates the **foil/sabre safety clip**.  
- A small **lamé patch** on the enclosure allows clipping the crocodile clip.

![image](https://github.com/user-attachments/assets/b53730c7-0f4e-4a38-8e9d-456210d586d0)
*(Power via USB-C)*

---

## Quick start

1. **Power on** via USB-C.  
2. **Connect** a body cord on both sides, or a cord + weapon; the tester selects the mode automatically.  
3. **Wiggle-test** connectors and cable to expose intermittent faults (they’ll be latched briefly).

---

## Operation

### Mode selection (automatic)
- A detected connection **between cord pins** → **Wire Test**.  
- A **cord + weapon** combination → **Weapon Test** (foil/sabre/épée detected by wiring).

### Wire testing

**Two phases:**

1) **Quick Check** — Fast screening for obvious problems (broken wires, swaps, shorts). An animation highlights faults right-to-left / bottom-to-top.  
2) **Accurate Resistance** — Fine measurement; color encodes ohmic range. Move the cable/plug to catch intermittents.

| Display cue        | Meaning                                  | Resistance (Ω)     |
|--------------------|-------------------------------------------|--------------------|
| **Green solid**     | Good connection                           | **< 3**            |
| **Yellow solid**    | Acceptable / watchlist                    | **3 – 5**          |
| **Red solid**       | Out of spec (may still “work” sometimes)  | **5 – 10**         |
| **Red dotted**      | Broken / open                             | **> 10**           |
| **Blue**            | Unintended short between lines            | —                  |

### Weapon testing

**Foil & Sabre**
- **Idle:** No lights.  
- **Point pressed / intermittent on either wire:** **White** latched for a few seconds.  
- **Point pressed on lamé:** **Green** (valid touch).

**Épée**
- **Point pressed:** **Green** (valid touch).  
- **Any wire-to-wire short:** **Blue**.

---

## Hardware

- **Enclosure (3D print):** https://www.printables.com/model/1006755-body-wire-weapon-test-box  
  Brass inserts used: **M2** (PCB) and **M6** (lamé patch).  
- **Display:** 5×5 **WS2812B** LED matrix (mounted to top panel).  
- **Main PCB:** shared with the [3-weapon scoring device](https://github.com/pietwauters/esp32_scoring_device_hardware).

![image](https://github.com/user-attachments/assets/35de2eb8-d56a-4edc-a73d-227cd0f82c77)
![image](https://github.com/user-attachments/assets/d95e0430-76b3-4548-81e5-733bd35f0fec)

---

## Availability (dev leftovers)

From time to time we may have **bare PCBs**, **populated PCBs**, or **complete kits** available from development batches.  
If you’re interested, **please contact us privately** (do **not** post personal details in public issues):

- 📧 **colloid-nougat-42@icloud.com**

Stock is limited and irregular; first-come, first-served.

---

## Project status / roadmap

- ✅ Body-cord testing (auto detect, quick + accurate modes)  
- ✅ Weapon testing (foil/sabre/épée)  
- 🛠️ Lamé continuity test (**in progress**)  

Contributions welcome — see `CONTRIBUTING.md`.

---

## Licensing

- **Software:** Apache-2.0  
- **Hardware designs & docs:** **CC BY-NC-SA 4.0** (remix allowed; attribution + share-alike; **non-commercial**)  
- **Commercial use:** available by agreement — see `COMMERCIAL-LICENSING.md`.  
  - Policy: producing **> 10 units** total (including remixes/derivatives), whether sold or given away, is **commercial**.

**Attribution example:**  
“Based on *WeaponWireTester* by **Piet Wauters & Claude Wolter**, licensed CC BY-NC-SA 4.0.”

---

## Safety note

This tester is **not** for mains/line-voltage use. Use only with fencing equipment and follow your federation’s safety rules.

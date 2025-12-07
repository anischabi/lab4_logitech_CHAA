

# How to Build and Use the Project

## 1. Build Everything

### Mandatory software:

* GCC compiler
* Make
* SDL2 development package
* SDL2_ttf development package
* `sdl2-config` tool (provided by libsdl2-dev)

### Install everything on Ubuntu:

```bash
sudo apt update
sudo apt install build-essential libsdl2-dev libsdl2-ttf-dev
```

### Build
From the project root:

```bash
make
```

This automatically builds:

* the **kernel driver** (`logitech_orbit_driver.ko`)
* the **SDL2 application** (`stream_interface`)

---

## 2. Load / Unload the Kernel Module

From the project root:

### Load the driver

```bash
make load
```

### Unload the driver

```bash
make unload
```

---

## 3. Run the Application

After the driver is loaded:

```bash
./app/bin/stream_interface
```

You will see:

* The **live video stream** on the left
* The **control panel** on the right

---

# Control Panel Usage

## Current Center

The top of the panel shows the current motor coordinates:

```
Current pan:   X
Current tilt:  Y
```

These values update automatically after every movement.

---

## Pan/Tilt Visualizer

A square box showing:

* Crosshairs (0,0 reference position)
* A **green dot** indicating the current position of the camera

---

## Input Boxes

Two text fields allow you to enter a **new desired center**:

```
Pan (X):  [ number ]
Tilt (Y): [ number ]
```

To use:

1. Click inside either box
2. Type the new target coordinate
3. Valid characters: digits `0–9` and optional leading `-`

Valid coordinate ranges:

* **Pan:** −4480 to +4480
* **Tilt:** −1920 to +1920

Values outside this range are automatically clamped.

---

## MOVE Button

Click the **MOVE** button to command the camera.

The system:

* Calculates the required movement
* Sends safe step-by-step commands
* Applies a 1-second delay between steps
* Updates the green dot to show the new position

---

# Keyboard Shortcuts

These shortcuts work anytime:

| Key         | Action                        |
| ----------- | ----------------------------- |
| **Q / ESC** | Quit the application          |
| **SPACE**   | Pause/resume the video stream |
| **I**       | Toggle the information bar    |
| **F**       | Toggle fullscreen mode        |

---

## Cleaning the Project

To remove all compiled files:

```bash
make clean
```


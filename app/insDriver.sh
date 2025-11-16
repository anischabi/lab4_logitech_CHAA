#!/bin/bash

set -e  # le script s'arrête si une commande fail

MODULE="logitech_orbit_driver"
KO_FILE="logitech_orbit_driver.ko"

echo "[*] Removing old module..."
cd "../driver"
sudo rmmod $MODULE 2>/dev/null || echo "Module not loaded."

echo "[*] Cleaning..."
make clean

echo "[*] Building..."
make

echo "[*] Inserting new module..."
sudo insmod $KO_FILE

echo "[*] Removing old code..."
cd "../app"
echo "[*] Building..."
gcc -o test_control test_control.c

echo "[*] Running test_control..."
sudo ./test_control

echo "[*] Done."

savedcmd_logitech_orbit_driver.mod := printf '%s\n'   logitech_orbit_driver.o | awk '!x[$$0]++ { print("./"$$0) }' > logitech_orbit_driver.mod

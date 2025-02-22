
# BLE controlled RGB LED Bicycle light
A RGB (worldsemi ws2812) bicycle stip light compatible with a clone of the [Nice!Nano](https://nicekeyboards.com/nice-nano) board from [Nice Keyboards](https://nicekeyboards.com/).

## Building
build with west build -b nicekeyboards_nicenano -p always --sysbuild -- -DBOARD_ROOT=<absolute proj dir>/zephyr-rgblights

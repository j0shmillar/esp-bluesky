WIP

**Reimplementation** of [**libbluesky**](https://github.com/briandowns/libbluesky), a **C/C++ library** for the BlueSky API, for the **ESP32**

## Setup

Set up [esp-idf](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)

Run ```idf.py menuconfig``` and paste BlueSky/Wifi details under ```components/{BlueSky | Wifi} configuration```

then 

```
idf.py build
idf.py flash
```
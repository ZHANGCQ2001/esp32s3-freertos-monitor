# ESP32-S3 Project Instructions

- Target: ESP32-S3
- ESP-IDF: v5.3.2
- Flash size: 16 MB
- Load ESP-IDF with `get_idf`
- Build with `idf.py build`
- Flash with `idf.py -p /dev/ttyACM0 flash`
- Monitor with `idf.py -p /dev/ttyACM0 monitor`
- Do not modify `~/esp/esp-idf`
- Run `idf.py build` after source changes
- Keep `build/` out of Git

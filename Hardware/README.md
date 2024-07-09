# Latar Belakang
Lab IoT dan KESTL memiliki Energy Meter, yakni Schneider IEM2050 yang mana dapat berkomunikasi dengan protokol Modbus RTU (RS485). Untuk mikrokontroller, pada awalnya menggunakan Teensy 4.0 yang mana sangat powerful. Akan tetapi, karena tidak memiliki built-in modul Wi-Fi maka diperlukan modul tambahan, yaitu Modul Ethernet. Modul ethernet itulah yang kerap menjadi masalah. Ketika tidak berhasil melakukan koneksi ke server / internet, ia kerap menyebabkan program lainnya menjadi terhenti dan eror. Alhasil data energy kerap tidak terkirim secara rutin.

# Solusi
Oleh karena itu, mikrokontroler diganti menjadi ESP32 karena memiliki built-in Wi-Fi module dan dirasa mampu untuk sekadar menjalankan program membaca komunikasi modbus menggunakan MAX485 sebagai konverternya. Setelah 2 percobaan sejauh ini ditulis, tidak ada kendala pengiriman data dari sisi hardware (ESP32). Dibandingkan dengan pendahulunnya (Teensy), ia terkadang tidak mampu mengirim data selama 1 hari secara penuh.

Kendala yang masih dialami ialah terkadang terdapat error dari sisi server hosting website yang tidak dapat menerima terlalu banyak request dalam waktu singkat atau sering juga disebut dengan pesan error HTTP 429.

## Penyesuaian
Interval waktu pengiriman juga berubah. Semula setiap 1,5 menit sekali kini menjadi 5 menit sekali. Hal itu menyebabkan adanya pergantian variable 'delay' dalam database table 'energy_cost' dari 90 -> 300 (dalam sekon).



<div align="left">
Dibuat  dengan ❤️ oleh &nbsp <a href="https://github.com/AlfandiMario/"><img src="https://img.shields.io/badge/AlfandiMario-inactive?style=plastic&logo=Github" />
</div>

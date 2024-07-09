#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "ModbusMaster.h" //menggunakan library ModbusMaster.h
#include "konversi.h"
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include "time.h"

// #define DEBUG_MODE  // Uncomment baris ini jika debugging
#ifdef DEBUG_MODE
String apiEnergy = "https://iotlab-uns.com/smart-bms/public/api/debug-energy";
String uriTotal = "/smart-bms/public/api/debug-energy";
const long interval = 60000; // Interval pengiriman data 1 menit (untuk troubleshooting)
#else
String apiEnergy = "https://iotlab-uns.com/smart-bms/public/api/ApiEnergy";
String uriTotal = "/smart-bms/public/api/total-energy";
const long interval = 300000; // Interval pengiriman data 5 menit (untuk production)
#endif

/* Instance Initiation */
HardwareSerial SerialPort(1);
ModbusMaster node;  // NODE TERHUBUNG PADA SLAVE 3 SERIAL 4
ModbusMaster node2; // NODE TERHUBUNG PADA SLAVE 3 SERIAL 4
WiFiClient client;
HTTPClient http;

/* Modbus Pin Definition */
// kWh Meter 1 untuk AC Saja
#define PIN_DE 4                // Pin untuk Data Enable (DE)
#define MODBUS_RX_PIN 0         // Pin Rx untuk Modbus (alt pin : 25)
#define MODBUS_TX_PIN 15        /// Pin Tx untuk Modbus (alt pin : 19)
#define MODBUS_SERIAL_BAUD 9600 // Kecepatan baud rate untuk koneksi serial Modbus

// kWh Meter 2 untuk Total
#define PIN_DE2 2                // Pin untuk Data Enable (DE) (alt pin : 18)
#define MODBUS2_RX_PIN 16        // Pin Rx untuk Modbus
#define MODBUS2_TX_PIN 17        /// Pin Tx untuk Modbu
#define MODBUS2_SERIAL_BAUD 9600 // Kecepatan baud rate untuk koneksi serial Modbus

/* Global Variables */
unsigned long previousMillis = 0;
float volt, volt2, current, current2, freq, freq2, activepower, activepower2;
float apparentpower, apparentpower2, reactivepower, reactivepower2, totalactive, totalactive2;
String response;                      // String untuk respons server
const char *ssid = "IoT";             // Nama jaringan WiFi
const char *password = "agusramelan"; // Kata sandi WiFi
// Untuk mendapat waktu real time dari internet
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200; // GMT +7 -> 25200 seconds
const int daylightOffset_sec = 3600;
const char restartTimes[][9] = {"11:30:00", "23:30:00"};
char currentTime[9];
const char *host = "iotlab-uns.com";
const int port = 443;

// Fungsi untuk memulai transmisi data
void startTrans()
{
  digitalWrite(PIN_DE, HIGH);
  digitalWrite(PIN_DE2, HIGH);
}
// Fungsi untuk mengakhiri transmisi data
void endTrans()
{
  digitalWrite(PIN_DE, LOW);
  digitalWrite(PIN_DE2, LOW);
}

void setup()
{
  delay(3000);                // Menunggu 5 detik agar perangkat lain dapat melakukan booting terlebih dahulu
  pinMode(PIN_DE, OUTPUT);    // Mengatur mode pin DE sebagai output
  pinMode(PIN_DE2, OUTPUT);   // Mengatur mode pin DE2 sebagai output
  digitalWrite(PIN_DE, LOW);  // Mematikan sinyal pada pin DE
  digitalWrite(PIN_DE2, LOW); // Mematikan sinyal2 pada pin DE

  Serial.begin(115200); // Mengaktifkan komunikasi serial dengan baud rate 115200

  WiFi.begin(ssid, password); // Memulai proses koneksi WiFi

  while (WiFi.status() != WL_CONNECTED)
  { // Memeriksa status koneksi WiFi
    Serial.println("Connecting to WiFi..");
    delay(1000);
  }
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // SerialPort.begin(MODBUS_SERIAL_BAUD, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN); // none parity
  SerialPort.begin(MODBUS_SERIAL_BAUD, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  Serial2.begin(MODBUS2_SERIAL_BAUD, SERIAL_8N1, MODBUS2_RX_PIN, MODBUS2_TX_PIN); // Mengaktifkan komunikasi serial Modbus dengan konfigurasi yang ditentukan
  // Mengatur timeout untuk komunikasi serial Modbus
  // Serial2.setTimeout(200);                                                        // Mengatur timeout untuk komunikasi serial Modbus

  Serial.println("Begin Transmission :");
  node.begin(4, SerialPort);         // Memulai komunikasi Modbus dengan alamat slave 4 melalui HardwareSerial
  node2.begin(2, Serial2);           // Memulai komunikasi Modbus dengan alamat slave 2 melalui Serial2
  node.preTransmission(startTrans);  // Mengatur tindakan sebelum transmisi Modbus dimulai
  node2.preTransmission(startTrans); // Mengatur tindakan sebelum transmisi Modbus dimulai
  node.postTransmission(endTrans);   // Mengatur tindakan setelah transmisi Modbus selesai
  node2.postTransmission(endTrans);  // Mengatur tindakan setelah transmisi Modbus selesai
}

void loop()
{
  unsigned long currentMillis = millis();
  wificek();
  delay(25);
  frekuensi();
  frekuensi2();
  delay(25);
  voltage();
  voltage2();
  delay(25);
  ampere();
  ampere2();
  delay(25);
  activePower();
  activePower2();
  delay(25);
  reactivePower();
  reactivePower2();
  delay(25);
  apparentPower();
  apparentPower2();
  delay(25);
  totalActive();
  totalActive2();
  delay(25);
  Serial.println("================================================================");
  Serial.print("Volt 1 = ");
  Serial.print(volt);
  Serial.print("\t Volt 2 = ");
  Serial.println(volt2);
  Serial.print("Amp 1 = ");
  Serial.print(current);
  Serial.print("\t Amp 2 = ");
  Serial.println(current2);
  Serial.print("Watt 1 = ");
  Serial.print(activepower);
  Serial.print("\t Watt 2 = ");
  Serial.println(activepower2);
  Serial.print("Total Watt 1 = ");
  Serial.print(totalactive);
  Serial.print("\t Total Watt 2 = ");
  Serial.println(totalactive2);

  // Kirim data ke server pada interval tertentu
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis; // Save the current time for the next interval
    kirim(1);                       // Panggil fungsi untuk mengirim data ke server HTTP
    delay(8000);
    kirimTotalActive(1);
    delay(8000);
    kirim(2); // Kirim data kwh ke-2
    delay(1000);
  }
  Serial.print("Timer millis: ");
  Serial.println((currentMillis - previousMillis) / 1000);

  // Restart ESP -> mencegah 429
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(currentTime, 9, "%H:%M:%S", &timeinfo);
  Serial.print("Current Time : ");
  Serial.println(currentTime);
  // Cek waktu untuk restart ESP
  for (int i = 0; i < sizeof(restartTimes) / sizeof(restartTimes[0]); i++)
  {
    if (strcmp(currentTime, restartTimes[i]) == 0)
    {
      for (int j = 0; j < 5; j++)
      {
        Serial.print("Waktunya merestart esp");
        ESP.restart();
      }
      break; // Exit the loop if a match is found
    }
  }
}

void kirim(int id_device)
{
  String dataout;

  delay(100);
  Serial.print("API Server : ");
  Serial.println(apiEnergy);
  // Menentukan URL tujuan untuk permintaan HTTP
  http.begin(apiEnergy); 

  // Menambahkan header Content-Type dengan tipe data yang akan dikirim
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  if (id_device == 1)
  {
    dataout = "id_kwh=1&frekuensi=" + String(freq) + "&arus=" + String(current) + "&tegangan=" + String(volt) + "&active_power=" + String(activepower) + "&reactive_power=" + String(reactivepower) + "&apparent_power=" + String(apparentpower);
  }
  else if (id_device == 2)
  {
    dataout = "id_kwh=2&frekuensi=" + String(freq2) + "&arus=" + String(current2) + "&tegangan=" + String(volt2) + "&active_power=" + String(activepower2) + "&reactive_power=" + String(reactivepower2) + "&apparent_power=" + String(apparentpower2);
  },

  Serial.println(dataout); // Menampilkan data yang akan dikirim dalam bentuk teks

  // Melakukan permintaan HTTP POST dengan data yang sudah disiapkan
  delay(100);
  int httpResponseCode = http.POST(dataout);
  if (httpResponseCode > 0)
  {
    response = http.getString();      // Mendapatkan respon dari permintaan HTTP
    Serial.println(httpResponseCode); // Menampilkan jawaban dari permintaan HTTP
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }
,
  http.end(); // Menutup koneksi HTTP dan membebaskan sumber daya yang digunakan
}

void kirimTotalActive(int id_device)
{
  String dataout;

  delay(100);
  Serial.print("API Server : ");
  Serial.println(uriTotal);

  // Menentukan URL tujuan untuk permintaan HTTP
  // http.begin(apiTotal);
  http.begin(host, port, uriTotal);
  // http.begin("iotlab-uns.com", 443, "/smart-bms/public/api/debug-energy");

  // Menambahkan header Content-Type dengan tipe data yang akan dikirim
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  if (id_device == 1)
  {
    dataout = "id_kwh=1&total_energy=" + String(totalactive);
  }
  else if (id_device == 2)
  {
    dataout = "id_kwh=2&total_energy=" + String(totalactive2);
  }

  Serial.println(dataout); // Menampilkan data yang akan dikirim dalam bentuk teks

  delay(100);
  // Melakukan permintaan HTTP POST dengan data yang sudah disiapkan
  int httpResponseCode = http.POST(dataout);
  if (httpResponseCode > 0)
  {

    response = http.getString();      // Mendapatkan respon dari permintaan HTTP
    Serial.println(httpResponseCode); // Menampilkan jawaban dari permintaan HTTP
  }
  else
  {

    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end(); // Menutup koneksi HTTP dan membebaskan sumber daya yang digunakan
}

void frekuensi()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0C26 dengan ukuran data 2
  result = node.readHoldingRegisters(0x0C26, 2); // memanggil fungsi read pada register (alamat, besar data)

  // Memeriksa apakah bacaan Modbus berhasil
  if (result == node.ku8MBSuccess)
  {
    data[0] = node.getResponseBuffer(0); // Membaca data pertama dari buffer respons Modbus
    data[1] = node.getResponseBuffer(1); // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];       // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];       // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    freq = (konversi.dataFloat);         // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'freq'
  }
}

void frekuensi2()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0C26 dengan ukuran data 2
  result = node2.readHoldingRegisters(0x0C26, 2); // memanggil fungsi read pada register (alamat, besar data)

  // Memeriksa apakah bacaan Modbus berhasil
  if (result == node2.ku8MBSuccess)
  {
    data[0] = node2.getResponseBuffer(0); // Membaca data pertama dari buffer respons Modbus
    data[1] = node2.getResponseBuffer(1); // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];        // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];        // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    freq2 = (konversi.dataFloat);         // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'freq2'
  }
  else
  {
    freq2 = 0; // Jika bacaan Modbus tidak berhasil, mengatur nilai frekuensi menjadi 0
  }
}

void voltage()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0BD4 dengan ukuran data 2
  result = node.readHoldingRegisters(0x0BD4, 2); // memanggil fungsi read pada register (alamat, besar data)
  if (result == node.ku8MBSuccess)
  {
    data[0] = node.getResponseBuffer(0); // Membaca data pertama dari buffer respons Modbus
    data[1] = node.getResponseBuffer(1); // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];       // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];       // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    volt = (konversi.dataFloat);         // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'volt'
  }
}

void voltage2()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0BD4 dengan ukuran data 2
  result = node2.readHoldingRegisters(0x0BD4, 2); // memanggil fungsi read pada register (alamat, besar data)
  if (result == node2.ku8MBSuccess)
  {
    data[0] = node2.getResponseBuffer(0); // Membaca data pertama dari buffer respons Modbus
    data[1] = node2.getResponseBuffer(1); // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];        // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];        // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    volt2 = (konversi.dataFloat);         // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'volt2'
  }
  else
  {
    volt2 = 0;
  }
}

void ampere()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0BB8 dengan ukuran data 2
  result = node.readHoldingRegisters(0x0BB8, 2); // Memanggil fungsi read pada register (alamat, besar data)
  if (result == node.ku8MBSuccess)
  {
    data[0] = node.getResponseBuffer(0); // Membaca data pertama dari buffer respons Modbus
    data[1] = node.getResponseBuffer(1); // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];       // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];       // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    current = (konversi.dataFloat);      // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'current'
  }
}

void ampere2()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0BB8 dengan ukuran data 2
  result = node2.readHoldingRegisters(0x0BB8, 2); // Memanggil fungsi read pada register (alamat, besar data)
  if (result == node2.ku8MBSuccess)
  {
    data[0] = node2.getResponseBuffer(0); // Membaca data pertama dari buffer respons Modbus
    data[1] = node2.getResponseBuffer(1); // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];        // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];        // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    current2 = (konversi.dataFloat);      // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'current2'
  }
}

void activePower()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0BEE dengan ukuran data 2
  result = node.readHoldingRegisters(0x0BEE, 2); // Memanggil fungsi read pada register (alamat, besar data)
  if (result == node.ku8MBSuccess)
  {
    data[0] = node.getResponseBuffer(0); // Membaca data pertama dari buffer respons Modbus
    data[1] = node.getResponseBuffer(1); // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];       // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];       // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    activepower = (konversi.dataFloat);  // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'activepower'
  }
}

void activePower2()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0BEE dengan ukuran data 2
  result = node2.readHoldingRegisters(0x0BEE, 2); // Memanggil fungsi read pada register (alamat, besar data)
  if (result == node2.ku8MBSuccess)
  {
    data[0] = node2.getResponseBuffer(0); // Membaca data pertama dari buffer respons Modbus
    data[1] = node2.getResponseBuffer(1); // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];        // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];        // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    activepower2 = (konversi.dataFloat);  // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'activepower'
  }
  else
  {
    activepower2 = 0;
  }
}

void reactivePower()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0BFC dengan ukuran data 2
  result = node.readHoldingRegisters(0x0BFC, 2); // Memanggil fungsi read pada register (alamat, besar data)
  if (result == node.ku8MBSuccess)
  {
    data[0] = node.getResponseBuffer(0);  // Membaca data pertama dari buffer respons Modbus
    data[1] = node.getResponseBuffer(1);  // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];        // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];        // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    reactivepower = (konversi.dataFloat); // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'var'
  }
}

void reactivePower2()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0BFC dengan ukuran data 2
  result = node2.readHoldingRegisters(0x0BFC, 2); // Memanggil fungsi read pada register (alamat, besar data)
  if (result == node2.ku8MBSuccess)
  {
    data[0] = node2.getResponseBuffer(0);  // Membaca data pertama dari buffer respons Modbus
    data[1] = node2.getResponseBuffer(1);  // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];         // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];         // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    reactivepower2 = (konversi.dataFloat); // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'var'
  }
  else
  {
    reactivepower2 = 0;
  }
}

void apparentPower()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0C04 dengan ukuran data 2
  result = node.readHoldingRegisters(0x0C04, 2); // Memanggil fungsi read pada register (alamat, besar data)
  if (result == node.ku8MBSuccess)
  {
    data[0] = node.getResponseBuffer(0);  // Membaca data pertama dari buffer respons Modbus
    data[1] = node.getResponseBuffer(1);  // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];        // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];        // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    apparentpower = (konversi.dataFloat); // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'va'
  }
}

void apparentPower2()
{
  uint8_t result;
  uint16_t data[10];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0x0C04 dengan ukuran data 2
  result = node2.readHoldingRegisters(0x0C04, 2); // Memanggil fungsi read pada register (alamat, besar data)
  if (result == node2.ku8MBSuccess)
  {
    data[0] = node2.getResponseBuffer(0);  // Membaca data pertama dari buffer respons Modbus
    data[1] = node2.getResponseBuffer(1);  // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];         // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];         // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    apparentpower2 = (konversi.dataFloat); // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'va'
  }
  else
  {
    apparentpower2 = 0;
  }
}

void totalActive()
{
  uint8_t result;
  uint16_t data[2];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0xB038 dengan ukuran data 2
  result = node.readHoldingRegisters(0xB038, 2); // Memanggil fungsi read pada register (alamat, besar data)
  if (result == node.ku8MBSuccess)
  {
    data[0] = node.getResponseBuffer(0); // Membaca data pertama dari buffer respons Modbus
    data[1] = node.getResponseBuffer(1); // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];       // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];       // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    totalactive = (konversi.dataFloat);  // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'totalactive'
  }
}

void totalActive2()
{
  uint8_t result;
  uint16_t data[2];

  // Memanggil fungsi Modbus untuk membaca data frekuensi dari holding register pada alamat 0xB038 dengan ukuran data 2
  result = node2.readHoldingRegisters(0xB038, 2); // Memanggil fungsi read pada register (alamat, besar data)
  if (result == node2.ku8MBSuccess)
  {
    data[0] = node2.getResponseBuffer(0); // Membaca data pertama dari buffer respons Modbus
    data[1] = node2.getResponseBuffer(1); // Membaca data kedua dari buffer respons Modbus
    konversi.dataInt[0] = data[1];        // Menyimpan data kedua ke dalam bagian dataInt struktur konversi
    konversi.dataInt[1] = data[0];        // Menyimpan data pertama ke dalam bagian dataInt struktur konversi
    totalactive2 = (konversi.dataFloat);  // Mengonversi data dari tipe union dataInt menjadi tipe float dan menyimpannya dalam variabel 'totalactive2'
  }
}

// Fungsi untuk memeriksa koneksi WiFi dan menghubungkan ulang jika terputus
void wificek()
{
  // Periksa status koneksi WiFi
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi connection lost. Reconnecting...");

    // Mulai ulang koneksi WiFi
    WiFi.begin(ssid, password);

    // Tunggu hingga koneksi ulang berhasil
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.println("Connecting to WiFi...");
    }

    Serial.println("Reconnected to WiFi");
  }
}

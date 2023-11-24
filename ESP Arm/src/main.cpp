#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>

#define ARM 1
// #define LEG 1

#ifdef ARM
#define my_readings arm_readings
#define other_readings leg_readings
#define MAC LegMAC
#else
#define my_readings leg_readings
#define other_readings arm_readings
#define MAC ArmMAC
#define ARM 0
#endif

Adafruit_MPU6050 mpu;

const uint8_t ArmMAC[] = {0xEC, 0x62, 0x60, 0x93, 0xB3, 0xE0};
const uint8_t LegMAC[] = {0x78, 0x21, 0x84, 0xE5, 0xE8, 0x24};

typedef struct imu_message
{
  double ax;
  double ay;
  double az;
  double gx;
  double gy;
  double gz;
} imu_message;

imu_message arm_readings;
imu_message leg_readings;

esp_now_peer_info_t peerInfo;

void setup_imu();
void decay();
void blink();
void print_readings(String string, imu_message readings);

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  // Serial.print("\r\nLast Packet Send Status:\t");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&other_readings, incomingData, sizeof(other_readings));
}

void setup()
{
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);

  setup_imu();

  WiFi.mode(WIFI_MODE_STA);
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  // Registering peer
  memcpy(peerInfo.peer_addr, MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  Serial.print("Adding Peer at MAC ");
  for (int i = 0; i < 6; i++)
  {
    Serial.print(peerInfo.peer_addr[i], HEX);
    Serial.print(":");
  }
  Serial.println();
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println(WiFi.macAddress());
  Serial.print("Peer status: ");
  Serial.println(esp_now_is_peer_exist(MAC) ? "Found" : "Missing");
  blink();
  decay();
}
int count = 0;
double my_a_map, my_g_map, other_a_map, other_g_map;
const double SCALE_A = 10.0;
const double SCALE_G = 5.0;
void loop()
{
  float ax, ay, az, gx, gy, gz;
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  my_readings.ax = a.acceleration.x;
  my_readings.ay = a.acceleration.y;
  my_readings.az = a.acceleration.z;
  my_readings.gx = g.gyro.x;
  my_readings.gy = g.gyro.y;
  my_readings.gz = g.gyro.z;

  esp_now_send(MAC, (uint8_t *)&my_readings, sizeof(my_readings));

  double my_aa = max(0.0, (sqrt(my_readings.ax * my_readings.ax + my_readings.ay * my_readings.ay + my_readings.az * my_readings.az) - 8)) / SCALE_A;
  double my_gg = sqrt(my_readings.gx * my_readings.gx + my_readings.gy * my_readings.gy + my_readings.gz * my_readings.gz) / SCALE_G;
  double other_aa = max(0.0, (sqrt(other_readings.ax * other_readings.ax + other_readings.ay * other_readings.ay + other_readings.az * other_readings.az) - 8)) / SCALE_A;
  double other_gg = sqrt(other_readings.gx * other_readings.gx + other_readings.gy * other_readings.gy + other_readings.gz * other_readings.gz) / SCALE_G;

  my_a_map = max(0.0, min(1.5, my_aa));
  my_g_map = max(0.0, min(1.0, my_gg));
  other_a_map = max(0.0, min(1.5, other_aa));
  other_g_map = max(0.0, min(1.0, other_gg));
  if (count % 30 == 0)
  {
    print_readings("My\t", my_readings);
    print_readings("Other\t", other_readings);
    Serial.print(my_aa);
    Serial.print("\t");
    Serial.print(my_gg);
    Serial.print("\t");
    Serial.print(other_aa);
    Serial.print("\t");
    Serial.print(other_gg);
    Serial.println("\t");
    Serial.println(String(my_a_map + my_g_map) + "-" + String(other_a_map + other_g_map));
  }

  if (ARM)
  {
    Serial2.println(String(my_a_map + my_g_map) + "-" + String(other_a_map + other_g_map) + "a");
    Serial1.println(String(my_a_map + my_g_map) + "-" + String(other_a_map + other_g_map));
  }
  else
  {
    Serial2.println(String(other_a_map + other_g_map) + "-" + String(my_a_map + my_g_map) + "a");
    Serial1.println(String(other_a_map + other_g_map) + "-" + String(my_a_map + my_g_map));
  }
  count += 1;
}

void setup_imu()
{
  Serial.begin(115200);
  Serial2.begin(115200);

  Serial.println("Adafruit MPU6050 test!");
  if (!mpu.begin())
  {
    Serial.println("Failed to find MPU6050 chip");
    while (1)
    {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  // setupt motion detection
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true); // Keep it latched.  Will turn off when reinitialized.
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);
}

void blink()
{
  for (int i = 0; i < 20; i++)
  {
    digitalWrite(BUILTIN_LED, HIGH);
    delay(50);
    digitalWrite(BUILTIN_LED, LOW);
    delay(50);
  }
}
void decay()
{
  for (int i = 255; i >= 0; i--)
  {
    analogWrite(BUILTIN_LED, i);
    delay(10);
  }
}

void print_readings(String string, imu_message readings)
{
  Serial.print(string);
  Serial.print(" ");
  Serial.print("a: ");
  Serial.print(readings.ax);
  Serial.print(" ");
  Serial.print(readings.ay);
  Serial.print(" ");
  Serial.print(readings.az);
  Serial.print(" ");
  Serial.print("g: ");
  Serial.print(readings.gx);
  Serial.print(" ");
  Serial.print(readings.gy);
  Serial.print(" ");
  Serial.print(readings.gz);
  Serial.println("");
}

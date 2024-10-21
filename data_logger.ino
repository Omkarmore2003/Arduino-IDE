#include <SD.h>
#include <SPI.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

#define VOLTAGE_PIN A0  // Analog pin for voltage
#define CURRENT_PIN A1  // Analog pin for current
#define PWM_PIN 2       // Digital pin for PWM input

#define SD_CS_PIN BUILTIN_SDCARD // Use the built-in SD card slot
#define SAMPLE_INTERVAL 1 // Sampling interval in milliseconds
#define BUFFER_SIZE 1000 // Number of samples to buffer before writing to SD

struct Data {
  time_t timestamp;
  float voltage;
  float current;
  int pwm;
};

Data dataBuffer[BUFFER_SIZE];
int head = 0;
int tail = 0;
bool bufferFull = false;
unsigned long lastSampleTime = 0;

// IST offset in seconds (5 hours 30 minutes)
const long IST_OFFSET = 19800;

// Mock sensor variables
float mockVoltage = 0.0;
float mockCurrent = 0.0;
int mockPwm = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) ; // Wait for Serial Monitor to open

  pinMode(VOLTAGE_PIN, INPUT);
  pinMode(CURRENT_PIN, INPUT);
  pinMode(PWM_PIN, INPUT);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  // Remove existing file
  SD.remove("/data.csv");

  // Initialize the RTC
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }
  
  Serial.println("Enter the current time as Unix timestamp (seconds since Jan 1 1970): ");
  while (Serial.available() == 0) ; // Wait for input
  time_t timestamp = Serial.parseInt();
  setTime(timestamp);
  RTC.set(now());
  Serial.println("Time set.");
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastSampleTime >= SAMPLE_INTERVAL) {
    captureData();
    lastSampleTime = currentTime;
  }

  if (bufferFull || (head != tail)) {
    writeDataToSD();
  }
}

void captureData() {
  dataBuffer[head].timestamp = now(); // Get the current time

  // Mock sensor data
  mockVoltage += 0.1;
  if (mockVoltage > 5.0) mockVoltage = 0.0;

  mockCurrent += 0.05;
  if (mockCurrent > 3.0) mockCurrent = 0.0;

  mockPwm += 50;
  if (mockPwm > 1023) mockPwm = 0;

  dataBuffer[head].voltage = mockVoltage;
  dataBuffer[head].current = mockCurrent;
  dataBuffer[head].pwm = mockPwm;

  Serial.print("Captured Data - Time: ");
  printTime(dataBuffer[head].timestamp);
  Serial.print(", Voltage: ");
  Serial.print(dataBuffer[head].voltage);
  Serial.print(" V, Current: ");
  Serial.print(dataBuffer[head].current);
  Serial.print(" A, PWM: ");
  Serial.print(dataBuffer[head].pwm);
  Serial.println(" us");

  head = (head + 1) % BUFFER_SIZE;

  if (head == tail) {
    bufferFull = true; // The buffer is full
  }
}

void writeDataToSD() {
  unsigned long writeStartTime = millis(); // Record the start time of the write operation

  File file = SD.open("/data.csv", FILE_WRITE);
  if (file) {
    while (head != tail || bufferFull) {
      // Adjust time for IST
      time_t localTime = dataBuffer[tail].timestamp + IST_OFFSET;
      
      char timeString[20];
      sprintf(timeString, "%04d-%02d-%02d %02d:%02d:%02d", 
              year(localTime), 
              month(localTime), 
              day(localTime), 
              hour(localTime), 
              minute(localTime), 
              second(localTime));

      file.print(timeString);
      file.print(",");
      file.print(dataBuffer[tail].voltage, 3); // Print voltage with 3 decimal places
      file.print(",");
      file.print(dataBuffer[tail].current, 3); // Print current with 3 decimal places
      file.print(",");
      file.println(dataBuffer[tail].pwm);

      Serial.print("Logged Data - Time: ");
      Serial.print(timeString);
      Serial.print(", Voltage: ");
      Serial.print(dataBuffer[tail].voltage);
      Serial.print(" V, Current: ");
      Serial.print(dataBuffer[tail].current);
      Serial.print(" A, PWM: ");
      Serial.print(dataBuffer[tail].pwm);
      Serial.println(" us");

      tail = (tail + 1) % BUFFER_SIZE;
      bufferFull = false; // Once we have written some data, we can reset the bufferFull flag
    }
    file.close();

    unsigned long writeEndTime = millis(); // Record the end time of the write operation
    unsigned long writeDuration = writeEndTime - writeStartTime; // Calculate the duration
    Serial.print("Time taken to write to SD card: ");
    Serial.print(writeDuration);
    Serial.println(" milliseconds");

  } else {
    Serial.println("Failed to open file for writing.");
  }
}

void printTime(time_t t) {
  time_t localTime = t + IST_OFFSET; // Adjust time for IST
  Serial.print(year(localTime));
  Serial.print("-");
  Serial.print(month(localTime));
  Serial.print("-");
  Serial.print(day(localTime));
  Serial.print(" ");
  Serial.print(hour(localTime));
  Serial.print(":");
  Serial.print(minute(localTime));
  Serial.print(":");
  Serial.print(second(localTime));
}

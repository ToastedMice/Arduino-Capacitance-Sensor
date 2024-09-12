unsigned int sampleSize = 200;
unsigned long workingAverageLeft = 0;
unsigned long workingAverageRight = 0;
unsigned int samplesTaken = 0;

float smoothingFactor = 0.1; // smoothing factor for EMA
float smoothedLeft = 0;
float smoothedRight = 0;

// Variables for calibration
unsigned int calibrationSamples = 500;
unsigned int calibrationCount = 0;
int baselineLeft = 0;
int baselineRight = 0;
int calibratedMinLeft = 50, calibratedMaxLeft = 0;
int calibratedMinRight = 50, calibratedMaxRight = 0;
bool calibrated = false;

int light = 7;
bool lightState = false; // Tracks the state of the light (on/off)
unsigned int threshold = 600; // Threshold for hand detection
bool handDetected = false; // Tracks whether a hand is currently detected (above threshold)

int thresholdCycles = 0;

void setup() {
  Serial.begin(19200);
  // Discharge sensors
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);

  pinMode(A5, OUTPUT);
  digitalWrite(A5, LOW);

  pinMode(light, OUTPUT);
  digitalWrite(light, LOW); // Ensure the light is initially off

  delay(1000);
}

void loop() {
  // Discharge left sensor (A0)
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);

  // Read from sensors
  int inputRight = readCapacitiveSensor(A5);
  int inputLeft = readCapacitiveSensor(A0);
  
  // Apply EMA smoothing
  smoothedLeft = (smoothingFactor * inputLeft) + ((1 - smoothingFactor) * smoothedLeft);
  smoothedRight = (smoothingFactor * inputRight) + ((1 - smoothingFactor) * smoothedRight);

  // Calibration phase
  if (!calibrated) {
    // Update min and max values for calibration
    if (smoothedLeft < calibratedMinLeft) calibratedMinLeft = smoothedLeft;
    if (smoothedLeft > calibratedMaxLeft) calibratedMaxLeft = smoothedLeft;
    if (smoothedRight < calibratedMinRight) calibratedMinRight = smoothedRight;
    if (smoothedRight > calibratedMaxRight) calibratedMaxRight = smoothedRight;

    calibrationCount++;
    
    if (calibrationCount >= calibrationSamples) {
      // Calculate average readings
      baselineLeft = (calibratedMinLeft + calibratedMaxLeft) / 2;
      baselineRight = (calibratedMinRight + calibratedMaxRight) / 2;
      
      calibrated = true;
      Serial.println("Calibration complete!");
      Serial.print("Left Sensor Range: ");
      Serial.print(baselineLeft);
      Serial.print(" - ");
      Serial.print("Right Sensor Range: ");
      Serial.print(baselineRight);
      Serial.print(" - ");
    }
  } else {
    // Map values based on the calibrated range
    int mappedLeft = map(smoothedLeft, calibratedMinLeft, calibratedMaxLeft, 3000, 0);
    int mappedRight = map(smoothedRight, calibratedMinRight, calibratedMaxRight, 3000, 0);

    

    workingAverageLeft += mappedLeft;
    workingAverageRight += mappedRight;
    samplesTaken++;

    // If we have taken enough samples, calculate the average and print it to Serial out.
    if (samplesTaken == sampleSize) {
      unsigned int averageLeft = workingAverageLeft / sampleSize;
      unsigned int averageRight = workingAverageRight / sampleSize;

      Serial.print("Average Left: ");
      Serial.print(averageLeft);
      Serial.print(", Average Right: ");
      Serial.println(averageRight);
      if (thresholdCycles < 1) {
        threshold = averageLeft + 80;
      }
      thresholdCycles = 1;

      samplesTaken = 0;
      workingAverageLeft = 0;
      workingAverageRight = 0;

      // Hand detection logic
      if (averageLeft > threshold) {
        if (!handDetected) {
          // Hand detected and it wasn't detected before
          lightState = !lightState;
          digitalWrite(light, lightState ? HIGH : LOW); // Toggle light on or off
          handDetected = true; // Mark that the hand is now detected
          delay(100);
        }
        
      } else {
        // Hand is no longer detected
        handDetected = false;
      }
    }
  }

  // Delay 10 microseconds
  delayMicroseconds(10);
}

int readCapacitiveSensor(int sensorPin) {
  // Discharge
  pinMode(sensorPin, OUTPUT);
  digitalWrite(sensorPin, LOW);
  // Charge
  digitalWrite(sensorPin, HIGH);
  // Read
  pinMode(sensorPin, INPUT);
  int input = analogRead(sensorPin);

  // Discharge again to avoid crosstalk
  pinMode(sensorPin, OUTPUT);
  digitalWrite(sensorPin, LOW);
  
  return input;
}

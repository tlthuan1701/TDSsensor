#define TdsSensorPin A0            // Chân ADC của ESP32 (thay A0 bằng 34 hoặc chân ADC phù hợp)
#define VREF 3.3                  // Điện áp tham chiếu analog (Volt) của ADC
#define SCOUNT 30                 // Số lượng điểm lấy mẫu
#define READ_INTERVAL 40          // Thời gian giữa các lần đọc mẫu (ms)
#define PRINT_INTERVAL 800        // Thời gian giữa các lần in kết quả (ms)

int analogBuffer[SCOUNT];         // Mảng lưu trữ giá trị analog đọc từ ADC
int analogBufferIndex = 0;        // Chỉ số hiện tại trong mảng analogBuffer
float temperature = 25;           // Nhiệt độ hiện tại để bù trừ (mặc định 25°C)

// Thuật toán lọc trung vị
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  // Sao chép mảng để không làm thay đổi mảng gốc
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  
  int i, j, bTemp;
  // Sắp xếp mảng theo thuật toán sắp xếp nổi bọt
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  
  // Trả về giá trị trung vị
  if ((iFilterLen & 1) > 0) {  // Nếu số phần tử là lẻ
    bTemp = bTab[(iFilterLen - 1) / 2];
  } else {                     // Nếu số phần tử là chẵn
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

// Đọc dữ liệu từ cảm biến TDS
void readTdsSensor() {
  static unsigned long analogSampleTimepoint = millis();
  
  if (millis() - analogSampleTimepoint > READ_INTERVAL) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }
}

// Tính toán giá trị TDS từ dữ liệu đã thu thập
float calculateTdsValue() {
  int analogBufferTemp[SCOUNT];
  
  // Sao chép mảng để xử lý
  for (int i = 0; i < SCOUNT; i++) {
    analogBufferTemp[i] = analogBuffer[i];
  }
  
  // Đọc giá trị analog ổn định hơn bằng thuật toán lọc trung vị
  float averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0;
  
  // Công thức bù nhiệt
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = averageVoltage / compensationCoefficient;
  
  // Chuyển đổi giá trị điện áp sang giá trị TDS
  return (133.42 * pow(compensationVoltage, 3) - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;
}

// In kết quả TDS ra Serial
void printTdsValue() {
  static unsigned long printTimepoint = millis();
  
  if (millis() - printTimepoint > PRINT_INTERVAL) {
    printTimepoint = millis();
    
    float tdsValue = calculateTdsValue();
    
    Serial.print("TDS Value: ");
    Serial.print(tdsValue, 0);  // In giá trị TDS không có số thập phân
    Serial.println(" ppm");
  }
}

// Cài đặt nhiệt độ để bù trừ
void setTemperature(float temp) {
  temperature = temp;
}

void setup() {
  Serial.begin(115200);            // Khởi tạo giao tiếp Serial với tốc độ 115200 baud
  pinMode(TdsSensorPin, INPUT);    // Cấu hình chân TdsSensorPin là đầu vào
  
  Serial.println("ESP32 TDS Sensor Initialized");
}

void loop() {
  readTdsSensor();     // Đọc dữ liệu từ cảm biến
  printTdsValue();     // In giá trị TDS
  
  // Nếu muốn cập nhật nhiệt độ từ cảm biến nhiệt độ
  // Ví dụ: setTemperature(readTemperatureSensor());
}
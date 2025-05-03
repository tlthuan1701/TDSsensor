#define TdsSensorPin A0           // Định nghĩa chân A0 làm chân đọc cảm biến TDS
#define VREF 3.3                  // Điện áp tham chiếu analog (Volt) của ADC
#define SCOUNT 30                 // Số lượng điểm lấy mẫu

int analogBuffer[SCOUNT];         // Mảng lưu trữ giá trị analog đọc từ ADC
int analogBufferTemp[SCOUNT];     // Mảng tạm để xử lý giá trị analog
int analogBufferIndex = 0;        // Chỉ số hiện tại trong mảng analogBuffer
int copyIndex = 0;                // Biến đếm khi sao chép mảng

float averageVoltage = 0;         // Điện áp trung bình
float tdsValue = 0;               // Giá trị TDS
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

void setup() {
  Serial.begin(115200);            // Khởi tạo giao tiếp Serial với tốc độ 115200 baud
  pinMode(TdsSensorPin, INPUT);    // Cấu hình chân TdsSensorPin là đầu vào
}

void loop() {
  static unsigned long analogSampleTimepoint = millis();
  // Đọc giá trị analog mỗi 40ms
  if (millis() - analogSampleTimepoint > 40U) {     
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    // Đọc giá trị analog và lưu vào buffer
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) { 
      analogBufferIndex = 0;    // Reset chỉ số khi đã lấy đủ mẫu
    }
  }   
  
  static unsigned long printTimepoint = millis();
  // In giá trị TDS ra Serial mỗi 800ms
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      
      // Đọc giá trị analog ổn định hơn bằng thuật toán lọc trung vị, và chuyển đổi sang giá trị điện áp
      averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0;
      
      // Công thức bù nhiệt: fFinalResult(25^C) = fFinalResult(hiện tại)/(1.0+0.02*(nhiệt độ-25.0))
      float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
      // Bù nhiệt độ
      float compensationVoltage = averageVoltage / compensationCoefficient;
      
      // Chuyển đổi giá trị điện áp sang giá trị TDS
      tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;
      
      // In ra Serial
    
      Serial.print("TDS Value:");
      Serial.print(tdsValue, 0);    // In giá trị TDS không có số thập phân
      Serial.println("ppm");        // ppm là đơn vị đo TDS (phần triệu)
    }
  }
}
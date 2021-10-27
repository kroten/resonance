#include <Wire.h>
#include <SPI.h>
#include <Adafruit_MCP4725.h>

#define bFreq_reg0 0x4000

//ускорение
#define FASTADC 1
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
//ускорение

Adafruit_MCP4725 MCP4725;

static bool exit_now = false; //переменная для досрочного прекращение выполнения основной функции тестирования. Если в процессе выполнения поступает
                              //команда прервать выполнение ('^f;'), значение меняется на единицу и происходит выход (в любой момент в пределах 3*300ms)
float Freq; 
//Вероятно изменяемые для прошивки параметры
byte AnalogIn = A7; // CD4051 pin 3 (Common in/out)
float volt_ref = 5.087; //опорное напряжение для ЦАП
int mcp_for_start = 1780; //значение ЦАП для выдачи высоковольтником 2,8КВт
float threshold = 4.2; //пороговое значение для суммы на пик
int waiting = 200; //время задержки после смены частоты

//здесь все для парсинга данных для старта и запуска тестирования
struct for_test {
    float freq_start;
    float freq_finish;
    boolean type_scan;
  }test_data;
boolean recievedFlag = false;
boolean getStarted;
boolean s = false; //если s - значит надо запускать тестирование
boolean t = false; //если t - меняем порожек
int index = 0;
String string_convert = "";
char control = 'd';
//Заработало, необходимо дотестировать

void setup() {
  //ускорение
  #if FASTADC
  // set prescale to 16
  sbi(ADCSRA,ADPS2) ;
  cbi(ADCSRA,ADPS1) ;
  cbi(ADCSRA,ADPS0) ;
  #endif
  //ускорение
  
  SPI.begin();
  Serial.begin(9600);

  pinMode(3,  OUTPUT);  // CD4051 pin 11 (A)
  pinMode(4,  OUTPUT);  // CD4051 pin 10 (B)
  pinMode(5, OUTPUT);  // CD4051 pin 9  (C)

  pinMode(6, OUTPUT); //светодиод для индикации работы ВВ

  analogReference(DEFAULT);//опорное напряжение для мультиплексора. internal - 1.1v; DEFAULT - опорное напряжение равно напряжению питания МК. 
  
  MCP4725.begin(0x60);
  MCP4725.setVoltage(0, false);  

  WriteAD9833(0x2100); //0010 0001 0000 0000 - Reset + DB28
  WriteAD9833(0x5432); //0101 0000 1100 0111 - Freq0 LSB (4295)  0101 0100 0011 0010(1074)
  WriteAD9833(0x4000); //0100 0000 0000 0000 - Freq0 MSB (0)
  WriteAD9833(0xC000); //1100 0000 0000 0000 - Phase0 (0)
  WriteAD9833(0x2000); //0010 0000 0000 0000 - Exit Reset 
  
  SetFrequency(0); 
}

void loop() {
  //ускорение
  #if FASTADC
  // set prescale to 16
  sbi(ADCSRA,ADPS2) ;
  cbi(ADCSRA,ADPS1) ;
  cbi(ADCSRA,ADPS0) ;
  #endif
  //ускорение

  parsing_start();
  exit_now = false;

  if(recievedFlag && t) {
    recievedFlag = false;
    t = false;    
    Serial.println("^t;");
    delay(1);
  }
  
  if(recievedFlag && s){    
    recievedFlag = false;
    s = false;    
    hv_starting();
    start_testing(test_data.freq_start, test_data.freq_finish, test_data.type_scan);  
    SetFrequency(0); 
    Serial.println("^e;");
    delay(1);    
    hv_finishing();    
  }
}

//Запуск высоковольтника с ЦАП с шагом и задержкой(default)
void hv_starting(){
  digitalWrite(6, HIGH);
  for(int i = 180; i <= mcp_for_start; i += 400) {
    MCP4725.setVoltage(i, false);
    delay(1000);
  }
}

//Запуск высоковольника с задаваемыми параметрами ЦАП. С какого уровня, до какого, с каким шагом + задержка 
void hv_starting(float voltage_start, float voltage_finish, float pace, int lag){
  int mcp_start = voltage_start/volt_ref*4096;
  int mcp_finish = voltage_finish/volt_ref*4096;
  int mcp_step = pace/volt_ref*4096;
  for (int i = mcp_start; i <= mcp_finish; i+= mcp_step){
    MCP4725.setVoltage(i, false);
    delay(lag);
  }
}

//остановка высоковольтника с ЦАП с шагом и задержкой (режим - default)
void hv_finishing(){
  for (int i = mcp_for_start-400; i >= 180; i -= 400){
     MCP4725.setVoltage(i, false);
     delay(1000);
  }
  MCP4725.setVoltage(0, false);
  digitalWrite(6, LOW);  
  Serial.println("^f;");
  delay(1);
}

//Запись значений в генератор сигналов
void WriteAD9833(uint16_t Data){
  SPI.beginTransaction(SPISettings(SPI_CLOCK_DIV2, MSBFIRST, SPI_MODE2));
  digitalWrite(SS, LOW);
  delayMicroseconds(1);
  SPI.transfer16(Data);
  digitalWrite(SS, HIGH);
  SPI.endTransaction();  
}

//Установка частоты в генератор сигналов
void SetFrequency(float val) {
  Freq = val;
  unsigned long FreqData = round((float) val * 10.73741 + 0.5);
  WriteAD9833(FreqData & 0x3FFF | bFreq_reg0);
  WriteAD9833((FreqData >> 14) | bFreq_reg0);
} 

/*
основная функция для тестирования. С какой частоты, до какой, тип сканирования. 
1 - шаг 1Гц и автоматическое уточнение по 0,1Гц.
0 - шаг 0,1Гц
*/
void start_testing(float freq_start, float freq_finish, bool type_scan){
  
  long sum[8];//для сохранения результатов суммирования для восьми проволочек
  float sum_v[8]; //для хранения результата перевода в вольтаж на пик на проволоке
  for (int i = 0; i < 8; i++){
    sum[i] = 0;
    sum_v[i] = 0;
  }
  //для прогона с шагом 0,1Гц
  if (!type_scan){
    for (float freq = freq_start; (freq <= freq_finish + 0.1) && !exit_now; freq += 0.1){
      WriteAD9833(0xC000);
      SetFrequency(freq);
      for(int i = 0; i < 8; i++){
        sum[i] = 0;
      }
      delay(waiting);
      uint32_t start;
      start = millis();
      for(int i = 0; i < 800; i++){
        for(int k = 0; k < 8; k++){
          setInput(k);
          sum[k]+= analogRead(AnalogIn);
        }
      }
      int measure = millis() - start;
      Serial.print("^d_");
/* далее перевод в вольтаж/количество пиков и отправка на комп. 800*8 измерений проводится за ~190-210ms. 
Общую сумму для проволоки делим на 1024, умножаем на опорное. -получаем общую сумму вольтажа не проволочке
Общий вольтаж делим на (частота/1000/время измерения) - получаем средний вольтаж на пик */
      for(int i = 0; i < 8; i++) {
        sum_v[i] = (sum[i]/1024*5.2)/(freq/(1000/measure)); //с учетом числа преиодов
        //для теста! чет количества съемов сигнала на период
        float x = 800/freq/(1000/measure);
        sum_v[i] = sum_v[i]/x;
        
        Serial.print(sum_v[i]);
        Serial.print("_");
      }
      Serial.print(freq);
      Serial.println(";");
      delay(20);
      //delay(1000);
      parsing_finish();
      if(recievedFlag){
        recievedFlag = false;
        if(control = 'f'){
          exit_now = 1;
          control = 'd';
        }
      }
    }
  }
//функция с шагом 1 и уточнением по 0,1 через while. почти не прыгает
  if(type_scan){
    float last_freq = 0;
    for(float freq = freq_start; (freq <= freq_finish) && !exit_now; freq += 1){
      WriteAD9833(0xC000);
      delayMicroseconds(100);
      for (int i = 0; i < 8; i++){
        sum[i] = 0;
        sum_v[i] = 0;
      }
      SetFrequency(freq);
      delay(waiting);
      uint32_t start;
      start = millis();
      bool g = 0;
      for(int i = 0; i < 800; i++){
        for(int k = 0; k < 8; k++){
          setInput(k);
          sum[k]+= analogRead(AnalogIn);
        }
      }
      int measure = millis() - start;      
      for(int i = 0; i < 8; i++) {
        sum_v[i] = (sum[i]/1024*5.2)/(freq/(1000/measure));
        
        //для теста! чет количества съемов сигнала на период
        float x = 800/freq/(1000/measure);
        sum_v[i] = sum_v[i]/x;
        
        if (sum_v[i] >= threshold) {
          if (freq != freq_start){
            if ((freq - 1) <= last_freq){
              freq = last_freq;
            }
            else {freq -= 1;}
            bool passability;
            passability = 1;
            g = 1;
            while(passability && !exit_now && (freq <= freq_finish)){
              passability = 0;
              freq += 0.1;
              WriteAD9833(0xC000);              
              SetFrequency(freq);
              delay(waiting);
              for(int i = 0; i < 8; i++){
                sum[i] = 0;
                sum_v[i] = 0;
              }
              uint32_t start;
              start = millis();
              for(int i = 0; i < 800; i++){
                for(int k = 0; k < 8; k++){
                  setInput(k);
                  sum[k]+= analogRead(AnalogIn);
                }
              }
              measure = millis() - start;
              for (int i = 0; i < 8; i++){
                 sum_v[i] = (sum[i]/1024*5.2)/(freq/(1000/measure));
                 
                 //для теста! чет количества съемов сигнала на период
                 float x = 800/freq/(1000/measure);
                 sum_v[i] = sum_v[i]/x;
                 
                 if (sum_v[i] > threshold){
                  passability = 1;  
                 }
              }
              Serial.print("^d_");
              for(int i = 0; i < 8; i++) {
                Serial.print(sum_v[i]);
                Serial.print("_");
              }
              Serial.print(freq);
              Serial.println(";");
              delay(20);
              parsing_finish();
              last_freq = freq;
              if(recievedFlag){
                recievedFlag = false;
                if(control = 'f'){
                  exit_now = true;
                  control = 'd';
                }
              }
            }
          }
        }
      }
      /*   функция для шага 1 герц и уточнения по 0,1. два фора, прыгает  
            for (int k = 0; k < 10; k++){
              freq += 0.1;
              WriteAD9833(0xC000);              
              SetFrequency(freq);
              if(k == 0){
                delay(waiting);
              }
              bool passability;
              passability = 0;
              delay(waiting);
               for(int i = 0; i < 8; i++){
                sum[i] = 0;
                sum_v[i] = 0;
              }
              uint32_t start;
              start = millis();
              for(int i = 0; i < 800; i++){
                for(int k = 0; k < 8; k++){
                  setInput(k);
                  sum[k]+= analogRead(AnalogIn);
                }
              }
              measure = millis() - start;
              for (int i = 0; i < 8; i++){
                 sum_v[i] = (sum[i]/1024*5.2)/(freq/(1000/measure));  
              }
              Serial.print("^d_");
              for(int i = 0; i < 8; i++) {
                Serial.print(sum_v[i]);
                Serial.print("_");
              }
              Serial.print(freq);
              Serial.println(";");
              delayMicroseconds(100);
              parsing_finish();
              if(recievedFlag){
                recievedFlag = false;
                if(control = 'f'){
                  exit_now = true;
                  control = 'd';
                }
              }
            }
            //freq += 0.1;
            g = 1;
          }          
        }
        if(g){
          i = 9;
        }
      }
*/
      if(g){
        freq = trunc(freq);
        continue;
      }
      Serial.print("^d_");
      for(int i = 0; i < 8; i++) {
        Serial.print(sum_v[i]);
        Serial.print("_");
      }
      Serial.print(freq);
      Serial.println(";");
      delay(20);
      //delay(1000);
      parsing_finish();
      if(recievedFlag){
        recievedFlag = false;
        if(control = 'f'){
          exit_now = true;
          control = 'd';
        }
      }
    }
  }
  /*для прогона с шагом 1Гц и автоматическим уточнением 0,1Гц . Работает через scan_detail
  if(type_scan){
    for(float freq = freq_start; (freq <= freq_finish+1) && !exit_now; freq += 1){
      WriteAD9833(0xC000);
      delayMicroseconds(100);
      SetFrequency(freq);
      delay(waiting);
      uint32_t start;
      start = millis();
      for(int i = 0; i < 800; i++){
        for(int k = 0; k < 8; k++){
          setInput(k);
          sum[k]+= analogRead(AnalogIn);
        }
      }
      int measure = millis() - start;      
      for(int i = 0; i < 8; i++) {
        sum_v[i] = (sum[i]/1024*5.2)/(freq/(1000/measure));
        if (sum_v[i] >= threshold) {
          if (freq != freq_start){
            freq = scan_detail(freq - 1);
            continue;
          }
        }
      }
      Serial.print("^d_");
      for(int i = 0; i < 8; i++) {
        Serial.print(sum_v[i]);
        Serial.print("_");
      }
      Serial.print(freq);
      Serial.println(";");
      //delay(1000);
      parsing_finish();
      if(recievedFlag){
        recievedFlag = false;
        if(control = 'f'){
          exit_now = true;
          control = 'd';
        }
      }
    }
  }
*/
}

/*функция для запуска прогона с детализацией 0,1Гц(поиск). Рекурсивная, не прыгает. Может сожрать всю память ардуины
float scan_detail(float freq){
  long sum[8];//для сохранения результатов суммирования для восьми проволочек
  float sum_v[8];
  for (int i = 0; i < 8; i++){
    sum[i] = 0;
    sum_v[i] = 0;
  }
  for(int i = 0; i <= 9 && !exit_now; i++){
    freq += 0.1;
    WriteAD9833(0xC000);
    delayMicroseconds(100);
    SetFrequency(freq);
    bool passability;
    passability = 0;
    for(int i = 0; i < 8; i++){
      sum[i] = 0;
    }
    delay(waiting);
    uint32_t start;
    start = millis();
    for(int i = 0; i < 800; i++){
      for(int k = 0; k < 8; k++){
        setInput(k);
        sum[k]+= analogRead(AnalogIn);
      }
    }
    int measure = millis() - start;
    for(int i = 0; i < 8; i++) {
      sum_v[i] = (sum[i]/1024*5.2)/(freq/(1000/measure));
      if (sum_v[i] >= threshold) {
        passability = 1;
      }
    }
    Serial.print("^d_");
    for(int i = 0; i < 8; i++) {
      Serial.print(sum_v[i]);
      Serial.print("_");
    }
    Serial.print(freq);
    Serial.println(";");
    if(i == 9) {
      if (passability){
        freq = scan_detail(freq);
      }              
    }
    parsing_finish();
    if(recievedFlag){
      recievedFlag = false;
      if(control = 'f'){
        exit_now = true;
        control = 'd';
      }
    }
  }
  delayMicroseconds(100);
  return freq;
}
*/
//парсинг сигнала остановки работы
void parsing_finish() {
  if(Serial.available() > 0){
    char incomingByte = Serial.read();
    if (getStarted){
      if(incomingByte != ';'){
        control = incomingByte;
      }
    }
    if (incomingByte == '^'){
      getStarted = true;
      control = 'd';
    }
    if (incomingByte == ';'){
      getStarted = false;
      recievedFlag = true;
    }
  }
}
//парсинг сигнала начала работы
void parsing_start(){
  if(Serial.available() > 0){
    char incomingByte = Serial.read();
    if(getStarted){
      if(incomingByte != '_' && incomingByte != ';'){
        string_convert += incomingByte;
      } else {
        switch(index){
          case 0:
            if(string_convert == "s"){
              s = true;
              index++;
              string_convert = "";
            } else if(string_convert == "t"){
              t = true;
              index++;
              string_convert = "";
              
            }
            else {             
            string_convert = "";
            getStarted = false;}
            break;
          case 1:
            if(s == true){
              float mfloat = string_convert.toFloat();
              test_data.freq_start = mfloat;
            } 
            else if(t == true){
              float mfloat = string_convert.toFloat(); 
              threshold = mfloat; 
            }
            string_convert = "";
            index++;
            if(t){
              index = 0;
            }
            break;
          case 2:
            if(s == true){
              float mfloat = string_convert.toFloat();
              test_data.freq_finish = mfloat;
            }
            string_convert = "";
            index++;
            break;
          case 3:
            if(s){
              test_data.type_scan = string_convert.toInt();
            }
            string_convert = "";
            index = 0;
            break;
        }
      }
    }
    if(incomingByte == '^'){
      getStarted = true;
      string_convert = "";
    }
    if(incomingByte == ';'){
      getStarted = false;
      recievedFlag = true;
    }
  }
}


//Установка значений в мультиплексоре(выбор канала)
void setInput(int i){
  switch(i) {
    case 0:
      digitalWrite(5, LOW);
      digitalWrite(4, LOW);
      digitalWrite(3, LOW);
      break;
    case 1:
      digitalWrite(5, LOW);
      digitalWrite(4, LOW);
      digitalWrite(3, HIGH);
      break;
    case 2:
      digitalWrite(5, LOW);
      digitalWrite(4, HIGH);
      digitalWrite(3, LOW);
      break;
    case 3:
      digitalWrite(5, LOW);
      digitalWrite(4, HIGH);
      digitalWrite(3, HIGH);
      break;
    case 4:
      digitalWrite(5, HIGH);
      digitalWrite(4, LOW);
      digitalWrite(3, LOW);
      break;
    case 5:
      digitalWrite(5, HIGH);
      digitalWrite(4, LOW);
      digitalWrite(3, HIGH);
      break;
    case 6:
      digitalWrite(5, HIGH);
      digitalWrite(4, HIGH);
      digitalWrite(3, LOW);
      break;
    case 7:
      digitalWrite(5, HIGH);
      digitalWrite(4, HIGH);
      digitalWrite(3, HIGH);
      break;  
  }
}

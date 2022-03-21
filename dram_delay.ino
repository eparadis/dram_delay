
#define DIN 5
#define _WE 4
#define _RAS 3
#define AD0 6
#define AD1 8
#define AD2 7
#define AD3 12
#define AD4 11
#define AD5 10
#define AD6 A3 // will be used as a digital output
#define AD7 9
#define DOUT A1 // will be used as a digital input
#define _CAS 2


void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // aka pin D13

  pinMode(DIN, OUTPUT);
  pinMode(_WE, OUTPUT);
  pinMode(_RAS, OUTPUT);
  pinMode(AD0, OUTPUT);
  pinMode(AD1, OUTPUT);
  pinMode(AD2, OUTPUT);
  pinMode(AD3, OUTPUT);
  pinMode(AD4, OUTPUT);
  pinMode(AD5, OUTPUT);
  pinMode(AD6, OUTPUT);
  pinMode(AD7, OUTPUT);
  pinMode(_WE, OUTPUT);
  pinMode(DOUT, INPUT);  // our one input!
  pinMode(_CAS, OUTPUT);

  Serial.begin(9600);
}

void setRowAddress( byte addr) {
  digitalWrite(AD0, addr & 0x01);
  digitalWrite(AD1, addr & 0x02);
  digitalWrite(AD2, addr & 0x04);
  digitalWrite(AD3, addr & 0x08);
  digitalWrite(AD4, addr & 0x10);
  digitalWrite(AD5, addr & 0x20);
  digitalWrite(AD6, addr & 0x40);
  digitalWrite(AD7, addr & 0x80);
}

void assertRAS() {
  digitalWrite(_RAS, LOW);
}

void setColumnAddress( byte addr) {
  digitalWrite(AD0, addr & 0x01);
  digitalWrite(AD1, addr & 0x02);
  digitalWrite(AD2, addr & 0x04);
  digitalWrite(AD3, addr & 0x08);
  digitalWrite(AD4, addr & 0x10);
  digitalWrite(AD5, addr & 0x20);
  digitalWrite(AD6, addr & 0x40);
  digitalWrite(AD7, addr & 0x80);
}

void assertCAS() {
  digitalWrite(_CAS, LOW);
}

byte readData() {
  return digitalRead(DOUT);
}

void unassertRAS() {
  digitalWrite(_RAS, HIGH);
}

void unassertCAS() {
  digitalWrite(_CAS, HIGH);
}

void assertWrite() {
  digitalWrite(_WE, LOW);
}

void unassertWrite() {
  digitalWrite(_WE, HIGH);
}

void writeData( byte d) {
  digitalWrite(DIN, d);
}

void writeToDRAM( byte row, byte col, byte val) {
  setRowAddress(row);
  assertRAS();
  assertWrite();
  writeData(val);
  setColumnAddress(col);
  assertCAS();
  unassertWrite();
  unassertCAS();
  unassertRAS();
}

byte readFromDRAM( byte row, byte col) {
  setRowAddress(row);
  assertRAS();
  setColumnAddress(col);
  assertCAS();
  byte val = readData();
  unassertCAS();
  unassertRAS();
  return val;
}

#define ROW_MAX 64
#define COL_MAX 64

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("writing...");
  for( int row = 0; row < ROW_MAX; row+=1) {
    for( int col = 0; col < COL_MAX; col+=1) {
      writeToDRAM(row, col, HIGH);
    }
  }

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("reading...");
  for( int row = 0; row < ROW_MAX; row+=1) {
    for( int col = 0; col < COL_MAX; col+=1) {
      if( !readFromDRAM(row, col)) {
        Serial.print("LOW ");
        Serial.print(row);
        Serial.print(" ");
        Serial.print(col);
        Serial.print("\n");
      }
    }
  }
}


#define DIN 5
#define _WE 4
#define _RAS 3
#define AD0 A0 // will be used as a digital output
#define AD1 8
#define AD2 A2 // will be used as a digital output
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

//  Serial.begin(9600);


  DIDR1 = 0x03; // turn off digital inputs for analog comparator (pg 259 of datasheet)
  ACSR = 0x00; // setup analog comparator
  TIMSK0 = 0x00; // turn off delay timer to reduce jitter
}

void setAddress( byte addr) {
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
  setAddress(row);
  assertRAS();
  assertWrite();
  writeData(val);
  setAddress(col);
  assertCAS();
  unassertWrite();
  unassertCAS();
  unassertRAS();
}

byte readFromDRAM( byte row, byte col) {
  setAddress(row);
  assertRAS();
  setAddress(col);
  assertCAS();
  byte val = readData();
  unassertCAS();
  unassertRAS();
  return val;
}

byte readAndWriteDRAM( byte row, byte col, byte val_in) {
  setAddress(row);
  assertRAS();
  setAddress(col);
  assertCAS();
  byte val_out = readData();
  writeData(val_in);
  assertWrite();
  //delayMicroseconds(1); // technicnally we need to wait 45 nanoseconds here... probably ok doing nothing?
  unassertWrite();
  unassertCAS();
  unassertRAS();
  return val_out;
}

#define ROW_MAX 256
#define COL_MAX 256

int row;
int col;
byte input;
byte val_out;

void loop() {
//  Serial.print(".");
  while(1) {
    for( row = 0; row < ROW_MAX; row+=1) {
      setAddress(row);
      assertRAS();

      for( col = 0; col < COL_MAX; col+=1) {
        input = ACSR & (1<<ACO); // get comparator data
        setAddress(col);
        assertCAS();
        val_out = readData();
        //writeData(input);
        // write data
        if( input)
          PORTD |= (1<<PD5);
        else
          PORTD &= ~(1<<PD5);
        
        assertWrite();
        unassertWrite();
        unassertCAS();
        //digitalWrite(LED_BUILTIN, val_out); // PB5
        if(val_out)
          PORTB |= (1<<PB5);
        else
          PORTB &= ~(1<<PB5);
      }

      unassertRAS();
    }
  }
}


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
#define AD8 A4 // will be used as a digital output

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
  pinMode(AD8, OUTPUT);
  pinMode(_WE, OUTPUT);
  pinMode(DOUT, INPUT);  // our one input!
  pinMode(_CAS, OUTPUT);

//  Serial.begin(115200);


  DIDR1 = 0x03; // turn off digital inputs for analog comparator (pg 259 of datasheet)
  ACSR = 0x00; // setup analog comparator
  TIMSK0 = 0x00; // turn off delay timer to reduce jitter


  ADCSRA = bit(ADEN) // Turn ADC on
           | bit(ADPS0) | bit(ADPS1) | bit(ADPS2); // Prescaler of 128
  byte adcPin = A7;
  ADMUX  = bit(REFS0) // AVCC
           | ((adcPin - 14) & 0x07); // Arduino Uno to ADC pin
}

void setAddress( int addr) {
  // ignore the order of the address bits because it's just a big mux grid anyhow
  //digitalWrite(AD1, addr & 0x02); // PB0
  //digitalWrite(AD7, addr & 0x80); // PB1
  //digitalWrite(AD5, addr & 0x20); // PB2
  //digitalWrite(AD4, addr & 0x10); // PB3
  //digitalWrite(AD3, addr & 0x08); // PB4

  // bottom five bits of the address go to the bottom five bits of PORTB, preserving whatever it already was
  byte z = addr & 0b00011111;
  z |= (PORTB &   0b11100000);
  PORTB = z;

  // top three bits we just do one at a time i guess
  // again we're ignoring the order these are really connected. if it matters, we'll swap the wires
  //digitalWrite(AD0, addr & 0x01); // PC0
  if(addr & 0x20)
    PORTC |= (1<<PC0);
  else
    PORTC &= ~(1<<PC0);

  //digitalWrite(AD2, addr & 0x04); // PC2
  if(addr & 0x40)
    PORTC |= (1<<PC2);
  else
    PORTC &= ~(1<<PC2);

  //digitalWrite(AD6, addr & 0x40); // PC3
  if(addr & 0x80)
    PORTC |= (1<<PC3);
  else
    PORTC &= ~(1<<PC3);

  //digitalWrite(AD8, something something..... PC4
  if(addr & 0x0100)
    PORTC |= (1<<PC4);
  else
    PORTC &= ~(1<<PC4);
}

void assertRAS() {
//  digitalWrite(_RAS, LOW);
  PORTD &= ~(1<<PD3);
}

void assertCAS() {
//  digitalWrite(_CAS, LOW);
  PORTD &= ~(1<<PD2);
}

byte readData() {
  return digitalRead(DOUT);
}

void unassertRAS() {
  //digitalWrite(_RAS, HIGH);
  PORTD |= (1<<PD3);
}

void unassertCAS() {
  //digitalWrite(_CAS, HIGH);
  PORTD |= (1<<PD2);
}

void assertWrite() {
  //digitalWrite(_WE, LOW);
  PORTD &= ~(1<<PD4);
}

void unassertWrite() {
  //digitalWrite(_WE, HIGH);
  PORTD |= (1<<PD4);
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

byte adcChannel = A7;
void setADCChannel(byte pin) {
  adcChannel = pin;
  ADMUX  = bit(REFS0) // AVCC
    | ((pin - 14) & 0x07); // Arduino Uno to ADC pin
}

#define ROW_MAX 511
#define COL_MAX 511

unsigned int row;
unsigned int col;
byte input;
byte val_out;
unsigned int dummy = 0;
unsigned int dummy2 = 0;
bool adc_conversion_working = false;
unsigned int start_row = 0;
unsigned int row_length = 512;

// inlining the assert___ and unassert___ did zero speed up, so I dropped that commit
// using bytes (and max of 255) for loops didn't do anything. 

void loop() {
//  Serial.print(".");
  while(1) {
    for( row = start_row; row < (start_row + row_length); row+=1) {
      setAddress(row);
      assertRAS();

      for( col = 0; col < COL_MAX; col+=1) {
        input = ACSR & (1<<ACO); // get comparator data
        setAddress(col);
        assertCAS();
        val_out = readData();

        // write data
        if( input)
          PORTD |= (1<<PD5);
        else
          PORTD &= ~(1<<PD5);
        
        assertWrite();
        unassertWrite();
        unassertCAS();

        if(val_out)
          PORTB |= (1<<PB5);
        else
          PORTB &= ~(1<<PB5);
      }

      unassertRAS();

      if( !adc_conversion_working) {
        bitSet(ADCSRA, ADSC);  // Start a conversion
        adc_conversion_working = true;
      }

      // The ADC clears the bit when done; we're looking for a change in value
      if (bit_is_clear(ADCSRA, ADSC)) {
        adc_conversion_working = false;
        if( adcChannel == A7) {
          // did A7 actually change values?
          dummy = (ADC >> 1) + 1; // 1..512
          if( row_length != dummy ) {
            row_length = dummy;
            // don't let our length be longer than our buffer
            if( (start_row + row_length) > ROW_MAX)
              row_length = ROW_MAX - start_row;
          }
          setADCChannel(A6);

//          Serial.print(start_row);
//          Serial.print(' ');
//          Serial.print(row_length);
//          Serial.print(' ');
//          Serial.println(start_row + row_length);
        } else {
          // A6 is the only other option, for now
          dummy = (ADC >> 1); // 0..511
          if( start_row != dummy) {
            start_row = dummy;
            if( start_row > ROW_MAX - 1)
              start_row = ROW_MAX - 1;
          }
          setADCChannel(A7);
        }

        
      }
    }
  }
}

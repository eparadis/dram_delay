
#define DIN 5 // PD5
#define _WE 4 // PD4
#define _RAS 3 // PD3
#define AD0 A1 // PC1 will be used as a digital output
#define AD1 8  // PB0
#define AD2 A2 // PC2 will be used as a digital output
#define AD3 12 // PB4
#define AD4 11 // PB3
#define AD5 10 // PB2
#define AD6 A3 // PC3 will be used as a digital output
#define AD7 9  // PB1
#define DOUT A0 // PC0 will be used as a digital input
#define _CAS 2 // PD2
#define AD8 A4 // PC4 will be used as a digital output
#define AUDIO_OUT 13 // PB5 also the LED, so it's a nice debug

void setup() {
  pinMode(AUDIO_OUT, OUTPUT);

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

  DIDR1 = 0x03; // turn off digital inputs for analog comparator (pg 259 of datasheet)
  ACSR = 0x00; // setup analog comparator
  TIMSK0 = 0x00; // turn off delay timer to reduce jitter

  ADCSRA = bit(ADEN) // Turn ADC on
           | bit(ADPS0) | bit(ADPS1) | bit(ADPS2); // Prescaler of 128
  ADMUX  = bit(REFS0) // AVCC
           | ((A7 - 14) & 0x07); // Arduino Uno to ADC pin
}

void setAddress( int addr) {
  // ignore the order of the address bits because it's just a big mux grid anyhow

  // bottom five bits of the address go to the bottom five bits of PORTB, preserving whatever it already was
  byte z = addr & 0b00011111;
  z |= (PORTB &   0b11100000);
  PORTB = z;

  // top four bits go to PC1 thru PC4
  bitWrite(PORTC, PC1, addr & 0x20);  // AD0 => PC1
  bitWrite(PORTC, PC2, addr & 0x40);  // AD2 => PC2
  bitWrite(PORTC, PC3, addr & 0x80);  // AD6 => PC3
  bitWrite(PORTC, PC4, addr & 0x100); // AD8 => PC4

// this was slower than just setting each pin one by one
//  int y = (addr & 0b111100000) >> 4; // so now at 0b0011110
//  y |= (PORTC & 0b1100001); // we take what PORTC already was
//  PORTC = (byte) y;

// this was also slower than just setting each pin one by one
//  byte y = (addr & 0b11100000) >> 4; // now at 0b00001110
//  y |= (PORTC & 0b11110001);
//  if(addr & 0x100)
//    y |= (1<<PC4);
//  else
//    y &= ~(1<<PC4);
//  PORTC = y;
}

void assertRAS() {
  PORTD &= ~(1<<PD3); // _RAS LOW
}

void assertCAS() {
  PORTD &= ~(1<<PD2); // _CAS LOW
}

byte readData() {
  return (PINC & (1<<PC0)); // DOUT is PC0
}

void unassertRAS() {
  PORTD |= (1<<PD3); // _RAS HIGH
}

void unassertCAS() {
  PORTD |= (1<<PD2); // _CAS HIGH
}

void assertWrite() {
  PORTD &= ~(1<<PD4); // _WE LOW
}

void unassertWrite() {
  PORTD |= (1<<PD4); // _WE HIGH
}

void writeData( byte d) {
  bitWrite(PORTD, PD5, d);   // DIN is PD5
}

void dacOut( byte v) {
  bitWrite(PORTB, PB5, v); // DOUT is PB5, which is also the D13 LED
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
bool adc_conversion_working = false;
unsigned int start_row = 0;
unsigned int row_length = 512;

// inlining the assert___ and unassert___ did zero speed up, so I dropped that commit
// using bytes (and max of 255) for loops didn't do anything. 

void loop() {
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
        writeData( input);
        
        assertWrite();
        unassertWrite();
        unassertCAS();

        dacOut(val_out);
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

// the setup function runs once when you press reset or power the board
/*
https://github.com/Fabio3rs/Monomotronic-MA1.7-Scanner

Test code

MIT License

Copyright (c) 2019 Fabio3rs

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <array>


// Test code
void setup()
{
  Serial.begin(115200);
  Serial2.begin(4800, SERIAL_8N1);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(13, INPUT);
}

template<int n>
void sendInit(int port1, int port2, std::array<int, n> v)
{
  for (int i = 0; i < v.size(); i++)
  {
    digitalWrite(port1, v[i]);
    digitalWrite(port2, v[i]);
    //Serial.println(digitalRead(port1));
    delay(200);
  }
}

void baudInit()
{
    digitalWrite(0, HIGH);
    digitalWrite(2, HIGH);
    delay(200);
    sendInit<8>(0, 2, std::array<int, 8>{HIGH, HIGH, HIGH, HIGH, LOW, HIGH, HIGH, HIGH}); // 0x10
    digitalWrite(0, LOW);
    digitalWrite(2, LOW);
}

int ab(int a)
{
  return a < 0? -a : a;
}

// the loop function runs over and over again forever
void loop()
{
  baudInit();
    //Serial2.write(0x10);
  //Serial1.write(0x10);
  /*int old = 0;
  for (int i = 0; i < 2560; i++)
  {
    int newv = analogRead(13);

    if (ab(old - newv) > 20)
    {
      Serial.println(newv);
      old = newv;
    }
    delay(1);
  }*/
  for (int i = 0; i < 256; i++)
  {
    if (Serial2.available() > 0)
    {
      // lê do buffer o dado recebido:
      int incomingByte = Serial2.read();
  
      // responde com o dado recebido:
      Serial.print("I received: ");
      Serial.println(incomingByte, DEC);
    }
    delay(10); 
  }
  
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second*/
}

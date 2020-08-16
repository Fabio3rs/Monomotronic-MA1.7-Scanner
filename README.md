# Monomotronic-MA1.7-Scanner
C++ scanner for bosch monomotronic MA1.7 Fiat Tipo 1.6ie

Tested using VAGCOM KKL with Fiat 3 pin to OBD2 adapter
Software in development 

For ESP32:
https://github.com/espressif/arduino-esp32/issues/600#issuecomment-354089442
line 217 of esp32-hal-uart.c
```
if ( uart->dev->conf0.stop_bit_num == TWO_STOP_BITS_CONF) {
    uart->dev->conf0.stop_bit_num = ONE_STOP_BITS_CONF;
    uart->dev->rs485_conf.dl1_en = 1;
}
```
```
uart->dev->idle_conf.tx_idle_num = 0;
uart->dev->idle_conf.tx_brk_num = 0;
uart->dev->idle_conf.rx_idle_thrhd = 0;
uart->dev->conf1.rxfifo_full_thrhd = 1;
uart->dev->conf1.rx_tout_thrhd = 1;
UART_MUTEX_UNLOCK();
```

Testing:
https://www.youtube.com/watch?v=awX4FsL3zvI
https://www.youtube.com/watch?v=0OTWC_7FS3s

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

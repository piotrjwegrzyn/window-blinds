# Window blinds
Arduino-compatibile ESP8266 module's firmware for WiFi connection, photoresistor and microservos in smart-home system.

## Hardware
* ESP8266-12E
* manually **modified** Micro servo MG90S
* photoresistor 10 kOhm
* some other resistors

### ESP8266-12E pinout
![pinout.png](docs/pinout.png) 


## Connenctor.kt

To test some funcionalities you can run Connector, sample program written in Kotlin:

```kotlinc Connenctor.kt -include-runtime -d connenctor.jar && java -jar connenctor.jar```
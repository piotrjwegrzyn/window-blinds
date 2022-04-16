/*******************************************
 *  Window blinds
 * April, 2022
 * 
 * Author: Piotr J. Węgrzyn
 * Email: piotrwegrzyn@protonmail.com
 *******************************************/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <Servo.h>

static const char *AP_SSID = "ssid";
static const char *AP_PASS = "pass";

static const uint16_t STD_DELAY = 1000;
static const uint16_t STD_PORT = 4210;
static const size_t STD_PKT_SIZE = 5;

static const uint8_t CMD_BRD = 0xF0;
static const uint8_t CMD_SET_REQ = 0x01;
static const uint8_t CMD_GET_REQ = 0x02;
static const uint8_t CMD_GET_RES = 0x10;


static bool _configured = true;
static WiFiUDP _udp_instance;
static uint16_t _id = 0x0;
static Servo _microservo;
static uint8_t _srv_pos = 0;
static uint32_t _srv_time = 5000;


/* Function send_pkt() sends packets
 * Returns true if packet was sent correctly
 */
bool send_pkt(IPAddress rcv_addr, uint16_t port=STD_PORT, uint8_t cmd=CMD_BRD) {
    char buffer[6];             // content holder
    buffer[0] = cmd;            // command
    buffer[1] = _id >> 8;       // module ID
    buffer[2] = _id & 0xFF;     //
    buffer[5] = '\0';
    switch (cmd) {
        case CMD_GET_RES:
            buffer[3] = get_phr_val();  // photoresistor value
            buffer[4] = _srv_pos;       // microservo position
            break;
        default:
            buffer[3] = 0x0;
            buffer[4] = 0x0;
            break;
    }
    
    _udp_instance.beginPacket(rcv_addr, STD_PORT);
    _udp_instance.print(buffer);
    return _udp_instance.endPacket() == 1 ? true : false;
}


/* Function handle_pkt() serves incoming packets
 * Returns true if packet was handled correctly
 */
bool handle_pkt() {
    char pkt[STD_PKT_SIZE];
    uint8_t len = _udp_instance.read(pkt, STD_PKT_SIZE);
    if (len > 0) {
        switch (pkt[0]){
            case CMD_SET_REQ:
                return set_params(pkt);
            case CMD_GET_REQ:
                return send_pkt(_udp_instance.remoteIP(), STD_PORT, CMD_GET_RES);
            default:
                return false;
        }
    }
    
    Serial.println("Empty packet");
    return false;
}


/* Function set_eeprom_id()
 * Copy EEPROM ID to program
 */
void set_eeprom_id() {
    EEPROM.begin(4);
    _id = EEPROM.read(1) << 8 | EEPROM.read(2);
}


/* Function set_udp_rcv() starts listening on given port
 * Returns true if listening started correctly
 */
bool set_udp_rcv(uint16_t port=STD_PORT) {
    Serial.printf("Listening on UDP port %d\n", port);
    return _udp_instance.begin(port) == 1 ? true : false;
}


/* Function set_wifi_con() establish connection to access point
 * Returns true if connection established correctly
 */
bool set_wifi_con(uint8_t timeout=60) {
    WiFi.begin(AP_SSID, AP_PASS);
    Serial.println("Connecting...");
    for (uint8_t t = 0; t < timeout; t+=2) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("Connected, IP address: %s\n", WiFi.localIP().toString().c_str());
            return true;
        }
        delay(STD_DELAY);
    }
    Serial.println("Connection failed (timeout)");
    return false;
}


/* Function set_params() set received parameters
 * Returns true if params are set correctly
 */
bool set_params(char *params) {
    if (params[1] & 0x10) { // ID
        uint16_t new_id = params[2] << 8 | params[3];
        if (new_id != _id) {
            _id = new_id;
            EEPROM.write(1, params[2]);
            EEPROM.write(2, params[3]);
            EEPROM.commit();
        }
    }
    if (params[1] & 0x01) { // microservo position
        return set_ser_pos(params[4]);
    }
    return true;
}


/* Function set_ser_con()
 * Set up microservo conectivity
 */
void set_ser_con() {
    _microservo.attach(2);  // D4 pin
    _microservo.write(90);
}


/* Function set_ser_pos()
 * Rotates microservo
 */
bool set_ser_pos(uint8_t pos) {
    uint32_t rot_time = static_cast<uint32_t>((_srv_time * abs(_srv_pos - pos))/255);
    if (_srv_pos > pos) {
        _microservo.write(0);
    }
    else if (_srv_pos < pos) {
        _microservo.write(180);
    }
    delay(rot_time);
    _microservo.write(90);
    _srv_pos = pos;
    return true;
}


/* Function get_pkt_size()
 * Returns received packet size
 */
size_t get_pkt_size() {
    return _udp_instance.parsePacket();
}


/* Function get_brd_addr()
 * Returns broadcast address for current network
 */
IPAddress get_brd_addr() {
    uint32_t network = WiFi.localIP();
    uint32_t subnet = WiFi.subnetMask();
    return IPAddress(network | (~subnet));
}


/* Function get_phr_val()
 * Returns value of resistance on photoresistor (0-255)
 */
uint8_t get_phr_val() {
    return static_cast<uint8_t>((analogRead(A0) * 255) / 1023);
}


void setup() {
    Serial.begin(9600);
    
    set_ser_con();

    set_eeprom_id();
    
    if (!set_wifi_con()) {
        Serial.println("Error while WiFi establishment");
    }
    
    if (!set_udp_rcv(STD_PORT)) {
        Serial.println("Error while UDP start");
    }
}


void loop() {
    if (_configured && get_pkt_size()) {
        if(!handle_pkt()) {
            Serial.println("Error occured while handling packet");
        }
    }
    send_pkt(get_brd_addr());
    delay(STD_DELAY);
}

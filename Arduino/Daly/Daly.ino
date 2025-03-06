// #include <AceBus.h>

#include "esp_private/esp_clk.h"

#include <ESP32-TWAI-CAN.hpp>

// #include "capture.h"

#define CAN_TX 5
#define CAN_RX 19
#define CAN_POLARITY 18

enum BMS_COMMAND {
    BMS_COMMAND_VOUT_IOUT_SOC = 0,
    // BMS_COMMAND_MIN_MAX_CELL_VOLTAGE,
    // BMS_COMMAND_MIN_MAX_TEMPERATURE,
    BMS_COMMAND_DISCHARGE_CHARGE_MOS_STATUS,
    // BMS_COMMAND_STATUS_INFO,
    BMS_COMMAND_CELL_VOLTAGES,
    BMS_COMMAND_CELL_TEMPERATURE,
    // BMS_COMMAND_CELL_BALANCE_STATE,
    BMS_COMMAND_FAILURE_CODES,
    BMS_COMMAND_COUNT,
};

const uint8_t bms_command_list[BMS_COMMAND_COUNT] = {
    [BMS_COMMAND_VOUT_IOUT_SOC] = 0x90, [BMS_COMMAND_DISCHARGE_CHARGE_MOS_STATUS] = 0x93,
    [BMS_COMMAND_CELL_VOLTAGES] = 0x95, [BMS_COMMAND_CELL_TEMPERATURE] = 0x96,
    [BMS_COMMAND_FAILURE_CODES] = 0x98,
};

const char *off_on_str[2] = {"Off", "On"};

const int GREEN_LED = 23;

CanFrame rxFrame;

// #define kSerialPort (Serial1)
// void rxCallback(uint8_t *data, uint8_t length);

// AceBus aceBus(kSerialPort, rxCallback);

void setup() {
    // put your setup code here, to run once:
    pinMode(GREEN_LED, OUTPUT);
    pinMode(CAN_POLARITY, OUTPUT);
    digitalWrite(CAN_POLARITY, LOW);

    Serial.begin(115200);

    ESP32Can.setPins(CAN_TX, CAN_RX);
    ESP32Can.setSpeed(TWAI_SPEED_250KBPS);
    if (ESP32Can.begin()) {
        Serial.println("CAN bus started!");
    } else {
        Serial.println("CAN bus failed!");
    }

    // capture_init();

    Serial.println(esp_clk_apb_freq());
}

void parseFrame(CanFrame *frame) {
    if ((frame->identifier & 0xFF00FFFF) != 0x18004001)
        return;

    uint8_t command = (frame->identifier >> 16) & 0xFF;

    if (command == bms_command_list[BMS_COMMAND_VOUT_IOUT_SOC]) {
        int32_t voltage = (((uint32_t)frame->data[0] << 8) + (uint32_t)frame->data[1]) * 100;
        int32_t current = ((((uint32_t)frame->data[4] << 8) + (uint32_t)frame->data[5]) - 30000) * 100;
        int32_t soc = (((uint32_t)frame->data[6] << 8) + (uint32_t)frame->data[7]) / 10;
        Serial.printf("Voltage %5d mV    Current %5d mA    State of Charge %5d %%\r\n", voltage, current, soc);
    }

    if (command == bms_command_list[BMS_COMMAND_CELL_TEMPERATURE]) {
        int16_t temperature_1 = (uint16_t)frame->data[1] - 40;
        int16_t temperature_2 = (uint16_t)frame->data[2] - 40;
        Serial.printf("Cell Temperatures %d C, %d C\r\n", temperature_1, temperature_2);
    }

    if (command == bms_command_list[BMS_COMMAND_CELL_VOLTAGES]) {
        uint8_t base_index = ((frame->data[0] - 1) * 3) + 1;
        static uint16_t cells_mv[8] = {0};
        for (uint8_t offset = 0; offset < 3; offset++) {
            uint8_t cell_number = base_index + offset;
            if (cell_number <= 8) {
                uint16_t cell_mv =
                    ((uint16_t)frame->data[1 + (2 * offset)] << 8) + (uint16_t)frame->data[2 + (2 * offset)];
                cells_mv[cell_number - 1] = cell_mv;
                // Serial.printf("Cell %u : %4u mV  ", cell_number, cell_mv);
            } else {
                uint16_t pack_mv = 0;
                uint16_t cell_max_mv = cells_mv[0];
                int16_t cell_min_mv = cells_mv[0];
                for (uint8_t i = 0; i < 8; i++) {
                    if (cells_mv[i] == 0)
                        break;
                    pack_mv += cells_mv[i];
                    if (cells_mv[i] > cell_max_mv)
                        cell_max_mv = cells_mv[i];
                    if (cells_mv[i] < cell_min_mv)
                        cell_min_mv = cells_mv[i];
                    cells_mv[i] = 0;
                    if (i == 7)
                        Serial.printf("Pack: %4u mV  Min: %4u mV  Max:  %4u mV "
                                      " Delta: %4u mV\r\n",
                                      pack_mv, cell_min_mv, cell_max_mv, cell_max_mv - cell_min_mv);
                }
            }
        }
    }
}

void sendFrame(uint8_t command) {
    CanFrame frame = {0};
    frame.identifier = 0x18000140 | ((uint32_t)command << 16);
    frame.extd = 1;
    frame.data_length_code = 8;
    memset(frame.data, 0, frame.data_length_code);
    ESP32Can.writeFrame(frame);
}

// enum {
//     IDLE =0,
//     RX_HI,
//     RX_LO
//     DONE
// }

// void decode(uint8_t event){
//     static uint8_t state = IDLE;
//     static uint16_t buffer = 0;
//     static uint8_t bit_count = 0;
//     static uint16_t mask = 0;

//     if((event == 0) || (event > 9)){
//         if(state == RXHI){
//             rxCallback(buffer, 1);
//         }
//         state = RXHI;
//         bit_count = 0;
//         buffer = 0;
//         mask = 0x80;
//     } else {
//         while(event--){
//             bit_count++;
//             if(bit_count == 1){

//             }
//         }
//     }
// }

void loop() {
    static uint8_t bms_command_index = 0;
    static unsigned long secondsTimer = 0;
    static bool timeout = false;

    // int32_t capture = capture_get_event();
    // if (capture) {
    //     Serial.println(capture);
    //     timeout = false;
    // }

    // int32_t time = capture_get_time();
    // if ((time > 20) && (timeout == false)){
    //     Serial.print("timeout ");
    //     Serial.println(time);
    //     timeout = true;
    // }

    // capture_event_t event = capture_get_event();
    // if (event.event != CAPTURE_NO_EVENT) {
    //     if (event.event == CAPTURE_EDGE_POS) {
    //         Serial.print("POS ");
    //         Serial.println(event.value);
    //         timeout = false;
    //     }
    //     if (event.event == CAPTURE_EDGE_NEG) {
    //         Serial.print("NEG ");
    //         Serial.println(event.value);
    //         timeout = false;
    //     }
    //     if ((event.event == CAPTURE_TIMEOUT) && (timeout == false)) {
    //         Serial.print("TIMEOUT ");
    //         Serial.println(event.value);
    //         timeout = true;
    //     }
    // }

    unsigned long m = millis();
    if (m - secondsTimer > 1000L) { // send message once per second
        secondsTimer = m;
        digitalWrite(GREEN_LED, !digitalRead(GREEN_LED)); // toggle led
        // Serial.println(m);

        // uint8_t data[] = "Hello World!";
        // uint8_t length = strlen(data);
        // aceBus.write(data, length, MEDIUM_PRIORITY);
        // hexDump("Send : ", data, length);

        bms_command_index++;
        if (bms_command_index >= BMS_COMMAND_COUNT)
            bms_command_index = 0;
        sendFrame(bms_command_list[bms_command_index]);
    }

    if (ESP32Can.readFrame(rxFrame, 0)) {
        parseFrame(&rxFrame);
        // for (int i = 0; i < rxFrame.data_length_code; i++) {
        //   Serial.printf("0x%2.2X ", rxFrame.data[i]);
        // }
        // Serial.printf(" : Received frame: 0x%03X  \r\n", rxFrame.identifier);
    }
}

void hexDump(char *tag, uint8_t *buffer, int size) {
    int i = 0;
    Serial.print(tag);
    Serial.print(" : ");
    while (i < size) {
        Serial.print(buffer[i] >> 4, HEX);
        Serial.print(buffer[i++] & 0x0F, HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void rxCallback(uint8_t *data, uint8_t length) { hexDump("Recieve : ", data, length); }
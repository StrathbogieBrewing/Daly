#include <ESP32-TWAI-CAN.hpp>

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
  [BMS_COMMAND_VOUT_IOUT_SOC] = 0x90,
  // [BMS_COMMAND_MIN_MAX_CELL_VOLTAGE] = 0x91,
  // [BMS_COMMAND_MIN_MAX_TEMPERATURE] = 0x92,
  [BMS_COMMAND_DISCHARGE_CHARGE_MOS_STATUS] = 0x93,
  // [BMS_COMMAND_STATUS_INFO] = 0x94,
  [BMS_COMMAND_CELL_VOLTAGES] = 0x95,
  [BMS_COMMAND_CELL_TEMPERATURE] = 0x96,
  // [BMS_COMMAND_CELL_BALANCE_STATE] = 0x97,
  [BMS_COMMAND_FAILURE_CODES] = 0x98,
};

const char *off_on_str[2] = { "Off", "On" };

const int GREEN_LED = 23;

CanFrame rxFrame;

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
}

void parseFrame(CanFrame *frame) {
  if ((frame->identifier & 0xFF00FFFF) != 0x18004001) return;

  uint8_t command = (frame->identifier >> 16) & 0xFF;

  if (command == bms_command_list[BMS_COMMAND_VOUT_IOUT_SOC]) {
    int32_t voltage = (((uint32_t)frame->data[0] << 8) + (uint32_t)frame->data[1]) * 100;
    int32_t current = ((((uint32_t)frame->data[4] << 8) + (uint32_t)frame->data[5]) - 30000) * 100;
    int32_t soc = (((uint32_t)frame->data[6] << 8) + (uint32_t)frame->data[7]) / 10;
    Serial.printf("Voltage %5d mV    Current %5d mA    State of Charge %5d %%\r\n", voltage, current, soc);
  }

  if (command == bms_command_list[BMS_COMMAND_DISCHARGE_CHARGE_MOS_STATUS]) {
    uint8_t charge_mos = frame->data[1] & 0x01;
    uint8_t discharge_mos = frame->data[2] & 0x01;
    uint32_t mahours = ((uint32_t)frame->data[4] << 24) + ((uint32_t)frame->data[5] << 16) + ((uint32_t)frame->data[6] << 8) + ((uint32_t)frame->data[7]);
    Serial.printf("Charge %s    Discharge %s    Remaining %u mAh\r\n", off_on_str[charge_mos], off_on_str[discharge_mos], mahours);
  }

  // if (command == bms_command_list[BMS_COMMAND_STATUS_INFO]) {
  //   uint8_t cell_count = frame->data[0];
  //   Serial.printf("Cell count %d\r\n", cell_count);
  // }

  if (command == bms_command_list[BMS_COMMAND_CELL_TEMPERATURE]) {
    int16_t temperature = (uint16_t)frame->data[1] - 40;
    Serial.printf("Cell Temperature %d C\r\n", temperature);
  }

  if (command == bms_command_list[BMS_COMMAND_CELL_VOLTAGES]) {
    uint8_t base_index = ((frame->data[0] - 1) * 3) + 1;
    static uint16_t cell_voltages[8] = { 0 };
    for (uint8_t offset = 0; offset < 3; offset++) {
      uint8_t cell_number = base_index + offset;
      if (cell_number <= 8) {
        uint16_t cell_voltage = ((uint16_t)frame->data[1 + (2 * offset)] << 8) + (uint16_t)frame->data[2 + (2 * offset)];
        cell_voltages[cell_number - 1] = cell_voltage;
        Serial.printf("Cell %u : %4u mV  ", cell_number, cell_voltage);
      } else {
        uint16_t battery_voltage = 0;
        for (uint8_t i = 0; i < 8; i++) {
          if (cell_voltages[i] == 0) break;
          battery_voltage += cell_voltages[i];
          cell_voltages[i] = 0;
          if (i == 7) Serial.printf("Pack : %4u mV\r\n", battery_voltage);
        }
      }
    }
  }
}

void sendFrame(uint8_t command) {
  CanFrame frame = { 0 };
  frame.identifier = 0x18000140 | ((uint32_t)command << 16);
  frame.extd = 1;
  frame.data_length_code = 8;
  memset(frame.data, 0, frame.data_length_code);
  ESP32Can.writeFrame(frame);
}

void loop() {
  static uint8_t bms_command_index = 0;

  digitalWrite(GREEN_LED, HIGH);  // turn the GREEN_LED on (HIGH is the voltage level)
  delay(1000);                    // wait for a second
  digitalWrite(GREEN_LED, LOW);   // turn the GREEN_LED off by making the voltage LOW
  delay(1000);                    // wait for a second

  bms_command_index++;
  if (bms_command_index >= BMS_COMMAND_COUNT) bms_command_index = 0;
  sendFrame(bms_command_list[bms_command_index]);

  // You can set custom timeout, default is 1000
  while (ESP32Can.readFrame(rxFrame, 100)) {
    parseFrame(&rxFrame);
    // for (int i = 0; i < rxFrame.data_length_code; i++) {
    //   Serial.printf("0x%2.2X ", rxFrame.data[i]);
    // }
    // Serial.printf(" : Received frame: 0x%03X  \r\n", rxFrame.identifier);
  }
}

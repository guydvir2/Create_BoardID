#include <Arduino.h>
#include <myIOT2.h>

myIOT2 iot;
const int JDOC_SIZE = 500;
const char *filename = "board_id.json";
const char *id_keys[] = {"id_num", "mcu_type", "input_pins", "output_pins", "RF_enabled", "ver"};

/* User defines following parameters */
bool overrun_board_id = false;

uint8_t idnum = 4;                    /* unique serial ID */
uint8_t mcutype = 0;                  /* 0: ESP8266; 1:ESP32 */
uint8_t inpins[] = {5, 4, 0, 2};      /* inputs */
uint8_t outpins[] = {16, 14, 12, 13}; /* relays */
bool rf_en = false;                   /* RF inputs */
uint8_t v = 1;
/* End */

void read_boardID(char *id)
{
  StaticJsonDocument<JDOC_SIZE> DOC;
  myJflash Jflash;
  Jflash.readFile(DOC, filename);
  serializeJson(DOC, id, 200);
}
bool check_file_exists()
{
  myJflash Jflash(true);
  return (Jflash.exists(filename));
}
bool write_boardID()
{
  myJflash Jflash;
  StaticJsonDocument<JDOC_SIZE> DOC;

  DOC[id_keys[0]] = idnum;
  DOC[id_keys[1]] = mcutype;

  for (uint8_t t = 0; t < sizeof(inpins) / sizeof(inpins[0]); t++)
  {
    DOC[id_keys[2]][t] = inpins[t];
  }
  for (uint8_t t = 0; t < sizeof(outpins) / sizeof(outpins[0]); t++)
  {
    DOC[id_keys[3]][t] = outpins[t];
  }

  DOC[id_keys[4]] = rf_en;
  DOC[id_keys[5]] = v;

  return (Jflash.writeFile(DOC, filename));
}

void addiotnalMQTT(char *incoming_msg, char *_topic)
{
  char msg[200];

  if (strcmp(incoming_msg, "status") == 0)
  {
    iot.pub_msg("Hi");
  }
  else if (strcmp(incoming_msg, "get_id") == 0)
  {
    read_boardID(msg);
    iot.pub_msg(msg);
  }
  else if (strcmp(incoming_msg, "help2") == 0)
  {
    sprintf(msg, "help2: get_id, force_update");
    iot.pub_msg(msg);
  }
  else if (strcmp(incoming_msg, "force_update") == 0)
  {
    if (write_boardID())
    {
      iot.pub_msg("Update: Success");
      iot.sendReset("update_flash");
    }
    else
    {
      iot.pub_msg("Update: Failed");
      iot.sendReset("update_flash_failed");
    }
  }
  else if (strcmp(incoming_msg, "bit") == 0)
  {
    if (check_file_exists())
    {
      myJflash Jflash;
      StaticJsonDocument<JDOC_SIZE> DOC;
      Jflash.readFile(DOC, filename);
      iot.pub_msg("Bit: Start");
      for (uint8_t i = 0; i < DOC[id_keys[3]].size(); i++)
      {
        pinMode(DOC[id_keys[3]][i], OUTPUT);
        digitalWrite(DOC[id_keys[3]][i], !digitalRead(DOC[id_keys[3]][i]));
        delay(1000);
        digitalWrite(DOC[id_keys[3]][i], !digitalRead(DOC[id_keys[3]][i]));
        delay(1000);
      }
      iot.pub_msg("Bit: Done");
    }
    else
    {
      iot.pub_msg("Bit: Failed. File Error");
    }
  }
}
void startIOT()
{
  iot.useSerial = true;
  iot.useFlashP = false;
  iot.ignore_boot_msg = false;
  iot.noNetwork_reset = 4;

  char topic[50];
  sprintf(topic, "myHome/emptyMCU");

  if (check_file_exists())
  {
    myJflash Jflash;
    StaticJsonDocument<JDOC_SIZE> DOC;
    Jflash.readFile(DOC, filename);
    sprintf(topic, "myHome/board_id%d", DOC[id_keys[0]].as<uint8_t>());
  }
  iot.add_gen_pubTopic("myHome/Messages");
  iot.add_gen_pubTopic("myHome/log");
  iot.add_gen_pubTopic("myHome/debug");

  char topic2[60];
  sprintf(topic2, "%s/%s", topic, "Avail");
  iot.add_pubTopic(topic2);
  sprintf(topic2, "%s/%s", topic, "State");
  iot.add_pubTopic(topic2);

  iot.add_subTopic(topic);
  iot.add_subTopic("myHome/All");

  iot.start_services(addiotnalMQTT);
}

uint8_t update_boardID()
{
  if (!check_file_exists() || overrun_board_id)
  {
    if (write_boardID())
    {
      Serial.println("Board ID was saved to flash");
      Serial.flush();
      return 1;
    }
    else
    {
      Serial.println("failed tp save Boad ID to flash");
      Serial.flush();
      return 0;
    }
  }
  else
  {
    Serial.println("Board ID already exists on flash");
    Serial.flush();
    return 2;
  }
}
void setup()
{
  Serial.begin(115200);
  update_boardID();
  startIOT();
}
void loop()
{
  iot.looper();
}

/*
BANK:
ESP8266 - 4 relays
uint8_t mcutype = 0;
uint8_t inpins[] = {5, 4, 0, 2, 15};
uint8_t outpins[] = {16, 14, 12, 13};

*/
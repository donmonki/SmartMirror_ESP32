#include "MyLD2410.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "[MyLD2410]";

// Status strings
static const char *status_strings[] = {
    "No target",
    "Moving only",
    "Stationary only",
    "Both moving and stationary",
    "Auto thresholds in progress",
    "Auto thresholds successful",
    "Auto thresholds failed"};

// Protocol data
static const uint8_t headData[4] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t tailData[4] = {0xF8, 0xF7, 0xF6, 0xF5};
static const uint8_t headConfig[4] = {0xFD, 0xFC, 0xFB, 0xFA};
static const uint8_t tailConfig[4] = {4, 3, 2, 1};
static const uint8_t configEnable[6] = {4, 0, 0xFF, 0, 1, 0};
static const uint8_t configDisable[4] = {2, 0, 0xFE, 0};
static const uint8_t firmware_cmd[4] = {2, 0, 0xA0, 0};
static const uint8_t param_cmd[4] = {2, 0, 0x61, 0};
static const uint8_t engOn[4] = {2, 0, 0x62, 0};
static const uint8_t engOff[4] = {2, 0, 0x63, 0};

static bool bufferEndsWith(const uint8_t *buf, int iMax, const uint8_t *other)
{
  for (int j = 3; j >= 0; j--)
  {
    if (--iMax < 0)
      iMax = 3;
    if (buf[iMax] != other[j])
      return false;
  }
  return true;
}

static uint64_t get_time_ms()
{
  return esp_timer_get_time() / 1000ULL;
}

MyLD2410::MyLD2410(uart_port_t uart_port, bool debug)
    : uart_num(uart_port), _debug(debug)
{
  memset(&sData, 0, sizeof(SensorData));
}

bool MyLD2410::begin()
{
  ESP_LOGI(TAG, "Initializing LD2410 sensor");
  
  uint64_t giveUp = get_time_ms() + 2000UL;
  bool online = false;
  isConfig = false;
  
  sendCommand(configDisable);
  
  while (get_time_ms() < giveUp)
  {
    if (check())
    {
      online = true;
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(110));
  }
  
  if (online)
  {
    ESP_LOGI(TAG, "LD2410 sensor initialized successfully");
  }
  else
  {
    ESP_LOGE(TAG, "Failed to initialize LD2410 sensor");
  }
  
  return online;
}

void MyLD2410::end()
{
  isConfig = false;
  isEnhanced = false;
}

bool MyLD2410::readFrame()
{
  int frameSize = -1, bytes = 2;
  inBufI = 0;
  uint64_t frameTimeout = get_time_ms() + 100;
  
  while (bytes)
  {
    int len = uart_read_bytes(uart_num, inBuf + inBufI, bytes, pdMS_TO_TICKS(10));
    if (len > 0)
    {
      inBufI += len;
      bytes -= len;
    }
    else if (get_time_ms() > frameTimeout)
      return false;
  }

  frameSize = (inBuf[0]) | (inBuf[1] << 8);

  if (frameSize <= 0)
    return false;
  frameSize += 4;
  if (frameSize > LD2410_BUFFER_SIZE)
    return false;
    
  inBufI = 0;
  frameTimeout = get_time_ms() + 100;
  
  while (frameSize > 0)
  {
    int len = uart_read_bytes(uart_num, inBuf + inBufI, frameSize, pdMS_TO_TICKS(10));
    if (len > 0)
    {
      inBufI += len;
      frameSize -= len;
    }
    else if (get_time_ms() > frameTimeout)
      return false;
  }
  
  return true;
}

bool MyLD2410::sendCommand(const uint8_t *command)
{
  uint8_t size = command[0] + 2;
  
  uart_write_bytes(uart_num, (const char *)headConfig, 4);
  uart_write_bytes(uart_num, (const char *)command, size);
  uart_write_bytes(uart_num, (const char *)tailConfig, 4);
  
  uint64_t giveUp = get_time_ms() + 2000UL;
  while (get_time_ms() < giveUp)
  {
    int len = uart_read_bytes(uart_num, headBuf + headBufI, 1, pdMS_TO_TICKS(10));
    if (len > 0)
    {
      headBufI = (headBufI + 1) % 4;
      if (bufferEndsWith(headBuf, headBufI, headConfig))
        return processAck();
    }
  }
  
  return false;
}

bool MyLD2410::processAck()
{
  if (!readFrame())
    return false;

  if (!bufferEndsWith(inBuf, inBufI, tailConfig))
    return false;

  uint16_t command = inBuf[0] | (inBuf[1] << 8);
  
  if (inBuf[2] | (inBuf[3] << 8))
    return false;

  switch (command)
  {
  case 0x1FF:
    isConfig = true;
    version = inBuf[4] | (inBuf[5] << 8);
    bufferSize = inBuf[6] | (inBuf[7] << 8);
    break;
  case 0x1FE:
    isConfig = false;
    break;
  case 0x1A0:
    firmwareMajor = inBuf[7];
    firmwareMinor = inBuf[6];
    break;
  case 0x161:
    maxRange = inBuf[5];
    movingThresholds.setN(inBuf[6]);
    stationaryThresholds.setN(inBuf[7]);
    for (uint8_t i = 0; i <= movingThresholds.N; i++)
      movingThresholds.values[i] = inBuf[8 + i];
    for (uint8_t i = 0; i <= stationaryThresholds.N; i++)
      stationaryThresholds.values[i] = inBuf[17 + i];
    noOne_window = inBuf[26] | (inBuf[27] << 8);
    break;
  case 0x162:
    isEnhanced = true;
    break;
  case 0x163:
    isEnhanced = false;
    break;
  }

  return true;
}

bool MyLD2410::processData()
{
  if (!readFrame())
    return false;

  uint64_t now = get_time_ms();

  if (!bufferEndsWith(inBuf, inBufI, tailData))
    return false;

  if (((inBuf[0] == 1) || (inBuf[0] == 2)) && (inBuf[1] == 0xAA))
  {
    ++dataFrames;
    sData.timestamp = now;
    sData.status = inBuf[2] & 7;
    sData.mTargetDistance = inBuf[3] | (inBuf[4] << 8);
    sData.mTargetSignal = inBuf[5];
    sData.sTargetDistance = inBuf[6] | (inBuf[7] << 8);
    sData.sTargetSignal = inBuf[8];
    sData.distance = inBuf[9] | (inBuf[10] << 8);

    if (inBuf[0] == 1)
    {
      isEnhanced = true;
      sData.mTargetSignals.setN(inBuf[11]);
      sData.sTargetSignals.setN(inBuf[12]);
      uint8_t *p = inBuf + 13;
      for (uint8_t i = 0; i <= sData.mTargetSignals.N; i++)
        sData.mTargetSignals.values[i] = *(p++);
      for (uint8_t i = 0; i <= sData.sTargetSignals.N; i++)
        sData.sTargetSignals.values[i] = *(p++);
      lightLevel = *(p++);
      outLevel = *p;
    }
    else
    {
      isEnhanced = false;
      sData.mTargetSignals.setN(0);
      sData.sTargetSignals.setN(0);
      lightLevel = 0;
      outLevel = 0;
    }
  }
  else
    return false;

  return true;
}

MyLD2410::Response MyLD2410::check()
{
  while (uart_read_bytes(uart_num, headBuf + headBufI, 1, 0) > 0)
  {
    headBufI = (headBufI + 1) % 4;

    if (bufferEndsWith(headBuf, headBufI, headConfig))
    {
      if (processAck())
        return ACK;
    }

    if (bufferEndsWith(headBuf, headBufI, headData))
    {
      if (processData())
        return DATA;
    }
  }

  return FAIL;
}

// Getters
bool MyLD2410::isDataValid()
{
  return (get_time_ms() < sData.timestamp + 500UL);
}

uint8_t MyLD2410::getStatus()
{
  return (isDataValid()) ? sData.status : 0xFF;
}

const char *MyLD2410::statusString()
{
  return status_strings[sData.status < 7 ? sData.status : 6];
}

bool MyLD2410::presenceDetected()
{
  return isDataValid() && (sData.status) && (sData.status < 4);
}

bool MyLD2410::stationaryTargetDetected()
{
  return isDataValid() && ((sData.status == 2) || (sData.status == 3));
}

bool MyLD2410::movingTargetDetected()
{
  return isDataValid() && ((sData.status == 1) || (sData.status == 3));
}

uint8_t MyLD2410::getResolution()
{
  if (fineRes >= 0)
    return ((fineRes == 1) ? 20 : 75);
  return 0;
}

uint32_t MyLD2410::getRange_cm()
{
  return (getRange() + 1) * getResolution();
}

const MyLD2410::ValuesArray &MyLD2410::getMovingThresholds()
{
  if (!maxRange)
    requestParameters();
  return movingThresholds;
}

const MyLD2410::ValuesArray &MyLD2410::getStationaryThresholds()
{
  if (!maxRange)
    requestParameters();
  return stationaryThresholds;
}

uint8_t MyLD2410::getRange()
{
  if (!maxRange)
    requestParameters();
  return maxRange;
}

uint8_t MyLD2410::getNoOneWindow()
{
  if (!maxRange)
    requestParameters();
  return noOne_window;
}

std::string MyLD2410::getFirmware()
{
  if (firmware.empty())
    requestFirmware();
  return firmware;
}

uint8_t MyLD2410::getFirmwareMajor()
{
  if (!firmwareMajor)
    requestFirmware();
  return firmwareMajor;
}

uint8_t MyLD2410::getFirmwareMinor()
{
  if (!firmwareMajor)
    requestFirmware();
  return firmwareMinor;
}

// Commands
bool MyLD2410::configMode(bool enable)
{
  if (enable && !isConfig)
    return sendCommand(configEnable);
  if (!enable && isConfig)
    return sendCommand(configDisable);
  return false;
}

bool MyLD2410::enhancedMode(bool enable)
{
  if (isConfig)
    return sendCommand(((enable) ? engOn : engOff));
  else
    return configMode() && sendCommand(((enable) ? engOn : engOff)) && configMode(false);
}

bool MyLD2410::requestFirmware()
{
  if (isConfig)
    return sendCommand(firmware_cmd);
  return configMode() && sendCommand(firmware_cmd) && configMode(false);
}

bool MyLD2410::requestParameters()
{
  if (isConfig)
    return sendCommand(param_cmd);
  return configMode() && sendCommand(param_cmd) && configMode(false);
}

bool MyLD2410::requestResolution()
{
  static const uint8_t res_cmd[4] = {2, 0, 0xAB, 0};
  if (isConfig)
    return sendCommand(res_cmd);
  return configMode() && sendCommand(res_cmd) && configMode(false);
}

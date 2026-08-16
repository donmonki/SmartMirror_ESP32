#ifndef MY_LD2410_H
#define MY_LD2410_H

#include <stdint.h>
#include <string.h>
#include <string>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef LD2410_BAUD_RATE
#define LD2410_BAUD_RATE 256000
#endif
#define LD2410_BUFFER_SIZE 0x40

/**
 * @brief The auxiliary light control status
 */
enum class LightControl
{
  NOT_SET = -1,
  NO_LIGHT_CONTROL,
  LIGHT_BELOW_THRESHOLD,
  LIGHT_ABOVE_THRESHOLD
};

/**
 * @brief The auxiliary output control status
 */
enum class OutputControl
{
  NOT_SET = -1,
  DEFAULT_LOW,
  DEFAULT_HIGH,
};

/**
 * @brief The status of the auto-thresholds routine
 */
enum class AutoStatus
{
  NOT_SET = -1,
  NOT_IN_PROGRESS,
  IN_PROGRESS,
  COMPLETED
};

class MyLD2410
{
public:
  enum Response
  {
    FAIL = 0,
    ACK,
    DATA
  };

  struct ValuesArray
  {
    uint8_t values[9];
    uint8_t N = 0;

    void setN(uint8_t n)
    {
      N = (n <= 8) ? n : 8;
    }
  };

  struct SensorData
  {
    uint8_t status;
    uint64_t timestamp;
    uint32_t mTargetDistance;
    uint8_t mTargetSignal;
    uint32_t sTargetDistance;
    uint8_t sTargetSignal;
    uint32_t distance;
    ValuesArray mTargetSignals;
    ValuesArray sTargetSignals;
  };

private:
  SensorData sData;
  ValuesArray stationaryThresholds;
  ValuesArray movingThresholds;
  uint8_t maxRange = 0;
  uint8_t noOne_window = 0;
  uint8_t lightLevel = 0;
  uint8_t outLevel = 0;
  uint8_t lightThreshold = 0;
  LightControl lightControl = LightControl::NOT_SET;
  OutputControl outputControl = OutputControl::NOT_SET;
  AutoStatus autoStatus = AutoStatus::NOT_SET;
  uint32_t version = 0;
  uint32_t bufferSize = 0;
  uint64_t dataFrames = 0;
  uint8_t MAC[6];
  std::string MACstr = "";
  std::string firmware = "";
  uint8_t firmwareMajor = 0;
  uint8_t firmwareMinor = 0;
  int fineRes = -1;
  bool isEnhanced = false;
  bool isConfig = false;
  uint8_t inBuf[LD2410_BUFFER_SIZE];
  uint8_t inBufI = 0;
  uint8_t headBuf[4];
  uint8_t headBufI = 0;
  uart_port_t uart_num;
  bool _debug = false;

  bool isDataValid();
  bool readFrame();
  bool sendCommand(const uint8_t *command);
  bool processAck();
  bool processData();

public:
  /**
   * @brief Construct a new MyLD2410 object
   *
   * @param uart_port UART port number (UART_NUM_0, UART_NUM_1, etc.)
   * @param debug - a flag that controls whether debug data will be printed
   */
  MyLD2410(uart_port_t uart_port, bool debug = false);

  /**
   * @brief Initialize the sensor
   * @return true if initialization successful
   */
  bool begin();

  /**
   * @brief Close the sensor connection
   */
  void end();

  /**
   * @brief Enable debug output
   */
  void debugOn() { _debug = true; }

  /**
   * @brief Disable debug output
   */
  void debugOff() { _debug = false; }

  /**
   * @brief Check for new sensor data
   * @return Response type (DATA, ACK, or FAIL)
   */
  Response check();

  // Getters
  bool inConfigMode() { return isConfig; }
  bool inBasicMode() { return !isEnhanced; }
  bool inEnhancedMode() { return isEnhanced; }
  uint8_t getStatus();
  const char *statusString();
  uint64_t getTimestamp() { return sData.timestamp; }
  uint64_t getFrameCount() { return dataFrames; }
  bool presenceDetected();
  bool stationaryTargetDetected();
  uint32_t stationaryTargetDistance() { return sData.sTargetDistance; }
  uint8_t stationaryTargetSignal() { return sData.sTargetSignal; }
  const ValuesArray &getStationarySignals() { return sData.sTargetSignals; }
  bool movingTargetDetected();
  uint32_t movingTargetDistance() { return sData.mTargetDistance; }
  uint8_t movingTargetSignal() { return sData.mTargetSignal; }
  const ValuesArray &getMovingSignals() { return sData.mTargetSignals; }
  uint32_t detectedDistance() { return sData.distance; }
  uint8_t getLightLevel() { return lightLevel; }
  uint8_t getOutLevel() { return outLevel; }
  uint8_t getResolution();
  uint32_t getRange_cm();
  const ValuesArray &getMovingThresholds();
  const ValuesArray &getStationaryThresholds();
  uint8_t getRange();
  uint8_t getNoOneWindow();
  std::string getFirmware();
  uint8_t getFirmwareMajor();
  uint8_t getFirmwareMinor();

  // Requests/Commands
  bool configMode(bool enable = true);
  bool enhancedMode(bool enable = true);
  bool requestFirmware();
  bool requestParameters();
  bool requestResolution();
};

#endif // MY_LD2410_H

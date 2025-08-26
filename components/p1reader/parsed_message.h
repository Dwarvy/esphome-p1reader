//-------------------------------------------------------------------------------------
// ESPHome P1 Electricity Meter custom sensor
// Copyright 2020 Pär Svanström
// 
// MIT License
//-------------------------------------------------------------------------------------

#pragma once

#include "esphome/core/log.h"
#include <cmath>

#define BUF_SIZE 256

namespace esphome
{
    namespace p1_reader
    {
        class ParsedMessage {
        public:
            bool telegramComplete;
            bool crcOk;

            // Standard power readings
            double cumulativeActiveImport;
            double cumulativeActiveExport;

            double cumulativeReactiveImport;
            double cumulativeReactiveExport;

            double momentaryActiveImport;
            double momentaryActiveExport;

            double momentaryReactiveImport;
            double momentaryReactiveExport;

            // Phase specific readings
            double momentaryActiveImportL1;
            double momentaryActiveExportL1;

            double momentaryActiveImportL2;
            double momentaryActiveExportL2;

            double momentaryActiveImportL3;
            double momentaryActiveExportL3;

            double momentaryReactiveImportL1;
            double momentaryReactiveExportL1;

            double momentaryReactiveImportL2;
            double momentaryReactiveExportL2;

            double momentaryReactiveImportL3;
            double momentaryReactiveExportL3;

            double voltageL1;
            double voltageL2;
            double voltageL3;

            double currentL1;
            double currentL2;
            double currentL3;
            
            // DSMR specific tariff readings
            double cumulativeActiveImportT1;
            double cumulativeActiveImportT2;
            double cumulativeActiveExportT1;
            double cumulativeActiveExportT2;
            
            // Gas and water consumption
            double gasConsumption;
            double waterConsumption;

            uint16_t crc16;

            void parseRow(const char* obisCode, const char* value)
            {
                // Convert string value to double
                double obisValue = atof(value);
                parseRow(obisCode, obisValue);
            }

            void parseRow(const char* obisCode, double obisValue)
            {
                // For debugging purposes, log the OBIS code and value
                ESP_LOGD("obis", "Processing OBIS code: %s = %f", obisCode, obisValue);
                
                size_t obisCodeLen = strlen(obisCode);
                
                // KAIFA meter specific OBIS codes for current values
                if (strstr(obisCode, "31.7.0") != nullptr) {
                    // Current L1 (A) - KAIFA/DSMR format - Following Python implementation
                    int currentValue = static_cast<int>(obisValue);
                    currentL1 = currentValue;
                    ESP_LOGI("obis", "Current L1: %d A (OBIS: %s)", currentValue, obisCode);
                    return;
                }
                
                if (strstr(obisCode, "51.7.0") != nullptr) {
                    // Current L2 (A) - KAIFA/DSMR format - Following Python implementation
                    int currentValue = static_cast<int>(obisValue);
                    currentL2 = currentValue;
                    ESP_LOGI("obis", "Current L2: %d A (OBIS: %s)", currentValue, obisCode);
                    return;
                }
                
                if (strstr(obisCode, "71.7.0") != nullptr) {
                    // Current L3 (A) - KAIFA/DSMR format - Following Python implementation
                    int currentValue = static_cast<int>(obisValue);
                    currentL3 = currentValue;
                    ESP_LOGI("obis", "Current L3: %d A (OBIS: %s)", currentValue, obisCode);
                    return;
                }
                
                // DSMR / KAIFA specific tariff readings (T1/T2)
                if (strstr(obisCode, "1.8.1") != nullptr || strstr(obisCode, "1-0:1.8.1") != nullptr) {
                    cumulativeActiveImportT1 = obisValue;
                    ESP_LOGI("obis", "T1 Import: %f kWh", cumulativeActiveImportT1);
                    return;
                }
                
                if (strstr(obisCode, "1.8.2") != nullptr || strstr(obisCode, "1-0:1.8.2") != nullptr) {
                    cumulativeActiveImportT2 = obisValue;
                    ESP_LOGI("obis", "T2 Import: %f kWh", cumulativeActiveImportT2);
                    return;
                }
                
                if (strstr(obisCode, "2.8.1") != nullptr || strstr(obisCode, "1-0:2.8.1") != nullptr) {
                    cumulativeActiveExportT1 = obisValue;
                    ESP_LOGI("obis", "T1 Export: %f kWh", cumulativeActiveExportT1);
                    return;
                }
                
                if (strstr(obisCode, "2.8.2") != nullptr || strstr(obisCode, "1-0:2.8.2") != nullptr) {
                    cumulativeActiveExportT2 = obisValue;
                    ESP_LOGI("obis", "T2 Export: %f kWh", cumulativeActiveExportT2);
                    return;
                }
                
                // Gas measurement patterns - DSMR format used by KAIFA
                if (strstr(obisCode, "24.2.1") != nullptr || strstr(obisCode, "24.3.0") != nullptr) {
                    gasConsumption = obisValue;
                    ESP_LOGI("obis", "Gas consumption: %f m³", gasConsumption);
                    return;
                }
                
                // Water measurement patterns (if available)
                if (strstr(obisCode, "1-0:8.0") != nullptr || strstr(obisCode, "8.0") != nullptr) {
                    waterConsumption = obisValue;
                    ESP_LOGI("obis", "Water consumption: %f m³", waterConsumption);
                    return;
                }
                
                // Generic OBIS code parsing for standard values
                if (obisCodeLen < 5) return;

                switch (obisCode[0])
                {
                    case '1':
                        switch (obisCode[2])
                        {
                            case '7':
                                switch (obisCode[4])
                                {
                                    case '0': 
                                        momentaryActiveImport = obisValue;
                                        break;
                                    default: break;
                                }
                                break;
                            case '8':
                                switch (obisCode[4])
                                {
                                    case '0': 
                                        cumulativeActiveImport = obisValue;
                                        break;
                                    // Specific tariff readings are handled above
                                    default: break;
                                }
                                break;
                            default: break;
                        }
                        break;
                    case '2':
                        switch (obisCode[2])
                        {
                            case '7':
                                switch (obisCode[4])
                                {
                                    case '0': 
                                        momentaryActiveExport = obisValue;
                                        break;
                                    default: break;
                                }
                                break;
                            case '8':
                                switch (obisCode[4])
                                {
                                    case '0': 
                                        cumulativeActiveExport = obisValue;
                                        break;
                                    // Specific tariff readings are handled above
                                    default: break;
                                }
                                break;
                            default: break;
                        }
                        break;
                    case '3':
                        switch (obisCode[2])
                        {
                            case '7':
                                switch (obisCode[4])
                                {
                                    case '0': 
                                        momentaryReactiveImport = obisValue;
                                        break;
                                    default: break;
                                }
                                break;
                            case '8':
                                switch (obisCode[4])
                                {
                                    case '0': 
                                        cumulativeReactiveImport = obisValue;
                                        break;
                                    default: break;
                                }
                                break;
                            default: break;
                        }
                        break;
                    case '4':
                        switch (obisCode[2])
                        {
                            case '7':
                                switch (obisCode[4])
                                {
                                    case '0': 
                                        momentaryReactiveExport = obisValue;
                                        break;
                                    default: break;
                                }
                                break;
                            case '8':
                                switch (obisCode[4])
                                {
                                    case '0': 
                                        cumulativeReactiveExport = obisValue;
                                        break;
                                    default: break;
                                }
                                break;
                            default: break;
                        }
                        break;
                    default: break;
                }
            }
            
            // Initialize CRC and telegram variables
            void initNewTelegram()
            {
                telegramComplete = false;
                crcOk = false;
                crc16 = 0;
            }
            
            // Update CRC16 with a new byte
            void updateCrc16(char b)
            {
                crc16 = (uint16_t)((crc16 >> 8) | (crc16 << 8));
                crc16 ^= (uint16_t) b & 0xFF;
                crc16 ^= (uint16_t) ((crc16 & 0xFF) >> 4);
                crc16 ^= (uint16_t) ((crc16 << 8) << 4);
                crc16 ^= (uint16_t) (((crc16 & 0xFF) << 4) << 1);
            }
            
            // Check if the CRC matches
            bool checkCrc(uint16_t messageCrc)
            {
                crcOk = (messageCrc == crc16);
                return crcOk;
            }
            
            // Constructor
            ParsedMessage()
            {
                telegramComplete = false;
                crcOk = false;
                
                // Initialize all values to 0
                cumulativeActiveImport = 0;
                cumulativeActiveExport = 0;
                cumulativeReactiveImport = 0;
                cumulativeReactiveExport = 0;
                momentaryActiveImport = 0;
                momentaryActiveExport = 0;
                momentaryReactiveImport = 0;
                momentaryReactiveExport = 0;
                momentaryActiveImportL1 = 0;
                momentaryActiveExportL1 = 0;
                momentaryActiveImportL2 = 0;
                momentaryActiveExportL2 = 0;
                momentaryActiveImportL3 = 0;
                momentaryActiveExportL3 = 0;
                momentaryReactiveImportL1 = 0;
                momentaryReactiveExportL1 = 0;
                momentaryReactiveImportL2 = 0;
                momentaryReactiveExportL2 = 0;
                momentaryReactiveImportL3 = 0;
                momentaryReactiveExportL3 = 0;
                voltageL1 = 0;
                voltageL2 = 0;
                voltageL3 = 0;
                currentL1 = 0;
                currentL2 = 0;
                currentL3 = 0;
                
                // DSMR specific tariff readings
                cumulativeActiveImportT1 = 0;
                cumulativeActiveImportT2 = 0;
                cumulativeActiveExportT1 = 0;
                cumulativeActiveExportT2 = 0;
                
                // Gas and water consumption
                gasConsumption = 0;
                waterConsumption = 0;
                
                crc16 = 0;
            }
        };
    } // namespace p1_reader
} // namespace esphome

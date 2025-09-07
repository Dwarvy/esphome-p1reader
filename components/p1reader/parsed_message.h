//-------------------------------------------------------------------------------------
// ESPHome P1 Electricity Meter custom sensor
// Copyright 2020 Pär Svanström
// 
// MIT License
//-------------------------------------------------------------------------------------

#pragma once

#include "esphome/core/log.h"
#include <cmath>

// Use a different name to avoid conflicting with ESPHome's BUF_SIZE
#define P1_BUF_SIZE 256

namespace esphome
{
    namespace p1_reader
    {
        class ParsedMessage {
        public:
            bool telegramComplete;
            bool crcOk;
            int sensorsToSend;

            // Standard power readings - totals calculated from T1+T2
            double totalCumulativeActiveImport;  // Total of T1+T2 imports
            double cumulativeActiveExport;       // Total of T1+T2 exports

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

            uint16_t crc;

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
                if (strstr(obisCode, "1.8.2") != nullptr || strstr(obisCode, "1-0:1.8.2") != nullptr) {
                    cumulativeActiveImportT1 = obisValue; // Now T1 = 1.8.2 (Day tariff)
                    ESP_LOGI("obis", "T1 Day Import: %f kWh", cumulativeActiveImportT1);
                    
                    totalCumulativeActiveImport = cumulativeActiveImportT1 + cumulativeActiveImportT2;
                    ESP_LOGI("obis", "Total Import updated: %f kWh", totalCumulativeActiveImport);
                    return;
                }
                
                if (strstr(obisCode, "1.8.1") != nullptr || strstr(obisCode, "1-0:1.8.1") != nullptr) {
                    cumulativeActiveImportT2 = obisValue; // Now T2 = 1.8.1 (Night tariff)
                    ESP_LOGI("obis", "T2 Night Import: %f kWh", cumulativeActiveImportT2);
                    
                    totalCumulativeActiveImport = cumulativeActiveImportT1 + cumulativeActiveImportT2;
                    ESP_LOGI("obis", "Total Import updated: %f kWh", totalCumulativeActiveImport);
                    return;
                }
                
                if (strstr(obisCode, "2.8.1") != nullptr || strstr(obisCode, "1-0:2.8.1") != nullptr) {
                    cumulativeActiveExportT1 = obisValue;
                    ESP_LOGI("obis", "T1 Export: %f kWh", cumulativeActiveExportT1);
                    // Set the generic cumulative export value to sum of tariffs
                    cumulativeActiveExport = cumulativeActiveExportT1 + cumulativeActiveExportT2;
                    ESP_LOGI("obis", "Total Export updated: %f kWh", cumulativeActiveExport);
                    return;
                }
                
                if (strstr(obisCode, "2.8.2") != nullptr || strstr(obisCode, "1-0:2.8.2") != nullptr) {
                    cumulativeActiveExportT2 = obisValue;
                    ESP_LOGI("obis", "T2 Export: %f kWh", cumulativeActiveExportT2);
                    // Set the generic cumulative export value to sum of tariffs
                    cumulativeActiveExport = cumulativeActiveExportT1 + cumulativeActiveExportT2;
                    ESP_LOGI("obis", "Total Export updated: %f kWh", cumulativeActiveExport);
                    return;
                }
                
                // Gas measurement patterns - Support all common DSMR formats
                // Standard DSMR 4/5 gas meter reading format: 0-1:24.2.1 (channel 0, OBIS 24.2.1 = Gas meter)
                if (strstr(obisCode, "24.2.1") != nullptr || strstr(obisCode, "24.3.0") != nullptr ||
                    strstr(obisCode, "0-1:24.2.1") != nullptr || strstr(obisCode, "0-1:24.3.0") != nullptr ||
                    strstr(obisCode, "0-2:24.2.1") != nullptr || strstr(obisCode, "0-2:24.3.0") != nullptr) {
                    gasConsumption = obisValue;
                    ESP_LOGI("obis", "Gas consumption: %f m³ (Code: %s)", gasConsumption, obisCode);
                    return;
                }
                
                // Water measurement patterns (if available)
                // Standard DSMR water meter: 0-n:24.2.1 where n=3,4,etc for water channels
                if (strstr(obisCode, "1-0:8.0") != nullptr || strstr(obisCode, "8.0") != nullptr ||
                    strstr(obisCode, "0-1:24.2.1.8") != nullptr || strstr(obisCode, "0-3:24.2.1") != nullptr ||
                    strstr(obisCode, "0-4:24.2.1") != nullptr) {
                    waterConsumption = obisValue;
                    ESP_LOGI("obis", "Water consumption: %f m³ (Code: %s)", waterConsumption, obisCode);
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
                                        totalCumulativeActiveImport = obisValue;
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
                crc = 0;
                sensorsToSend = 33; // Include all important sensors including T1/T2 and other data
            }
            
            // Update CRC16 with a new byte
            void updateCrc16(char b)
            {
                crc = (uint16_t)((crc >> 8) | (crc << 8));
                crc ^= (uint16_t) b & 0xFF;
                crc ^= (uint16_t) ((crc & 0xFF) >> 4);
                crc ^= (uint16_t) ((crc << 8) << 4);
                crc ^= (uint16_t) (((crc & 0xFF) << 4) << 1);
            }
            
            // Check if the CRC matches
            bool checkCrc(uint16_t messageCrc)
            {
                crcOk = (messageCrc == crc);
                return crcOk;
            }
            
            // Update cumulative totals from tariffs
            void updateCumulativeTotals() {
                // Calculate total cumulative import from T1 + T2
                totalCumulativeActiveImport = cumulativeActiveImportT1 + cumulativeActiveImportT2;
                
                // Calculate total cumulative export from T1 + T2
                cumulativeActiveExport = cumulativeActiveExportT1 + cumulativeActiveExportT2;
                
                ESP_LOGI("totals", "Updated cumulative totals - Import: %f kWh (T1: %f, T2: %f), Export: %f kWh", 
                         totalCumulativeActiveImport, cumulativeActiveImportT1, cumulativeActiveImportT2, cumulativeActiveExport);
            }
            
            // Constructor
            ParsedMessage()
            {
                telegramComplete = false;
                crcOk = false;
                sensorsToSend = 32;
                
                // Initialize all values to 0
                totalCumulativeActiveImport = 0;
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
                
                crc = 0;
                sensorsToSend = 33; // Initialize to include all important sensors including T1/T2 and other data
            }
        };
    } // namespace p1_reader
} // namespace esphome

#pragma once

namespace esphome
{
    namespace p1_reader
    {
        class ParsedMessage {
        public:
            // Standard readings
            double cumulativeActiveImport;
            double cumulativeActiveExport;

            double cumulativeReactiveImport;
            double cumulativeReactiveExport;

            double momentaryActiveImport;
            double momentaryActiveExport;

            double momentaryReactiveImport;
            double momentaryReactiveExport;

            // Phase-specific readings
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
            
            // DSMR-specific multi-tariff readings
            double cumulativeActiveImportT1;  // Tariff 1 import
            double cumulativeActiveImportT2;  // Tariff 2 import
            double cumulativeActiveExportT1;  // Tariff 1 export
            double cumulativeActiveExportT2;  // Tariff 2 export
            
            // Additional measurements
            double gasConsumption;      // Gas consumption
            double waterConsumption;    // Water consumption
            double externalTemperature; // External temperature if available
            
            // Timestamp from telegram if available
            char timestamp[20];         // For storing timestamp from telegram

            uint16_t crc;
            bool telegramComplete;
            bool crcOk;
            uint8_t sensorsToSend;

            void parseRow(const char* obisCode, const char* value)
            {
                double obisValue = simpleatof(value);

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
                
                // Electricity tariff 1 import (DSMR: 1-0:1.8.1)
                if (strstr(obisCode, "1.8.1") != nullptr || strstr(obisCode, "1-0:1.8.1") != nullptr) {
                    cumulativeActiveImportT1 = obisValue;
                    ESP_LOGI("obis", "T1 Import: %f kWh", cumulativeActiveImportT1);
                    return;
                }
                
                // Electricity tariff 2 import (DSMR: 1-0:1.8.2)
                if (strstr(obisCode, "1.8.2") != nullptr || strstr(obisCode, "1-0:1.8.2") != nullptr) {
                    cumulativeActiveImportT2 = obisValue;
                    ESP_LOGI("obis", "T2 Import: %f kWh", cumulativeActiveImportT2);
                    return;
                }
                
                // Electricity tariff 1 export (DSMR: 1-0:2.8.1)
                if (strstr(obisCode, "2.8.1") != nullptr || strstr(obisCode, "1-0:2.8.1") != nullptr) {
                    cumulativeActiveExportT1 = obisValue;
                    ESP_LOGI("obis", "T1 Export: %f kWh", cumulativeActiveExportT1);
                    return;
                }
                
                // Electricity tariff 2 export (DSMR: 1-0:2.8.2)
                if (strstr(obisCode, "2.8.2") != nullptr || strstr(obisCode, "1-0:2.8.2") != nullptr) {
                    cumulativeActiveExportT2 = obisValue;
                    ESP_LOGI("obis", "T2 Export: %f kWh", cumulativeActiveExportT2);
                    return;
                }
                
                // Gas measurement patterns - DSMR format used by KAIFA
                // Common patterns: 0-1:24.2.1, 0-1:24.3.0, or 0-2:24.2.1
                if (strstr(obisCode, "24.2.1") != nullptr || 
                    strstr(obisCode, "24.3.0") != nullptr) {
                    gasConsumption = obisValue;
                    ESP_LOGI("obis", "Gas consumption: %f m³", gasConsumption);
                    return;
                }
                
                // Water measurement patterns
                if (strstr(obisCode, "24.4") != nullptr) {
                    waterConsumption = obisValue;
                    ESP_LOGI("obis", "Water consumption: %f", waterConsumption);
                    return;
                }
                
                // External temperature - varies by meter
                if (strstr(obisCode, "96.1.10") != nullptr) {
                    externalTemperature = obisValue;
                    ESP_LOGI("obis", "External temperature: %f", externalTemperature);
                    return;
                }
                
                // Standard electricity measurements
                if (obisCodeLen >= 5 && 
                    obisCode[obisCodeLen-1] == '0' &&
                    obisCode[obisCodeLen-2] == '.' &&
                    obisCode[obisCodeLen-4] == '.')
                {
                    switch (obisCodeLen)
                    {
                        case 5: switch(obisCode[obisCodeLen-3])
                        {
                            case '7': switch (obisCode[0])
                            {
                                case '1': 
                                    momentaryActiveImport = obisValue;
                                    break;
                                case '2': 
                                    momentaryActiveExport = obisValue;
                                    break;
                                case '3': 
                                    momentaryReactiveImport = obisValue;
                                    break;
                                case '4': 
                                    momentaryReactiveExport = obisValue;
                                    break;
                                default: break;
                            }
                            break;
                            case '8': switch (obisCode[0])
                            {
                                case '1': 
                                    cumulativeActiveImport = obisValue;
                                    break;
                                case '2': 
                                    cumulativeActiveExport = obisValue;
                                    break;
                                case '3': 
                                    cumulativeReactiveImport = obisValue;
                                    break;
                                case '4': 
                                    cumulativeReactiveExport = obisValue;
                                    break;
                                default: break;
                            }
                            break;
                            default: break;
                        }
                        break;
                        case 6: if (obisCode[obisCodeLen-3] == '7') {
                            switch(obisCode[1]) {
                                case '1': switch (obisCode[0])
                                {
                                    case '2': 
                                        momentaryActiveImportL1 = obisValue;
                                        if (strstr(obisCode, "31.7.0") != nullptr) // Current L1 (A) - KAIFA/DSMR format
                                        {
                                            // Parse KAIFA format: value is typically in the form "002*A" where A is the unit
                                            // Convert to integer as the Python implementation does
                                            int currentValue = static_cast<int>(obisValue);
                                            currentL1 = currentValue;
                                            ESP_LOGI("obis", "Current L1: %d A (OBIS: %s, Raw: %f)", currentValue, obisCode, obisValue);
                                        }
                                        else if (strstr(obisCode, "32.7.0") != nullptr) // Alternative OBIS code for Current L1
                                        {
                                            // Fix for the current parsing - limit to realistic values
                                            if (obisValue > -1000 && obisValue < 1000) {
                                                currentL1 = obisValue;
                                                ESP_LOGI("obis", "Current L1 (alt): %f A (OBIS: %s)", obisValue, obisCode);
                                            } else {
                                                ESP_LOGW("obis", "Unrealistic current value ignored: %f (OBIS: %s)", obisValue, obisCode);
                                            }
                                        }
                                        break;
                                    case '4': 
                                        momentaryActiveImportL2 = obisValue;
                                        break;
                                    case '5': 
                                        if (strstr(obisCode, "31.7.0") != nullptr) // Current L2 (A) - KAIFA/DSMR format
                                        {
                                            // Parse KAIFA format: value is typically in the form "002*A" where A is the unit
                                            // Convert to integer as the Python implementation does
                                            int currentValue = static_cast<int>(obisValue);
                                            currentL2 = currentValue;
                                            ESP_LOGI("obis", "Current L2: %d A (OBIS: %s, Raw: %f)", currentValue, obisCode, obisValue);
                                        }
                                        else if (strstr(obisCode, "32.7.0") != nullptr) // Alternative OBIS code for Current L2
                                        {
                                            // Fix for the current parsing - limit to realistic values
                                            if (obisValue > -1000 && obisValue < 1000) {
                                                currentL2 = obisValue;
                                                ESP_LOGI("obis", "Current L2 (alt): %f A (OBIS: %s)", obisValue, obisCode);
                                            } else {
                                                ESP_LOGW("obis", "Unrealistic current value ignored: %f (OBIS: %s)", obisValue, obisCode);
                                            }
                                        if (obisValue > -1000 && obisValue < 1000) {
                                            currentL2 = obisValue;
                                            ESP_LOGI("obis", "Current L2: %f A (OBIS: %s)", obisValue, obisCode);
                                        } else {
                                            ESP_LOGW("obis", "Unrealistic current value ignored: %f (OBIS: %s)", obisValue, obisCode);
                                        }
                                        break;
                                    case '5': 
                                        momentaryActiveImportL3 = obisValue;
                                        if (strstr(obisCode, "31.7.0") != nullptr) // Current L3 (A) - KAIFA/DSMR format
                                        {
                                            // Parse KAIFA format: value is typically in the form "002*A" where A is the unit
                                            // Convert to integer as the Python implementation does
                                            int currentValue = static_cast<int>(obisValue);
                                            currentL3 = currentValue;
                                            ESP_LOGI("obis", "Current L3: %d A (OBIS: %s, Raw: %f)", currentValue, obisCode, obisValue);
                                        }
                                        else if (strstr(obisCode, "32.7.0") != nullptr) // Alternative OBIS code for Current L3
                                        {
                                            // Fix for the current parsing - limit to realistic values
                                            if (obisValue > -1000 && obisValue < 1000) {
                                                currentL3 = obisValue;
                                                ESP_LOGI("obis", "Current L3 (alt): %f A (OBIS: %s)", obisValue, obisCode);
                                            } else {
                                                ESP_LOGW("obis", "Unrealistic current value ignored: %f (OBIS: %s)", obisValue, obisCode);
                                            }
                                        }
                                        break;
                                    default: break;
                                }
                                break;
                                case '2': switch (obisCode[0])
                                {
                                    case '2': 
                                        momentaryActiveExportL1 = obisValue;
                                        break;
                                    case '3': 
                                        voltageL1 = obisValue;
                                        break;
                                    case '4': 
                                        momentaryActiveExportL2 = obisValue;
                                        break;
                                    case '5': 
                                        voltageL2 = obisValue;
                                        break;
                                    case '6': 
                                        momentaryActiveExportL3 = obisValue;
                                        break;
                                    case '7': 
                                        voltageL3 = obisValue;
                                        break;
                                    default: break;
                                }
                                break;
                                case '3': switch (obisCode[0])
                                {
                                    case '2': 
                                        momentaryReactiveImportL1 = obisValue;
                                        break;
                                    case '4': 
                                        momentaryReactiveImportL2 = obisValue;
                                        break;
                                    case '6': 
                                        momentaryReactiveImportL3 = obisValue;
                                        break;
                                    default: break;
                                }
                                break;
                                case '4': switch (obisCode[0])
                                {
                                    case '2': 
                                        momentaryReactiveExportL1 = obisValue;
                                        break;
                                    case '4': 
                                        momentaryReactiveExportL2 = obisValue;
                                        break;
                                    case '6': 
                                        momentaryReactiveExportL3 = obisValue;
                                        break;
                                    default: break;
                                }
                                break;
                                default: break;
                            }
                        }
                        break;
                        default: break;
                    }
                }
            }

            // Limitations: 
            //   Numbers larger than 2G will fail (but spec only goes to 99999999.999 so ok)
            //   And numbers have no more than 3 decimals in the spec.
            //   spec == Swedish spec for H1
            double simpleatof(const char* value)
            {
                double decFactors[10] = {10.0, 100.0, 1000.0, 100000.0,
                                        1000000.0, 10000000.0, 100000000.0,
                                        1000000000.0, 10000000000.0,
                                        100000000000.0};
                int idx = 0;
                int intPart = 0;
                bool negative = false;

                if (value[idx] == '-')
                {
                    negative = true;
                    idx++;
                }

                while (value[idx] != '.')
                {
                    intPart = intPart*10 + (value[idx]-'0');
                    idx++;
                }

                int len = strlen(value), startIdx = ++idx;
                int decPart = 0;
                while (idx < len)
                {
                    decPart = decPart*10 + (value[idx]-'0');
                    idx++;
                }

                if (negative)
                {
                    return -(intPart + decPart/decFactors[len-startIdx-1]);
                }
                else
                {
                    return intPart + decPart/decFactors[len-startIdx-1];
                }
            }

            void initNewTelegram()
            {
                crc = 0x0000;
                telegramComplete = false;
                crcOk = false;
                sensorsToSend = 32; // Updated to include tariff (T1/T2) and gas/water consumption sensors
            }

            void updateCrc16(uint8_t a)
            {
                int i;
                crc ^= a;
                for (i = 0; i < 8; ++i)
                {
                    if (crc & 1)
                    {
                        crc = (crc >> 1) ^ 0xA001;
                    }
                    else
                    {
                        crc = (crc >> 1);
                    }
                }
            }
            
            void checkCrc(uint16_t crcFromMessage)
            {
                crcOk = crc == crcFromMessage;
                telegramComplete = true;
            }
        };
    }
}
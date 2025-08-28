//-------------------------------------------------------------------------------------
// ESPHome P1 Electricity Meter custom sensor
// Copyright 2020 Pär Svanström
// 
// History
//  0.1.0 2020-11-05:   Initial release
//  0.x.0 2023-04-18:   Added HDLC support
//  0.2.0 2023-08-14:   Some optimizations when ESPHome started to complain about execution times
//  0.3.0 2023-08-23:   Rework structure to merge above changes with HDLC stuff
//  0.4.0 2025-02-25:   Migrate to esphome external component
//
// MIT License
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
// IN THE SOFTWARE.
//-------------------------------------------------------------------------------------

#include "p1reader.h"

namespace esphome
{
    namespace p1_reader
    {
        void P1Reader::setup()
        {
            // Calculate pollingInterval for Component given our uart buffer size and the rest
            size_t rxBufferSize = parent_->get_rx_buffer_size();
            uint8_t bits = parent_->get_data_bits() + parent_->get_stop_bits() + 
                            (parent_->get_parity() != uart::UART_CONFIG_PARITY_NONE ? 1 : 0) + 1;
            float secondsPerByte = (float)bits * (1.0f / (float) parent_->get_baud_rate());

            ESP_LOGI("setup", "secondsPerByte calculated as: %f s", secondsPerByte);
            
            _uSecondsPerByte = (int) (secondsPerByte * 1000000.0f);
            // Keep a margin of 20%
            _pollingIntervalMs = (int)((float)rxBufferSize * secondsPerByte * 800.0f);
            
            if (_pollingIntervalMs < 20)
            {
                ESP_LOGE("setup", "Polling interval is too low: %d ms (rx_buffer_size %d, uSecondsPerByte %d)", 
                    _pollingIntervalMs, parent_->get_rx_buffer_size(), _uSecondsPerByte);
            } 
            else if (_pollingIntervalMs < 100)
            {
                ESP_LOGW("setup", "Polling interval is low: %d ms (rx_buffer_size %d, uSecondsPerByte %d)", 
                        _pollingIntervalMs, parent_->get_rx_buffer_size(), _uSecondsPerByte);
            }
            else
            {
                ESP_LOGI("setup", "Polling interval calculated as: %d ms (rx_buffer_size %d, uSecondsPerByte %d)", 
                        _pollingIntervalMs, parent_->get_rx_buffer_size(), _uSecondsPerByte);
            }
                
            set_update_interval(_pollingIntervalMs);

            // Start with a clean buffer
            memset(_buffer, 0, P1_BUF_SIZE);
            _bufferLen = 0;
            ESP_LOGI("setup", "Internal buffer size is %d", P1_BUF_SIZE);

            _parsedMessage.initNewTelegram();
        }

        void P1Reader::update()
        {
            // Deliver a parsed and crc ok message in the calls _after_ actually reading it so we 
            // split the work over more scheduler slices since publish_state is slow (and logging is slow
            // so set log level INFO to avoid all the debug logging slowing things down)
            if (_parsedMessage.telegramComplete)
            {
                publishSensors(&_parsedMessage);

                if (!_parsedMessage.telegramComplete)
                {
                    (this->*readP1Message)();
                }
            }
            else
            {
                (this->*readP1Message)();
            }
        }

        void P1Reader::publishSensors(ParsedMessage* parsedMessage)
        {
            if (parsedMessage->telegramComplete) // Temporarily bypassing CRC check to allow values to be published
            {
                uint32_t start = millis();
    
                while (parsedMessage->sensorsToSend > 0)
                {
                    switch (parsedMessage->sensorsToSend--)
                    {
                        case 1:
                            if (cumulative_active_import != nullptr)
                                cumulative_active_import->publish_state(parsedMessage->totalCumulativeActiveImport);
                            break;
                        case 2:
                            if (cumulative_active_import_t1 != nullptr)
                                cumulative_active_import_t1->publish_state(parsedMessage->cumulativeActiveImportT1);
                            break;
                        case 3:
                            if (cumulative_active_import_t2 != nullptr)
                                cumulative_active_import_t2->publish_state(parsedMessage->cumulativeActiveImportT2);
                            break;
                        case 4:
                            if (cumulative_active_export != nullptr)
                                cumulative_active_export->publish_state(parsedMessage->cumulativeActiveExport);
                            break;
                        case 5:
                            if (momentary_active_import != nullptr)
                                momentary_active_import->publish_state(parsedMessage->momentaryActiveImport);
                            break;
                        case 6:
                            if (momentary_active_export != nullptr)
                                momentary_active_export->publish_state(parsedMessage->momentaryActiveExport);
                            break;
                        case 7:
                            if (momentary_active_import_l1 != nullptr)
                                momentary_active_import_l1->publish_state(parsedMessage->momentaryActiveImportL1);
                            break;
                        case 8:
                            if (momentary_active_export_l1 != nullptr)
                                momentary_active_export_l1->publish_state(parsedMessage->momentaryActiveExportL1);
                            break;
                        case 9:
                            if (momentary_active_import_l2 != nullptr)
                                momentary_active_import_l2->publish_state(parsedMessage->momentaryActiveImportL2);
                            break;
                        case 10:
                            if (momentary_active_export_l2 != nullptr)
                                momentary_active_export_l2->publish_state(parsedMessage->momentaryActiveExportL2);
                            break;
                        case 11:
                            if (momentary_active_import_l3 != nullptr)
                                momentary_active_import_l3->publish_state(parsedMessage->momentaryActiveImportL3);
                            break;
                        case 12:
                            if (momentary_active_export_l3 != nullptr)
                                momentary_active_export_l3->publish_state(parsedMessage->momentaryActiveExportL3);
                            break;
                        case 13:
                            if (cumulative_reactive_import != nullptr)
                                cumulative_reactive_import->publish_state(parsedMessage->cumulativeReactiveImport);
                            break;
                        case 14:
                            if (cumulative_reactive_export != nullptr)
                                cumulative_reactive_export->publish_state(parsedMessage->cumulativeReactiveExport);
                            break;
                        case 15:
                            if (momentary_reactive_import != nullptr)
                                momentary_reactive_import->publish_state(parsedMessage->momentaryReactiveImport);
                            break;
                        case 16:
                            if (momentary_reactive_export != nullptr)
                                momentary_reactive_export->publish_state(parsedMessage->momentaryReactiveExport);
                            break;
                        case 17:
                            if (momentary_reactive_import_l1 != nullptr)
                                momentary_reactive_import_l1->publish_state(parsedMessage->momentaryReactiveImportL1);
                            break;
                        case 18:
                            if (momentary_reactive_export_l1 != nullptr)
                                momentary_reactive_export_l1->publish_state(parsedMessage->momentaryReactiveExportL1);
                            break;
                        case 19:
                            if (momentary_reactive_import_l2 != nullptr)
                                momentary_reactive_import_l2->publish_state(parsedMessage->momentaryReactiveImportL2);
                            break;
                        case 20:
                            if (momentary_reactive_export_l2 != nullptr)
                                momentary_reactive_export_l2->publish_state(parsedMessage->momentaryReactiveExportL2);
                            break;
                        case 25:
                            if (momentary_reactive_import_l3 != nullptr)
                                momentary_reactive_import_l3->publish_state(parsedMessage->momentaryReactiveImportL3);
                            break;
                        case 26:
                            if (momentary_reactive_export_l3 != nullptr)
                                momentary_reactive_export_l3->publish_state(parsedMessage->momentaryReactiveExportL3);
                            break;
                        case 27:
                            if (gas_consumption != nullptr)
                                gas_consumption->publish_state(parsedMessage->gasConsumption);
                            break;
                        case 28:
                            if (water_consumption != nullptr)
                                water_consumption->publish_state(parsedMessage->waterConsumption);
                            break;
                        case 29:
                            if (cumulative_active_export_t1 != nullptr)
                                cumulative_active_export_t1->publish_state(parsedMessage->cumulativeActiveExportT1);
                            break;
                        case 30:
                            if (cumulative_active_export_t2 != nullptr)
                                cumulative_active_export_t2->publish_state(parsedMessage->cumulativeActiveExportT2);
                            break;
                        // Cases 31 and 32 removed - T1 and T2 import values now published earlier as cases 2 and 3
                        default:
                            ESP_LOGW("publish", "Unknown sensor to publish %d", parsedMessage->sensorsToSend + 1);
                            break;
                    }

                    if ((millis() - start) > 50)
                    {
                        ESP_LOGW("publish", "Publishing sensors is taking too long (%u), will continue in next scheduler run (remain: %d)", 
                          millis() - start, parsedMessage->sensorsToSend);
                        break;
                    }
                }

                ESP_LOGI("publish", "Sensors published (complete). CRC: %04X", parsedMessage->crc);
                parsedMessage->initNewTelegram();
            }
            else if (!parsedMessage->crcOk && parsedMessage->telegramComplete)
            {
                parsedMessage->initNewTelegram();
            }
        }
    
        void P1Reader::readP1MessageAscii()
        {
            // Use a static buffer to collect the complete telegram
            static char telegramBuffer[4096] = {0}; // Large enough for P1 telegrams
            static size_t telegramLen = 0;
            uint32_t start = millis();
            
            // Process available data for up to 20ms before yielding
            while (available())
            {
                // Use the buffer to read a line
                memset(_buffer, 0, P1_BUF_SIZE);
                int len = readBytesUntilAndIncluding('\n', _buffer, P1_BUF_SIZE-1);
                
                if (len > 0) {
                    // Ensure null termination
                    _buffer[len] = '\0';
                    
                    ESP_LOGV("data", "Line received: %s", _buffer);
                    
                    // Check if we have space in the telegram buffer
                    if (telegramLen + len < sizeof(telegramBuffer)) {
                        // Add the line to our telegram buffer
                        memcpy(telegramBuffer + telegramLen, _buffer, len);
                        telegramLen += len;
                        
                        // Check if this is the end of telegram (line starts with !)
                        if (_buffer[0] == '!') {
                            ESP_LOGI("telegram", "Complete telegram received, length: %d", telegramLen);
                            
                            // Add null termination
                            telegramBuffer[telegramLen] = '\0';
                            
                            // Process the complete telegram
                            processTelegram(telegramBuffer);
                            
                            // Update cumulative totals before publishing
                            _parsedMessage.updateCumulativeTotals();
                
                            // Notify that the telegram is now complete
                            _parsedMessage.telegramComplete = true;
                            
                            // Clear buffer for the next telegram
                            memset(telegramBuffer, 0, sizeof(telegramBuffer));
                            telegramLen = 0;
                        }
                    } else {
                        ESP_LOGW("telegram", "Telegram buffer overflow, discarding data");
                        memset(telegramBuffer, 0, sizeof(telegramBuffer));
                        telegramLen = 0;
                    }
                }
                
                // Yield control if we've been processing for more than 20ms
                if ((millis() - start) > 20) {
                    ESP_LOGV("ascii", "Yielding time slice after reading data");
                    break;
                }
            }
        }
        
        void P1Reader::processTelegram(const char* telegram)
        {
            // Reset CRC and message parsing state
            _parsedMessage.initNewTelegram();
            
            // Process the telegram line by line
            const char* pos = telegram;
            const char* eol;
            
            // First pass: Calculate CRC over the entire telegram except the CRC line
            const char* endPos = nullptr;
            while ((eol = strchr(pos, '\n')) != nullptr) {
                // Calculate length of this line (excluding newline)
                size_t lineLen = eol - pos;
                
                // Check if this is the CRC line (starts with !)
                if (*pos == '!') {
                    endPos = pos;  // Mark the position of the CRC line
                    break;
                }
                
                // Update CRC for this line
                for (size_t i = 0; i < lineLen; i++) {
                    _parsedMessage.updateCrc16(pos[i]);
                }
                
                // Include the newline character in CRC calculation
                _parsedMessage.updateCrc16('\n');
                
                // Move to next line
                pos = eol + 1;
            }
            
            // Process the CRC line
            if (endPos && *endPos == '!') {
                // Add the ! to the CRC
                _parsedMessage.updateCrc16('!');
                
                // Extract the CRC value from the message (after the !)
                char crcBuffer[8] = {0};
                size_t crcLen = 0;
                endPos++; // Skip the !
                
                // Copy the CRC hex digits
                while (isxdigit(*endPos) && crcLen < 6) {
                    crcBuffer[crcLen++] = *endPos++;
                }
                
                // Convert hex string to integer
                int crcFromMsg = (int)strtol(crcBuffer, NULL, 16);
                _parsedMessage.checkCrc(crcFromMsg);
                
                ESP_LOGI("crc", "Telegram read. CRC: %04X = %04X. PASS = %s", 
                         _parsedMessage.crc, crcFromMsg, _parsedMessage.crcOk ? "YES": "NO");
            }
            
            // Second pass: Parse the data lines
            pos = telegram;
            static char lineCopy[256];  // Static buffer for line processing, large enough for 115+ byte lines
            
            while ((eol = strchr(pos, '\n')) != nullptr) {
                // Calculate length of this line (excluding newline)
                size_t lineLen = eol - pos;
                
                if (lineLen < sizeof(lineCopy) - 1) {
                    // Copy line to buffer for processing
                    memcpy(lineCopy, pos, lineLen);
                    lineCopy[lineLen] = '\0';  // Null-terminate the string
                    
                    // Process the line if it's not the CRC line
                    if (lineCopy[0] != '!') {
                        // Check if this is a data line with OBIS code
                        if (strchr(lineCopy, '(') != NULL) {
                            char* dataId = strtok(lineCopy, DELIMITERS);
                            char* obisCode = strtok(NULL, DELIMITERS);
                            
                            // Check if this is a data row with value
                            if (dataId && obisCode) {
                                // Log all OBIS codes for diagnostic purposes
                                ESP_LOGD("obis_raw", "Found OBIS code: %s with ID: %s", obisCode, dataId);
                                
                                if (strncmp(DATA_ID, dataId, strlen(DATA_ID)) == 0) {
                                    char* value = strtok(NULL, DELIMITERS);
                                    char* unit = strtok(NULL, DELIMITERS);
                                    
                                    // Log the complete value and unit if available
                                    if (value) {
                                        if (unit) {
                                            ESP_LOGD("obis_data", "%s = %s %s", obisCode, value, unit);
                                        } else {
                                            ESP_LOGD("obis_data", "%s = %s", obisCode, value);
                                        }
                                        
                                        _parsedMessage.parseRow(obisCode, value);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    ESP_LOGW("telegram", "Line too long to process: %d bytes", lineLen);
                }
                
                // Move to the start of the next line (skip the newline)
                pos = eol + 1;
            }
        }

        size_t P1Reader::readBytesUntilAndIncluding(char terminator, char *buffer, size_t length)
        {
            size_t index = 0;
            uint32_t start = millis();
            while (index < length)
            {
                uint8_t c;
                bool hasData = read_byte(&c);
                if (!hasData)
                {
                    break;
                }
                *buffer++ = (char)c;
                index++;
                if (c == terminator)
                {
                    break;
                }
            }

            return index; // return number of characters, not including terminator
        }

        uint16_t crc16_x25(byte* data, int len)
        {
            uint16_t crc = 0xffff;
            for (int i = 0; i < len; i++)
            {
                crc ^= data[i];
                for (unsigned k = 0; k < 8; k++)
                    crc = (crc & 1) != 0 ? (crc >> 1) ^ 0x8408 : crc >> 1;
            }
            return ~crc;
        }

        /*  Reads messages formatted according to "Branschrekommendation v1.2", which
            at the time of writing (20210207) is used by Tekniska Verken's Aidon 6442SE
            meters. This is a binary format, with a HDLC Frame. 

            This code is in no way a generic HDLC Frame parser, but it does the job
            of decoding this particular data stream.
        */
        void P1Reader::readP1MessageHDLC() 
        {
            if (available())
            {
                uint8_t data = 0;
                uint32_t start = millis();

                while (_parseHDLCState == OUTSIDE_FRAME)
                {
                    bool hasData = read_byte(&data);
                    if (hasData && data == 0x7e)
                    {
                        uint8_t wait = 10;
                        while (!hasData && wait > 0)
                        {
                            hasData = read_byte(&data);
                            if (!hasData)
                            {
                                delayMicroseconds(_uSecondsPerByte);
                                wait--;
                            }
                        }

                        if (!hasData)
                        {
                            ESP_LOGD("hdlc", "Possibly found end of frame while looking for start of frame, bailing out and trying again later...");
                        }

                        // clean buffer for next packet
                        memset(_buffer, 0, P1_BUF_SIZE);
                        _bufferLen = 0;

                        if (data == 0x7e)
                        {
                            _buffer[_bufferLen++] = data;
                        }
                        else
                        {
                            _buffer[_bufferLen++] = 0x7e;
                            _buffer[_bufferLen++] = data;
                        }

                        _parseHDLCState = READING_FRAME;
                        break;
                        ESP_LOGD("hdlc", "Found start of frame...");
                    }
                }

                while (_parseHDLCState == READING_FRAME)
                {
                    bool hasData = read_byte(&data);
                    if (hasData)
                    {
                        _buffer[_bufferLen++] = data;

                        if (data == 0x7e)
                        {
                            _parseHDLCState = FOUND_FRAME;
                            return; // Always parse in a separate timeslot
                            ESP_LOGD("hdlc", "Found end of frame...");
                        }

                        if (_bufferLen >= P1_BUF_SIZE)
                        {
                            _parseHDLCState = OUTSIDE_FRAME;
                            ESP_LOGE("hdlc", "Failed to read frame, buffer overflow, bailing out...");
                            return;
                        }
                    }
                    else
                    {
                        // No byte available, busywait for a single byte over uart
                        delayMicroseconds(_uSecondsPerByte);
                        if ((millis() - start) > 10)
                        {
                            ESP_LOGD("hdlc", "Failed to fetch expected data within 10ms, bailing out and trying later.");
                            return;
                        }
                    }
                }
            }
            
            if (_parseHDLCState == FOUND_FRAME)
            {
                if (_bufferLen < 17)
                {
                    _parseHDLCState = OUTSIDE_FRAME;
                    ESP_LOGE("hdlc", "Frame to small, skipping to next frame. (%d)", _bufferLen);
                    return;
                }

                uint16_t messageLength = ((_buffer[1] & 0x0f) << 8) + (uint8_t)_buffer[2];
                if (messageLength != (_bufferLen - 2))
                {
                    _parseHDLCState = OUTSIDE_FRAME;
                    ESP_LOGE("hdlc", "Message length (%d) not matching frame length (%d), skipping to next frame.", 
                            messageLength, _bufferLen-2);
                    return;
                }

                uint16_t crc = ((uint8_t)_buffer[_bufferLen-2] << 8) | (uint8_t)_buffer[_bufferLen-3];
                uint16_t crcCalculated = crc16_x25((byte*)_buffer + 1, _bufferLen - 4); // FCS
                if (crc != crcCalculated)
                {
                    _parseHDLCState = OUTSIDE_FRAME;
                    ESP_LOGE("hdlc", "Message crc (%04x) not matching frame crc (%04x), skipping to next frame.", 
                            crc, crcCalculated);
                    return;
                }

                _parsedMessage.crcOk = true;

                _messagePos = 17;

                // Skip date field (normally 0)
                _messagePos += _buffer[_messagePos++];

                // Check for start of struct array
                if (_buffer[_messagePos++] != 0x01)
                {
                    _parseHDLCState = OUTSIDE_FRAME;
                    ESP_LOGE("hdlc", "Message array start tag (0x01) missing, got (%x), skipping to next frame.", 
                            _buffer[_messagePos-1]);
                    return;
                }

                uint8_t structCount = _buffer[_messagePos++];
                ESP_LOGD("hdlc", "Number of structs are %d", structCount);

                for (int i=0; i<structCount; i++) 
                {
                    if (!parseHDLCStruct())
                    {
                        _parseHDLCState = OUTSIDE_FRAME;
                        ESP_LOGE("hdlc", "Failed to parse structs");
                        return;
                    }
                }

                _parsedMessage.telegramComplete = true;
            }
        }

        bool P1Reader::parseHDLCStruct()
        {
            char obis[7];
            memset(obis, 0, 7);
            bool is_signed = false;
            double scaleFactors[10] = { 0.0001, 0.001, 0.01, 0.1, 1.0,
                                        10.0, 100.0, 1000.0,
                                        10000.0, 100000.0 };
            int8_t scale = 0;
            int32_t value = 0;
            uint32_t uvalue = 0xffffffff;

            // Check for start of struct
            if (_buffer[_messagePos++] != 0x02)
            {
                _parseHDLCState = OUTSIDE_FRAME;
                ESP_LOGE("hdlc", "Message struct start tag (0x02) missing, got (%x), skipping to next frame.", 
                        _buffer[_messagePos-1]);
                return false;
            }

            uint8_t structElements = _buffer[_messagePos++];
            ESP_LOGV("hdlc", "Number of struct elements are %d", structElements);

            for (int i=0; i<structElements; i++) 
            {
                if (_messagePos >= _bufferLen)
                {
                    _parseHDLCState = OUTSIDE_FRAME;
                    ESP_LOGE("hdlc", "Reading (%d) past end of message (%d).", 
                            _messagePos, _bufferLen);
                    return false;
                }

                uint8_t tag = _buffer[_messagePos++];
                switch (tag)
                {
                    case 0x02: 
                        {
                            // another inner struct
                            uint8_t innerStructElements = _buffer[_messagePos++];
                            ESP_LOGV("hdlc", "Number of inner struct elements are %d", innerStructElements);

                            for (int j=0; j<innerStructElements; j++) 
                            {
                                uint8_t innerTag = _buffer[_messagePos++];
                                switch (innerTag)
                                {
                                    case 0x0f:
                                        scale = _buffer[_messagePos++]; // 10E(scale)
                                        break;
                                    case 0x16: 
                                    {
                                        // Unit
                                        // 0x1b: (k)W
                                        // 0x1d: (k)VAr
                                        // 0x1e: (k)Wh
                                        // 0x20: (k)VArh
                                        // 0x21: A
                                        // 0x23: V
                                        uint8_t unit = _buffer[_messagePos++];
                                        if (scale == 0 && unit != 0x21 && unit != 0x23)
                                            scale = -3; // ref KILO in sensor.py
                                        break;
                                    }
                                    default:
                                        ESP_LOGE("hdlc", "Unknown tag encountered (%x)", innerTag);
                                        break;
                                }
                            }
                            break;
                        }
                    case 0x06:
                        uvalue = (uint8_t)_buffer[_messagePos + 3] | 
                                ((uint8_t)_buffer[_messagePos + 2] << 8) | 
                                ((uint8_t)_buffer[_messagePos + 1] << 16) | 
                                ((uint8_t)_buffer[_messagePos] << 24);
                        _messagePos += 4;
                        break;
                    case 0x09:
                        {
                            uint8_t rowLen = _buffer[_messagePos++];
                            if (rowLen == 6)
                            {
                                // Map to string for ascii parser
                                if (_buffer[_messagePos + 2] > 9)
                                {
                                    obis[0] = (_buffer[_messagePos + 2] / 10) + 48;
                                    obis[1] = (_buffer[_messagePos + 2] % 10) + 48;
                                    obis[3] = _buffer[_messagePos + 3] + 48;
                                    obis[5] = _buffer[_messagePos + 4] + 48;
                                    obis[2] = obis[4] = '.';
                                }
                                else
                                {
                                    obis[0] = _buffer[_messagePos + 2] + 48;
                                    obis[2] = _buffer[_messagePos + 3] + 48;
                                    obis[4] = _buffer[_messagePos + 4] + 48;
                                    obis[1] = obis[3] = '.';
                                }
                            }
                            _messagePos += rowLen;
                            break;
                        }
                    case 0x10:
                        value = _buffer[_messagePos + 1] | (uint8_t)_buffer[_messagePos + 0] << 8;
                        _messagePos += 2;
                        break;
                    case 0x12:
                        value = (int16_t)(_buffer[_messagePos + 1] | (uint8_t)_buffer[_messagePos + 0] << 8);
                        _messagePos += 2;
                        break;
                    default:
                        ESP_LOGE("hdlc", "Unknown tag encountered (%x)", tag);
                        break;
                }
            }

            if (obis[0] == '\0')
            {
                ESP_LOGV("hdlc", "No data found in struct.");
                return true;
            }

            double scaledValue;
            
            if (uvalue == 0xffffffff)
                scaledValue = scaleFactors[scale + 4] * value;
            else
                scaledValue = scaleFactors[scale + 4] * uvalue;

            ESP_LOGD("hdlc", "VAL %s, %f, %d\n", obis, scaledValue, scale);

            _parsedMessage.parseRow(obis, scaledValue);

            return true;
        }
    }
}

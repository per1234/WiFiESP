/**
 * @file at_drv.h
 * @brief The definition of class at_drv for the ESP8266. 
 * @author Pierce Nichols<pierce@logos-electro.com
 * @author Wu Pengfei<pengfei.wu@itead.cc> 
 * @date 2015.08
 * 
 * @par Copyright:
 * Copyright (c) 2015 ITEAD Intelligent Systems Co., Ltd. \n\n
 * Copyright (c) 2015 Logos Electromechanical LLC \n\n
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version. \n\n
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
#include "utility/at_drv.h"
#include <IPAddress.h>
#include <avr/pgmspace.h>

#define LOG_OUTPUT_DEBUG            (1)
#define LOG_OUTPUT_DEBUG_PREFIX     (1)

#define logDebug(arg)\
    do {\
        if (LOG_OUTPUT_DEBUG)\
        {\
            if (LOG_OUTPUT_DEBUG_PREFIX)\
            {\
                Serial.print("[LOG Debug: ");\
                Serial.print((const char*)__FILE__);\
                Serial.print(",");\
                Serial.print((unsigned int)__LINE__);\
                Serial.print(",");\
                Serial.print((const char*)__FUNCTION__);\
                Serial.print("] ");\
            }\
            Serial.print(arg);\
        }\
    } while(0)
		
// This flag tells the event handler whether incoming data should be treated
// as coming on one of the muxes (true) or whether it's a response to something.
bool IPDenable = true;

// Grab the serial stream when it comes in, assuming we are not waiting on other things
void serialEvent1(void) {
	if (IPDenable) {
		// place incoming data in appropriate buffer
	}
}

ATDrvClass::ATDrvClass(HardwareSerial &uart, uint32_t baud): m_puart(&uart)
{
    m_puart->begin(baud);
	m_puart->setTimeout(WL_AT_TIMEOUT);
    rx_empty();
}

void ATDrvClass::rx_empty(void) 
{
    while(m_puart->available() > 0) {
        m_puart->read();
    }
}

uint32_t ATDrvClass::recvPkg(uint8_t *buffer, uint32_t buffer_size, uint32_t *data_len, uint32_t timeout, uint8_t *coming_mux_id)
{
    String data;
    char a;
    int32_t index_PIPDcomma = -1;
    int32_t index_colon = -1; /* : */
    int32_t index_comma = -1; /* , */
    int32_t len = -1;
    int8_t id = -1;
    bool has_data = false;
    uint32_t ret;
    unsigned long start;
    uint32_t i;
    
    if (buffer == NULL) {
        return 0;
    }
    
    start = millis();
    while (millis() - start < timeout) {
        if(m_puart->available() > 0) {
            a = m_puart->read();
            data += a;
        }
        
        index_PIPDcomma = data.indexOf("+IPD,");
        if (index_PIPDcomma != -1) {
            index_colon = data.indexOf(':', index_PIPDcomma + 5);
            if (index_colon != -1) {
                index_comma = data.indexOf(',', index_PIPDcomma + 5);
                /* +IPD,id,len:data */
                if (index_comma != -1 && index_comma < index_colon) { 
                    id = data.substring(index_PIPDcomma + 5, index_comma).toInt();
                    if (id < 0 || id > 4) {
                        return 0;
                    }
                    len = data.substring(index_comma + 1, index_colon).toInt();
                    if (len <= 0) {
                        return 0;
                    }
                } else { /* +IPD,len:data */
                    len = data.substring(index_PIPDcomma + 5, index_colon).toInt();
                    if (len <= 0) {
                        return 0;
                    }
                }
                has_data = true;
                break;
            }
        }
    }
    
    if (has_data) {
        i = 0;
        ret = len > buffer_size ? buffer_size : len;
        start = millis();
        while (millis() - start < timeout) {
            while(m_puart->available() > 0 && i < ret) {
                a = m_puart->read();
                buffer[i++] = a;
            }
            if (i == ret) {
                rx_empty();
                if (data_len) {
                    *data_len = len;    
                }
                if (index_comma != -1 && coming_mux_id) {
                    *coming_mux_id = id;
                }
                return ret;
            }
        }
    }
    return 0;
}

String ATDrvClass::recvString(String target, uint32_t timeout)
{
    String data;
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(m_puart->available() > 0) {
            a = m_puart->read();
			if(a == '\0') continue;
            data += a;
        }
        if (data.indexOf(target) != -1) {
            break;
        }   
    }
    
    return data;
}

String ATDrvClass::recvString(String target1, String target2, uint32_t timeout)
{
    String data;
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(m_puart->available() > 0) {
            a = m_puart->read();
			if(a == '\0') continue;
            data += a;
        }
        if (data.indexOf(target1) != -1) {
            break;
        } else if (data.indexOf(target2) != -1) {
            break;
        }
    }
    return data;
}

String ATDrvClass::recvString(String target1, String target2, String target3, uint32_t timeout)
{
    String data;
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(m_puart->available() > 0) {
            a = m_puart->read();
			if(a == '\0') continue;
            data += a;
        }
        if (data.indexOf(target1) != -1) {
            break;
        } else if (data.indexOf(target2) != -1) {
            break;
        } else if (data.indexOf(target3) != -1) {
            break;
        }
    }
    return data;
}

bool ATDrvClass::recvFind(String target, uint32_t timeout)
{
    String data_tmp;
    data_tmp = recvString(target, timeout);
    if (data_tmp.indexOf(target) != -1) {
        return true;
    }
    return false;
}

bool ATDrvClass::recvFindAndFilter(String target, String begin, String end, String &data, uint32_t timeout)
{
    String data_tmp;
    data_tmp = recvString(target, timeout);
    if (data_tmp.indexOf(target) != -1) {
        int32_t index1 = data_tmp.indexOf(begin);
        int32_t index2 = data_tmp.indexOf(end);
        if (index1 != -1 && index2 != -1) {
            index1 += begin.length();
            data = data_tmp.substring(index1, index2);
            return true;
        }
    }
    data = data_tmp;
    return false;
}

bool ATDrvClass::eAT(void)
{
	bool ret = false;
	IPDenable = false;
    rx_empty();
    m_puart->println(F("AT"));
    ret = recvFind("OK", WL_AT_TIMEOUT);
	IPDenable = true;
	return ret;
}

bool ATDrvClass::sATCIPDINFO(uint8_t mode) {
	bool ret = false;
	IPDenable = false;
    rx_empty();
    m_puart->print(F("AT+CIPDINFO="));
	m_puart->println(mode);
    ret = recvFind("OK", WL_AT_TIMEOUT);
	IPDenable = true;
	return ret;
}

bool ATDrvClass::eATRST(uint32_t timeout) 
{
	bool ret = false;
	IPDenable = false;
    rx_empty();
    m_puart->println(F("AT+RST"));
    ret = recvFind("OK", timeout);
	IPDenable = true;
	return ret;
}

bool ATDrvClass::eATGMR(String &version)
{
	bool ret = false;
	IPDenable = false;
    rx_empty();
    //delay(3000);
    m_puart->println(F("AT+GMR"));
    ret = recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", version, WL_AT_TIMEOUT); 
	IPDenable = true;
	return ret;
}

bool ATDrvClass::qATCWMODE(uint8_t *mode, uint8_t pattern) 
{
    String str_mode;
    bool ret;
	IPDenable = false;
    if (!mode||!pattern) {
		IPDenable = true;
        return false;
    }
    rx_empty();
    switch(pattern)
    {
        case ESP_AT_CUR:
            m_puart->println(F("AT+CWMODE_DEF?"));
            break;
        case ESP_AT_DEF:
            m_puart->println(F("AT+CWMODE_CUR?"));
            break;
        default:
            m_puart->println(F("AT+CWMODE?"));
    }
    ret = recvFindAndFilter("OK", ":", "\r\n\r\nOK", str_mode, WL_AT_TIMEOUT); 
    if (ret) {
        *mode = (uint8_t)str_mode.toInt(); 
		IPDenable = true;
        return true;
    } else {
		IPDenable = true;
        return false;
    }
}

bool ATDrvClass::sATCWMODE(uint8_t mode, uint8_t pattern)
{
    String data;
	IPDenable = false;
    if(!pattern){
		IPDenable = true;
        return false;
	}
    rx_empty();
    switch(pattern)
    {
        case ESP_AT_CUR:
            m_puart->print(F("AT+CWMODE_DEF="));
            break;
        case ESP_AT_DEF:
            m_puart->print(F("AT+CWMODE_CUR="));
            break;
        default:
            m_puart->print(F("AT+CWMODE="));
    }
    m_puart->println(mode);
    data = recvString("OK", "no change", WL_AT_TIMEOUT);

    if (data.indexOf("OK") != -1 || data.indexOf("no change") != -1) {
		IPDenable = true;
        return true;
    }
	IPDenable = true;
    return false;
}

bool ATDrvClass::qATCWJAP(String &ssid, uint8_t pattern) 
{
    bool ret;
	IPDenable = false;
    if (!pattern) {
		IPDenable = true;
        return false;
    }
    rx_empty();
    switch(pattern)
    {
        case ESP_AT_CUR:
            m_puart->println(F("AT+CWJAP_DEF?"));
            break;
        case ESP_AT_DEF:
            m_puart->println(F("AT+CWJAP_CUR?"));
            break;
        default:
            m_puart->println(F("AT+CWJAP?"));
    }
    ssid = recvString("OK", "No AP", WL_AT_TIMEOUT);
    if (ssid.indexOf("OK") != -1 || ssid.indexOf("No AP") != -1) {
		IPDenable = true;
        return true;
    }
	IPDenable = true;
    return false;
 
}

bool ATDrvClass::sATCWJAP(String ssid, String pwd,uint8_t pattern)
{
    String data;
	IPDenable = false;
    if (!pattern) {
		IPDenable = true;
        return false;
    }
    rx_empty();
    switch(pattern)
    {
        case ESP_AT_CUR:
            m_puart->print(F("AT+CWJAP_DEF=\""));
            break;
        case ESP_AT_DEF:
            m_puart->print(F("AT+CWJAP_CUR=\""));
            break;
        default:
            m_puart->print(F("AT+CWJAP=\""));
    }
    
    m_puart->print(ssid);
    m_puart->print(F("\",\""));
    m_puart->print(pwd);
    m_puart->println(F("\""));
    
    data = recvString("OK", "FAIL", WL_CONNECT_TIMEOUT);
    if (data.indexOf("OK") != -1) {
		IPDenable = true;
        return true;
    }
	IPDenable = true;
    return false;
}

bool ATDrvClass::eATCWQAP(void) {
	bool ret = false;
	IPDenable = false;
    rx_empty();
    m_puart->println(F("AT+CWQAP"));
    ret = recvFind("OK");
	IPDenable = true;
    return ret;
}

bool ATDrvClass::eATCWLAP(String &list) 
{
	bool ret = false;
	IPDenable = false;
    rx_empty();
    m_puart->println(F("AT+CWLAP"));
    ret = recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list, WL_SCAN_TIMEOUT);
	IPDenable = true;
	return ret;
}

bool ATDrvClass::qATCWDHCP(uint8_t *mode, uint8_t *en, uint8_t pattern) 
{
	String data;
	bool ret = false;
	IPDenable = false;
    if (!pattern) {
		IPDenable = true;
        return false;
    }
    rx_empty();
    switch(pattern)
    {
        case ESP_AT_CUR:
            m_puart->println(F("AT+CWDHCP_DEF?"));
            break;
        case ESP_AT_DEF:
            m_puart->println(F("AT+CWDHCP_CUR?"));
            break;
        default:
            m_puart->println(F("AT+CWDHCP?"));
    }

    ret = recvFindAndFilter("OK", "\r\r\n", "\r\nOK", data, WL_AT_TIMEOUT);
	if (ret && (data.indexOf(',') != -1)) {
		*mode = data.toInt();
		*en = (data.substring(data.indexOf(',')).toInt();
		IPDenable = true;
		return true;
	}
	IPDenable = true;
	return false;
}

bool ATDrvClass::sATCWDHCP(uint8_t mode, uint8_t en, uint8_t pattern)
{
    String data;
	bool ret = false;
	IPDenable = false;
    if (!pattern) {
		IPDenable = true;
        return false;
    }
    rx_empty();
    switch(pattern){
         case ESP_AT_CUR:
            m_puart->print(F("AT+CWDHCP_DEF="));

            break;
        case ESP_AT_DEF:
            m_puart->print(F("AT+CWDHCP_CUR="));
            break;
        default:
            m_puart->print(F("AT+CWDHCP="));

    }
    m_puart->print(mode);
    m_puart->print(F(","));
    m_puart->println(en);    
    data = recvString("OK", "ERROR", WL_AT_TIMEOUT);

    if (data.indexOf("OK") != -1) {
		IPDenable = true;
        return true;
    }
	IPDenable = true;
    return false;
}

bool ATDrvClass::qATCIPSTAMAC(String &mac, uint8_t pattern)
{
	bool ret = false;
	IPDenable = false;
    rx_empty();
    if (!pattern) {
		IPDenable = true;
        return false;
    }
    switch(pattern){
         case ESP_AT_CUR:
            m_puart->println(F("AT+CIPSTAMAC_DEF?"));

            break;
        case ESP_AT_DEF:
            m_puart->println(F("AT+CIPSTAMAC_CUR?"));
            break;
        default:
            m_puart->println(F("AT+CIPSTAMAC?"));

    }
    ret = recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", mac, WL_AT_TIMEOUT);
	IPDenable = true;
    return ret;
}

bool ATDrvClass::qATCIPSTAIP(String &ip, uint8_t pattern)
{
	bool ret = false;
	IPDenable = false;
    rx_empty();
    if (!pattern) {
        return false;
    }
    switch(pattern){
         case ESP_AT_CUR:
            m_puart->println(F("AT+CIPSTA_DEF?"));

            break;
        case ESP_AT_DEF:
            m_puart->println(F("AT+CIPSTA_CUR?"));
            break;
        default:
            m_puart->println(F("AT+CIPSTA?"));

    }
    ret = recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", ip, WL_AT_TIMEOUT);
	IPDenable = true;
    return ret;
}

bool ATDrvClass::sATCIPSTAIP(uint8_t validParams, uint32_t local_ip, uint32_t gateway, uint32_t subnet, uint8_t pattern)
{
	bool ret = false;
	String ip;
	IPAddress _local_ip_ = IPAddress(local_ip);
	IPAddress _gateway_ = IPAddress(gateway);
	IPAddress _subnet_ = IPAddress(subnet);
	IPDenable = false;
    rx_empty();
    if (!pattern) {
        return false;
    }
    switch(pattern){
         case ESP_AT_CUR:
            m_puart->print(F("AT+CIPSTA_DEF=\""));

            break;
        case ESP_AT_DEF:
            m_puart->print(F("AT+CIPSTA_CUR=\""));
            break;
        default:
            m_puart->print(F("AT+CIPSTA=\""));
    }
	switch (validParams) {
		case 1:
			_local_ip_.printTo(m_puart);
			m_puart->println(F("\""));
			break;
		case 2:
			_local_ip_.printTo(m_puart);
			m_puart->print(F("\",\""));
			_gateway_.printTo(m_puart);
			m_puart->println(F("\""));
			break;
		case 3:
			_local_ip_.printTo(m_puart);
			m_puart->print(F("\",\""));
			_gateway_.printTo(m_puart);
			m_puart->print(F("\",\""));
			_subnet_.printTo(m_puart);
			m_puart->println(F("\""));
			break;
		default:
			m_puart->println(F(""));
			IPDenable = true;
			return false;
	}
    ret = recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", &ip, WL_AT_TIMEOUT);
	IPDenable = true;
    return ret;
}

bool ATDrvClass::eATCIPSTATUS(String &list)
{
    String data;
	bool ret = false;
	IPDenable = false;
    //delay(100);
    rx_empty();
    m_puart->println(F("AT+CIPSTATUS"));
    ret = recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list, WL_AT_TIMEOUT);
	IPDenable = true;
    return ret;
}

bool ATDrvClass::sATCIPSTARTMultiple(uint8_t mux_id, String type, String addr, uint32_t port) {
    String data;
	IPDenable = false;
    rx_empty();
    m_puart->print(F("AT+CIPSTART="));
    m_puart->print(mux_id);
    m_puart->print(F(",\""));
    m_puart->print(type);
    m_puart->print(F("\",\""));
    m_puart->print(addr);
    m_puart->print(F("\","));
    m_puart->println(port);
    
    data = recvString("OK", "ERROR", "ALREADY CONNECT", WL_CONNECT_TIMEOUT);
    if (data.indexOf("OK") != -1 || data.indexOf("ALREADY CONNECT") != -1) {
		IPDenable = true;
        return true;
    }
    return false;
	IPDenable = true;
}

bool ATDrvClass::sATCIPSTARTMultiple(uint8_t mux_id, String type, IPAddress addr, uint32_t port)  {
    String data;
	IPDenable = false;
    rx_empty();
    m_puart->print(F("AT+CIPSTART="));
    m_puart->print(mux_id);
    m_puart->print(F(",\""));
    m_puart->print(type);
    m_puart->print(F("\",\""));
    m_puart->print(addr);
    m_puart->print(F("\","));
    m_puart->println(port);
    
    data = recvString("OK", "ERROR", "ALREADY CONNECT", WL_CONNECT_TIMEOUT);
    if (data.indexOf("OK") != -1 || data.indexOf("ALREADY CONNECT") != -1) {
		IPDenable = true;
        return true;
    }
    return false;
	IPDenable = true;
}

bool ATDrvClass::sATCIPSENDMultiple(uint8_t mux_id, const uint8_t *buffer, uint32_t len)
{
	IPDenable = false;
	bool ret = false;
    rx_empty();
    m_puart->print(F("AT+CIPSEND="));
    m_puart->print(mux_id);
    m_puart->print(F(","));
    m_puart->println(len);
    if (recvFind(">", WL_AT_TIMEOUT)) {
        rx_empty();
        for (uint32_t i = 0; i < len; i++) {
            m_puart->write(buffer[i]);
        }
        ret = recvFind("SEND OK", WL_AT_TIMEOUT);
    }
	IPDenable = true;
    return ret;
}

bool ATDrvClass::sATCIPSENDMultipleFromFlash(uint8_t mux_id, const uint8_t *buffer, uint32_t len)
{
	IPDenable = false;
	bool ret = false;
    rx_empty();
    m_puart->print(F("AT+CIPSEND="));
    m_puart->print(mux_id);
    m_puart->print(F(","));
    m_puart->println(len);
    if (recvFind(">", WL_AT_TIMEOUT)) {
        rx_empty();
        for (uint32_t i = 0; i < len; i++) {
            m_puart->write((char) pgm_read_byte(&buffer[i]));
        }
        ret = recvFind("SEND OK", WL_AT_TIMEOUT);
    }
	IPDenable = true;
    return ret;
}

bool ATDrvClass::sATCIPCLOSEMulitple(uint8_t mux_id)
{
	IPDenable = false;
    String data;
    rx_empty();
    m_puart->print(F("AT+CIPCLOSE="));
    m_puart->println(mux_id);
    
    data = recvString("OK", "link is not", WL_CONNECT_TIMEOUT);
    if (data.indexOf("OK") != -1 || data.indexOf("link is not") != -1) {
		IPDenable = true;
        return true;
    }
	IPDenable = true;
    return false;
}

bool ATDrvClass::eATCIFSR(String &list)
{
	IPDenable = false;
	bool ret = false;
    rx_empty();
    m_puart->println(F("AT+CIFSR"));
    ret = recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list, WL_AT_TIMEOUT);
	IPDenable = true;
    return ret;
}

bool ATDrvClass::sATCIPMUX(uint8_t mode)
{
    String data;
	IPDenable = false;
    rx_empty();
    m_puart->print(F("AT+CIPMUX="));
    m_puart->println(mode);
    
    data = recvString("OK", "Link is builded", WL_AT_TIMEOUT);
    if (data.indexOf("OK") != -1) {
		IPDenable = true;
        return true;
    }
	IPDenable = true;
    return false;
}

bool ATDrvClass::sATCIPSERVER(uint8_t mode, uint32_t port)
{
    String data;
	bool ret = false;
	IPDenable = false;
    if (mode) {
        rx_empty();
        m_puart->print(F("AT+CIPSERVER="));
		m_puart->print(mode);
		m_puart->print(F(","));
        m_puart->println(port);
        
        data = recvString("OK", "no change", WL_AT_TIMEOUT);
        if (data.indexOf("OK") != -1 || data.indexOf("no change") != -1) {
			IPDenable = true;
            return true;
        }
		IPDenable = true;
        return false;
    } else {
        rx_empty();
        m_puart->println(F("AT+CIPSERVER=0"));
        ret = recvFind("\r\r\n", WL_AT_TIMEOUT);
		IPDenable = true;
        return ret;
    }
}

bool ATDrvClass::sATCIPMODE(uint8_t mode)
{
    String data;
	IPDenable = false;
    if((mode > 1)||(mode < 0)) {
		IPDenable = true;
        return false;	
	}
    rx_empty();
    m_puart->print(F("AT+CIPMODE="));
    m_puart->println(mode);
    
    data = recvString("OK", "Link is builded", WL_AT_TIMEOUT);
    if (data.indexOf("OK") != -1 ) {
		IPDenable = true;
        return true;
    }
	IPDenable = true;
    return false;
}

bool ATDrvClass::eATPING(String ip)
{
	bool ret = false;
	IPDenable = false;
    rx_empty();
    m_puart->print(F("AT+PING="));
    m_puart->print(F("\""));
    m_puart->print(ip);
    m_puart->println(F("\""));
    ret = recvFind("OK", WL_CONNECT_TIMEOUT);
	IPDenable = true;
    return ret;
}

bool ATDrvClass::sATCIPSTO(uint32_t timeout)
{
	bool ret = false;
	IPDenable = false;
    rx_empty();
    m_puart->print(F("AT+CIPSTO="));
    m_puart->println(timeout);
    ret = recvFind(F("OK"), WL_CONNECT_TIMEOUT);
	IPDenable = true;
    return ret;
}

bool ATDrvClass::qCIPBUFSTATUS(uint8_t mux_id) {
	bool ret = false;
	IPDenable = false;
    rx_empty();
    m_puart->print(F("AT+CIPBUFSTATUS="));
	m_puart->println(mux_id);
    ret = recvFind(F("0\r\n\r\nOK"), WL_AT_TIMEOUT);
	IPDenable = true;
    return ret;
}



/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "user_json.h"

LOCAL char strTemperature[10] = "";
LOCAL char strHumidity[10]    = "";
LOCAL char strWatts[10]       = "";
LOCAL int intLightState       = 0;
LOCAL int intStartingState    = 0;
LOCAL int intFanState         = 0;
LOCAL int intFanSpeed         = 0;
LOCAL int intLedState         = 0;
LOCAL char strAlarmState[20]  = "";
LOCAL int intBacklight        = 1;
LOCAL int intLightCommand     = 0;
LOCAL int intFanCommand       = 0;
LOCAL int intFanSpeedCommand  = 0;
LOCAL int intLedCommand       = 0;
LOCAL int intLedModeCommand   = 0;
LOCAL int intLedSpeedCommand  = 0;
LOCAL int intLedColorRed      = 0;
LOCAL int intLedColorBlue     = 0;
LOCAL int intLedColorGreen    = 0;

LOCAL int intLedSpeedState         = 0;
LOCAL int intLedModeState          = 0;
LOCAL int intLedColorRedState      = 0;
LOCAL int intLedColorBlueState     = 0;
LOCAL int intLedColorGreenState    = 0;

LOCAL int intTimestampHour    = 0;
LOCAL int intTimestampMinute  = 0;
LOCAL int intTimestampPM      = 0;


MQTT_Client mqttClient;

static volatile bool OLED;
#define user_procTaskPrio        1
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

#define DELAY 60000 /* milliseconds */
LOCAL os_timer_t timer;

LOCAL void oled_clear(void)
{
    OLED_CLS();
    OLED_Print(2, 0, "Socket 1 ESP8266", 1);
}


/******************************************************************************
 * FunctionName : data_get
 * Description  : Process the data received through MQTT and send it to Arduino
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
data_get(struct jsontree_context *js_ctx)
{
    const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);

   if (os_strncmp(path, "LightCommand", 12) == 0)
    {
        jsontree_write_int(js_ctx, intLightCommand);
    }
    else if (os_strncmp(path, "FanCommand", 10) == 0)
    {
        jsontree_write_int(js_ctx, intFanCommand);
    }
    else if (os_strncmp(path, "FanSpeedCommand", 15) == 0)
    {
        jsontree_write_int(js_ctx, intFanSpeedCommand);
    }
    else if (os_strncmp(path, "LedCommand", 10) == 0)
    {
        jsontree_write_int(js_ctx, intLedCommand);
    }
    else if (os_strncmp(path, "LedModeCommand", 14) == 0)
    {
        jsontree_write_int(js_ctx, intLedModeCommand);
    }
    else if (os_strncmp(path, "LedSpeedCommand", 15) == 0)
    {
        jsontree_write_int(js_ctx, intLedSpeedCommand);
    }
    else if (os_strncmp(path, "LedColorRed", 11) == 0)
    {
       jsontree_write_int(js_ctx, intLedColorRed);
    }
    else if (os_strncmp(path, "LedColorGreen", 13) == 0)
    {
       jsontree_write_int(js_ctx, intLedColorGreen);
    }
    else if (os_strncmp(path, "LedColorBlue", 12) == 0)
    {
       jsontree_write_int(js_ctx, intLedColorBlue);
    }
    else if (os_strncmp(path, "TimestampHour", 13) == 0)
    {
       jsontree_write_int(js_ctx, intTimestampHour);
    }
    else if (os_strncmp(path, "TimestampMinute", 15) == 0)
    {
       jsontree_write_int(js_ctx, intTimestampMinute);
    }
    else if (os_strncmp(path, "TimestampPM", 11) == 0)
    {
       jsontree_write_int(js_ctx, intTimestampPM);
    }

    return 0;
}


/******************************************************************************
 * FunctionName : data_set
 * Description  : Receive data from Arduino and store it in global variables
 * Parameters   : js_ctx -- A pointer to a JSON set up
 *                parser -- A pointer to a JSON parser state
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
data_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
{
    int type;
    char str_buf[64];

    while ((type = jsonparse_next(parser)) != 0) {
        if (type == JSON_TYPE_PAIR_NAME) {

            if (jsonparse_strcmp_value(parser, "StateTemperature") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
                jsonparse_copy_value(parser, strTemperature, 10);
//                INFO("Temperature:%s:\n", strTemperature);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%s", strTemperature);

                oled_clear();
            	OLED_Print(3, 6, "MQTT Sending...", 1);
            	OLED_Print(1, 2, "/socket1/Temperature", 1);
            	OLED_Print(5, 3, strTemperature, 1);
            	MQTT_Publish(&mqttClient, "/socket1/StateTemperature", strTemperature, os_strlen(strTemperature), 2, 0);
            }
            else if (jsonparse_strcmp_value(parser, "StateHumidity") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
                jsonparse_copy_value(parser, strHumidity, 10);
//                INFO("Humidity:%s:\n", strHumidity);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%s", strHumidity);

                oled_clear();
            	OLED_Print(1, 4, "/socket1/Humidity", 1);
            	OLED_Print(5, 5, strHumidity, 1);
            	MQTT_Publish(&mqttClient, "/socket1/StateHumidity", strHumidity, os_strlen(strHumidity), 2, 0);
            }
            else if (jsonparse_strcmp_value(parser, "StateWatts") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
                jsonparse_copy_value(parser, strWatts, 10);
//                INFO("Watts:%s:\n", strWatts);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%s", strWatts);

            	oled_clear();
            	OLED_Print(3, 6, "MQTT Sending...", 1);
            	OLED_Print(1, 2, "/socket1/Watts", 1);
            	OLED_Print(5, 3, strWatts, 1);
            	MQTT_Publish(&mqttClient, "/socket1/StateWatts", strWatts, os_strlen(strWatts), 2, 0);
            }
            else if (jsonparse_strcmp_value(parser, "ChangedLight") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
            	intLightState = jsonparse_get_value_as_int(parser);
//            	INFO("LightState:%d:\n", intLightState);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%d", intLightState);

            	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/ChangedLight", 1);
    			OLED_Print(5, 3, str_buf, 1);

            	MQTT_Publish(&mqttClient, "/socket1/ChangedLight", str_buf, os_strlen(str_buf), 2, 0);
            }
            else if (jsonparse_strcmp_value(parser, "ChangedFan") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
            	intFanState = jsonparse_get_value_as_int(parser);
//            	INFO("FanState:%d:\n", intFanState);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%d", intFanState);

            	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/ChangedFan", 1);
    			OLED_Print(5, 3, str_buf, 1);

            	MQTT_Publish(&mqttClient, "/socket1/ChangedFan", str_buf, os_strlen(str_buf), 2, 0);
            }
            else if (jsonparse_strcmp_value(parser, "ChangedFanSpeed") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
            	intFanSpeed = jsonparse_get_value_as_int(parser);
//            	INFO("FanSpeed:%d:\n", intFanSpeed);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%d", intFanSpeed);

            	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/ChangedFanSpeed", 1);
    			OLED_Print(5, 3, str_buf, 1);

            	MQTT_Publish(&mqttClient, "/socket1/ChangedFanSpeed", str_buf, os_strlen(str_buf), 2, 0);
            }
            else if (jsonparse_strcmp_value(parser, "ChangedLed") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
            	intLedState = jsonparse_get_value_as_int(parser);
//            	INFO("LedState:%d:\n", intLedState);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%d", intLedState);

            	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/ChangedLed", 1);
    			OLED_Print(5, 3, str_buf, 1);

            	MQTT_Publish(&mqttClient, "/socket1/ChangedLed", str_buf, os_strlen(str_buf), 2, 0);
            }
   	        else if (jsonparse_strcmp_value(parser, "ChangedLedMode") == 0) {
   	            jsonparse_next(parser);
   	            jsonparse_next(parser);
               	intLedModeState = jsonparse_get_value_as_int(parser);
               	os_bzero(str_buf, sizeof(str_buf));
               	os_sprintf(str_buf,"%d", intLedModeState);
               	MQTT_Publish(&mqttClient, "/socket1/ChangedLedMode", str_buf, os_strlen(str_buf), 2, 0);
	        }
  	        else if (jsonparse_strcmp_value(parser, "ChangedLedSpeed") == 0) {
   	            jsonparse_next(parser);
   	            jsonparse_next(parser);
               	intLedSpeedState = jsonparse_get_value_as_int(parser);
               	os_bzero(str_buf, sizeof(str_buf));
               	os_sprintf(str_buf,"%d", intLedSpeedState);
               	MQTT_Publish(&mqttClient, "/socket1/ChangedLedSpeed", str_buf, os_strlen(str_buf), 2, 0);
	        }
  	        else if (jsonparse_strcmp_value(parser, "ChangedLedColorRed") == 0) {
   	            jsonparse_next(parser);
   	            jsonparse_next(parser);
               	intLedColorRedState = jsonparse_get_value_as_int(parser);
               	os_bzero(str_buf, sizeof(str_buf));
               	os_sprintf(str_buf,"%d", intLedColorRedState);
               	MQTT_Publish(&mqttClient, "/socket1/ChangedLedColorRed", str_buf, os_strlen(str_buf), 2, 0);
	        }
  	        else if (jsonparse_strcmp_value(parser, "ChangedLedColorGreen") == 0) {
   	            jsonparse_next(parser);
   	            jsonparse_next(parser);
               	intLedColorGreenState = jsonparse_get_value_as_int(parser);
              	os_bzero(str_buf, sizeof(str_buf));
              	os_sprintf(str_buf,"%d", intLedColorGreenState);
               	MQTT_Publish(&mqttClient, "/socket1/ChangedLedColorGreen", str_buf, os_strlen(str_buf), 2, 0);
	        }
  	        else if (jsonparse_strcmp_value(parser, "ChangedLedColorBlue") == 0) {
   	            jsonparse_next(parser);
   	            jsonparse_next(parser);
               	intLedColorBlueState = jsonparse_get_value_as_int(parser);
               	os_bzero(str_buf, sizeof(str_buf));
               	os_sprintf(str_buf,"%d", intLedColorBlueState);
               	MQTT_Publish(&mqttClient, "/socket1/ChangedLedColorBlue", str_buf, os_strlen(str_buf), 2, 0);
	        }
            else if (jsonparse_strcmp_value(parser, "StateLight") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
            	intLightState = jsonparse_get_value_as_int(parser);
//            	INFO("LightState:%d:\n", intLightState);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%d", intLightState);

            	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/StateLight", 1);
    			OLED_Print(5, 3, str_buf, 1);

            	MQTT_Publish(&mqttClient, "/socket1/StateLight", str_buf, os_strlen(str_buf), 2, 0);
            }
            else if (jsonparse_strcmp_value(parser, "ArduinoStarting") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
            	intStartingState = jsonparse_get_value_as_int(parser);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"1");

            	MQTT_Publish(&mqttClient, "/socket1/ArduinoStarting", str_buf, os_strlen(str_buf), 2, 0);
            }
            else if (jsonparse_strcmp_value(parser, "StateFan") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
            	intFanState = jsonparse_get_value_as_int(parser);
//            	INFO("FanState:%d:\n", intFanState);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%d", intFanState);

            	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/StateFan", 1);
    			OLED_Print(5, 3, str_buf, 1);

            	MQTT_Publish(&mqttClient, "/socket1/StateFan", str_buf, os_strlen(str_buf), 2, 0);
            }
            else if (jsonparse_strcmp_value(parser, "StateFanSpeed") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
            	intFanSpeed = jsonparse_get_value_as_int(parser);
//            	INFO("FanSpeed:%d:\n", intFanSpeed);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%d", intFanSpeed);

            	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/StateFanSpeed", 1);
    			OLED_Print(5, 3, str_buf, 1);

            	MQTT_Publish(&mqttClient, "/socket1/StateFanSpeed", str_buf, os_strlen(str_buf), 2, 0);
            }
      	    else if (jsonparse_strcmp_value(parser, "StateLed") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
            	intLedState = jsonparse_get_value_as_int(parser);
//            	INFO("LedState:%d:\n", intLedState);

            	os_bzero(str_buf, sizeof(str_buf));
            	os_sprintf(str_buf,"%d", intLedState);

            	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/StateLed", 1);
    			OLED_Print(5, 3, str_buf, 1);

            	MQTT_Publish(&mqttClient, "/socket1/StateLed", str_buf, os_strlen(str_buf), 2, 0);
            }
       	    else if (jsonparse_strcmp_value(parser, "StateLedMode") == 0) {
       	        jsonparse_next(parser);
       	        jsonparse_next(parser);
               	intLedModeState = jsonparse_get_value_as_int(parser);

               	os_bzero(str_buf, sizeof(str_buf));
               	os_sprintf(str_buf,"%d", intLedModeState);

               	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/StateLedMode", 1);
    			OLED_Print(5, 3, str_buf, 1);

               	MQTT_Publish(&mqttClient, "/socket1/StateLedMode", str_buf, os_strlen(str_buf), 2, 0);
    	    }
      	    else if (jsonparse_strcmp_value(parser, "StateLedSpeed") == 0) {
       	        jsonparse_next(parser);
       	        jsonparse_next(parser);
               	intLedSpeedState = jsonparse_get_value_as_int(parser);

               	os_bzero(str_buf, sizeof(str_buf));
               	os_sprintf(str_buf,"%d", intLedSpeedState);

               	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/StateLedSpeed", 1);
    			OLED_Print(5, 3, str_buf, 1);

               	MQTT_Publish(&mqttClient, "/socket1/StateLedSpeed", str_buf, os_strlen(str_buf), 2, 0);
    	    }
      	    else if (jsonparse_strcmp_value(parser, "StateLedColorRed") == 0) {
       	        jsonparse_next(parser);
       	        jsonparse_next(parser);
               	intLedColorRedState = jsonparse_get_value_as_int(parser);
               	os_bzero(str_buf, sizeof(str_buf));
               	os_sprintf(str_buf,"%d", intLedColorRedState);
               	MQTT_Publish(&mqttClient, "/socket1/StateLedColorRed", str_buf, os_strlen(str_buf), 2, 0);
    	    }
      	    else if (jsonparse_strcmp_value(parser, "StateLedColorGreen") == 0) {
       	        jsonparse_next(parser);
       	        jsonparse_next(parser);
               	intLedColorGreenState = jsonparse_get_value_as_int(parser);
              	os_bzero(str_buf, sizeof(str_buf));
               	os_sprintf(str_buf,"%d", intLedColorGreenState);
               	MQTT_Publish(&mqttClient, "/socket1/StateLedColorGreen", str_buf, os_strlen(str_buf), 2, 0);
    	    }
      	    else if (jsonparse_strcmp_value(parser, "StateLedColorBlue") == 0) {
       	        jsonparse_next(parser);
       	        jsonparse_next(parser);
               	intLedColorBlueState = jsonparse_get_value_as_int(parser);
              	os_bzero(str_buf, sizeof(str_buf));
               	os_sprintf(str_buf,"%d", intLedColorBlueState);
                MQTT_Publish(&mqttClient, "/socket1/StateLedColorBlue", str_buf, os_strlen(str_buf), 2, 0);
            }
      	    else if (jsonparse_strcmp_value(parser, "StateAlarm") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
                jsonparse_copy_value(parser, strAlarmState, 20);
//                INFO("AlarmState:%s:\n", strAlarmState);

            	oled_clear();
    			OLED_Print(3, 6, "MQTT sending...", 1);
    			OLED_Print(1, 2, "/socket1/StateAlarm", 1);
    			OLED_Print(2, 3, strAlarmState, 1);

            	MQTT_Publish(&mqttClient, "/socket1/StateAlarm", strAlarmState, os_strlen(strAlarmState), 2, 0);
            }
      	    else if (jsonparse_strcmp_value(parser, "StateBacklight") == 0) {
            	jsonparse_next(parser);
            	jsonparse_next(parser);
            	intBacklight = jsonparse_get_value_as_int(parser);
//            	INFO("Backlight:%d:\n", intBacklight);

            	oled_clear();
            	if (intBacklight == 1)
            	{
            		OLED_ON();
        			OLED_Print(1, 2, "Turning on", 1);
    	    		OLED_Print(7, 3, "OLED...", 1);
            	}
            	else
            	{
        			OLED_Print(1, 2, "Turning off", 1);
    	    		OLED_Print(7, 3, "OLED...", 1);
            		OLED_OFF();
            	}
            }
        }
    }

    return 0;
}

LOCAL struct jsontree_callback data_callback =
    JSONTREE_CALLBACK(data_get, data_set);

//JSONTREE_OBJECT(get_data_tree,
//                JSONTREE_PAIR("LightCommand", &data_callback),
//                JSONTREE_PAIR("FanCommand", &data_callback),
//                JSONTREE_PAIR("LedCommand", &data_callback));

//JSONTREE_OBJECT(data_get_tree,
//                JSONTREE_PAIR("socket1", &get_data_tree));


JSONTREE_OBJECT(get_light_command_data_tree,
                JSONTREE_PAIR("LightCommand", &data_callback));
JSONTREE_OBJECT(get_fan_command_data_tree,
                JSONTREE_PAIR("FanCommand", &data_callback));
JSONTREE_OBJECT(get_fan_speed_command_data_tree,
                JSONTREE_PAIR("FanSpeedCommand", &data_callback));
JSONTREE_OBJECT(get_led_command_data_tree,
                JSONTREE_PAIR("LedCommand", &data_callback));
JSONTREE_OBJECT(get_led_mode_command_data_tree,
                JSONTREE_PAIR("LedModeCommand", &data_callback));
JSONTREE_OBJECT(get_led_speed_command_data_tree,
                JSONTREE_PAIR("LedSpeedCommand", &data_callback));
//JSONTREE_OBJECT(get_led_color_red_data_tree,
//                JSONTREE_PAIR("LedColorRed", &data_callback));
//JSONTREE_OBJECT(get_led_color_green_data_tree,
//                JSONTREE_PAIR("LedColorGreen", &data_callback));
//JSONTREE_OBJECT(get_led_color_blue_data_tree,
//                JSONTREE_PAIR("LedColorBlue", &data_callback));

JSONTREE_OBJECT(light_command_data_get_tree,
                JSONTREE_PAIR("socket1", &get_light_command_data_tree));
JSONTREE_OBJECT(fan_command_data_get_tree,
                JSONTREE_PAIR("socket1", &get_fan_command_data_tree));
JSONTREE_OBJECT(fan_speed_command_data_get_tree,
                JSONTREE_PAIR("socket1", &get_fan_speed_command_data_tree));
JSONTREE_OBJECT(led_command_data_get_tree,
                JSONTREE_PAIR("socket1", &get_led_command_data_tree));
JSONTREE_OBJECT(led_mode_command_data_get_tree,
                JSONTREE_PAIR("socket1", &get_led_mode_command_data_tree));
JSONTREE_OBJECT(led_speed_command_data_get_tree,
                JSONTREE_PAIR("socket1", &get_led_speed_command_data_tree));
//JSONTREE_OBJECT(led_color_red_data_get_tree,
//                JSONTREE_PAIR("socket1", &get_led_color_red_data_tree));
//JSONTREE_OBJECT(led_color_green_data_get_tree,
//                JSONTREE_PAIR("socket1", &get_led_color_green_data_tree));
//JSONTREE_OBJECT(led_color_blue_data_get_tree,
//                JSONTREE_PAIR("socket1", &get_led_color_blue_data_tree));
JSONTREE_OBJECT(led_color_command_data_get_tree,
                JSONTREE_PAIR("LedColorRed",   &data_callback),
                JSONTREE_PAIR("LedColorGreen", &data_callback),
                JSONTREE_PAIR("LedColorBlue",  &data_callback));
JSONTREE_OBJECT(timestamp_command_data_get_tree,
                JSONTREE_PAIR("TimestampHour",   &data_callback),
                JSONTREE_PAIR("TimestampMinute", &data_callback),
                JSONTREE_PAIR("TimestampPM",     &data_callback));



JSONTREE_OBJECT(set_data_tree,
                JSONTREE_PAIR("ChangedLight", &data_callback),
                JSONTREE_PAIR("ChangedFan", &data_callback),
                JSONTREE_PAIR("ChangedFanSpeed", &data_callback),
                JSONTREE_PAIR("ChangedLed", &data_callback),
				JSONTREE_PAIR("ChangedLedMode", &data_callback),
				JSONTREE_PAIR("ChangedLedSpeed", &data_callback),
				JSONTREE_PAIR("ChangedLedColorRed", &data_callback),
				JSONTREE_PAIR("ChangedLedColorGreen", &data_callback),
				JSONTREE_PAIR("ChangedLedColorBlue", &data_callback),
                JSONTREE_PAIR("StateLight", &data_callback),
                JSONTREE_PAIR("ArduinoStarting", &data_callback),
                JSONTREE_PAIR("StateFan", &data_callback),
                JSONTREE_PAIR("StateFanSpeed", &data_callback),
                JSONTREE_PAIR("StateLed", &data_callback),
				JSONTREE_PAIR("StateLedMode", &data_callback),
				JSONTREE_PAIR("StateLedSpeed", &data_callback),
				JSONTREE_PAIR("StateLedColorRed", &data_callback),
				JSONTREE_PAIR("StateLedColorGreen", &data_callback),
				JSONTREE_PAIR("StateLedColorBlue", &data_callback),
                JSONTREE_PAIR("StateTemperature", &data_callback),
                JSONTREE_PAIR("StateHumidity", &data_callback),
                JSONTREE_PAIR("StateWatts", &data_callback),
                JSONTREE_PAIR("StateAlarm", &data_callback),
                JSONTREE_PAIR("StateBacklight", &data_callback));

JSONTREE_OBJECT(data_set_tree,
                JSONTREE_PAIR("socket1", &set_data_tree));



LOCAL void ICACHE_FLASH_ATTR timer_callback(void *arg)
{
}

void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}

void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;

//	MQTT_Subscribe(client,"/socket1/Temperature",       0);
//	MQTT_Subscribe(client,"/socket1/Humidity",          0);
//	MQTT_Subscribe(client,"/socket1/Watts",             0);
//	MQTT_Subscribe(client,"/socket1/LightState",        0);
//	MQTT_Subscribe(client,"/socket1/FanState",          0);
//	MQTT_Subscribe(client,"/socket1/FanSpeed",          0);
//	MQTT_Subscribe(client,"/socket1/LedState",          0);
//	MQTT_Subscribe(client,"/socket1/AlarmState",        0);
	MQTT_Subscribe(client,"/socket1/LightCommand",      2);
	MQTT_Subscribe(client,"/socket1/FanCommand",        2);
	MQTT_Subscribe(client,"/socket1/FanSpeedCommand",   2);
	MQTT_Subscribe(client,"/socket1/LedCommand",        2);
	MQTT_Subscribe(client,"/socket1/LedModeCommand",    2);
	MQTT_Subscribe(client,"/socket1/LedSpeedCommand",   2);
	MQTT_Subscribe(client,"/socket1/LedColorCommand",   2);
	MQTT_Subscribe(client,"/TimestampCommand",          2);

    MQTT_Publish(&mqttClient, "/socket1/ESP8266Starting", "1", 1, 2, 0);
}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
//	INFO("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
//	INFO("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *pbuf = (char *)os_zalloc(jsonSize);
	char *topicBuf = (char*)os_zalloc(topic_len+1);
	char *dataBuf  = (char*)os_zalloc(data_len+1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;
	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

//	INFO("Received on topic: %s, data: %s \r\n", topicBuf, dataBuf);
//	os_delay_us(500);
	oled_clear();
	OLED_Print(3, 6, "MQTT received...", 1);
	OLED_Print(1, 2, topicBuf, 1);
	OLED_Print(5, 3, dataBuf, 1);

	if (os_strcmp(topicBuf, "/socket1/LightCommand") == 0)
	{
		intLightCommand = atoi(dataBuf);
        json_ws_send((struct jsontree_value *)&light_command_data_get_tree, "socket1", pbuf);
	}
	else if (os_strcmp(topicBuf, "/socket1/FanCommand") == 0)
    {
		intFanCommand = atoi(dataBuf);
        json_ws_send((struct jsontree_value *)&fan_command_data_get_tree, "socket1", pbuf);
	}
	else if (os_strcmp(topicBuf, "/socket1/FanSpeedCommand") == 0)
    {
		intFanSpeedCommand = atoi(dataBuf);
        json_ws_send((struct jsontree_value *)&fan_speed_command_data_get_tree, "socket1", pbuf);
	}
	else if (os_strcmp(topicBuf, "/socket1/LedCommand") == 0)
    {
		intLedCommand = atoi(dataBuf);
        json_ws_send((struct jsontree_value *)&led_command_data_get_tree, "socket1", pbuf);
	}
	else if (os_strcmp(topicBuf, "/socket1/LedModeCommand") == 0)
    {
		intLedModeCommand = atoi(dataBuf);
        json_ws_send((struct jsontree_value *)&led_mode_command_data_get_tree, "socket1", pbuf);
	}
	else if (os_strcmp(topicBuf, "/socket1/LedSpeedCommand") == 0)
    {
		intLedSpeedCommand = atoi(dataBuf);
        json_ws_send((struct jsontree_value *)&led_speed_command_data_get_tree, "socket1", pbuf);
	}
	else if (os_strcmp(topicBuf, "/socket1/LedColorCommand") == 0)
    {
		char *p = dataBuf;
		int n=0;
		while (*p)
		{ // While there are more characters to process...
		    if (isdigit(*p))
		    { // Upon finding a digit, ...
		        long val = strtol(p, &p, 10); // Read a number, ...
		        if (n==1)
		        {
		            intLedColorRed = (int)val;
		        }
		        else if (n==2)
		        {
		            intLedColorGreen = (int)val;
		        }
		        else if (n==3)
		        {
		            intLedColorBlue = (int)val;
		        }
		        n++;
//		        INFO("COLOR: %ld\n", val); // and print it.
		    }
		    else
		    { // Otherwise, move on to the next character.
		        p++;
		    }
		}
		if (n>=4)
		{
			json_ws_send((struct jsontree_value *)&led_color_command_data_get_tree, "socket1", pbuf);
		}
    }
	else if (os_strcmp(topicBuf, "/TimestampCommand") == 0)
	{
		char *p = dataBuf;
		int n=0;
		while (*p)
		{ // While there are more characters to process...
		    if (isdigit(*p))
		    { // Upon finding a digit, ...
		        long val = strtol(p, &p, 10); // Read a number, ...
                    // Timestamp format: Thu Apr 09 2015 15:43:34
		        if (n==2)
		        {
		            intTimestampHour = (int)val;
	            	intTimestampPM = 0;
		            if (intTimestampHour == 0)
		            {
		            	intTimestampPM = 0;
		            	intTimestampHour = 12;
		            }
		            else if (intTimestampHour > 12)
		            {
		            	intTimestampPM = 1;
		            	intTimestampHour = intTimestampHour - 12;
		            }
		        }
		        else if (n==3)
		        {
		            intTimestampMinute = (int)val;
		        }
		        n++;
//		        INFO("TIME: %ld\n", val); // and print it.
		    }
		    else
		    { // Otherwise, move on to the next character.
		        p++;
		    }
		}
		if (n>=3)
		{
			json_ws_send((struct jsontree_value *)&timestamp_command_data_get_tree, "socket1", pbuf);
		}
    }
	// Send to Arduino as Json string
	INFO("%s",pbuf);
//	os_delay_us(500);

	os_free(dataBuf);
	os_free(topicBuf);
	os_free(pbuf);
}

//Main code function
static void ICACHE_FLASH_ATTR loop(os_event_t *events)
{
	char str[256];
	int position = 0;
	int opening_brace = 0;

	int c = uart0_rx_one_char();

	if (c != -1)
	{
		if (c == 123) // {
		{
			os_bzero(str, sizeof(str));
			struct jsontree_context js;
			str[position] = (char)c;
			position++;
			opening_brace++;

			while(c = uart0_rx_one_char())
			{
				if ((c >= 32) && (c <= 126))
				{
					str[position++] = (char)c;
				}

				if (c == 125)  // }
				{
					opening_brace--;
				}

				if ((opening_brace == 0 ) || (position>254))
				{
					break;
				}
			}

			if (opening_brace == 0 )
			{
    			str[position++] = 0;
//	    	    INFO("String received=%s:position=%d\n",str,position);

//		    	os_delay_us(200000);

    			oled_clear();
	    		OLED_Print(3, 6, "Serial received...", 1);
		    	OLED_Print(1, 2, str, 1);

                jsontree_setup(&js, (struct jsontree_value *)&data_set_tree, json_putchar);
                json_parse(&js, str);
			}

   		}
	}
}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	gpio_init();

	// check GPIO setting (for config mode selection)
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);

	i2c_init();
  	OLED = OLED_Init();
	OLED_Print(2, 0, "Socket 1 ESP8266", 1);
	OLED_Print(2, 2, "................", 3);
	OLED_Print(2, 6, "Initializing....", 1);
	os_delay_us(1000000);

	CFG_Load();

	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 2, 0);

	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

	//Start os task
	system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
	system_os_post(user_procTaskPrio, 0, 0 );

	// Start a timer that calls timer_callback every DELAY milliseconds.
//	os_timer_disarm(&timer);
//	os_timer_setfn(&timer, (os_timer_func_t *)timer_callback, (void *)0);
//	os_timer_arm(&timer, DELAY, 1);

//	INFO("\r\nSystem started ...\r\n");
}

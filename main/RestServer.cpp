#include <esp_http_server.h>
#include <esp_log.h>
#include <string>
#include <string.h>
#include <iostream>
#include "cJSON.h"


#include "IntexSWG.h"
#include "utils.h"
#include "RestServer.h"

/********************************* OTA *******************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include "esp_ota_ops.h"
#include "freertos/event_groups.h"
#include "OTAServer.h"
/********************************* OTA *******************************************/


using namespace std;


/********************************* OTA *******************************************/
// Embedded Files. To add or remove make changes is component.mk file as well. 
extern const uint8_t index_html_start[] asm("_binary_indexOTA_html_start");
extern const uint8_t index_html_end[]   asm("_binary_indexOTA_html_end");
extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]   asm("_binary_favicon_ico_end");
extern const uint8_t jquery_3_4_1_min_js_start[] asm("_binary_jquery_3_4_1_min_js_start");
extern const uint8_t jquery_3_4_1_min_js_end[]   asm("_binary_jquery_3_4_1_min_js_end");


int8_t flash_status = 0;
int8_t enableota = 0;

EventGroupHandle_t reboot_event_group;
const int REBOOT_BIT = BIT0;
/********************************* OTA *******************************************/


/* @brief tag used for ESP serial console messages */
static const char TAG[] = "api_rest";

/* @brief the HTTP server handle */
static httpd_handle_t server = NULL;


// HTTP GET General info request
static esp_err_t general_info_get_handler(httpd_req_t *req) {

    httpd_resp_set_type(req, "application/json");
    
    std::string displayDigits;
    displayDigits += getDisplayDigitFromCode(displayingDigit2);
    displayDigits += getDisplayDigitFromCode(displayingDigit1);
    if (displayingDigit1 == DISP_1_CLEAN_06P || displayingDigit1 == DISP_1_CLEAN_10P || displayingDigit1 == DISP_1_CLEAN_14P) {
        displayDigits += '.';
    }
    
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    
    cJSON *display = cJSON_CreateObject();
    cJSON_AddStringToObject(display, "status", (!displayON || powerStatus == POWER_STATUS_OFF) ? "OFF" : "ON");    
    cJSON_AddNumberToObject(display, "brightness", displayIntensity);
    cJSON_AddStringToObject(display, "current_code", displayDigits.c_str());
    cJSON_AddItemToObject(data, "display", display);

    cJSON *status = cJSON_CreateObject();
    cJSON_AddStringToObject(status, "power", (powerStatus == POWER_STATUS_BOOTING) ? "BOOTING" : (powerStatus == POWER_STATUS_ON) ? "ON" : (powerStatus == POWER_STATUS_STANDBY) ? "STANDBY" : (powerStatus == POWER_STATUS_BUS_ERROR) ? "BUS_ERROR" : "OFF");
    cJSON_AddStringToObject(status, "boost", (statusDigit3 & (0x01 << LED_BOOST)) >> LED_BOOST == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "sleep", (statusDigit3 & (0x01 << LED_SLEEP)) >> LED_SLEEP == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "o3_generation", (statusDigit3 & (0x01 << LED_OZONE)) >> LED_OZONE == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "pump_low_flow", (statusDigit3 & (0x01 << LED_PUMP_LOW_FLOW)) >> LED_PUMP_LOW_FLOW == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "low_salt", (statusDigit3 & (0x01 << LED_LOW_SALT)) >> LED_LOW_SALT == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "high_salt", (statusDigit3 & (0x01 << LED_HIGH_SALT)) >> LED_HIGH_SALT == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "service", (statusDigit3 & (0x01 << LED_SERVICE)) >> LED_SERVICE == 1 ? "ON" : "OFF");
    cJSON_AddItemToObject(data, "status", status);

    cJSON *mode = cJSON_CreateObject();
    cJSON_AddBoolToObject(mode, "working", powerStatus == POWER_STATUS_ON);
    cJSON_AddBoolToObject(mode, "programming", displayBlinking);
    cJSON_AddItemToObject(data, "mode", mode);

    cJSON_AddItemToObject(root, "data", data);
    
    const char *response = cJSON_Print(root);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(root);
    return ESP_OK;
}

// HTTP GET General info request
static esp_err_t debug_get_handler(httpd_req_t *req) {
        httpd_resp_set_type(req, "application/json");
    
    std::string displayDigits;
    displayDigits += getDisplayDigitFromCode(displayingDigit2);
    displayDigits += getDisplayDigitFromCode(displayingDigit1);
    if (displayingDigit1 == DISP_1_CLEAN_06P || displayingDigit1 == DISP_1_CLEAN_10P || displayingDigit1 == DISP_1_CLEAN_14P) {
        displayDigits += '.';
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    
    char powerStatusChar[10];
    char statusDigit1Char[10];
    char statusDigit2Char[10];
    char statusDigit3Char[10];
    char displayingDigit1Char[10];
    char displayingDigit2Char[10];
    sprintf(powerStatusChar, "0x%02X", powerStatus);
    sprintf(statusDigit1Char, "0x%02X", statusDigit1);
    sprintf(statusDigit2Char, "0x%02X", statusDigit2);
    sprintf(statusDigit3Char, "0x%02X", statusDigit3);
    sprintf(displayingDigit1Char, "0x%02X", displayingDigit1);
    sprintf(displayingDigit2Char, "0x%02X", displayingDigit2);
    
    std::string powerStatusStr;
    std::string statusDigit1Str;
    std::string statusDigit2Str;
    std::string statusDigit3Str;
    std::string displayingDigit1Str;
    std::string displayingDigit2Str;
    std::string readBufferStr;

    powerStatusStr+=powerStatusChar;
    statusDigit1Str+=statusDigit1Char;
    statusDigit2Str+=statusDigit2Char;
    statusDigit3Str+=statusDigit3Char;
    displayingDigit1Str+=displayingDigit1Char;
    displayingDigit2Str+=displayingDigit2Char;

    char buf[4096], *pos = buf;
    for (int i = 0 ; i != 128 ; i++) {
        if (i) {
            pos += sprintf(pos, ", ");
        }
        pos += sprintf(pos, "[0X%02X, 0X%02X]", dataReceivedBuffer[i][0], dataReceivedBuffer[i][1]);
    }
    readBufferStr+=buf;
    /*
    const char* powerStatusStr = powerStatusChar;
    const char* statusDigit1Str = statusDigit1Char;
    const char* statusDigit2Str = statusDigit2Char;
    const char* statusDigit3Str = statusDigit3Char;
    const char* displayingDigit1Str = displayingDigit1Char;
    const char* displayingDigit2Str = displayingDigit2Char;
    */
    /*
    std::string powerStatusStr(powerStatusChar);
    std::string statusDigit1Str(statusDigit1Char);
    std::string statusDigit2Str(statusDigit2Char);
    std::string statusDigit3Str(statusDigit3Char);
    std::string displayingDigit1Str(displayingDigit1Char);
    std::string displayingDigit2Str(displayingDigit2Char);  
    */

    cJSON_AddStringToObject(data, "powerStatus", powerStatusStr.c_str());
    cJSON_AddStringToObject(data, "statusDigit1", statusDigit1Str.c_str());
    cJSON_AddStringToObject(data, "statusDigit2", statusDigit2Str.c_str());
    cJSON_AddStringToObject(data, "statusDigit3", statusDigit3Str.c_str());    
    cJSON_AddStringToObject(data, "displayingDigit1", displayingDigit1Str.c_str());
    cJSON_AddStringToObject(data, "displayingDigit2", displayingDigit2Str.c_str());
    cJSON_AddStringToObject(data, "current_code", displayDigits.c_str());
    cJSON_AddStringToObject(data, "readBuffer", readBufferStr.c_str());
    
    cJSON_AddStringToObject(data, "compilation_date", __DATE__);
    cJSON_AddStringToObject(data, "compilation_time", __TIME__);
    
    cJSON_AddBoolToObject(data, "otaUpdating", otaUpdating);
    cJSON_AddBoolToObject(data, "removeWifiConfig", removeWifiConfig);
    cJSON_AddBoolToObject(data, "machineON", machineON);
    cJSON_AddBoolToObject(data, "wifiReconnecting", wifiReconnecting);
    cJSON_AddBoolToObject(data, "readingMaster", readingMaster);
    cJSON_AddBoolToObject(data, "sendingKeyCode", sendingKeyCode);
    
    cJSON_AddItemToObject(root, "data", data);
    
    const char *response = cJSON_Print(root);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(root);
    return ESP_OK;
}


/* Simple handler for power control */
static esp_err_t general_info_post_handler(httpd_req_t *req)
{
    bool responseStatus = false;
    int remaining = req->content_len;
    char buffer[100];
    int received = 0;
    if (remaining >= 100) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (remaining > 0) {
        received = httpd_req_recv(req, buffer, remaining);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        remaining -= received;
    }

    cJSON *root = cJSON_Parse(buffer);
    cJSON* cjson_data = cJSON_GetObjectItem(root, "data");
    char* power = cJSON_GetObjectItem(cjson_data, "power")->valuestring;

    if (strcmp(power, "on") == 0) {        
        switch (powerStatus) {
            case POWER_STATUS_OFF:
                machinePower(true);
                nextButtonStatus = BUTTON_POWER;
                responseStatus = true;
                break;
            case POWER_STATUS_STANDBY:
                keyCodeSetByAPI = true;
                virtualPressButtonTime = 250;
                buttonStatus = BUTTON_POWER;   
                responseStatus = true;
                break;
            case POWER_STATUS_BUS_ERROR:
                keyCodeSetByAPI = true;
                virtualPressButtonTime = 250;
                buttonStatus = BUTTON_POWER;
                responseStatus = true;   
                break;
            default:
                responseStatus = false;
        }        
    }
    else if (strcmp(power, "off") == 0) {
        switch (powerStatus) {
            case POWER_STATUS_BOOTING:            
            case POWER_STATUS_ON:
                keyCodeSetByAPI = true;
                virtualPressButtonTime = 250;
                buttonStatus = BUTTON_POWER;
                delayedPowerOff = true;
                responseStatus = true;
                break;
            case POWER_STATUS_STANDBY:
                machinePower(false);
                responseStatus = true;
                break;
            default:
                responseStatus = false;
        } 
    }
    else if (strcmp(power, "standby") == 0) {
        switch (powerStatus) {
            case POWER_STATUS_BOOTING:            
            case POWER_STATUS_ON:
                keyCodeSetByAPI = true;
                virtualPressButtonTime = 250;
                buttonStatus = BUTTON_POWER;
                responseStatus = true;
                break;
            case POWER_STATUS_OFF:
                machinePower(true);
                responseStatus = true;
                break;
            case POWER_STATUS_BUS_ERROR:
                keyCodeSetByAPI = true;
                virtualPressButtonTime = 250;
                buttonStatus = BUTTON_POWER;
                responseStatus = true;   
                break;
            default:
                responseStatus = false;
        } 
    }  
    
    /*
    if (strcmp(power, "on") == 0) {        
        if (powerStatus == POWER_STATUS_STANDBY) {
            buttonStatus = BUTTON_POWER;   
            responseStatus = true;         
        }
    }
    // TODO: Change when relay is installed
    else if (strcmp(power, "off") == 0) { 
        if (powerStatus == POWER_STATUS_BOOTING || powerStatus == POWER_STATUS_ON) {
            buttonStatus = BUTTON_POWER;
            responseStatus = true;
        }
    }
    else if (strcmp(power, "standby") == 0) {
        if (powerStatus == POWER_STATUS_BOOTING || powerStatus == POWER_STATUS_ON) {
            buttonStatus = BUTTON_POWER;
            responseStatus = true;
        }
    } 
    */           

    cJSON_Delete(root);

    // Response
    cJSON *responseRoot = cJSON_CreateObject();
    cJSON *responseData = cJSON_CreateObject();    
    cJSON_AddBoolToObject(responseData, "status", responseStatus);
    cJSON_AddItemToObject(responseRoot, "data", responseData);
    
    const char *response = cJSON_Print(responseRoot);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(responseRoot);

    return ESP_OK;
}

static esp_err_t reboot_post_handler(httpd_req_t *req)
{
    bool responseStatus = false;
    int remaining = req->content_len;
    char buffer[100];
    int received = 0;
    if (remaining >= 100) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (remaining > 0) {
        received = httpd_req_recv(req, buffer, remaining);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        remaining -= received;
    }
    cJSON *root = cJSON_Parse(buffer);
    cJSON* cjson_data = cJSON_GetObjectItem(root, "data");
    char* myreboot = cJSON_GetObjectItem(cjson_data, "reboot")->valuestring;
    responseStatus=false; 
    if (strcmp(myreboot, "yes") == 0) {
        responseStatus=true; 
        esp_restart();
    }
    
    //esp_restart();
    cJSON_Delete(root);
    
    // Response
    cJSON *responseRoot = cJSON_CreateObject();
    cJSON *responseData = cJSON_CreateObject();    
    cJSON_AddBoolToObject(responseData, "status", responseStatus);
    cJSON_AddItemToObject(responseRoot, "data", responseData);
    
    const char *response = cJSON_Print(responseRoot);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(responseRoot);

    return ESP_OK;
}

static esp_err_t enableota_post_handler(httpd_req_t *req)
{
    bool responseStatus = false;
    int remaining = req->content_len;
    char buffer[100];
    int received = 0;
    if (remaining >= 100) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (remaining > 0) {
        received = httpd_req_recv(req, buffer, remaining);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        remaining -= received;
    }
    cJSON *root = cJSON_Parse(buffer);
    cJSON* cjson_data = cJSON_GetObjectItem(root, "data");
    char* myreboot = cJSON_GetObjectItem(cjson_data, "enableota")->valuestring;
    responseStatus=false; 
    if (strcmp(myreboot, "yes") == 0) {
        enableota=1; 
        responseStatus=true;
    }
    if (strcmp(myreboot, "no") == 0) {
        enableota=0; 
        responseStatus=false;
    }
    
    cJSON_Delete(root);
    
    // Response
    cJSON *responseRoot = cJSON_CreateObject();
    cJSON *responseData = cJSON_CreateObject();    
    cJSON_AddBoolToObject(responseData, "status", responseStatus);
    cJSON_AddItemToObject(responseRoot, "data", responseData);
    
    const char *response = cJSON_Print(responseRoot);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(responseRoot);

    return ESP_OK;
}

static esp_err_t slef_clean_post_handler(httpd_req_t *req)
{
    bool responseStatus = false;
    int remaining = req->content_len;
    char buffer[100];
    int received = 0;
    if (remaining >= 100) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (remaining > 0) {
        received = httpd_req_recv(req, buffer, remaining);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        remaining -= received;
    }

    cJSON *root = cJSON_Parse(buffer);
    cJSON* cjson_data = cJSON_GetObjectItem(root, "data");
    int selfCleanRequestedTime = cJSON_GetObjectItem(cjson_data, "time")->valueint;

    keyCodeSetByAPI = true;
    virtualPressButtonTime = 6000;
    buttonStatus = BUTTON_SELF_CLEAN;
    
    if (selfCleanRequestedTime <= 6) {
        selfCleanTime = DISP_1_CLEAN_06P;
    }
    else if (selfCleanRequestedTime <= 10) {
        selfCleanTime = DISP_1_CLEAN_10P;
    }
    else {
        selfCleanTime = DISP_1_CLEAN_14P;
    }        

    cJSON_Delete(root);

    // Response
    cJSON *responseRoot = cJSON_CreateObject();
    cJSON *responseData = cJSON_CreateObject();    
    cJSON_AddBoolToObject(responseData, "status", responseStatus);
    cJSON_AddItemToObject(responseRoot, "data", responseData);
    
    const char *response = cJSON_Print(responseRoot);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(responseRoot);

    return ESP_OK;
}

/* Simple handler for display control */
static esp_err_t display_post_handler(httpd_req_t *req)
{
    int remaining = req->content_len;
    char buffer[100];
    int received = 0;
    if (remaining >= 100) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (remaining > 0) {
        received = httpd_req_recv(req, buffer, remaining);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        remaining -= received;
    }

    cJSON *root = cJSON_Parse(buffer);
    cJSON* cjson_data = cJSON_GetObjectItem(root, "data");
    displayIntensity = cJSON_GetObjectItem(cjson_data, "brightness")->valueint % 8; 
    ESP_LOGI(TAG, "Set display brightness to '%d'", displayIntensity);     

    cJSON_Delete(root);

    // Response
    cJSON *responseRoot = cJSON_CreateObject();
    cJSON *responseData = cJSON_CreateObject();    
    cJSON_AddBoolToObject(responseData, "status", true);
    cJSON_AddItemToObject(responseRoot, "data", responseData);
    
    const char *response = cJSON_Print(responseRoot);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(responseRoot);

    return ESP_OK;
}

/* Simple handler for display control */
static esp_err_t wifi_config_delete_handler(httpd_req_t *req)
{
    removeWifiConfig = true;

    // Response
    cJSON *responseRoot = cJSON_CreateObject();
    cJSON *responseData = cJSON_CreateObject();    
    cJSON_AddBoolToObject(responseData, "status", true);
    cJSON_AddItemToObject(responseRoot, "data", responseData);

    const char *response = cJSON_Print(responseRoot);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(responseRoot);

    return ESP_OK;
}

static const httpd_uri_t swg_status_get_uri = {
    .uri       = "/api/v1/intex/swg/status",
    .method    = HTTP_GET,
    .handler   = general_info_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t swg_debug_get_uri = {
    .uri       = "/api/v1/intex/swg/debug",
    .method    = HTTP_GET,
    .handler   = debug_get_handler,
    .user_ctx  = NULL
};

/* URI handler for power control */
static const httpd_uri_t swg_status_post_uri = {
    .uri = "/api/v1/intex/swg",
    .method = HTTP_POST,
    .handler = general_info_post_handler,
    .user_ctx = NULL
};

/* URI handler for power control */
static const httpd_uri_t swg_reboot_post_uri = {
    .uri = "/api/v1/intex/swg/reboot",
    .method = HTTP_POST,
    .handler = reboot_post_handler,
    .user_ctx = NULL
};

static const httpd_uri_t swg_enableota_post_uri = {
    .uri = "/api/v1/intex/swg/enableota",
    .method = HTTP_POST,
    .handler = enableota_post_handler,
    .user_ctx = NULL
};

/* URI handler for self clean control */
static const httpd_uri_t self_clean_post_uri = {
    .uri = "/api/v1/intex/swg/self_clean",
    .method = HTTP_POST,
    .handler = slef_clean_post_handler,
    .user_ctx = NULL
};

/* URI handler for display control */
static const httpd_uri_t display_post_uri = {
    .uri = "/api/v1/intex/swg/display",
    .method = HTTP_POST,
    .handler = display_post_handler,
    .user_ctx = NULL
};

/* URI handler for delete wifi config */
static const httpd_uri_t wifi_config_delete_uri = {
    .uri = "/api/v1/intex/swg/wifi",
    .method = HTTP_DELETE,
    .handler = wifi_config_delete_handler,
    .user_ctx = NULL
};




/******************* OTA ***********************************/

/*****************************************************
 
	systemRebootTask()
 
	NOTES: This had to be a task because the web page needed
			an ack back. So i could not call this in the handler
 
 *****************************************************/
void systemRebootTask(void * parameter)
{

	// Init the event group
	reboot_event_group = xEventGroupCreate();
	
	// Clear the bit
	xEventGroupClearBits(reboot_event_group, REBOOT_BIT);

	
	for (;;)
	{
		// Wait here until the bit gets set for reboot
		EventBits_t staBits = xEventGroupWaitBits(reboot_event_group, REBOOT_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		
		// Did portMAX_DELAY ever timeout, not sure so lets just check to be sure
		if ((staBits & REBOOT_BIT) != 0)
		{
			ESP_LOGI("OTA", "Reboot Command, Restarting");
			vTaskDelay(2000 / portTICK_PERIOD_MS);

			esp_restart();
		}
	}
}
/* Send index.html Page */
static esp_err_t OTA_index_html_handler(httpd_req_t *req)
{
	ESP_LOGI("OTA", "index.html Requested");

	// Clear this every time page is requested
	flash_status = 0;
	
	httpd_resp_set_type(req, "text/html");

	httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}
/* Send .ICO (icon) file  */
static esp_err_t OTA_favicon_ico_handler(httpd_req_t *req)
{
	ESP_LOGI("OTA", "favicon_ico Requested");
    
	httpd_resp_set_type(req, "image/x-icon");

	httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);

	return ESP_OK;
}
/* jquery GET handler */
static esp_err_t jquery_3_4_1_min_js_handler(httpd_req_t *req)
{
	ESP_LOGI("OTA", "jqueryMinJs Requested");

	httpd_resp_set_type(req, "application/javascript");

	httpd_resp_send(req, (const char *)jquery_3_4_1_min_js_start, (jquery_3_4_1_min_js_end - jquery_3_4_1_min_js_start)-1);

	return ESP_OK;
}

/* Status */
static esp_err_t OTA_update_status_handler(httpd_req_t *req)
{
	char ledJSON[100];
	
	ESP_LOGI("OTA", "Status Requested");
	
	sprintf(ledJSON, "{\"status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", flash_status, __TIME__, __DATE__);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, ledJSON, strlen(ledJSON));
	
	// This gets set when upload is complete
	if (flash_status == 1)
	{
		// We cannot directly call reboot here because we need the 
		// browser to get the ack back. 
		xEventGroupSetBits(reboot_event_group, REBOOT_BIT);		
	}

	return ESP_OK;
}
/* Receive .Bin file */
static esp_err_t OTA_update_post_handler(httpd_req_t *req)
{
    ESP_LOGI("OTA", "Update handler called");
    if(enableota==1){
    otaUpdating = true;
    
	esp_ota_handle_t ota_handle; 
	
	char ota_buff[1024];
	int content_length = req->content_len;
	int content_received = 0;
    uint8_t percentage = 0;
	int recv_len;
	bool is_req_body_started = false;
	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    char complete_percent[12];

	// Unsucessful Flashing
	flash_status = -1;
	
	do
	{
		/* Read the data for the request */
		if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0) 
		{
			if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) 
			{
				ESP_LOGI("OTA", "Socket Timeout");
				/* Retry receiving if timeout occurred */
				continue;
			}
			ESP_LOGI("OTA", "OTA Other Error %d", recv_len);
			return ESP_FAIL;
		}

		//printf("OTA RX: %d of %d\r", content_received, content_length);
		
	    // Is this the first data we are receiving
		// If so, it will have the information in the header we need. 
		if (!is_req_body_started)
		{
			is_req_body_started = true;
			
			// Lets find out where the actual data staers after the header info		
			char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;	
			int body_part_len = recv_len - (body_start_p - ota_buff);
			
			//int body_part_sta = recv_len - body_part_len;
			//printf("OTA File Size: %d : Start Location:%d - End Location:%d\r\n", content_length, body_part_sta, body_part_len);
			printf("OTA File Size: %d\r\n", content_length);

			esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
			if (err != ESP_OK)
			{
				printf("Error With OTA Begin, Cancelling OTA\r\n");
				return ESP_FAIL;
			}
			else
			{
				printf("Writing to partition subtype %d at offset 0x%x\r\n", update_partition->subtype, update_partition->address);
			}

			// Lets write this first part of data out
			esp_ota_write(ota_handle, body_start_p, body_part_len);
		}
		else
		{
			// Write OTA data
			esp_ota_write(ota_handle, ota_buff, recv_len);
			
			content_received += recv_len;
            percentage = MIN(content_received * 100 / content_length, 99);
            std::sprintf(complete_percent, "%d", percentage);
            statusDigit1 = getCodeFromDisplayDigit((percentage < 10) ? complete_percent[0] : complete_percent[1]);
            statusDigit2 = getCodeFromDisplayDigit((percentage < 10) ? '0' : complete_percent[0]);
		}
 
	} while (recv_len > 0 && content_received < content_length);

	// End response
	//httpd_resp_send_chunk(req, NULL, 0);

	
	if (esp_ota_end(ota_handle) == ESP_OK)
	{
		// Lets update the partition
		if(esp_ota_set_boot_partition(update_partition) == ESP_OK) 
		{
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();

			// Webpage will request status when complete 
			// This is to let it know it was successful
			flash_status = 1;
		
			ESP_LOGI("OTA", "Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, boot_partition->address);
			ESP_LOGI("OTA", "Please Restart System...");
		}
		else
		{
			ESP_LOGI("OTA", "\r\n\r\n !!! Flashed Error !!!");
		}
		
	}
	else
	{
		ESP_LOGI("OTA", "\r\n\r\n !!! OTA End Error !!!");
	}
    
	return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}


static const httpd_uri_t OTA_index_html = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = OTA_index_html_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	.user_ctx = NULL
};

static const httpd_uri_t OTA_favicon_ico = {
	.uri = "/favicon.ico",
	.method = HTTP_GET,
	.handler = OTA_favicon_ico_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	.user_ctx = NULL
};
static const httpd_uri_t OTA_jquery_3_4_1_min_js = {
	.uri = "/jquery-3.4.1.min.js",
	.method = HTTP_GET,
	.handler = jquery_3_4_1_min_js_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	.user_ctx = NULL
};

static const httpd_uri_t OTA_update = {
	.uri = "/update",
	.method = HTTP_POST,
	.handler = OTA_update_post_handler,
	.user_ctx = NULL
};
static const httpd_uri_t OTA_status = {
	.uri = "/status",
	.method = HTTP_POST,
	.handler = OTA_update_status_handler,
	.user_ctx = NULL
};

/******************* OTA ***********************************/


void start_rest_server(unsigned int port) {
    server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.server_port = port;
    config.ctrl_port = 32769; 
    config.stack_size = 8192;
    //config.stack_size = 16384;
    config.max_uri_handlers = 13;    
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &swg_status_get_uri);
        httpd_register_uri_handler(server, &swg_debug_get_uri);
        httpd_register_uri_handler(server, &swg_status_post_uri);
        httpd_register_uri_handler(server, &swg_reboot_post_uri);
        httpd_register_uri_handler(server, &swg_enableota_post_uri);
        httpd_register_uri_handler(server, &self_clean_post_uri);
        httpd_register_uri_handler(server, &display_post_uri);
        httpd_register_uri_handler(server, &wifi_config_delete_uri);

        httpd_register_uri_handler(server, &OTA_index_html);
		httpd_register_uri_handler(server, &OTA_favicon_ico);
		httpd_register_uri_handler(server, &OTA_jquery_3_4_1_min_js);
		httpd_register_uri_handler(server, &OTA_update);
		httpd_register_uri_handler(server, &OTA_status);
        //httpd_register_basic_auth(server);
    } 
    else {
        ESP_LOGI(TAG, "Error starting server!");
    }
}

void stop_webserver() {
    // Stop the httpd server
    httpd_stop(server);
}

/*
 * atCommand.h
 *
 *  Created on: 7 Nov 2017
 *      Author: Fifth
 */

#ifndef NRF52SDKWRAP_H_
#define NRF52SDKWRAP_H_

#include <stdint.h>
#include <string.h>
#include "stdio.h"
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_nvic.h"
#include "nrf_sdm.h"
#include <ble_gap.h>
#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#define DEFAULT_DEVICE_NAME "Nordic_UART"
#define DEFAULT_APP_ADV_INTERVAL "64"
#define DEFAULT_APP_ADV_TIMEOUT_IN_SECONDS  "0"
#define DEFAULT_MIN_CONN_INTERVAL "16"
#define DEFAULT_MAX_CONN_INTERVAL "60"
#define DEFAULT_ADV_DATA "1"
#define DEFAULT_FULL_NAME 1
#define DEFAULT_SHORT_NAME_LENGTH 6
#define DEFAULT_ATT_MTU_SIZE 64
#define DEFAULT_SLAVE_LATENCY "1"
#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#define UART_TX_BUF_SIZE                512                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                512                                         /**< UART RX buffer size. */
#define AT_COMMAND_MAX_LENGTH			30
#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */
#define APP_FEATURE_NOT_SUPPORTED       BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2        /**< Reply when unsupported features are requested. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */
#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */
#define TYPEAPP "Peripheral"

#define help1 "\r\nat+reset: restore default values\r\n"
#define help2 "at+send=: Send text to connected master\r\n"
#define help3 "at+dev_name=: Set device name\r\n"
#define help4 "at+adv_data=: Set advertisement data (extra text in advertise package)\r\n"
#define help5 "at+uuids: Displays overview of services\r\n"
#define help6 "at+disconnect=value: Disconnect from current master\r\n"
#define help7 "at+adv_stop: Stop advertising\r\n"

void printSettings();
void power_manage(void);
void log_init(void);
void buttons_leds_init(bool * p_erase_bonds);
void uart_init(void);
void uart_event_handle(app_uart_evt_t * p_event);
void getCommandType(char array[], int length);
void advertising_init(void);
void bsp_event_handler(bsp_event_t event);
void gatt_init(void);
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt);
void ble_stack_init(void);
void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
void on_adv_evt(ble_adv_evt_t ble_adv_evt);
void sleep_mode_enter(void);
void conn_params_init(void);
void conn_params_error_handler(uint32_t nrf_error);
void on_conn_params_evt(ble_conn_params_evt_t * p_evt);
void services_init(ble_nus_data_handler_t data_handler);
void gap_params_init(void);
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name);
uint32_t startSoftdevice();
uint32_t changeAdvInt(char array[]);
uint32_t changeDevName(char array[]);
uint32_t changeMinConInt(char array[]);
uint32_t changeMaxConInt(char array[]);
uint32_t changeSlaveLatency(char array[]);
uint32_t changeAdvData(uint8_t  array[],uint8_t length);
uint32_t setAdvFullName(uint8_t fullName);
uint32_t setAdvNameLength(uint8_t length);
uint32_t changeMtuAttSize(uint8_t size);
uint32_t resetDevice();
uint32_t stopSoftDevice();
uint32_t stopAdv();
uint32_t startAdv();
uint32_t disconnectFromMaster();
uint32_t changeTransmitPower(int8_t power);
uint32_t sendBleText(uint8_t array[],uint16_t length1);
uint32_t changePhyMode(uint8_t mode);

#endif /* NRF52SDKWRAP_H_ */

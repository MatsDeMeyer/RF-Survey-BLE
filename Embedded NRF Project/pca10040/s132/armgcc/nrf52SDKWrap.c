/*
 * atCommand.c
 *
 *  Created on: 7 Nov 2017
 *      Author: Fifth
 */


#include "defaultParameters.h"
#include "nrf_delay.h"
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
#include <nrf52SDKWrap.h>
#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "fifo.h"

fifo_t uartfifo;
uint8_t uartByte;
uint8_t uart_buffer[50];

BLE_NUS_DEF(m_nus);                                                                 /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
BLE_ADVERTISING_DEF(m_advertising);													/**< Advertising module instance. */


char* APP_ADV_INTERVAL = DEFAULT_APP_ADV_INTERVAL ;                                 /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
char* APP_ADV_TIMEOUT_IN_SECONDS = DEFAULT_APP_ADV_TIMEOUT_IN_SECONDS    ;          /**< The advertising timeout (in units of seconds). */
char* MIN_CONN_INTERVAL = DEFAULT_MIN_CONN_INTERVAL;          						/**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
char* MAX_CONN_INTERVAL = DEFAULT_MAX_CONN_INTERVAL;       							/**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
char *DEVICE_NAME=DEFAULT_DEVICE_NAME;                             					/**< Name of device. Will be included in the advertising data. */
char* SLAVE_LATENCY= DEFAULT_SLAVE_LATENCY;                                        	/**< Slave latency. */
char* commands;
static uint8_t sendBuffer[AT_COMMAND_MAX_LENGTH];

static uint16_t   m_conn_handle          = BLE_CONN_HANDLE_INVALID;                 /**< Handle of the current connection. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
ble_uuid_t m_adv_uuids[]          =                                          		/**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};

ble_nus_data_handler_t client_data_handler;
bool setup=true;
bool initBluetooth=false;
bool fullName=DEFAULT_FULL_NAME;
uint8_t advertisementData[20] = DEFAULT_ADV_DATA;
uint8_t advertisementDataSize=1;
uint8_t shortNameLength=DEFAULT_SHORT_NAME_LENGTH ;
uint8_t attMtuSize=DEFAULT_ATT_MTU_SIZE ;
char attMtuSizeArray[4];
char shortNameLengthArray[3];
char deviceName[20];
char slaveLatency[10];
char maxConInt[10];
char minConInt[10];
char advInterval[10];
char power[10];




void printSettings()
{
	printf("%s", help1);
	printf("%s", help2);
	printf("%s", help3);
	printf("%s", help4);
	printf("%s", help5);
	printf("%s", help6);
	printf("%s", help7);
}

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
	printf("%c\r\n", line_num);
	    			printf("%s\r\n", p_file_name);
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    //BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = atoi(MIN_CONN_INTERVAL);
    gap_conn_params.max_conn_interval = atoi(MAX_CONN_INTERVAL);
    gap_conn_params.slave_latency     = atoi(SLAVE_LATENCY);
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);


}


/**@brief Function for initializing services that will be used by the application.
 */
void services_init(ble_nus_data_handler_t data_handler)
{
	client_data_handler=data_handler;
    uint32_t       err_code;
    ble_nus_init_t nus_init;

    memset(&nus_init, 0, sizeof(nus_init));

    //nus_init.data_handler = nus_data_handler;
    nus_init.data_handler = client_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    //err_code = sd_power_system_off();
    //APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;
        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            // LED indication will be changed when advertising starts.
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

#ifndef S140
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;
#endif

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;
#if !defined (S112)
         case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
        {
            ble_gap_data_length_params_t dl_params;

            // Clearing the struct will effectivly set members to @ref BLE_GAP_DATA_LENGTH_AUTO
            memset(&dl_params, 0, sizeof(ble_gap_data_length_params_t));
            err_code = sd_ble_gap_data_length_update(p_ble_evt->evt.gap_evt.conn_handle, &dl_params, NULL);
            APP_ERROR_CHECK(err_code);
        } break;
#endif //!defined (S112)
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_EVT_USER_MEM_REQUEST:
            err_code = sd_ble_user_mem_reply(p_ble_evt->evt.gattc_evt.conn_handle, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
        {
            ble_gatts_evt_rw_authorize_request_t  req;
            ble_gatts_rw_authorize_reply_params_t auth_reply;

            req = p_ble_evt->evt.gatts_evt.params.authorize_request;

            if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
            {
                if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ)     ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
                {
                    if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                    }
                    else
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                    }
                    auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                    err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                               &auth_reply);
                    APP_ERROR_CHECK(err_code);
                }
            }
        } break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}


/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, attMtuSize);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}
void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    ble_advdata_manuf_data_t        manuf_data; // Variable to hold manufacturer specific data
        //uint8_t data[]                      = "1"; // Our data to adverise
        manuf_data.company_identifier       = 0x0059; // Nordics company ID
        manuf_data.data.p_data              = advertisementData;
        manuf_data.data.size                = advertisementDataSize;

    memset(&init, 0, sizeof(init));
    if(fullName)
    {
    	init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    }
    else
    {
		init.advdata.name_type          = BLE_ADVDATA_SHORT_NAME;
		init.advdata.short_name_len = shortNameLength; // Advertise only first 6 letters of name
    }
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE; //allow unlimited timeout
    init.advdata.p_manuf_specific_data   = &manuf_data;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;


    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = atoi(APP_ADV_INTERVAL);
    init.config.ble_adv_fast_timeout  = atoi(APP_ADV_TIMEOUT_IN_SECONDS);

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}
uint32_t startSoftdevice(){
	uint32_t err_code;
	gap_params_init();
	gatt_init();
	services_init(client_data_handler);
	advertising_init();
	conn_params_init();
	err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
	return err_code;
}
uint32_t changeAdvInt(char array[]){
	uint32_t err_code;


	APP_ADV_INTERVAL=&array[0];

	sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	sd_ble_gap_adv_stop();
	advertising_init();
	err_code=ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST); //start
	return err_code;
}
uint32_t changeDevName(char array[]){
	uint32_t err_code;

	DEVICE_NAME=&array[0];

	 ble_gap_conn_sec_mode_t sec_mode;
	 sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	 sd_ble_gap_adv_stop();

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

	sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *) DEVICE_NAME,  strlen(DEVICE_NAME));
	advertising_init();
	err_code=ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST); //start
	return err_code;
}


uint32_t changeMinConInt(char array[]){
	MIN_CONN_INTERVAL=&array[0];

	if(atoi(&array[0])<=atoi(MAX_CONN_INTERVAL))
	{
		MIN_CONN_INTERVAL=&array[0];
		gap_params_init();
		return NRF_SUCCESS;
	}
	else
		return 8;

}

uint32_t changeMaxConInt(char array[]){

	if(atoi(MIN_CONN_INTERVAL)<=atoi(&array[0]))
	{
		MAX_CONN_INTERVAL=&array[0];
			gap_params_init();
		return NRF_SUCCESS;
	}
	else
		return 8;
}
uint32_t changeSlaveLatency(char array[]){
	uint32_t err_code;

	if(atoi(&slaveLatency[0])>=0)
	{
		SLAVE_LATENCY=&slaveLatency[0];
		gap_params_init();
		err_code=NRF_SUCCESS;
	}
	else
		return 8;
	return err_code;
}
uint32_t changeAdvData(uint8_t array[],uint8_t length){
	uint32_t err_code;
	advertisementDataSize=length;
	 sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	 sd_ble_gap_adv_stop();
	advertising_init();
	err_code=ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST); //start
	return err_code;
}
uint32_t setAdvFullName(uint8_t fullName2){
	uint32_t err_code;
	fullName=(fullName2==1);
	 sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	 sd_ble_gap_adv_stop();
	advertising_init();
	err_code=ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST); //start
	return err_code;
}
uint32_t setAdvNameLength(uint8_t length){
	uint32_t err_code;
	shortNameLength=length;

	 sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	 sd_ble_gap_adv_stop();
	advertising_init();
	err_code=ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST); //start
	return err_code;
}
uint32_t changeMtuAttSize(uint8_t size)
{
	uint32_t err_code;
	attMtuSize=size;

	err_code=nrf_ble_gatt_att_mtu_periph_set(&m_gatt, attMtuSize);
	return err_code;
}
uint32_t resetDevice(){
	uint32_t err_code;
	sd_nvic_SystemReset();
	err_code=1;
	return err_code;
}
uint32_t stopSoftDevice(){
	uint32_t err_code;
	err_code=nrf_sdh_disable_request();
	ble_stack_init();
	return err_code;
}
uint32_t stopAdv(){
	uint32_t err_code;
	//disconnectFromMaster();
	//nrf_delay_ms(1000);
	err_code=sd_ble_gap_adv_stop();
	return err_code;
}
uint32_t startAdv(){
	uint32_t err_code;
	err_code =ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST); //start
	return err_code;
}
uint32_t disconnectFromMaster(){
	uint32_t err_code;
	err_code=sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	return err_code;
}
uint32_t changeTransmitPower(int8_t power){
	uint32_t err_code;
	err_code=sd_ble_gap_tx_power_set(power);
	return err_code;
}
uint32_t sendBleText(uint8_t array[],uint16_t length1){
	uint32_t err_code;
	do
	{
		//uint16_t length1 = sizeof(array);
		err_code = ble_nus_string_send(&m_nus, array, &length1);
		if ( (err_code != NRF_ERROR_INVALID_STATE) && (err_code != NRF_ERROR_BUSY) )
		{
			APP_ERROR_CHECK(err_code);
		}
	} while (err_code == NRF_ERROR_BUSY);
	return err_code;
}
uint32_t changePhyMode(uint8_t mode){
	uint32_t err_code;
	ble_gap_phys_t gap_phys_settings;
	if(mode==1)
	{
		gap_phys_settings.tx_phys =BLE_GAP_PHY_1MBPS;
		gap_phys_settings.rx_phys = BLE_GAP_PHY_1MBPS;
	}
	else if(mode==2)
	{
		gap_phys_settings.tx_phys =BLE_GAP_PHY_2MBPS;
		gap_phys_settings.rx_phys = BLE_GAP_PHY_2MBPS;
	}
	else if(mode==3)
	{
		gap_phys_settings.tx_phys =BLE_GAP_PHY_CODED;
		gap_phys_settings.rx_phys = BLE_GAP_PHY_CODED;
	}
	else if(mode==0)
	{
		gap_phys_settings.tx_phys =BLE_GAP_PHY_AUTO;
		gap_phys_settings.rx_phys = BLE_GAP_PHY_AUTO;
	}
	else
		return 1;
	err_code=sd_ble_gap_phy_update(m_nus.conn_handle, &gap_phys_settings);
	return err_code;
}
void getCommandType(char array[], int length)//AT+SETTINGS:
{
	int i=0;
	char a[length];
	strncpy(a, array, length);
	commands= &a[0];
	uint32_t err_code;


	if(strstr(commands,"at+")!=NULL)
		if(strstr(commands,"at+start")!=NULL)
		{
			err_code = startSoftdevice();

			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+type=")!=NULL)
		{
			printf("%s", TYPEAPP);
		}
		else if(strstr(commands,"at+help")!=NULL)
		{
			printSettings();

		}
		else if(strstr(commands,"at+settings")!=NULL)
		{
			printf("Device name: ");
			printf("%s\r\n", DEVICE_NAME);

			printf("APP_ADV_INTERVAL: ");
			printf("%s\r\n", APP_ADV_INTERVAL);

			printf("APP_ADV_TIMEOUT_IN_SECONDS: ");
			printf("%s\r\n", APP_ADV_TIMEOUT_IN_SECONDS);

			printf("MAX_CONN_INTERVAL: ");
			printf("%s\r\n", MAX_CONN_INTERVAL);

			printf("MIN_CONN_INTERVAL: ");
			printf("%s\r\n", MIN_CONN_INTERVAL);

			printf("ADV_DATA: ");
			printf("%s\r\n", &advertisementData[0]);
		}
		else if(strstr(commands,"at+adv_int=")!=NULL)
		{
			memset(&advInterval[0], 0, sizeof(advInterval));
			for (i=0;i<(length-11);i++)
			{
				advInterval[i]=array[11+i];
			}

			err_code=changeAdvInt(advInterval); //start

			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);

		}
		else if(strstr(commands,"at+dev_name=")!=NULL)
		{
			memset(&deviceName[0], 0, sizeof(deviceName));

			for (i=0;i<(length-12);i++)
			{
				deviceName[i]=array[12+i];
			}

			DEVICE_NAME=&deviceName[0];


			err_code=changeDevName(deviceName); //start

			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+min_con_int=")!=NULL)
		{
			memset(&minConInt[0], 0, sizeof(minConInt));

			for (i=0;i<(length-15);i++)
			{
				minConInt[i]=array[15+i];
			}

			err_code=changeMinConInt(minConInt);

			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+max_con_int=")!=NULL)
		{
			memset(&maxConInt[0], 0, sizeof(maxConInt));

			for (i=0;i<(length-15);i++)
			{
				maxConInt[i]=array[15+i];
			}

			err_code=changeMaxConInt(maxConInt);
			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+slave_latency=")!=NULL)
		{
			memset(&slaveLatency[0], 0, sizeof(slaveLatency));

			for (i=0;i<(length-17);i++)
			{
				slaveLatency[i]=array[17+i];
			}

			err_code=changeSlaveLatency(slaveLatency);

			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+adv_data=")!=NULL)
		{
			memset(&advertisementData[0], 0, sizeof(advertisementData));

			for (i=0;i<(length-12);i++)
			{
				advertisementData[i]=array[12+i];
			}

			err_code=changeAdvData(advertisementData,length-12); //start

			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+adv_full_name=")!=NULL)
		{

			err_code=setAdvFullName((uint8_t)array[17]); //start

			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+adv_name_length=")!=NULL)
		{
			memset(&shortNameLengthArray[0], 0, sizeof(shortNameLengthArray));
			for (i=0;i<(length-19);i++)
			{
				shortNameLengthArray[i]=array[19+i];
			}

			err_code=setAdvNameLength(atoi(shortNameLengthArray));

			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+mtu_att_size=")!=NULL)
		{
			memset(&attMtuSizeArray[0], 0, sizeof(attMtuSizeArray));
			for (i=0;i<(length-16);i++)
			{
				attMtuSizeArray[i]=array[16+i];
			}

			err_code=changeMtuAttSize(atoi(attMtuSizeArray));

			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+reset")!=NULL)
		{
			err_code=resetDevice();
			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+softdevStop")!=NULL)
		{
			err_code=stopSoftDevice();

			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+shutdown")!=NULL)
		{
			stopSoftDevice();
			sd_power_system_off();
		}
		else if(strstr(commands,"at+adv_stop")!=NULL)
		{
			err_code=stopAdv();
			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+adv_start")!=NULL)
		{

			err_code =startAdv(); //start
			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+disconnect")!=NULL)
		{
			err_code=disconnectFromMaster();
			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+power=")!=NULL)
		{
			memset(&power[0], 0, sizeof(power));

			for (i=0;i<(length-9);i++)
			{
				power[i]=array[9+i];
			}

			err_code=changeTransmitPower(atoi(&power[0]));
			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+addr")!=NULL)
		{
			ble_gap_addr_t  p_addr;

			sd_ble_gap_addr_get(&p_addr);
			 printf("MAC Address : %02x:%02x:%02x:%02x:%02x:%02x\n",
			  (unsigned char) p_addr.addr[5],
			  (unsigned char) p_addr.addr[4],
			  (unsigned char) p_addr.addr[3],
			  (unsigned char) p_addr.addr[2],
			  (unsigned char) p_addr.addr[1],
			  (unsigned char) p_addr.addr[0]);
		}
		else if(strstr(commands,"at+send=")!=NULL)
		{
			memset(&sendBuffer[0], 0, sizeof(sendBuffer));

			for (i=0;i<(length-8);i++)
			{
				sendBuffer[i]=array[8+i];
			}
			err_code=sendBleText(sendBuffer,(length-8));
			if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+phy_mode=")!=NULL)
		{

			err_code=changePhyMode((uint8_t)array[12]);

		  if(err_code==NRF_SUCCESS)
				printf("OK\r\n");
			else
				printf("ERROR: %lu \r\n", (unsigned long)err_code);
		}
		else if(strstr(commands,"at+uuids")!=NULL)
		{
			for (i=0;i<(sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]));i++)
			{
				printf("UUID TYPE: ");
				printf("%d\r\n", m_adv_uuids[i].type);
				printf("UUID VALUE: ");
				printf("%d\r\n", m_adv_uuids[i].uuid);
			}
		}

		else
			printf("\r\nCommand not recognized\r\n");
	else
		printf("\r\nNot valid, try again\r\n");
}
void atCommandAction(char data_array[],uint8_t index)
{
	getCommandType(data_array,index+1);

}

/**@brief   Function for handling app_uart events.
 *
 */
/**@snippet [Handling the data received over UART] */
void uart_event_handle(app_uart_evt_t * p_event)
{
	switch (p_event->evt_type)
	{
		case APP_UART_DATA_READY:
			scanf("%c",&uartByte);
			fifo_put_byte(&uartfifo, uartByte);

			break;

		case APP_UART_COMMUNICATION_ERROR:
			APP_ERROR_HANDLER(p_event->data.error_communication);
			break;

		case APP_UART_FIFO_ERROR:
			APP_ERROR_HANDLER(p_event->data.error_code);
			break;

		default:
			break;
	}

}
/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
void uart_init(void)
{
    uint32_t                     err_code;
    app_uart_comm_params_t const comm_params =
    {
        .rx_pin_no    = RX_PIN_NUMBER,
        .tx_pin_no    = TX_PIN_NUMBER,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
        .baud_rate    = NRF_UART_BAUDRATE_115200
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);
    APP_ERROR_CHECK(err_code);
}
/**@snippet [UART Initialization] */


/**@brief Function for initializing the Advertising functionality.
 */



/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
void buttons_leds_init(bool * p_erase_bonds)
{
    bsp_event_t startup_event;

    uint32_t err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.
 */
void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for placing the application in low power state while waiting for events.
 */
void power_manage(void)
{
	uint32_t err_code = sd_app_evt_wait();
	//if(!setup)
	    APP_ERROR_CHECK(err_code);


}

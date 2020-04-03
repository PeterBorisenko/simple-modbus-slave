/*!
 * @file proto.h 
 *
 * @date 28 апр. 2017 г.
 * @author Peter Borisenko (peter@awsmtek.com)
 * @version 0.1a
 * @brief Simple implementation of MODBUS standart.
 * 
 * @details
 *  How to use this driver:
 *
 *  	- Running tests:
 *      	1. Install ruby. You can obtain ruby from https://www.ruby-lang.org
 *      	2. Install ceedling. From your command prompt run "gem install ceedling".
 *      	3. Navigate to this_library_sources/Test directory
 *      	4. Press Shift+RMB and select "Open command prompt" from context menu
 *      	5. Type "rake test:all" to launch the test sequence
 *      	6. All tests should be passed
 *  	- Adding library to your project
 *      	1. Add this_library_sources/Include directory to include path in your IDE or makefile
 *      	2. Add files from this_library_sources/Source directory to sources path or just copy it into your Source folder
 *      	3. Use modbus_user_data.template.h and proto_bindings.template.h to implement your own modbus_user_data.h and
 *          	proto_bindings.h, placed in you project Include folder.
 *  	- Binding to low level drivers:
 *      	1. You should implement two drivers with appropriate API. The first is Receiver/Transmitter (usually UART)
 *          	The second is hardware timer which will run in one-shot mode. Software/Virtual timers can also be used.
 *      	2. See the timer_driver.template.h and uart_driver.template.h to understand which API is needed.
 *      	3. Write appropriate functions to your proto_bindings.h file.
 *  	- Implementing in the Application:
 *      	1. Define an entity of proto_t and modbus_user_data_t.
 *      	2. Define and initialize an entity of proto_init_t.
 *      	3. Assign actual data addresses to the pointers, defined in modbus_user_data_t.
 *          	Keep in mind that modbus registers are 16-bit wide.
 *      	4. Set WE field on those registers that should be write accessible.
 *      	5. Initialize protocol state using protoInit().
 *      	6. Place protoBackgroundProcess() to your main loop or call it periodically
 *      	7. See an example in example folder.
 *
 *	@warning Only 2 Modbus functions are currently implemented: READ HOLDING REGISTERS and WRITE SINGLE REGISTER,
 *			which is enough in most situations.
 *	@warning Only static address are used in this version (Reading and assigning device address at initialization stage)
 */

#ifndef INCLUDE_PROTO_H_
#define INCLUDE_PROTO_H_

#include <stdint.h>
#include <stdbool.h>
#include "modbus.h"
#include "proto_types.h"

/*!
 * @typedef struct proto_init_t
 * @brief Used to initialize protocol state
 */
typedef struct {
    void * modbus_user_data;		//!< Pointer to user data struct (cast to void)
    uint16_t registers_number;		//!< Number of 16-bit registers are used (continuously placed in data struct)
    void (*tx_en)(uint16_t state);	//!< Pointer to function that switch driver direction. Can be NULL if unused
    uint32_t cpuFreqMhz;			//!< CPU or Timer (if differ) clock frequency
    uint16_t address;				//!< Device address
} proto_init_t;

/*!
 * @typedef struct proto_t
 * @brief Holds protocol state
 */
typedef struct proto {						
    proto_state_t state;					//!< State of internal automata
    uint16_t ownAddress;					//!< Stored device address
    void (*tx_en_cb)(uint16_t state);       //!< Pointer to function that switch driver direction (1 - TX, 0 - RX)
    struct proto_flags {					
        uint16_t packetReceived     :   1;  //!< End of packet reception (delimiter have been received)
        uint16_t broadcast          :   1;  //!< Broadcast packet received - do not answer
        uint16_t txlocked           :   1;  //!< Software lock of TX interface (Currently not used)
        uint16_t rts                :   1;  //!< Data Transmit request
        uint16_t data_overrun       :   1;  //!< Data overrun flag (Currently not used)
        uint16_t reserved           :   11;	//!< Resereved for future use
    } flags;
    struct tx_t {
        uint16_t * buffer;					//!< Pointer to Transmit buffer (used internal buffer - the same as Receive buffer)
        uint16_t idx;						//!< Receive index (will copy to RX length when delimiter is received)
        uint16_t length;					//!< Received data length
    } tx;
    struct rx_t {
        uint16_t * buffer;					//!< Pointer to Receive buffer (used internal buffer - the same as Transmit buffer)
        uint16_t idx;						//!< Transmit index (transmitting stops if TX index becomes equal to TX length)
        uint16_t length;					//!< Length of data to be transmitted
    } rx;
    proto_modbus_data_t data;				//!< Internal protocol data (contains pointer to user data and registers number)
    write_result_t modbus_data_write_result;	//!< Result of writing the register (Currently not used - always return WRITE_OK)
} proto_t;

/*! 
 * @fn void protoInit(proto_t * proto, proto_init_t * protoInitStruct)
 * @brief Initializes protocol structures and hardware layer
 * @param[inout] proto Entity of protocol state struct
 * @param[in]	 protoInitStruct Set of protocol parameters
 * @retval none
 */
void protoInit(proto_t * proto, proto_init_t * protoInitStruct);

/*!
 * @fn void protoBackgroundProcess(void)
 * @brief Performs all protocol actions
 * @param none
 * @retval none
 * 
 * Polling for data
 * Get data from iface
 * Parse command
 * Write / Read / Send data
 * Place it in main cycle
 */
void protoBackgroundProcess(proto_t * proto);


#endif /* INCLUDE_PROTO_H_ */

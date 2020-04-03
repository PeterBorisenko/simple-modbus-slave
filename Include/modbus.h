/*
 * modbus.h
 *
 *  Created on: 4 мая 2017 г.
 *      Author: p.borisenko
 */

#ifndef INCLUDE_MODBUS_H_
#define INCLUDE_MODBUS_H_

// Common parameters
#define MODBUS_MESSAGE_LEN_MAX                              256
#define MODBUS_DELIMITER_TIMER_PERIOD_syms                  3.5
#define MODBUS_BROADCAST_ADDRESS                            0x00

// MODBUS packet structure
#define MODBUS_ADU_DEVICE_ADDRESS_idx                       0
#define MODBUS_ADU_PDU_START_idx                            1
#define MODBUS_PDU_FUNCTION_CODE_idx                        0
// Master write holding register request structure
#define MODBUS_PDU_WRITE_REGISTER_ADDRESS_HI_idx            1
#define MODBUS_PDU_WRITE_REGISTER_ADDRESS_LO_idx            2
#define MODBUS_PDU_WRITE_REGISTER_VALUE_HI_idx              3
#define MODBUS_PDU_WRITE_REGISTER_VALUE_LO_idx              4
#define MODBUS_ADU_WRITE_REGISTER_CRC_LO_idx                6
#define MODBUS_ADU_WRITE_REGISTER_CRC_HI_idx                7

// Master read holding registers request structure
#define MODBUS_PDU_READ_REGISTER_ADDRESS_HI_idx             1
#define MODBUS_PDU_READ_REGISTER_ADDRESS_LO_idx             2
#define MODBUS_PDU_READ_REGISTER_NUM_REGS_HI_idx            3
#define MODBUS_PDU_READ_REGISTER_NUM_REGS_LO_idx            4
#define MODBUS_ADU_READ_REGISTER_CRC_LO_idx                 6
#define MODBUS_ADU_READ_REGISTER_CRC_HI_idx                 7

// Master read holding registers response structure
#define MODBUS_PDU_READ_RESP_REGISTER_BYTE_COUNT_idx        1
#define MODBUS_PDU_READ_RESP_REGISTER_FIRST_VALUE_idx       2

// Master write single register responce/ack structure
#define MODBUS_PDU_WRITE_RESP_REGISTER_BYTE_ADDR_H_idx      1
#define MODBUS_PDU_WRITE_RESP_REGISTER_BYTE_ADDR_L_idx      2
#define MODBUS_PDU_WRITE_RESP_REGISTER_BYTE_COUNT_H_idx     3
#define MODBUS_PDU_WRITE_RESP_REGISTER_BYTE_COUNT_L_idx     4

// PDU exception response structure
#define MODBUS_PDU_EXCEPTION_CODE_idx                       1

// Function codes
#define MODBUS_FUNCTION_READ_COIL_STATUS                    0x01                    // чтение значений из нескольких регистров флагов
#define MODBUS_FUNCTION_READ_DESCRETE_INPUTS                0x02                    // чтение значений из нескольких дискретных входов
#define MODBUS_FUNCTION_READ_HOLDING_REGISTERS              0x03                    // чтение значений из нескольких регистров хранения
#define MODBUS_FUNCTION_READ_INPUT_REGISTERS                0x04                    // чтение значений из нескольких регистров ввода
#define MODBUS_FUNCTION_FORCE_SINGLE_COIL                   0x05                    // запись значения одного флага
#define MODBUS_FUNCTION_WRITE_SINGLE_REGISTER               0x06                    // запись значения в один регистр хранения
#define MODBUS_FUNCTION_READ_EXCEPTION_STATUS               0x07                    // чтение сигналов состояния
#define MODBUS_FUNCTION_DIAGNOSTIC                          0x08                    // диагностика
#define MODBUS_FUNCTION_GET_COM_EVENT_COUNTER               0x0B                    // чтение счетчика событий
#define MODBUS_FUNCTION_GET_COM_EVENT_LOG                   0x0C                    // чтение журнала событий
#define MODBUS_FUNCTION_FORCE_MULTIPLY_COILS                0x0F                    // запись значений в несколько регистров флагов
#define MODBUS_FUNCTION_WRITE_MULTIPLY_REGISTERS            0x10                    // запись значений в несколько регистров хранения
#define MODBUS_FUNCTION_READ_FILE_RECORD                    0x14                    // чтение из файла
#define MODBUS_FUNCTION_WRITE_FILE_RECORD                   0x15                    // запись в файл
#define MODBUS_FUNCTION_MASK_WRITE_REGISTER                 0x16                    // запись в один регистр хранения с использованием маски "И" и маски "ИЛИ"
#define MODBUS_FUNCTION_READ_FIFO_QUEUE                     0x18                    // чтение данных из очереди
#define MODBUS_FUNCTION_ENCAPSULATED_INTERFACE_TRANSPORT    0x28


#define MODBUS_FUNCTION_ERROR(function_code)        (function_code|0x80)            //


// Error codes
#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION                   0x01                    // Принятый код функции не может быть обработан
#define MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS               0x02                    // Адрес данных, указанный в запросе, недоступен
#define MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE                 0x03                    // Значение, содержащееся в поле данных запроса, является недопустимой величиной
#define MODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE               0x04                    // Невосстанавливаемая ошибка имела место, пока ведомое устройство пыталось выполнить затребованное действие
#define MODBUS_EXCEPTION_ACKNOWLEDGE                        0x05                    // Ведомое устройство приняло запрос и обрабатывает его, но это требует много времени
#define MODBUS_EXCEPTION_SLAVE_DEVICE_BUSY                  0x06                    // Ведомое устройство занято обработкой команды
#define MODBUS_EXCEPTION_CANT_PERFORM_ACTION                0x07                    // Ведомое устройство не может выполнить программную функцию, заданную в запросе
#define MODBUS_EXCEPTION_MEMORY_PARITY_ERROR                0x08                    // Ведомое устройство при чтении расширенной памяти обнаружило ошибку контроля четности
#define MODBUS_EXCEPTION_GATEWAY_PATH_UNAVAILABLE                   0x0A
#define MODBUS_EXCEPTION_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND    0x0B



#endif /* INCLUDE_MODBUS_H_ */

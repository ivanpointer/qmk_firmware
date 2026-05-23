#pragma once

#define EE_HANDS
#define TAPPING_TOGGLE 1
#define EECONFIG_USER_DATA_SIZE 32

#define MATRIX_ROW_PINS { GP6, GP7, GP8, GP9, GP10, GP11, GP12, GP13 }
// #define MATRIX_COL_PINS { GP18, GP19, GP20, GP21, GP22, GP26, GP27 }
#define MATRIX_COL_PINS { GP27, GP26, GP22, GP21, GP20, GP19, GP18 }

/* Split Comms */
#define SERIAL_USART_FULL_DUPLEX
#define SERIAL_USART_TX_PIN GP0
#define SERIAL_USART_RX_PIN GP1
#define SPLIT_TRANSACTION_IDS_USER RPC_ID_STATUS_FRAME, RPC_ID_REMOTE_SENSOR_STATE

/* Trackball */
#define SPI_DRIVER SPID0
#define SPI_SCK_PIN GP2
#define SPI_MISO_PIN GP4
#define SPI_MOSI_PIN GP3
#define PMW33XX_SPI_DIVISOR 256
// #define POINTING_DEVICE_CS_PIN GP5
#define PMW33XX_CS_PIN GP5
#define ROTATIONAL_TRANSFORM_ANGLE -75 // Rotates the trackball sensor data into keyboard orientation
#define POINTING_DEVICE_INVERT_X // Inverts trackball X
#define POINTING_DEVICE_INVERT_X_RIGHT // Inverts right trackball X
#define SPLIT_POINTING_ENABLE
#define POINTING_DEVICE_COMBINED

/* Reset */
// Keep bootloader entry behind the explicit STAT_BOOT key path.
// Double-tap reset can be confused by hosts that repeatedly reset or reject USB.
// #define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
// #define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_LED GP25

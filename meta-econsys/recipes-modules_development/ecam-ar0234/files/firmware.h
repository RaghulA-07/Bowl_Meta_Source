
/*   Local Defines */
#define MAX_BUF_LEN 2048

#define MAX_PAGES 			512
#define TOTAL_PAGES 		1536
#define NUM_ERASE_CYCLES	(TOTAL_PAGES / MAX_PAGES)

#define FLASH_START_ADDRESS 0x08000000
#define FLASH_SIZE 			192*1024
#define FLASH_READ_LEN		256

#define CR 13                   /*   Carriage return */
#define LF 10                   /*   Line feed */

/*MCU Buffer size increased - Fix for loading menu based controls */
#define MCU_BUFFER_SIZE 1024

#define CMD_SIGNATURE		0x43
#define TX_LEN_PKT			5
#define RX_LEN_PKT			6
#define HEADER_FOOTER_SIZE	4
#define CMD_STATUS_MSG_LEN	7

#define VERSION_SIZE			32
#define VERSION_FILE_OFFSET			100

#define MCU_CMD_STATUS_SUCCESS		0x0000
#define MCU_CMD_STATUS_PENDING		0xF000
#define MCU_CMD_STATUS_ISP_PWDN		0x0FF0
#define MCU_CMD_STATUS_ISP_UNINIT	0x0FF1

#define MAX_NUM_FRATES                 10
#define MAX_CTRL_DATA_LEN 		100
#define MAX_CTRL_UI_STRING_LEN 		32
#define MAX_MODE_SUPPORTED              3

/*   TODO: Only necessary commands added */
enum _i2c_cmds
{
        BL_GET_VERSION = 0x01,
        BL_GO = 0x21,
        BL_READ_MEM = 0x11,
        BL_WRITE_MEM = 0x31,
        BL_WRITE_MEM_NS = 0x32,
        BL_ERASE_MEM = 0x44,
        BL_ERASE_MEM_NS = 0x45,
};

enum _i2c_resp
{
        RESP_ACK = 0x79,
        RESP_NACK = 0x1F,
        RESP_BUSY = 0x76,
};

enum 
{
	NUM_LANES_1 = 0x01,
	NUM_LANES_2 = 0x02,
	NUM_LANES_3 = 0x02,
	NUM_LANES_4 = 0x04,
	NUM_LANES_UNKWN = 0xFF,
};

enum _ihex_rectype
{
        /*   Normal data */
        REC_TYPE_DATA = 0x00,
        /*  End of File */
        REC_TYPE_EOF = 0x01,

        /*   Extended Segment Address */
        REC_TYPE_ESA = 0x02,
        /*   Start Segment Address */
        REC_TYPE_SSA = 0x03,

        /*   Extended Linear Address */
        REC_TYPE_ELA = 0x04,
        /*   Start Linear Address */
        REC_TYPE_SLA = 0x05,
};


typedef enum _errno {
	ERRCODE_SUCCESS = 0x00,
	ERRCODE_BUSY = 0x01,
	ERRCODE_INVAL = 0x02,
	ERRCODE_PERM = 0x03,
	ERRCODE_NODEV = 0x04,
	ERRCODE_IO = 0x05,
	ERRCODE_HW_SPEC = 0x06,
	ERRCODE_AGAIN = 0x07,
	ERRCODE_ALREADY = 0x08,
	ERRCODE_NOTIMPL = 0x09,
	ERRCODE_RANGE = 0x0A,

	/*   Reserved 0x0B - 0xFE */

	ERRCODE_UNKNOWN = 0xFF,
} RETCODE;

typedef enum _cmd_id {
	CMD_ID_VERSION = 0x00,
	CMD_ID_GET_SENSOR_ID = 0x01,
	CMD_ID_GET_STREAM_INFO = 0x02,
	CMD_ID_GET_CTRL_INFO = 0x03,
	CMD_ID_INIT_CAM = 0x04,
	CMD_ID_GET_STATUS = 0x05,
	CMD_ID_DE_INIT_CAM = 0x06,
	CMD_ID_STREAM_ON = 0x07,
	CMD_ID_STREAM_OFF = 0x08,
	CMD_ID_STREAM_CONFIG = 0x09,
	CMD_ID_GET_CTRL_UI_INFO = 0x0A,

	/* Reserved 0x0B to 0x0F */

	CMD_ID_GET_CTRL = 0x10,
	CMD_ID_SET_CTRL = 0x11,

	/* Reserved 0x12, 0x13 */

	CMD_ID_FW_UPDT = 0x14,
	CMD_ID_ISP_PDOWN = 0x15,
	CMD_ID_ISP_PUP = 0x16,

	/* Configuring MIPI Lanes */
	CMD_ID_LANE_CONFIG = 0x17,
	CMD_ID_MIPI_CLK_CONFIG = 0x18,

	/* Reserved - 0x17 to 0xFE (except 0x43) */

	CMD_ID_UNKNOWN = 0xFF,

} HOST_CMD_ID;

enum {
	FRAME_RATE_DISCRETE = 0x01,
	FRAME_RATE_CONTINOUS = 0x02,
};

enum {
	CTRL_STANDARD = 0x01,
	CTRL_EXTENDED = 0x02,
};

enum {
/*  0x01 - Integer (32bit)
		0x02 - Long Int (64 bit)
		0x03 - String
		0x04 - Pointer to a 1-Byte Array
		0x05 - Pointer to a 2-Byte Array
		0x06 - Pointer to a 4-Byte Array
		0x07 - Pointer to Generic Data (custom Array)
*/

	EXT_CTRL_TYPE_INTEGER = 0x01,
	EXT_CTRL_TYPE_LONG = 0x02,
	EXT_CTRL_TYPE_STRING = 0x03,
	EXT_CTRL_TYPE_PTR8 = 0x04,
	EXT_CTRL_TYPE_PTR16 = 0x05,
	EXT_CTRL_TYPE_PTR32 = 0x06,
	EXT_CTRL_TYPE_VOID = 0x07,
};


typedef struct __attribute__ ((packed)) _ihex_rec {
        unsigned char datasize;
        unsigned short int addr;
        unsigned char rectype;
        unsigned char recdata[];
} IHEX_RECORD;

unsigned int g_bload_flashaddr = 0x0000;

uint8_t *fw_version = NULL;

/* MCU communication variables */
unsigned char mc_data[MCU_BUFFER_SIZE];
unsigned char mc_ret_data[MCU_BUFFER_SIZE];

/*   Buffer to Send Bootloader CMDs */
unsigned char g_bload_buf[MAX_BUF_LEN] = { 0 };

unsigned short int g_bload_crc16 = 0x0000;

const char g_mcu_fw_buf[] =
#include "ecam25_cuxvr.txt"
    ;



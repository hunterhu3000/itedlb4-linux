#define ITE_DEBUG 0

#if defined(ITE_DEBUG) && ITE_DEBUG > 0
 #define ITE_DBG(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
 #define ITE_DBG(fmt, args...) /* Don't do anything in release builds */
#endif


#define msleep(msecs) nanosleep( \
                &(struct timespec) \
                { msecs / 1000, (msecs * 1000000) % 1000000000UL},\
                NULL);

#if !defined(bool)
#define bool int
#endif
#if !defined(true)
#define true (1 == 1)
#endif
#if !defined(false)
#define false (!true)
#endif

#define ERR_EXIT(errcode) do { perr("   %s\n", libusb_strerror((enum libusb_error)errcode)); return -1; } while (0)
#define CALL_CHECK(fcall) do { r=fcall; if (r < 0) ERR_EXIT(r); } while (0);
#define B(x) (((x)!=0)?1:0)
#define be_to_int32(buf) (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|(buf)[3])

#define RETRY_MAX                     5

// _DLB4_CBW
typedef struct _DLB4_CBW {
        //uint8_t dCBWSignature[4];
        uint32_t dSignature;
        uint32_t dTag;
        uint32_t dDataLength;
        uint8_t bmFlags;
        uint8_t bCBLength;
        uint8_t CB[15];
} DLB4_CBW;

// _DLB4_CSW
typedef struct _DLB4_CSW {
        uint32_t dSignature;
        uint32_t dTag;
        uint32_t dDataResidue;
        uint8_t bStatus;
} DLB4_CSW;

#define DLB4_CBW_Signature 0x43424c44
#define DLB4_CSW_Signature 0x53424c44
#define DLB4_CBW_CBLength 15
#define CSW_CMD_PASSED 0x00
#define CSW_CMD_FAILED 0x01

typedef struct _FlashInfo_
{
        uint8_t readid_mode; //read id command mode
        uint8_t read_mode;   //read command mode
        uint8_t write_mode;  //write command mode
        uint8_t write_type;  //write type
        uint8_t erase_mode;  //erase command mode
        uint8_t erase_type;

}FlashInfo;

FlashInfo Flash;

typedef struct _DLB4_INFO_
{
        uint8_t endpoint_in;
        uint8_t endpoint_out;
        libusb_device_handle *handle;
}DLB4_INFO;

DLB4_INFO devinfo;


typedef struct _DLB4_OP_
{
        uint8_t op_code;
        uint8_t fun_code;
        uint8_t p1;
        uint8_t p2;
        uint8_t p3;
        uint8_t p4;
        uint8_t p5;
        uint8_t p6;
        uint8_t p7;
        uint8_t direction;
        uint8_t *buffer;
        uint32_t size;
        //libusb_device_handle *handle;

}DLB4_OP;

DLB4_OP cmdParam;

#define ITE_FW_CTL               	0xF0
#define ITE_FW_CTL_READ_FW_VER          0x02

#define ITE_OP_CODE_DBGR_X              0xF2
#define ITE_OP_CODE_DBGR_O              0xF3
#define ITE_FUN_CODE_CHIPID_READ        0x00
#define ITE_FUN_CODE_FLASHID_READ       0x01
#define ITE_FUN_CODE_FLASH_READ         0x02
#define ITE_FUN_CODE_FLASH_ERASE        0x03
#define ITE_FUN_CODE_FLASH_WRITE        0x04
#define ITE_FUN_CODE_SetPinDef        	0x05
#define ITE_FUN_CODE_FLASH_R_SPI_STATUS 0x09

#define ITE_FUN_CODE_FLASHID_READ_SPI	0x11
#define ITE_FUN_CODE_FLASH_READ_SPI	0x12
#define ITE_FUN_CODE_FLASH_ERASE_SPI	0x13
#define ITE_FUN_CODE_FLASH_WRITE_SPI	0x14

#define ITE_FUN_CODE_START_D2EC         0x20
#define ITE_FUN_CODE_WRITE_REG          0x23
#define ITE_FUN_CODE_READ_REG           0x24
#define ITE_FUN_CODE_RUN_CTRL           0x27
#define ITE_FUN_CODE_DBGR_CMD_SET       0x2F

#define ITE_DIR_IN                      0x00
#define ITE_DIR_OUT                     0x01

#define ITE_MODE0_STOP_SPECIAL_WAV	0x00
#define ITE_MODE1_SEND_120K_WAV		0x01
#define ITE_MODE2_ENTER_FLASH		0x02
#define ITE_MODE3_ENTER_DEBUG		0x03
#define ITE_MODE4_REST_STOP_EC		0x04
#define ITE_MODE6_REST			0x06
#define ITE_MODE7_SEND_100K_WAV		0x07
#define ITE_MODE10_FLASH_INTERNAL	0x0A
#define ITE_MODE11_FLASH_EXTERNAL	0x0B

#define ITE_ERASE_MODE_0_CHIP_ERASE	0x00
#define ITE_ERASE_MODE_1_SECTOR_ERASE	0x01
#define ITE_ERASE_MODE_2_BLOCK_ERASE	0x02

#define ITE_ERASE_TYPE_3_UNPROTECT_E	0x03

#define ITE_PROGRAM_MODE		0x03
#define ITE_PROGRAM_TYPE		0x00

#define ITE_ERR				0xF0

#define ITE_DLB_GPIO_C1 1
#define ITE_DLB_GPIO_C2 2
#define ITE_DLB_GPIO_H1 3
#define ITE_DLB_GPIO_H2 4
#define ITE_DLB_GPIO_G6 5 // SPI Clock Pin
#define ITE_DLB_GPIO_ALT 	0
#define ITE_DLB_GPIO_OUTPUT 	1
#define ITE_DLB_GPIO_HIGH 	2
#define ITE_DLB_GPIO_LOW 	3

#define ITE_SKIP_CHECK  0x01
#define ITE_SKIP_VERIFY 0x02
#define ITE_USE_SPI 	0x04

#define ITE_CONNECT_MODE_NODBGR	0x02
#define ITE_CONNECT_MODE_DBGR   0x03


static uint16_t VID, PID;
FILE *fi;
unsigned char *g_readbuf;
unsigned char *g_writebuf;
int g_flash_size;
int g_blk_size;
int g_blk_no;
int g_flag=0;

unsigned char g_fw_ver[4];
unsigned char g_chip_id[6];
unsigned char g_flash_id[6];

unsigned char ITE_CONNECT_MODE;

unsigned char ITE_OP_CODE;
unsigned char ITE_FUN_CODE_FLASHID;
unsigned char ITE_FUN_CODE_READ;
unsigned char ITE_FUN_CODE_ERASE;
unsigned char ITE_FUN_CODE_WRITE;

void show_time();
int enter_spi();


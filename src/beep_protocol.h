/* Previous content remains, add new communication-related definitions */

/* Communication method selection */
typedef enum {
    COMM_METHOD_LORAWAN,
    COMM_METHOD_CELLULAR,
    COMM_METHOD_AUTO     /* Automatically select based on availability and signal strength */
} COMM_METHOD_e;

/* Communication configuration */
typedef struct {
    COMM_METHOD_e method;        /* Selected communication method */
    bool auto_fallback;          /* Automatically fallback to other method if primary fails */
    uint16_t retry_count;        /* Number of retries before fallback */
    uint16_t retry_interval;     /* Interval between retries in seconds */
} COMM_CONFIG_s;

/* Communication status */
typedef struct {
    COMM_METHOD_e active_method; /* Currently active communication method */
    bool lorawan_available;      /* LoRaWAN availability */
    bool cellular_available;     /* Cellular availability */
    int8_t lorawan_rssi;        /* LoRaWAN RSSI */
    int8_t cellular_rssi;       /* Cellular RSSI */
    uint16_t failed_transmissions; /* Count of failed transmissions */
    uint32_t last_success_time;   /* Timestamp of last successful transmission */
} COMM_STATUS_s;

/* Command IDs - Add new cellular commands */
typedef enum {
    /* Previous commands remain the same */

    /* Cellular configuration */
    READ_CELLULAR_CONFIG = 0x80,
    WRITE_CELLULAR_CONFIG = 0x81,
    READ_CELLULAR_STATUS = 0x82,
    READ_CELLULAR_SIGNAL = 0x83,
    READ_CELLULAR_INFO = 0x84,

    /* Communication method selection */
    READ_COMM_METHOD = 0x90,
    WRITE_COMM_METHOD = 0x91,
    READ_COMM_STATUS = 0x92,
} BEEP_CID;

/* Status flags - Add new flags */
#define STATUS_CELLULAR_ACTIVE   BIT(8)
#define STATUS_CELLULAR_ERROR    BIT(9)
#define STATUS_AUTO_FALLBACK     BIT(10)

/* Error codes - Add new codes */
#define ERR_CELLULAR_INIT       0x10
#define ERR_CELLULAR_CONNECT    0x11
#define ERR_CELLULAR_SEND       0x12
#define ERR_NO_COMM_METHOD      0x13
#define ERR_FALLBACK_FAILED     0x14

/* Rest of the file remains the same */

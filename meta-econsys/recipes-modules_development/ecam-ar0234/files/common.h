#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include "firmware.h"

#define V4L2_CTRL_CLASS_CAMERA		0x009a0000	
#define V4L2_CID_CAMERA_CLASS_BASE 	(V4L2_CTRL_CLASS_CAMERA | 0x900)

#define V4L2_CID_EXPOSURE_AUTO	        (V4L2_CID_CAMERA_CLASS_BASE + 1)
#define V4L2_CID_EXPOSURE_ABSOLUTE	(V4L2_CID_CAMERA_CLASS_BASE + 2)
#define V4L2_CID_ROI_WINDOW_SIZE        (V4L2_CID_CAMERA_CLASS_BASE + 36)
#define V4L2_CID_ROI_EXPOSURE           (V4L2_CID_CAMERA_CLASS_BASE + 38)
#define V4L2_FRAME_SYNC                 (V4L2_CID_CAMERA_CLASS_BASE + 42)  
#define V4L2_DENOISE                    (V4L2_CID_CAMERA_CLASS_BASE + 45)
#define EXPOSURE_COMPENSATION           (V4L2_CID_CAMERA_CLASS_BASE + 49)


enum ar0234_frame_rate {
	AR0234_60_FPS,
	AR0234_65_FPS,
	AR0234_120_FPS,
	AR0234_NUM_FRAMERATES,
};

static const int ar0234_framerates[] = {
	[AR0234_60_FPS] = 60,
	[AR0234_65_FPS] = 65,
	[AR0234_120_FPS] = 120,
};

struct ar0234_pixfmt {
	u32 code;
	u32 colorspace;
};

static const struct ar0234_pixfmt ar0234_formats[] = {
	{ MEDIA_BUS_FMT_UYVY8_2X8, V4L2_COLORSPACE_SRGB, },
};


/* regulator supplies */
static const char * const ar0234_supply_name[] = {
	"vdddo", /* Digital I/O (1.8V) supply */
	"vdda",  /* Analog (2.8V) supply */
	"vddd",  /* Digital Core (1.5V) supply */
};

#define AR0234_NUM_SUPPLIES ARRAY_SIZE(ar0234_supply_name)

struct reg_value {
	u16 reg;
	u8 val;
};

struct ar0234_mode_info {
	u32 width;
	u32 height;
	const struct reg_value *data;
	u32 data_size;
	u32 pixel_clock;
	u32 link_freq;
	u32 max_fps;
};

typedef struct _isp_ctrl_ui_info {
	struct {
		char ctrl_name[MAX_CTRL_UI_STRING_LEN];
		uint8_t ctrl_ui_type;
		uint8_t ctrl_ui_flags;
	} ctrl_ui_info;

	/* This Struct is valid only if ctrl_ui_type = 0x03 */
	struct {
		uint8_t num_menu_elem;
		char **menu;
	} ctrl_menu_info;
} ISP_CTRL_UI_INFO;

/* Stream and Control Info Struct */
typedef struct _isp_stream_info {
	uint32_t fmt_fourcc;
	uint16_t width;
	uint16_t height;
	uint8_t frame_rate_type;
	union {
		struct {
			uint16_t frame_rate_num;
			uint16_t frame_rate_denom;
		} disc;
		struct {
			uint16_t frame_rate_min_num;
			uint16_t frame_rate_min_denom;
			uint16_t frame_rate_max_num;
			uint16_t frame_rate_max_denom;
			uint16_t frame_rate_step_num;
			uint16_t frame_rate_step_denom;
		} cont;
	} frame_rate;
} ISP_STREAM_INFO;

typedef struct _isp_ctrl_info_std {
	uint32_t ctrl_id;
	uint8_t ctrl_type;
	union {
		struct {
			int32_t ctrl_min;
			int32_t ctrl_max;
			int32_t ctrl_def;
			int32_t ctrl_step;
		} std;
		struct {
			uint8_t val_type;
			uint32_t val_length;
			// This size may vary according to ctrl types
			uint8_t val_data[MAX_CTRL_DATA_LEN];
		} ext;
	} ctrl_data;
	ISP_CTRL_UI_INFO ctrl_ui_data;
} ISP_CTRL_INFO;



struct camera_common_frmfmt {
        struct v4l2_frmsize_discrete    size;
        const int       *framerates;
        int     num_framerates;
        bool    hdr_en;
        int     mode;
};

struct ar0234 {
	struct i2c_client *i2c_client;
	struct device *dev;
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_fwnode_endpoint ep;
	struct v4l2_mbus_framefmt fmt;
	struct v4l2_rect crop;
	struct clk *xclk;

	struct regulator_bulk_data supplies[AR0234_NUM_SUPPLIES];

	const struct ar0234_mode_info *current_mode;
	enum ar0234_frame_rate current_fr;
	struct v4l2_fract frame_interval;

	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *pixel_clock;
	struct v4l2_ctrl *link_freq;

	/* Cached register values */
	u8 aec_pk_manual;
	u8 timing_tc_reg20;
	u8 timing_tc_reg21;

	struct mutex power_lock; /* lock to protect power state */
	int power_count;

	struct gpio_desc *pwdn_gpio;
	struct gpio_desc *rst_gpio;

	bool streaming;
        bool stream_on_set;

	uint16_t mipi_lane_config;
	uint16_t mipi_clk_config;
	uint8_t last_sync_mode;

	int *streamdb;
	uint32_t *ctrldb;

	int num_ctrls;
	ISP_STREAM_INFO *stream_info;
	ISP_CTRL_INFO *mcu_ctrl_info;
	uint16_t prev_index,numfmts;
        struct camera_common_frmfmt *mcu_cam_frmfmt;
        uint32_t format_fourcc;
	struct v4l2_ctrl *ctrl[];

};

static const s64 link_freq[] = {
	224000000,
	336000000,
	400000000
};

static const struct ar0234_mode_info ar0234_mode_info_data[] = {
	{
		.width = 1280,
		.height = 720,
		.pixel_clock = 200000000,   
		.link_freq = 2, /* an index in link_freq[] */
		.max_fps = AR0234_120_FPS
	},
	{
		.width = 1920,
		.height = 1080,
		.pixel_clock = 200000000, 
		.link_freq = 2, /* an index in link_freq[] */
		.max_fps = AR0234_65_FPS
	},
	{
		.width = 1920,
		.height = 1200,
		.pixel_clock = 200000000,
		.link_freq = 2, /* an index in link_freq[] */
		.max_fps = AR0234_60_FPS
	},
};


#define FREE_SAFE(dev, ptr) \
	if(ptr) { \
		devm_kfree(dev, ptr); \
	}

static inline struct ar0234 *to_ar0234(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ar0234, sd);
}

DEFINE_MUTEX(mcu_i2c_mutex);

extern int camera_initialization(struct ar0234 *);
static int ar0234_try_add_ctrls(struct ar0234 *ar0234, int index,ISP_CTRL_INFO * mcu_ctrl);

static int mcu_list_fmts(struct i2c_client *client, ISP_STREAM_INFO *stream_info, int *frm_fmt_size, struct ar0234 *);
static int mcu_list_ctrls(struct i2c_client *client,
			  ISP_CTRL_INFO * mcu_ctrl_info, struct ar0234 *);
static int mcu_get_sensor_id(struct i2c_client *client, uint16_t * sensor_id);
static int mcu_get_cmd_status(struct i2c_client *client, uint8_t * cmd_id,
			      uint16_t * cmd_status, uint8_t * ret_code);
static int mcu_cam_init(struct i2c_client *client);
static int mcu_stream_config(struct i2c_client *client, uint32_t format,
			     int mode, int frate_index);
static int mcu_set_ctrl(struct i2c_client *client, uint32_t ctrl_id,
			uint8_t ctrl_type, int32_t curr_val);
static int mcu_get_ctrl(struct i2c_client *client, uint32_t ctrl_id,
			uint8_t * ctrl_type, int32_t * curr_val);
static int mcu_get_ctrl_ui(struct i2c_client *client,
			   ISP_CTRL_INFO * mcu_ui_info, int index);
static int mcu_fw_update(struct i2c_client *client, unsigned char *txt_fw_version);

static int mcu_cam_stream_on(struct i2c_client *client);
static int mcu_cam_stream_off(struct i2c_client *client);


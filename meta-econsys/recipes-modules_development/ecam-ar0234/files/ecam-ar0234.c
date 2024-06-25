/*
 * Driver for the AR0234 camera sensor.
 *
 */

#include "common.h"

#define DEBUG

#define DEBUG_PRINTK 
#ifndef DEBUG_PRINTK
#define debug_printk(s , ... )
#else
#define debug_printk pr_info
#endif

static int ar0234_read(struct i2c_client *client, u8 * val, u32 count)
{
	int ret;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.buf = val,
	};

	msg.flags = I2C_M_RD;
	msg.len = count;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0)
		goto err;

	return 0;

 err:
	dev_err(&client->dev, "Failed reading register ret = %d!\n", ret);
	return ret;
}

static int ar0234_write(struct i2c_client *client, u8 * val, u32 count)
{
	int ret;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = count,
		.buf = val,
	};

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev, "Failed writing register ret = %d!\n",
			ret);
		return ret;
	}

	return 0;
}

int mcu_bload_ascii2hex(unsigned char ascii)
{
	if (ascii <= '9') {
		return (ascii - '0');
	} else if ((ascii >= 'a') && (ascii <= 'f')) {
		return (0xA + (ascii - 'a'));
	} else if ((ascii >= 'A') && (ascii <= 'F')) {
		return (0xA + (ascii - 'A'));
	}
	return -1;
}

unsigned char errorcheck(char *data, unsigned int len)
{
	unsigned int i = 0;
	unsigned char crc = 0x00;

	for (i = 0; i < len; i++) {
		crc ^= data[i];
	}

	return crc;
}

static int mcu_jump_bload(struct i2c_client *client)
{
	uint32_t payload_len = 0;
	int err = 0;

	/*lock semaphore */
	mutex_lock(&mcu_i2c_mutex);
	/* First Txn Payload length = 0 */
	payload_len = 0;

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_FW_UPDT;
	mc_data[2] = payload_len >> 8;
	mc_data[3] = payload_len & 0xFF;
	mc_data[4] = errorcheck(&mc_data[2], 2);

	err = ar0234_write(client, mc_data, TX_LEN_PKT);
	if (err !=0 ) {
		dev_err(&client->dev, " %s(%d) Error - %d \n",
				__func__, __LINE__, err);
		goto exit;
	}

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_FW_UPDT;
	err = ar0234_write(client, mc_data, 2);
	if (err != 0) {
		dev_err(&client->dev, " %s(%d) Error - %d \n",
			__func__, __LINE__, err);
		goto exit;
	} 

 exit:
	/* unlock semaphore */
	mutex_unlock(&mcu_i2c_mutex);
	return err;

}

static int mcu_stream_config(struct i2c_client *client, uint32_t format,
			     int mode, int frate_index)
{

	struct device *dev = &client->dev;
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ar0234 *ar0234 = to_ar0234(sd);

	uint32_t payload_len = 0;

	uint16_t cmd_status = 0, index = 0xFFFF;
	uint8_t retcode = 0, cmd_id = 0;
	int loop = 0, ret = 0, err = 0, retry = 1000;

	/* lock semaphore */
	mutex_lock(&mcu_i2c_mutex);

	for (loop = 0;(&ar0234->streamdb[loop]) != NULL; loop++) {
		if (ar0234->streamdb[loop] == mode) {
			index = loop + frate_index;
			break;
		}
	}

	debug_printk("Index = 0x%04x , format = 0x%08x, width = %hu,"
		     " height = %hu, frate num = %hu \n", index, format,
		     ar0234->mcu_cam_frmfmt[mode].size.width,
		     ar0234->mcu_cam_frmfmt[mode].size.height,
		     ar0234->mcu_cam_frmfmt[mode].framerates[frate_index]);

	if (index == 0xFFFF) {
		ret = -EINVAL;
		goto exit;
	}

	if(ar0234->prev_index == index) {
		debug_printk("Skipping Previous mode set ... \n");
		ret = 0;
		goto exit;
	}

issue_cmd:
	/* First Txn Payload length = 0 */
	payload_len = 14;

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_STREAM_CONFIG;
	mc_data[2] = payload_len >> 8;
	mc_data[3] = payload_len & 0xFF;
	mc_data[4] = errorcheck(&mc_data[2], 2);

	ar0234_write(client, mc_data, TX_LEN_PKT);

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_STREAM_CONFIG;
	mc_data[2] = index >> 8;
	mc_data[3] = index & 0xFF;

	/* Format Fourcc - currently only UYVY */
	mc_data[4] = format >> 24;
	mc_data[5] = format >> 16;
	mc_data[6] = format >> 8;
	mc_data[7] = format & 0xFF;

	/* width */
	mc_data[8] = ar0234->mcu_cam_frmfmt[mode].size.width >> 8;
	mc_data[9] = ar0234->mcu_cam_frmfmt[mode].size.width & 0xFF;

	/* height */
	mc_data[10] = ar0234->mcu_cam_frmfmt[mode].size.height >> 8;
	mc_data[11] = ar0234->mcu_cam_frmfmt[mode].size.height & 0xFF;

	/* frame rate num */
	mc_data[12] = ar0234->mcu_cam_frmfmt[mode].framerates[frate_index] >> 8;
	mc_data[13] = ar0234->mcu_cam_frmfmt[mode].framerates[frate_index] & 0xFF;

	/* frame rate denom */
	mc_data[14] = 0x00;
	mc_data[15] = 0x01;

	mc_data[16] = errorcheck(&mc_data[2], 14);
	err = ar0234_write(client, mc_data, 17);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	while (--retry > 0) {
		cmd_id = CMD_ID_STREAM_CONFIG;
		if (mcu_get_cmd_status
		    (client, &cmd_id, &cmd_status, &retcode) < 0) {
			dev_err(&client->dev,
				" %s(%d) MCU GET CMD Status Error : loop : %d \n",
				__func__, __LINE__, loop);
			ret = -EIO;
			goto exit;
		}

		if ((cmd_status == MCU_CMD_STATUS_SUCCESS) &&
		    (retcode == ERRCODE_SUCCESS)) {
			ret = 0;
			goto exit;
		}

		if(retcode == ERRCODE_AGAIN) {
			/* Issue Command Again if Set */
			retry = 1000;
			goto issue_cmd;
		}

		if ((retcode != ERRCODE_BUSY) &&
		    ((cmd_status != MCU_CMD_STATUS_PENDING))) {
			dev_err(&client->dev,
				"(%s) %d Error STATUS = 0x%04x RET = 0x%02x\n",
				__func__, __LINE__, cmd_status, retcode);
			ret = -EIO;
			goto exit;
		}

		/* Delay after retry */
		mdelay(10);
	}

	dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
			__LINE__, err);
	ret = -ETIMEDOUT;

exit:
	if(!ret)
		ar0234->prev_index = index;

	/* unlock semaphore */
	mutex_unlock(&mcu_i2c_mutex);

	return ret;
}

static int mcu_get_ctrl(struct i2c_client *client, uint32_t arg_ctrl_id,
			uint8_t * ctrl_type, int32_t * curr_val)
{
	struct device *dev = &client->dev;
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ar0234 *ar0234 = to_ar0234(sd);

	uint32_t payload_len = 0;
	uint8_t errcode = ERRCODE_SUCCESS, orig_crc = 0, calc_crc = 0;
	uint16_t index = 0xFFFF;
	int loop = 0, ret = 0, err = 0;

	uint32_t ctrl_id = 0;

	/* lock semaphore */
	mutex_lock(&mcu_i2c_mutex);

	ctrl_id = arg_ctrl_id;

	/* Read the Ctrl Value from Micro controller */

	for (loop = 0; loop < ar0234->num_ctrls; loop++) {
		if (ar0234->ctrldb[loop] == ctrl_id) {
			index = loop;
			break;
		}
	}

	if (index == 0xFFFF) {
		ret = -EINVAL;
		goto exit;
	}

	/* First Txn Payload length = 2 */
	payload_len = 2;

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_GET_CTRL;
	mc_data[2] = payload_len >> 8;
	mc_data[3] = payload_len & 0xFF;
	mc_data[4] = errorcheck(&mc_data[2], 2);

	ar0234_write(client, mc_data, TX_LEN_PKT);

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_GET_CTRL;
	mc_data[2] = index >> 8;
	mc_data[3] = index & 0xFF;
	mc_data[4] = errorcheck(&mc_data[2], 2);
	err = ar0234_write(client, mc_data, 5);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	err = ar0234_read(client, mc_ret_data, RX_LEN_PKT);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	/* Verify CRC */
	orig_crc = mc_ret_data[4];
	calc_crc = errorcheck(&mc_ret_data[2], 2);
	if (orig_crc != calc_crc) {
		dev_err(&client->dev," %s(%d) CRC 0x%02x != 0x%02x \n",
		       __func__, __LINE__, orig_crc, calc_crc);
		ret = -1;
		goto exit;
	}

	if (((mc_ret_data[2] << 8) | mc_ret_data[3]) == 0) {
		ret = -EIO;
		goto exit;
	}

	errcode = mc_ret_data[5];
	if (errcode != ERRCODE_SUCCESS) {
		dev_err(&client->dev," %s(%d) Errcode - 0x%02x \n",
		       __func__, __LINE__, errcode);
		ret = -EIO;
		goto exit;
	}

	payload_len =
	    ((mc_ret_data[2] << 8) | mc_ret_data[3]) + HEADER_FOOTER_SIZE;
	memset(mc_ret_data, 0x00, payload_len);
	err = ar0234_read(client, mc_ret_data, payload_len);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	/* Verify CRC */
	orig_crc = mc_ret_data[payload_len - 2];
	calc_crc =
	    errorcheck(&mc_ret_data[2], payload_len - HEADER_FOOTER_SIZE);
	if (orig_crc != calc_crc) {
		dev_err(&client->dev," %s(%d) CRC 0x%02x != 0x%02x \n",
		       __func__, __LINE__, orig_crc, calc_crc);
		ret = -EINVAL;
		goto exit;
	}

	/* Verify Errcode */
	errcode = mc_ret_data[payload_len - 1];
	if (errcode != ERRCODE_SUCCESS) {
		dev_err(&client->dev," %s(%d) Errcode - 0x%02x \n",
		       __func__, __LINE__, errcode);
		ret = -EINVAL;
		goto exit;
	}

	/* Ctrl type starts from index 6 */

	*ctrl_type = mc_ret_data[6];

	switch (*ctrl_type) {
	case CTRL_STANDARD:
		*curr_val =
		    mc_ret_data[7] << 24 | mc_ret_data[8] << 16 | mc_ret_data[9]
		    << 8 | mc_ret_data[10];
		break;

	case CTRL_EXTENDED:
		/* Not Implemented */
		break;
	}

 exit:
	/* unlock semaphore */
	mutex_unlock(&mcu_i2c_mutex);

	return ret;
}

static int mcu_set_ctrl(struct i2c_client *client, uint32_t arg_ctrl_id,
			uint8_t ctrl_type, int32_t curr_val)
{
	struct device *dev = &client->dev;
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ar0234 *ar0234 = to_ar0234(sd);

	uint32_t payload_len = 0;

	uint16_t cmd_status = 0, index = 0xFFFF;
	uint8_t retcode = 0, cmd_id = 0;
	int loop = 0, ret = 0, err = 0;
	uint32_t ctrl_id = 0;

        debug_printk("DEBUG: %s\n",__func__);

	/* lock semaphore */
	mutex_lock(&mcu_i2c_mutex);

	ctrl_id = arg_ctrl_id;

	/* call ISP Ctrl config command */

	for (loop = 0; loop < ar0234->num_ctrls; loop++) {
		if (ar0234->ctrldb[loop] == ctrl_id) {
			index = loop;
			break;
		}
	}

	if (index == 0xFFFF) {
		ret = -EINVAL;
		goto exit;
	}

	/* First Txn Payload length = 0 */
	payload_len = 11;

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_SET_CTRL;
	mc_data[2] = payload_len >> 8;
	mc_data[3] = payload_len & 0xFF;
	mc_data[4] = errorcheck(&mc_data[2], 2);

	ar0234_write(client, mc_data, TX_LEN_PKT);

	/* Second Txn */
	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_SET_CTRL;

	/* Index */
	mc_data[2] = index >> 8;
	mc_data[3] = index & 0xFF;

	/* Control ID */
	mc_data[4] = ctrl_id >> 24;
	mc_data[5] = ctrl_id >> 16;
	mc_data[6] = ctrl_id >> 8;
	mc_data[7] = ctrl_id & 0xFF;

	/* Ctrl Type */
	mc_data[8] = ctrl_type;

	/* Ctrl Value */
	mc_data[9] = curr_val >> 24;
	mc_data[10] = curr_val >> 16;
	mc_data[11] = curr_val >> 8;
	mc_data[12] = curr_val & 0xFF;

	/* CRC */
	mc_data[13] = errorcheck(&mc_data[2], 11);

	err = ar0234_write(client, mc_data, 14);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	while (1) {
		cmd_id = CMD_ID_SET_CTRL;
		if (mcu_get_cmd_status
		    (client, &cmd_id, &cmd_status, &retcode) < 0) {
			dev_err(&client->dev," %s(%d) Error \n",
			       __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}

		if ((cmd_status == MCU_CMD_STATUS_SUCCESS) &&
		    (retcode == ERRCODE_SUCCESS)) {
			ret = 0;
			goto exit;
		}

		if ((retcode != ERRCODE_BUSY) &&
		    ((cmd_status != MCU_CMD_STATUS_PENDING))) {
			   dev_err(&client->dev, "(%s) %d ISP Error STATUS = 0x%04x RET = 0x%02x\n",
			     __func__, __LINE__, cmd_status, retcode);
			ret = -EIO;
			goto exit;
		}
	}

 exit:
	/* unlock semaphore */
	mutex_unlock(&mcu_i2c_mutex);

	return ret;
}

static int mcu_list_fmts(struct i2c_client *client, ISP_STREAM_INFO *stream_info, int *frm_fmt_size, struct ar0234 *ar0234)
{
	uint32_t payload_len = 0, err = 0;
	uint8_t errcode = ERRCODE_SUCCESS, orig_crc = 0, calc_crc = 0, skip = 0;
	uint16_t index = 0, mode = 0;

	int loop = 0, num_frates = 0, ret = 0;

	/* Stream Info Variables */

	/* lock semaphore */
	mutex_lock(&mcu_i2c_mutex);

	/* List all formats from MCU and append to mcu_ar0234_frmfmt array */

	for (index = 0;; index++) {
		/* First Txn Payload length = 0 */
		payload_len = 2;

		mc_data[0] = CMD_SIGNATURE;
		mc_data[1] = CMD_ID_GET_STREAM_INFO;
		mc_data[2] = payload_len >> 8;
		mc_data[3] = payload_len & 0xFF;
		mc_data[4] = errorcheck(&mc_data[2], 2);

		ar0234_write(client, mc_data, TX_LEN_PKT);

		mc_data[0] = CMD_SIGNATURE;
		mc_data[1] = CMD_ID_GET_STREAM_INFO;
		mc_data[2] = index >> 8;
		mc_data[3] = index & 0xFF;
		mc_data[4] = errorcheck(&mc_data[2], 2);
		err = ar0234_write(client, mc_data, 5);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) Error - %d \n",
			       __func__, __LINE__, err);
			ret = -EIO;
			goto exit;
		}

		err = ar0234_read(client, mc_ret_data, RX_LEN_PKT);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) Error - %d \n",
			       __func__, __LINE__, err);
			ret = -EIO;
			goto exit;
		}

		/* Verify CRC */
		orig_crc = mc_ret_data[4];
		calc_crc = errorcheck(&mc_ret_data[2], 2);
		if (orig_crc != calc_crc) {
			pr_err
			    (" %s(%d) CRC 0x%02x != 0x%02x \n",
			     __func__, __LINE__, orig_crc, calc_crc);
			ret = -EINVAL;
			goto exit;
		}

		if (((mc_ret_data[2] << 8) | mc_ret_data[3]) == 0) {
			if(stream_info == NULL) {
				*frm_fmt_size = index;
			} else {
				*frm_fmt_size = mode;
			}
			break;
		}

		payload_len =
		    ((mc_ret_data[2] << 8) | mc_ret_data[3]) +
		    HEADER_FOOTER_SIZE;
		errcode = mc_ret_data[5];
		if (errcode != ERRCODE_SUCCESS) {
			pr_err
			    (" %s(%d) Errcode - 0x%02x \n",
			     __func__, __LINE__, errcode);
			ret = -EIO;
			goto exit;
		}

		memset(mc_ret_data, 0x00, payload_len);
		err = ar0234_read(client, mc_ret_data, payload_len);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) Error - %d \n",
			       __func__, __LINE__, err);
			ret = -1;
			goto exit;
		}

		/* Verify CRC */
		orig_crc = mc_ret_data[payload_len - 2];
		calc_crc =
		    errorcheck(&mc_ret_data[2],
				 payload_len - HEADER_FOOTER_SIZE);
		if (orig_crc != calc_crc) {
			pr_err
			    (" %s(%d) CRC 0x%02x != 0x%02x \n",
			     __func__, __LINE__, orig_crc, calc_crc);
			ret = -EINVAL;
			goto exit;
		}

		/* Verify Errcode */
		errcode = mc_ret_data[payload_len - 1];
		if (errcode != ERRCODE_SUCCESS) {
			pr_err
			    (" %s(%d) Errcode - 0x%02x \n",
			     __func__, __LINE__, errcode);
			ret = -EIO;
			goto exit;
		}
		if(stream_info != NULL) {
		/* check if any other format than UYVY is queried - do not append in array */
		stream_info->fmt_fourcc =
		    mc_ret_data[2] << 24 | mc_ret_data[3] << 16 | mc_ret_data[4]
		    << 8 | mc_ret_data[5];
		stream_info->width = mc_ret_data[6] << 8 | mc_ret_data[7];
		stream_info->height = mc_ret_data[8] << 8 | mc_ret_data[9];
		stream_info->frame_rate_type = mc_ret_data[10];

		switch (stream_info->frame_rate_type) {
		case FRAME_RATE_DISCRETE:
			stream_info->frame_rate.disc.frame_rate_num =
			    mc_ret_data[11] << 8 | mc_ret_data[12];

			stream_info->frame_rate.disc.frame_rate_denom =
			    mc_ret_data[13] << 8 | mc_ret_data[14];

			break;

		case FRAME_RATE_CONTINOUS:
			dev_dbg
			    (&client->dev, "The Stream format at index 0x%04x has FRAME_RATE_CONTINOUS,"
			     "which is unsupported !! \n", index);

#if 0
			stream_info.frame_rate.cont.frame_rate_min_num =
			    mc_ret_data[11] << 8 | mc_ret_data[12];
			stream_info.frame_rate.cont.frame_rate_min_denom =
			    mc_ret_data[13] << 8 | mc_ret_data[14];

			stream_info.frame_rate.cont.frame_rate_max_num =
			    mc_ret_data[15] << 8 | mc_ret_data[16];
			stream_info.frame_rate.cont.frame_rate_max_denom =
			    mc_ret_data[17] << 8 | mc_ret_data[18];

			stream_info.frame_rate.cont.frame_rate_step_num =
			    mc_ret_data[19] << 8 | mc_ret_data[20];
			stream_info.frame_rate.cont.frame_rate_step_denom =
			    mc_ret_data[21] << 8 | mc_ret_data[22];
			break;
#endif
			continue;
		}

		switch (stream_info->fmt_fourcc) {
		case V4L2_PIX_FMT_UYVY:
			/* ar0234_codes is already populated with V4L2_MBUS_FMT_UYVY8_1X16 */
			/* check if width and height are already in array - update frame rate only */
			for (loop = 0; loop < (mode); loop++) {
				if ((ar0234->mcu_cam_frmfmt[loop].size.width ==
				     stream_info->width)
				    && (ar0234->mcu_cam_frmfmt[loop].size.height ==
					stream_info->height)) {

					num_frates =
					    ar0234->mcu_cam_frmfmt
					    [loop].num_framerates;
					*((int *)(ar0234->mcu_cam_frmfmt[loop].framerates) + num_frates)
					    = (int)(stream_info->frame_rate.
						    disc.frame_rate_num /
						    stream_info->frame_rate.
						    disc.frame_rate_denom);

					ar0234->mcu_cam_frmfmt
					    [loop].num_framerates++;

					ar0234->streamdb[index] = loop;
					skip = 1;
					break;
				}
			}

			if (skip) {
				skip = 0;
				continue;
			}

			/* Add Width, Height, Frame Rate array, Mode into mcu_ar0234_frmfmt array */
			ar0234->mcu_cam_frmfmt[mode].size.width = stream_info->width;
			ar0234->mcu_cam_frmfmt[mode].size.height =
			    stream_info->height;
			num_frates = ar0234->mcu_cam_frmfmt[mode].num_framerates;

			*((int *)(ar0234->mcu_cam_frmfmt[mode].framerates) + num_frates) =
			    (int)(stream_info->frame_rate.disc.frame_rate_num /
				  stream_info->frame_rate.disc.frame_rate_denom);

			ar0234->mcu_cam_frmfmt[mode].num_framerates++;

			ar0234->mcu_cam_frmfmt[mode].mode = mode;
			ar0234->streamdb[index] = mode;
			mode++;
			break;

		default:
			dev_dbg
			    (&client->dev,"The Stream format at index 0x%04x has format 0x%08x ,"
			     "which is unsupported !! \n", index,
			     stream_info->fmt_fourcc);
		}
		}
	}

 exit:
	/* unlock semaphore */
	mutex_unlock(&mcu_i2c_mutex);

	return ret;
}

static int mcu_get_ctrl_ui(struct i2c_client *client,
			   ISP_CTRL_INFO * mcu_ui_info, int index)
{
	uint32_t payload_len = 0;
	uint8_t errcode = ERRCODE_SUCCESS, orig_crc = 0, calc_crc = 0;
	int ret = 0, i = 0, err = 0;

	/* lock semaphore */
	mutex_lock(&mcu_i2c_mutex);

	/* First Txn Payload length = 0 */
	payload_len = 2;

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_GET_CTRL_UI_INFO;
	mc_data[2] = payload_len >> 8;
	mc_data[3] = payload_len & 0xFF;
	mc_data[4] = errorcheck(&mc_data[2], 2);

	ar0234_write(client, mc_data, TX_LEN_PKT);

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_GET_CTRL_UI_INFO;
	mc_data[2] = index >> 8;
	mc_data[3] = index & 0xFF;
	mc_data[4] = errorcheck(&mc_data[2], 2);
	err = ar0234_write(client, mc_data, 5);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	err = ar0234_read(client, mc_ret_data, RX_LEN_PKT);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	/* Verify CRC */
	orig_crc = mc_ret_data[4];
	calc_crc = errorcheck(&mc_ret_data[2], 2);
	if (orig_crc != calc_crc) {
		dev_err(&client->dev," %s(%d) CRC 0x%02x != 0x%02x \n",
		       __func__, __LINE__, orig_crc, calc_crc);
		ret = -EINVAL;
		goto exit;
	}

	payload_len =
	    ((mc_ret_data[2] << 8) | mc_ret_data[3]) + HEADER_FOOTER_SIZE;
	errcode = mc_ret_data[5];
	if (errcode != ERRCODE_SUCCESS) {
		dev_err(&client->dev," %s(%d) Errcode - 0x%02x \n",
		       __func__, __LINE__, errcode);
		ret = -EINVAL;
		goto exit;
	}

	memset(mc_ret_data, 0x00, payload_len);
	err = ar0234_read(client, mc_ret_data, payload_len);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	/* Verify CRC */
	orig_crc = mc_ret_data[payload_len - 2];
	calc_crc =
	    errorcheck(&mc_ret_data[2], payload_len - HEADER_FOOTER_SIZE);
	if (orig_crc != calc_crc) {
		dev_err(&client->dev," %s(%d) CRC 0x%02x != 0x%02x \n",
		       __func__, __LINE__, orig_crc, calc_crc);
		ret = -EINVAL;
		goto exit;
	}

	/* Verify Errcode */
	errcode = mc_ret_data[payload_len - 1];
	if (errcode != ERRCODE_SUCCESS) {
		dev_err(&client->dev," %s(%d) Errcode - 0x%02x \n",
		       __func__, __LINE__, errcode);
		ret = -EIO;
		goto exit;
	}

	strncpy((char *)mcu_ui_info->ctrl_ui_data.ctrl_ui_info.ctrl_name, &mc_ret_data[2],MAX_CTRL_UI_STRING_LEN);

	mcu_ui_info->ctrl_ui_data.ctrl_ui_info.ctrl_ui_type = mc_ret_data[34];
	mcu_ui_info->ctrl_ui_data.ctrl_ui_info.ctrl_ui_flags = mc_ret_data[35] << 8 |
	    mc_ret_data[36];

	if (mcu_ui_info->ctrl_ui_data.ctrl_ui_info.ctrl_ui_type == V4L2_CTRL_TYPE_MENU) {
		mcu_ui_info->ctrl_ui_data.ctrl_menu_info.num_menu_elem = mc_ret_data[37];

		mcu_ui_info->ctrl_ui_data.ctrl_menu_info.menu =
		    devm_kzalloc(&client->dev,((mcu_ui_info->ctrl_ui_data.ctrl_menu_info.num_menu_elem +1) * sizeof(char *)), GFP_KERNEL);
		for (i = 0; i < mcu_ui_info->ctrl_ui_data.ctrl_menu_info.num_menu_elem; i++) {
			mcu_ui_info->ctrl_ui_data.ctrl_menu_info.menu[i] =
			    devm_kzalloc(&client->dev,MAX_CTRL_UI_STRING_LEN, GFP_KERNEL);
			strncpy((char *)mcu_ui_info->ctrl_ui_data.ctrl_menu_info.menu[i],
				&mc_ret_data[38 +(i *MAX_CTRL_UI_STRING_LEN)], MAX_CTRL_UI_STRING_LEN);

			dev_dbg(&client->dev,"Menu Element %d : %s \n",
				     i, mcu_ui_info->ctrl_ui_data.ctrl_menu_info.menu[i]);
		}

		mcu_ui_info->ctrl_ui_data.ctrl_menu_info.menu[i] = NULL;
	}

 exit:
	/* unlock semaphore */
	mutex_unlock(&mcu_i2c_mutex);

	return ret;

}

static int mcu_mipi_configuration(struct i2c_client *client, struct ar0234 *ar0234, u8 cmd_id)
{
	int ret = 0, err, retry = 1000;
	uint16_t payload_data;
        unsigned char mc_data[10];
        uint32_t payload_len = 0;
        uint16_t cmd_status = 0; 
        uint8_t retcode = 0;

        /* lock semaphore */
        mutex_lock(&mcu_i2c_mutex);

	payload_len = 2; 
		
	mc_data[0] = CMD_SIGNATURE;
        mc_data[1] = cmd_id;
        mc_data[2] = payload_len >> 8;
        mc_data[3] = payload_len & 0xFF;
        mc_data[4] = errorcheck(&mc_data[2], 2);

        ar0234_write(client, mc_data, TX_LEN_PKT);

        /* Second Txn */
        mc_data[0] = CMD_SIGNATURE;
        mc_data[1] = cmd_id;

		switch(cmd_id) {
			case CMD_ID_LANE_CONFIG:
				/*Lane configuration */
				payload_data = ar0234->mipi_lane_config == 4 ? NUM_LANES_4 : NUM_LANES_2; 
				mc_data[2] = payload_data >> 8;
				mc_data[3] = payload_data & 0xFF;
				break;
			case CMD_ID_MIPI_CLK_CONFIG:
				/* MIPI CLK Configuration */
				payload_data = ar0234->mipi_clk_config; 
				mc_data[2] = payload_data >> 8;
				mc_data[3] = payload_data & 0xFF;
				break;
			default:
				dev_err(&client->dev, "MCU MIPI CONF Error\n");
				err = -1;
				goto exit;
		}
		
       	/* CRC */
       	mc_data[4] = errorcheck(&mc_data[2], payload_len);
        err = ar0234_write(client, mc_data, payload_len+3);
	
        if (err != 0) {
                dev_err(&client->dev," %s(%d) MCU Set Ctrl Error - %d \n", __func__,
                       __LINE__, err);
                ret = -1;
                goto exit;
        }

	while (--retry > 0) {
		msleep(20);
                if (mcu_get_cmd_status(client, &cmd_id, &cmd_status, &retcode) <
                    0) {
                        dev_err(&client->dev," %s(%d) MCU Get CMD Status Error \n", __func__,
                               __LINE__);
                        ret = -1;
                        goto exit;
                }

                if ((cmd_status == MCU_CMD_STATUS_ISP_UNINIT) &&
                    (retcode == ERRCODE_SUCCESS)) {
                        ret = 0;
                        goto exit;
                }

                if ((retcode != ERRCODE_BUSY) &&
                    ((cmd_status != MCU_CMD_STATUS_ISP_UNINIT))) {
                       dev_err(&client->dev, 
                           "(%s) %d MCU Get CMD Error STATUS = 0x%04x RET = 0x%02x\n",
                             __func__, __LINE__, cmd_status, retcode);
                        ret = -1;
                        goto exit;
                }
        }
	err = -ETIMEDOUT;

 exit:
        /* unlock semaphore */
        mutex_unlock(&mcu_i2c_mutex);

        return ret;
}

static int mcu_list_ctrls(struct i2c_client *client,
			  ISP_CTRL_INFO * mcu_cam_ctrl, struct ar0234 *ar0234)
{
	uint32_t payload_len = 0;
	uint8_t errcode = ERRCODE_SUCCESS, orig_crc = 0, calc_crc = 0;
	uint16_t index = 0;
	int ret = 0, err = 0,retry=30;

	/* lock semaphore */
	mutex_lock(&mcu_i2c_mutex);

	/* Array of Ctrl Info */
	while (retry-- > 0) {
		/* First Txn Payload length = 0 */
		payload_len = 2;

		mc_data[0] = CMD_SIGNATURE;
		mc_data[1] = CMD_ID_GET_CTRL_INFO;
		mc_data[2] = payload_len >> 8;
		mc_data[3] = payload_len & 0xFF;
		mc_data[4] = errorcheck(&mc_data[2], 2);

		err = ar0234_write(client, mc_data, TX_LEN_PKT);
		if (err != 0) {
                        dev_err(&client->dev," %s(%d) Error - %d \n",
                               __func__, __LINE__, err);
                        continue;
                }
		mc_data[0] = CMD_SIGNATURE;
		mc_data[1] = CMD_ID_GET_CTRL_INFO;
		mc_data[2] = index >> 8;
		mc_data[3] = index & 0xFF;
		mc_data[4] = errorcheck(&mc_data[2], 2);
		err = ar0234_write(client, mc_data, 5);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) Error - %d \n",
			       __func__, __LINE__, err);
			continue;
		}

		err = ar0234_read(client, mc_ret_data, RX_LEN_PKT);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) Error - %d \n",
			       __func__, __LINE__, err);
			continue;
		}

		/* Verify CRC */
		orig_crc = mc_ret_data[4];
		calc_crc = errorcheck(&mc_ret_data[2], 2);
		if (orig_crc != calc_crc) {
			dev_err(&client->dev,
			    " %s(%d) CRC 0x%02x != 0x%02x \n",
			     __func__, __LINE__, orig_crc, calc_crc);
			continue;
		}

		if (((mc_ret_data[2] << 8) | mc_ret_data[3]) == 0) {
			ar0234->num_ctrls = index;
			break;
		}

		payload_len =
		    ((mc_ret_data[2] << 8) | mc_ret_data[3]) +
		    HEADER_FOOTER_SIZE;
		errcode = mc_ret_data[5];
		if (errcode != ERRCODE_SUCCESS) {
			dev_err(&client->dev,
			    " %s(%d) Errcode - 0x%02x \n",
			     __func__, __LINE__, errcode);
			continue;
		}

		memset(mc_ret_data, 0x00, payload_len);
		err = ar0234_read(client, mc_ret_data, payload_len);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) Error - %d \n",
			       __func__, __LINE__, err);
			continue;
		}

		/* Verify CRC */
		orig_crc = mc_ret_data[payload_len - 2];
		calc_crc =
		    errorcheck(&mc_ret_data[2],
				 payload_len - HEADER_FOOTER_SIZE);
		if (orig_crc != calc_crc) {
			dev_err(&client->dev,
			    " %s(%d) CRC 0x%02x != 0x%02x \n",
			     __func__, __LINE__, orig_crc, calc_crc);
			continue;
		}

		/* Verify Errcode */
		errcode = mc_ret_data[payload_len - 1];
		if (errcode != ERRCODE_SUCCESS) {
			dev_err(&client->dev,
			    " %s(%d) Errcode - 0x%02x \n",
			     __func__, __LINE__, errcode);
			continue;
		}

		if(mcu_cam_ctrl != NULL) {

			/* append ctrl info in array */
			mcu_cam_ctrl[index].ctrl_id =
				mc_ret_data[2] << 24 | mc_ret_data[3] << 16 | mc_ret_data[4]
				<< 8 | mc_ret_data[5];
			mcu_cam_ctrl[index].ctrl_type = mc_ret_data[6];

			switch (mcu_cam_ctrl[index].ctrl_type) {
				case CTRL_STANDARD:
					mcu_cam_ctrl[index].ctrl_data.std.ctrl_min =
						mc_ret_data[7] << 24 | mc_ret_data[8] << 16
						| mc_ret_data[9] << 8 | mc_ret_data[10];

					mcu_cam_ctrl[index].ctrl_data.std.ctrl_max =
						mc_ret_data[11] << 24 | mc_ret_data[12] <<
						16 | mc_ret_data[13]
						<< 8 | mc_ret_data[14];

					mcu_cam_ctrl[index].ctrl_data.std.ctrl_def =
						mc_ret_data[15] << 24 | mc_ret_data[16] <<
						16 | mc_ret_data[17]
						<< 8 | mc_ret_data[18];

					mcu_cam_ctrl[index].ctrl_data.std.ctrl_step =
						mc_ret_data[19] << 24 | mc_ret_data[20] <<
						16 | mc_ret_data[21]
						<< 8 | mc_ret_data[22];
					break;

				case CTRL_EXTENDED:
					/* Not Implemented */
					break;
			}

			ar0234->ctrldb[index] = mcu_cam_ctrl[index].ctrl_id;
		}
		index++;
		if(retry == 0) {
			ret = -EIO;
                        goto exit;
		}
	}

 exit:
	/* unlock semaphore */
	mutex_unlock(&mcu_i2c_mutex);

	return ret;

}

static int mcu_get_fw_version(struct i2c_client *client, unsigned char *fw_version, unsigned char *txt_fw_version)
{
	uint32_t payload_len = 0;
	uint8_t errcode = ERRCODE_SUCCESS, orig_crc = 0, calc_crc = 0;
	int ret = 0, err = 0, loop, i=0, retry = 5;
	unsigned long txt_fw_pos = ARRAY_SIZE(g_mcu_fw_buf)-VERSION_FILE_OFFSET;

	/* lock semaphore */
	mutex_lock(&mcu_i2c_mutex);

	debug_printk("DEBUG:  %s\n",__func__);

	/* Get Text Firmware version*/
	for(loop = txt_fw_pos; loop < (txt_fw_pos+64); loop=loop+2) {
		*(txt_fw_version+i) = (mcu_bload_ascii2hex(g_mcu_fw_buf[loop]) << 4 |
				mcu_bload_ascii2hex(g_mcu_fw_buf[loop+1]));
		i++;
	}

	while (retry-- > 0) {
		/* Query firmware version from MCU */
		payload_len = 0;

		mc_data[0] = CMD_SIGNATURE;
		mc_data[1] = CMD_ID_VERSION;
		mc_data[2] = payload_len >> 8;
		mc_data[3] = payload_len & 0xFF;
		mc_data[4] = errorcheck(&mc_data[2], 2);

		err = ar0234_write(client, mc_data, TX_LEN_PKT);

		mc_data[0] = CMD_SIGNATURE;
		mc_data[1] = CMD_ID_VERSION;
		err = ar0234_write(client, mc_data, 2);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) MCU CMD ID Write PKT fw Version Error - %d \n", __func__,
					__LINE__, ret);
			ret = -EIO;
			continue;
		}

		err = ar0234_read(client, mc_ret_data, RX_LEN_PKT);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) MCU CMD ID Read PKT fw Version Error - %d \n", __func__,
					__LINE__, ret);
			ret = -EIO;
			continue;
		}

		/* Verify CRC */
		orig_crc = mc_ret_data[4];
		calc_crc = errorcheck(&mc_ret_data[2], 2);
		if (orig_crc != calc_crc) {
			dev_err(&client->dev," %s(%d) MCU CMD ID fw Version Error CRC 0x%02x != 0x%02x \n",
					__func__, __LINE__, orig_crc, calc_crc);
			ret = -EINVAL;
			continue;
		}

		errcode = mc_ret_data[5];
		if (errcode != ERRCODE_SUCCESS) {
			dev_err(&client->dev," %s(%d) MCU CMD ID fw Errcode - 0x%02x \n", __func__,
					__LINE__, errcode);
			ret = -EIO;
			continue;
		}

		/* Read the actual version from MCU*/
		payload_len =
			((mc_ret_data[2] << 8) | mc_ret_data[3]) + HEADER_FOOTER_SIZE;
		memset(mc_ret_data, 0x00, payload_len);
		err = ar0234_read(client, mc_ret_data, payload_len);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) MCU fw CMD ID Read Version Error - %d \n", __func__,
					__LINE__, ret);
			ret = -EIO;
			continue;
		}

		/* Verify CRC */
		orig_crc = mc_ret_data[payload_len - 2];
		calc_crc = errorcheck(&mc_ret_data[2], 32);
		if (orig_crc != calc_crc) {
			dev_err(&client->dev," %s(%d) MCU fw  CMD ID Version CRC ERROR 0x%02x != 0x%02x \n",
					__func__, __LINE__, orig_crc, calc_crc);
			ret = -EINVAL;
			continue;
		}

		/* Verify Errcode */
		errcode = mc_ret_data[payload_len - 1];
		if (errcode != ERRCODE_SUCCESS) {
			dev_err(&client->dev," %s(%d) MCU fw CMD ID Read Payload Error - 0x%02x \n", __func__,
					__LINE__, errcode);
			ret = -EIO;
			continue;
		}
		if(ret == ERRCODE_SUCCESS) 
			break; 
	}
	if (retry == 0 && ret != ERRCODE_SUCCESS) {
		goto exit;
	}

	for (loop = 0 ; loop < VERSION_SIZE ; loop++ )
		*(fw_version+loop) = mc_ret_data[2+loop];

	/* Check for forced/always update field in the text firmware version*/
	if(txt_fw_version[17] == '1') {
		dev_err(&client->dev, "Forced Update Enabled - Firmware Version - (%.32s) \n",
			fw_version);
		ret = 2;
		goto exit;
	}			

	for(i = 0; i < VERSION_SIZE; i++) {
		if(txt_fw_version[i] != fw_version[i]) {
			dev_dbg(&client->dev, "Previous Firmware Version - (%.32s)\n", fw_version);
			dev_dbg(&client->dev, "Current Firmware Version - (%.32s)\n", txt_fw_version);
			ret = 1;
			goto exit;
		}
	}

	ret = ERRCODE_SUCCESS;
exit:
	/* unlock semaphore */
	mutex_unlock(&mcu_i2c_mutex);

	return ret;
}

static int mcu_get_sensor_id(struct i2c_client *client, uint16_t * sensor_id)
{
	uint32_t payload_len = 0;
	uint8_t errcode = ERRCODE_SUCCESS, orig_crc = 0, calc_crc = 0;

	int ret = 0, err = 0;

	/* lock semaphore */
	mutex_lock(&mcu_i2c_mutex);

	/* Read the version info. from Micro controller */

	/* First Txn Payload length = 0 */
	payload_len = 0;

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_GET_SENSOR_ID;
	mc_data[2] = payload_len >> 8;
	mc_data[3] = payload_len & 0xFF;
	mc_data[4] = errorcheck(&mc_data[2], 2);

	ar0234_write(client, mc_data, TX_LEN_PKT);

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_GET_SENSOR_ID;
	err = ar0234_write(client, mc_data, 2);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	err = ar0234_read(client, mc_ret_data, RX_LEN_PKT);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	/* Verify CRC */
	orig_crc = mc_ret_data[4];
	calc_crc = errorcheck(&mc_ret_data[2], 2);
	if (orig_crc != calc_crc) {
		dev_err(&client->dev," %s(%d) CRC 0x%02x != 0x%02x \n",
		       __func__, __LINE__, orig_crc, calc_crc);
		ret = -EINVAL;
		goto exit;
	}

	errcode = mc_ret_data[5];
	if (errcode != ERRCODE_SUCCESS) {
		dev_err(&client->dev," %s(%d) Errcode - 0x%02x \n",
		       __func__, __LINE__, errcode);
		ret = -EIO;
		goto exit;
	}

	payload_len =
	    ((mc_ret_data[2] << 8) | mc_ret_data[3]) + HEADER_FOOTER_SIZE;

	memset(mc_ret_data, 0x00, payload_len);
	err = ar0234_read(client, mc_ret_data, payload_len);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		ret = -EIO;
		goto exit;
	}

	/* Verify CRC */
	orig_crc = mc_ret_data[payload_len - 2];
	calc_crc = errorcheck(&mc_ret_data[2], 2);
	if (orig_crc != calc_crc) {
		dev_err(&client->dev," %s(%d) CRC 0x%02x != 0x%02x \n",
		       __func__, __LINE__, orig_crc, calc_crc);
		ret = -EINVAL;
		goto exit;
	}

	/* Verify Errcode */
	errcode = mc_ret_data[payload_len - 1];
	if (errcode != ERRCODE_SUCCESS) {
		dev_err(&client->dev," %s(%d) Errcode - 0x%02x \n",
		       __func__, __LINE__, errcode);
		ret = -EIO;
		goto exit;
	}

	*sensor_id = mc_ret_data[2] << 8 | mc_ret_data[3];

 exit:
	/* unlock semaphore */
	mutex_unlock(&mcu_i2c_mutex);

	return ret;
}

int mcu_get_cmd_status(struct i2c_client *client,
			      uint8_t * cmd_id, uint16_t * cmd_status,
			      uint8_t * ret_code)
{
	uint32_t payload_len = 0;
	uint8_t orig_crc = 0, calc_crc = 0;
	int err = 0;

	/* No Semaphore in Get command Status */

	/* First Txn Payload length = 0 */
	payload_len = 1;

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_GET_STATUS;
	mc_data[2] = payload_len >> 8;
	mc_data[3] = payload_len & 0xFF;
	mc_data[4] = errorcheck(&mc_data[2], 2);

	ar0234_write(client, mc_data, TX_LEN_PKT);

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_GET_STATUS;
	mc_data[2] = *cmd_id;
	err = ar0234_write(client, mc_data, 3);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		return -EIO;
	}

	payload_len = CMD_STATUS_MSG_LEN;
	memset(mc_ret_data, 0x00, payload_len);
	err = ar0234_read(client, mc_ret_data, payload_len);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		return -EIO;
	}

	/* Verify CRC */
	orig_crc = mc_ret_data[payload_len - 2];
	calc_crc = errorcheck(&mc_ret_data[2], 3);
	if (orig_crc != calc_crc) {
		dev_err(&client->dev," %s(%d) CRC 0x%02x != 0x%02x \n",
		       __func__, __LINE__, orig_crc, calc_crc);
		return -EINVAL;
	}

	*cmd_id = mc_ret_data[2];
	*cmd_status = mc_ret_data[3] << 8 | mc_ret_data[4];
	*ret_code = mc_ret_data[payload_len - 1];

	return 0;
}

static int mcu_cam_stream_on(struct i2c_client *client)
{
        unsigned char mc_data[100];
        uint32_t payload_len = 0;

        uint16_t cmd_status = 0;
        uint8_t retcode = 0, cmd_id = 0;
	int retry = 5,status_retry=1000, err = 0;

	/*lock semaphore*/
        mutex_lock(&mcu_i2c_mutex);

	while(retry-- < 0) {
		/* First Txn Payload length = 0 */
		payload_len = 0;

        mc_data[0] = CMD_SIGNATURE;
        mc_data[1] = CMD_ID_STREAM_ON;
        mc_data[2] = payload_len >> 8;
        mc_data[3] = payload_len & 0xFF;
        mc_data[4] = errorcheck(&mc_data[2], 2);

        err= ar0234_write(client, mc_data, TX_LEN_PKT);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) MCU Stream On Write Error - %d \n", __func__,
					__LINE__, err);
			continue;
		}

		mc_data[0] = CMD_SIGNATURE;
		mc_data[1] = CMD_ID_STREAM_ON;
		err = ar0234_write(client, mc_data, 2);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) MCU Stream On Write Error - %d \n", __func__,
					__LINE__, err);
			continue;
		}

		while (status_retry-- > 0) {
			/* Some Sleep for init to process */
			yield();

			cmd_id = CMD_ID_STREAM_ON;
			if (mcu_get_cmd_status(client, &cmd_id, &cmd_status, &retcode) <
					0) {
				dev_err(&client->dev," %s(%d) MCU Get CMD Stream On Error \n", __func__,
						__LINE__);
				err = -1;
				goto exit;
			}

			if ((cmd_status == MCU_CMD_STATUS_SUCCESS) &&
					(retcode == ERRCODE_SUCCESS)) {
				dev_dbg(&client->dev,"%s %d MCU Stream On Success !! \n", __func__, __LINE__);
				debug_printk("Func %s retry %d\n",__func__, retry);
				err = 0;
				goto exit;
			}

			if ((retcode != ERRCODE_BUSY) &&
					((cmd_status != MCU_CMD_STATUS_PENDING))) {
				dev_err(&client->dev,
						"(%s) %d MCU Get CMD Stream On Error STATUS = 0x%04x RET = 0x%02x\n",
						__func__, __LINE__, cmd_status, retcode);
				err = -1;
				goto exit;
			}
			mdelay(1);
		}
		if(retry == 0) 
			err = -1;
		break;
	}
 exit:
	/* unlock semaphore */
	        mutex_unlock(&mcu_i2c_mutex);
		return err;

}



static int mcu_cam_stream_off(struct i2c_client *client)
{
        unsigned char mc_data[100];
        uint32_t payload_len = 0;

        uint16_t cmd_status = 0;
        uint8_t retcode = 0, cmd_id = 0;
	int retry = 1000, err = 0;
        /* call ISP init command */

	/*lock semaphore*/
        mutex_lock(&mcu_i2c_mutex);

        /* First Txn Payload length = 0 */
        payload_len = 0;

        mc_data[0] = CMD_SIGNATURE;
        mc_data[1] = CMD_ID_STREAM_OFF;
        mc_data[2] = payload_len >> 8;
        mc_data[3] = payload_len & 0xFF;
        mc_data[4] = errorcheck(&mc_data[2], 2);

        ar0234_write(client, mc_data, TX_LEN_PKT);

        mc_data[0] = CMD_SIGNATURE;
        mc_data[1] = CMD_ID_STREAM_OFF;
        err = ar0234_write(client, mc_data, 2);
        if (err != 0) {
                dev_err(&client->dev," %s(%d) MCU Stream OFF Write Error - %d \n", __func__,
                       __LINE__, err);
                goto exit;
        }

        while (--retry > 0) {
                /* Some Sleep for init to process */
                yield();

                cmd_id = CMD_ID_STREAM_OFF;
                if (mcu_get_cmd_status(client, &cmd_id, &cmd_status, &retcode) <
                    0) {
                       dev_err(&client->dev," %s(%d) MCU Get CMD Stream Off Error \n", __func__,
                               __LINE__);
		       err = -1;
                        goto exit;
                }

                if ((cmd_status == MCU_CMD_STATUS_SUCCESS) &&
                    (retcode == ERRCODE_SUCCESS)) {
                        dev_dbg(&client->dev,"%s %d MCU Get CMD Stream off Success !! \n", __func__, __LINE__ );
			err = 0;
                        goto exit;
                }

                if ((retcode != ERRCODE_BUSY) &&
                    ((cmd_status != MCU_CMD_STATUS_PENDING))) {
                       dev_err(&client->dev, 
                            "(%s) %d MCU Get CMD Stream off Error STATUS = 0x%04x RET = 0x%02x\n",
                             __func__, __LINE__, cmd_status, retcode);
		       err = -1;
                        goto exit;
                }
		mdelay(1);
        }
exit:
	/* unlock semaphore */
	mutex_unlock(&mcu_i2c_mutex);
	return err;
}

static int mcu_cam_init(struct i2c_client *client)
{
	uint32_t payload_len = 0;

	uint16_t cmd_status = 0;
	uint8_t retcode = 0, cmd_id = 0;
	int retry = 1000, err = 0;
	
	/* check current status - if initialized, no need for Init */
	cmd_id = CMD_ID_INIT_CAM;
	if (mcu_get_cmd_status(client, &cmd_id, &cmd_status, &retcode) < 0) {
		dev_err(&client->dev," %s(%d) Error \n", __func__, __LINE__);
		return -EIO;
	}

	if ((cmd_status == MCU_CMD_STATUS_SUCCESS) &&
	    (retcode == ERRCODE_SUCCESS)) {
		dev_err(&client->dev," Already Initialized !! \n");
		return 0;
	}

	/* call ISP init command */

	/* First Txn Payload length = 0 */
	payload_len = 0;

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_INIT_CAM;
	mc_data[2] = payload_len >> 8;
	mc_data[3] = payload_len & 0xFF;
	mc_data[4] = errorcheck(&mc_data[2], 2);

	ar0234_write(client, mc_data, TX_LEN_PKT);

	mc_data[0] = CMD_SIGNATURE;
	mc_data[1] = CMD_ID_INIT_CAM;
	err = ar0234_write(client, mc_data, 2);
	if (err != 0) {
		dev_err(&client->dev," %s(%d) Error - %d \n", __func__,
		       __LINE__, err);
		return -EIO;
	}

	while (--retry > 0) {
		/* Some Sleep for init to process */
		mdelay(500);

		cmd_id = CMD_ID_INIT_CAM;
		if (mcu_get_cmd_status
		    (client, &cmd_id, &cmd_status, &retcode) < 0) {
			dev_err(&client->dev," %s(%d) Error \n",
			       __func__, __LINE__);
			return -EIO;
		}

		if ((cmd_status == MCU_CMD_STATUS_SUCCESS) &&
		    ((retcode == ERRCODE_SUCCESS) || (retcode == ERRCODE_ALREADY))) {
			dev_err(&client->dev,"ISP Already Initialized !! \n");
			//dev_err(" ISP Already Initialized !! \n");
			return 0;
		}

		if ((retcode != ERRCODE_BUSY) &&
		    ((cmd_status != MCU_CMD_STATUS_PENDING))) {
			dev_err(&client->dev,
			    "(%s) %d Init Error STATUS = 0x%04x RET = 0x%02x\n",
			     __func__, __LINE__, cmd_status, retcode);
			return -EIO;
		}
	}
	dev_err(&client->dev,"ETIMEDOUT Error\n");
	return -ETIMEDOUT;
}

unsigned short int mcu_bload_calc_crc16(unsigned char *buf, int len)
{
	unsigned short int crc = 0;
	int i = 0;

	if (!buf || !(buf + len))
		return 0;

	for (i = 0; i < len; i++) {
		crc ^= buf[i];
	}

	return crc;
}

unsigned char mcu_bload_inv_checksum(unsigned char *buf, int len)
{
	unsigned int checksum = 0x00;
	int i = 0;

	if (!buf || !(buf + len))
		return 0;

	for (i = 0; i < len; i++) {
		checksum = (checksum + buf[i]);
	}

	checksum &= (0xFF);
	return (~(checksum) + 1);
}

int mcu_bload_get_version(struct i2c_client *client)
{
	int ret = 0;

	/*----------------------------- GET VERSION -------------------- */

	/*   Write Get Version CMD */
	g_bload_buf[0] = BL_GET_VERSION;
	g_bload_buf[1] = ~(BL_GET_VERSION);

	ret = ar0234_write(client, g_bload_buf, 2);
	if (ret < 0) {
		dev_err(&client->dev,"Write Failed \n");
		return -1;
	}

	/*   Wait for ACK or NACK */
	ret = ar0234_read(client, g_bload_buf, 1);
	if (ret < 0) {
		dev_err(&client->dev,"Read Failed \n");
		return -1;
	}

	if (g_bload_buf[0] != 'y') {
		/*   NACK Received */
		dev_err(&client->dev," NACK Received... exiting.. \n");
		return -1;
	}

	ret = ar0234_read(client, g_bload_buf, 1);
	if (ret < 0) {
		dev_err(&client->dev,"Read Failed \n");
		return -1;
	}

	ret = ar0234_read(client, g_bload_buf, 1);
	if (ret < 0) {
		dev_err(&client->dev,"Read Failed\n");
		return -1;
	}

	/* ---------------- GET VERSION END ------------------- */

	return 0;
}

int mcu_bload_parse_send_cmd(struct i2c_client *client,
			     unsigned char *bytearray, int rec_len)
{
	IHEX_RECORD *ihex_rec = NULL;
	unsigned char checksum = 0, calc_checksum = 0;
	int i = 0, ret = 0;

	if (!bytearray)
		return -1;

	ihex_rec = (IHEX_RECORD *) bytearray;
	ihex_rec->addr = htons(ihex_rec->addr);

	checksum = bytearray[rec_len - 1];

	calc_checksum = mcu_bload_inv_checksum(bytearray, rec_len - 1);
	if (checksum != calc_checksum) {
		dev_err(&client->dev," Invalid Checksum 0x%02x != 0x%02x !! \n",
		       checksum, calc_checksum);
		return -1;
	}

	if ((ihex_rec->rectype == REC_TYPE_ELA)
	    && (ihex_rec->addr == 0x0000)
	    && (ihex_rec->datasize = 0x02)) {
		/*   Upper 32-bit configuration */
		g_bload_flashaddr = (ihex_rec->recdata[0] <<
				     24) | (ihex_rec->recdata[1]
					    << 16);

		dev_dbg(&client->dev,"Updated Flash Addr = 0x%08x \n",
			     g_bload_flashaddr);

	} else if (ihex_rec->rectype == REC_TYPE_DATA) {
		/*   Flash Data into Flashaddr */

		g_bload_flashaddr =
		    (g_bload_flashaddr & 0xFFFF0000) | (ihex_rec->addr);
		g_bload_crc16 ^=
		    mcu_bload_calc_crc16(ihex_rec->recdata, ihex_rec->datasize);

		/*   Write Erase Pages CMD */
		g_bload_buf[0] = BL_WRITE_MEM_NS;
		g_bload_buf[1] = ~(BL_WRITE_MEM_NS);

		ret = ar0234_write(client, g_bload_buf, 2);
		if (ret < 0) {
			dev_err(&client->dev,"Write Failed \n");
			return -1;
		}

		/*   Wait for ACK or NACK */
		ret = ar0234_read(client, g_bload_buf, 1);
		if (ret < 0) {
			dev_err(&client->dev,"Read Failed \n");
			return -1;
		}

		if (g_bload_buf[0] != RESP_ACK) {
			/*   NACK Received */
			dev_err(&client->dev," NACK Received... exiting.. \n");
			return -1;
		}

		g_bload_buf[0] = (g_bload_flashaddr & 0xFF000000) >> 24;
		g_bload_buf[1] = (g_bload_flashaddr & 0x00FF0000) >> 16;
		g_bload_buf[2] = (g_bload_flashaddr & 0x0000FF00) >> 8;
		g_bload_buf[3] = (g_bload_flashaddr & 0x000000FF);
		g_bload_buf[4] =
		    g_bload_buf[0] ^ g_bload_buf[1] ^ g_bload_buf[2] ^
		    g_bload_buf[3];

		ret = ar0234_write(client, g_bload_buf, 5);
		if (ret < 0) {
			dev_err(&client->dev,"Write Failed \n");
			return -1;
		}

		/*   Wait for ACK or NACK */
		ret = ar0234_read(client, g_bload_buf, 1);
		if (ret < 0) {
			dev_err(&client->dev,"Read Failed \n");
			return -1;
		}

		if (g_bload_buf[0] != RESP_ACK) {
			/*   NACK Received */
			dev_err(&client->dev," NACK Received... exiting.. \n");
			return -1;
		}

		g_bload_buf[0] = ihex_rec->datasize - 1;
		checksum = g_bload_buf[0];
		for (i = 0; i < ihex_rec->datasize; i++) {
			g_bload_buf[i + 1] = ihex_rec->recdata[i];
			checksum ^= g_bload_buf[i + 1];
		}

		g_bload_buf[i + 1] = checksum;

		ret = ar0234_write(client, g_bload_buf, i + 2);
		if (ret < 0) {
			dev_err(&client->dev,"Write Failed \n");
			return -1;
		}

 poll_busy:
		/*   Wait for ACK or NACK */
		ret = ar0234_read(client, g_bload_buf, 1);
		if (ret < 0) {
			dev_err(&client->dev,"Read Failed \n");
			return -1;
		}

		if (g_bload_buf[0] == RESP_BUSY)
			goto poll_busy;

		if (g_bload_buf[0] != RESP_ACK) {
			/*   NACK Received */
			dev_err(&client->dev," NACK Received... exiting.. \n");
			return -1;
		}

	} else if (ihex_rec->rectype == REC_TYPE_SLA) {
		/*   Update Instruction pointer to this address */

	} else if (ihex_rec->rectype == REC_TYPE_EOF) {
		/*   End of File - Issue I2C Go Command */
		return 0;
	} else {

		/*   Unhandled Type */
		dev_err(&client->dev,"Unhandled Command Type \n");
		return -1;
	}

	return 0;
}

int mcu_bload_go(struct i2c_client *client)
{
	int ret = 0;

	g_bload_buf[0] = BL_GO;
	g_bload_buf[1] = ~(BL_GO);

	ret = ar0234_write(client, g_bload_buf, 2);
	if (ret < 0) {
		dev_err(&client->dev,"Write Failed \n");
		return -1;
	}

	ret = ar0234_read(client, g_bload_buf, 1);
	if (ret < 0) {
		dev_err(&client->dev,"Failed Read 1 \n");
		return -1;
	}

	/*   Start Address */
	g_bload_buf[0] = (FLASH_START_ADDRESS & 0xFF000000) >> 24;
	g_bload_buf[1] = (FLASH_START_ADDRESS & 0x00FF0000) >> 16;
	g_bload_buf[2] = (FLASH_START_ADDRESS & 0x0000FF00) >> 8;
	g_bload_buf[3] = (FLASH_START_ADDRESS & 0x000000FF);
	g_bload_buf[4] =
	    g_bload_buf[0] ^ g_bload_buf[1] ^ g_bload_buf[2] ^ g_bload_buf[3];

	ret = ar0234_write(client, g_bload_buf, 5);
	if (ret < 0) {
		dev_err(&client->dev,"Write Failed \n");
		return -1;
	}

	ret = ar0234_read(client, g_bload_buf, 1);
	if (ret < 0) {
		dev_err(&client->dev,"Failed Read 1 \n");
		return -1;
	}

	if (g_bload_buf[0] != RESP_ACK) {
		/*   NACK Received */
		dev_err(&client->dev," NACK Received... exiting.. \n");
		return -1;
	}

	return 0;
}

int mcu_bload_update_fw(struct i2c_client *client)
{
	/* exclude NULL character at end of string */
	unsigned long hex_file_size = ARRAY_SIZE(g_mcu_fw_buf) - 1;
	unsigned char wbuf[MAX_BUF_LEN];
	int i = 0, recindex = 0, ret = 0;

	for (i = 0; i < hex_file_size; i++) {
		if ((recindex == 0) && (g_mcu_fw_buf[i] == ':')) {
			/*  debug_printk("Start of a Record \n"); */
		} else if (g_mcu_fw_buf[i] == CR) {
			/*   No Implementation */
		} else if (g_mcu_fw_buf[i] == LF) {
			if (recindex == 0) {
				/*   Parsing Complete */
				break;
			}

			/*   Analyze Packet and Send Commands */
			ret = mcu_bload_parse_send_cmd(client, wbuf, recindex);
			if (ret < 0) {
				dev_err(&client->dev,"Error in Processing Commands \n");
				break;
			}

			recindex = 0;

		} else {
			/*   Parse Rec Data */
			if ((ret = mcu_bload_ascii2hex(g_mcu_fw_buf[i])) < 0) {
				dev_err(&client->dev,
					"Invalid Character - 0x%02x !! \n",
				     g_mcu_fw_buf[i]);
				break;
			}

			wbuf[recindex] = (0xF0 & (ret << 4));
			i++;

			if ((ret = mcu_bload_ascii2hex(g_mcu_fw_buf[i])) < 0) {
				dev_err(&client->dev,
				    "Invalid Character - 0x%02x !!!! \n",
				     g_mcu_fw_buf[i]);
				break;
			}

			wbuf[recindex] |= (0x0F & ret);
			recindex++;
		}
	}

	dev_dbg(&client->dev,"Program FLASH Success !! - CRC = 0x%04x \n",
		     g_bload_crc16);

	/* ------------ PROGRAM FLASH END ----------------------- */

	return ret;
}

int mcu_bload_erase_flash(struct i2c_client *client)
{
	unsigned short int pagenum = 0x0000;
	int ret = 0, i = 0, checksum = 0;

	/* --------------- ERASE FLASH --------------------- */

	for (i = 0; i < NUM_ERASE_CYCLES; i++) {

		checksum = 0x00;
		/*   Write Erase Pages CMD */
		g_bload_buf[0] = BL_ERASE_MEM_NS;
		g_bload_buf[1] = ~(BL_ERASE_MEM_NS);

		ret = ar0234_write(client, g_bload_buf, 2);
		if (ret < 0) {
			dev_err(&client->dev,"Write Failed \n");
			return -1;
		}

		/*   Wait for ACK or NACK */
		ret = ar0234_read(client, g_bload_buf, 1);
		if (ret < 0) {
			dev_err(&client->dev,"Read Failed \n");
			return -1;
		}

		if (g_bload_buf[0] != RESP_ACK) {
			/*   NACK Received */
			dev_err(&client->dev," NACK Received... exiting.. \n");
			return -1;
		}

		g_bload_buf[0] = (MAX_PAGES - 1) >> 8;
		g_bload_buf[1] = (MAX_PAGES - 1) & 0xFF;
		g_bload_buf[2] = g_bload_buf[0] ^ g_bload_buf[1];

		ret = ar0234_write(client, g_bload_buf, 3);
		if (ret < 0) {
			dev_err(&client->dev,"Write Failed \n");
			return -1;
		}

		/*   Wait for ACK or NACK */
		ret = ar0234_read(client, g_bload_buf, 1);
		if (ret < 0) {
			dev_err(&client->dev,"Read Failed \n");
			return -1;
		}

		if (g_bload_buf[0] != RESP_ACK) {
			/*   NACK Received */
			dev_err(&client->dev," NACK Received... exiting.. \n");
			return -1;
		}

		for (pagenum = 0; pagenum < MAX_PAGES; pagenum++) {
			g_bload_buf[(2 * pagenum)] =
			    (pagenum + (i * MAX_PAGES)) >> 8;
			g_bload_buf[(2 * pagenum) + 1] =
			    (pagenum + (i * MAX_PAGES)) & 0xFF;
			checksum =
			    checksum ^ g_bload_buf[(2 * pagenum)] ^
			    g_bload_buf[(2 * pagenum) + 1];
		}
		g_bload_buf[2 * MAX_PAGES] = checksum;

		ret = ar0234_write(client, g_bload_buf, (2 * MAX_PAGES) + 1);
		if (ret < 0) {
			dev_err(&client->dev,"Write Failed \n");
			return -1;
		}

 poll_busy:
		/*   Wait for ACK or NACK */
		ret = ar0234_read(client, g_bload_buf, 1);
		if (ret < 0) {
			dev_err(&client->dev,"Read Failed \n");
			return -1;
		}

		if (g_bload_buf[0] == RESP_BUSY)
			goto poll_busy;

		if (g_bload_buf[0] != RESP_ACK) {
			/*   NACK Received */
			dev_err(&client->dev," NACK Received... exiting.. \n");
			return -1;
		}

		dev_dbg(&client->dev," ERASE Sector %d success !! \n", i + 1);
	}

	/* ------------ ERASE FLASH END ----------------------- */

	return 0;
}

int mcu_bload_read(struct i2c_client *client,
		   unsigned int g_bload_flashaddr, char *bytearray,
		   unsigned int len)
{
	int ret = 0;

	g_bload_buf[0] = BL_READ_MEM;
	g_bload_buf[1] = ~(BL_READ_MEM);

	ret = ar0234_write(client, g_bload_buf, 2);
	if (ret < 0) {
		dev_err(&client->dev,"Write Failed \n");
		return -1;
	}

	/*   Wait for ACK or NACK */
	ret = ar0234_read(client, g_bload_buf, 1);
	if (ret < 0) {
		dev_err(&client->dev,"Read Failed \n");
		return -1;
	}

	if (g_bload_buf[0] != RESP_ACK) {
		/*   NACK Received */
		dev_err(&client->dev," NACK Received... exiting.. \n");
		return -1;
	}

	g_bload_buf[0] = (g_bload_flashaddr & 0xFF000000) >> 24;
	g_bload_buf[1] = (g_bload_flashaddr & 0x00FF0000) >> 16;
	g_bload_buf[2] = (g_bload_flashaddr & 0x0000FF00) >> 8;
	g_bload_buf[3] = (g_bload_flashaddr & 0x000000FF);
	g_bload_buf[4] =
	    g_bload_buf[0] ^ g_bload_buf[1] ^ g_bload_buf[2] ^ g_bload_buf[3];

	ret = ar0234_write(client, g_bload_buf, 5);
	if (ret < 0) {
		dev_err(&client->dev,"Write Failed \n");
		return -1;
	}

	/*   Wait for ACK or NACK */
	ret = ar0234_read(client, g_bload_buf, 1);
	if (ret < 0) {
		dev_err(&client->dev,"Read Failed \n");
		return -1;
	}

	if (g_bload_buf[0] != RESP_ACK) {
		/*   NACK Received */
		dev_err(&client->dev," NACK Received... exiting.. \n");
		return -1;
	}

	g_bload_buf[0] = len - 1;
	g_bload_buf[1] = ~(len - 1);

	ret = ar0234_write(client, g_bload_buf, 2);
	if (ret < 0) {
		dev_err(&client->dev,"Write Failed \n");
		return -1;
	}

	/*   Wait for ACK or NACK */
	ret = ar0234_read(client, g_bload_buf, 1);
	if (ret < 0) {
		dev_err(&client->dev,"Read Failed \n");
		return -1;
	}

	if (g_bload_buf[0] != RESP_ACK) {
		/*   NACK Received */
		dev_err(&client->dev," NACK Received... exiting.. \n");
		return -1;
	}

	ret = ar0234_read(client, bytearray, len);
	if (ret < 0) {
		dev_err(&client->dev,"Read Failed \n");
		return -1;
	}

	return 0;
}

int mcu_bload_verify_flash(struct i2c_client *client,
			   unsigned short int orig_crc)
{
	char bytearray[FLASH_READ_LEN];
	unsigned short int calc_crc = 0;
	unsigned int flash_addr = FLASH_START_ADDRESS, i = 0;

	while ((i + FLASH_READ_LEN) <= FLASH_SIZE) {
		memset(bytearray, 0x0, FLASH_READ_LEN);

		if (mcu_bload_read
		    (client, flash_addr + i, bytearray, FLASH_READ_LEN) < 0) {
			dev_err(&client->dev," i2c_bload_read FAIL !! \n");
			return -1;
		}

		calc_crc ^= mcu_bload_calc_crc16(bytearray, FLASH_READ_LEN);
		i += FLASH_READ_LEN;
	}

	if ((FLASH_SIZE - i) > 0) {
		memset(bytearray, 0x0, FLASH_READ_LEN);

		if (mcu_bload_read
		    (client, flash_addr + i, bytearray, (FLASH_SIZE - i))
		    < 0) {
			dev_err(&client->dev," i2c_bload_read FAIL !! \n");
			return -1;
		}

		calc_crc ^= mcu_bload_calc_crc16(bytearray, FLASH_READ_LEN);
	}

	if (orig_crc != calc_crc) {
		dev_err(&client->dev," CRC verification fail !! 0x%04x != 0x%04x \n",
		       orig_crc, calc_crc);
		return -1;
	}

	dev_dbg(&client->dev,"CRC Verification Success 0x%04x == 0x%04x \n",
		     orig_crc, calc_crc);

	return 0;
}

static int mcu_fw_update(struct i2c_client *client, unsigned char *mcu_fw_version)
{
	int ret = 0;
	g_bload_crc16 = 0;

	/* Read Firmware version from bootloader MCU */
	ret = mcu_bload_get_version(client);
	if (ret < 0) {
		dev_err(&client->dev," Error in Get Version \n");
		goto exit;
	}

	dev_dbg(&client->dev,"Get Version SUCCESS !! \n");

	/* Erase firmware present in the MCU and flash new firmware*/
	ret = mcu_bload_erase_flash(client);
	if (ret < 0) {
		dev_err(&client->dev," Error in Erase Flash \n");
		goto exit;
	}

	dev_dbg(&client->dev,"Erase Flash Success !! \n");

	/* Read the firmware present in the text file */
	if ((ret = mcu_bload_update_fw(client)) < 0) {
		dev_err(&client->dev," Write Flash FAIL !! \n");
		goto exit;
	}

	/* Verify the checksum for the update firmware */
	if ((ret = mcu_bload_verify_flash(client, g_bload_crc16)) < 0) {
		dev_err(&client->dev," verify_flash FAIL !! \n");
		goto exit;
	}

	/* Reverting from bootloader mode */
	/* I2C GO Command */
	if ((ret = mcu_bload_go(client)) < 0) {
		dev_err(&client->dev," i2c_bload_go FAIL !! \n");
		goto exit;
	}
	
	if(mcu_fw_version) {
		dev_dbg(&client->dev,"(%s) - Firmware Updated - (%.32s).",
				__func__, mcu_fw_version);
	}
 exit:
	return ret;
}


int camera_initialization(struct ar0234 *ar0234)
{
        struct i2c_client *client = ar0234->i2c_client;  
	struct device *dev = &client->dev;
	uint32_t mipi_lane = 0, mipi_clk = 0;
	int ret,loop,frm_fmt_size;
	unsigned char fw_version[32] = {0}, txt_fw_version[32] = {0};
	int err = 0, pwdn_gpio_toggle = 0;
	int retry = 5,i=0;
	uint16_t sensor_id = 0;

	debug_printk("DEBUG:  %s\n",__func__);

	ret = of_property_read_u32(dev->of_node, "camera-mipi-clk", &mipi_clk);
	if (ret) {
		dev_err(dev, "could not get camera-mipi-clk\n");
		return ret;
	}

	ret = of_property_read_u32(dev->of_node,"camera-mipi-lanes", &mipi_lane);
	if (ret) {
		dev_err(dev, "could not get camera-mipi-lanes\n");
		return ret;
	}

 	debug_printk("DEBUG: lane config received is %d\n",mipi_lane);
 	debug_printk("DEBUG: clk config received is %d\n",mipi_clk);

        ar0234->mipi_lane_config = mipi_lane;    
        ar0234->mipi_clk_config = mipi_clk;
	ar0234->last_sync_mode = 1;

	/*MCU Reset Release*/
	gpiod_set_value_cansleep(ar0234->pwdn_gpio, 0);
	msleep(10);
	gpiod_set_value_cansleep(ar0234->rst_gpio, 0);
	msleep(10);
	gpiod_set_value_cansleep(ar0234->rst_gpio, 1);
	msleep(50);

	ret = mcu_get_fw_version(client, fw_version, txt_fw_version);

		if (ret != 0) 
		{
			
			pr_info("Trying to Detect Bootloader mode .. \n");

			gpiod_set_value_cansleep(ar0234->rst_gpio, 0);
			msleep(10);
			gpiod_set_value_cansleep(ar0234->pwdn_gpio, 1);
			msleep(100);
			gpiod_set_value_cansleep(ar0234->rst_gpio, 1);
			msleep(100);
	
			for(loop = 0;loop < 10; loop++) 
			{
				err = mcu_bload_get_version(client);
				if (err < 0) {
					/* Trial and Error for 1 second (100ms * 10) */
					msleep(100);
					continue;
				} else {
					dev_err(dev," Get Bload Version Success\n");
					pwdn_gpio_toggle = 1;
					break;
				}
			}
		
		if(loop == 10) {
			dev_err(dev, "Error updating firmware \n");
			return -EINVAL;
		}

		if (mcu_fw_update(client, NULL) < 0)
			return -EFAULT;

		if( pwdn_gpio_toggle == 1)
			gpiod_set_value_cansleep(ar0234->pwdn_gpio, 0);

		/* Allow FW Updated Driver to reboot */
		msleep(10);

		for(loop = 0;loop < 100; loop++) {
			err = mcu_get_fw_version(client, fw_version, txt_fw_version);
			if (err < 0) {
				/* Trial and Error for 10 seconds (100ms * 100) */
				msleep(100);
				continue;
			} else {
				dev_err(dev," Get FW Version Success\n");
				break;
			}
		}
		if(loop == 100) {
			dev_err(dev, "Error updating firmware \n");
			return -EINVAL;
		}						
		debug_printk("Current Firmware Version - (%.32s)",fw_version);

	} else {
		/* Same firmware version in MCU and Text File */
		pr_info("DEBUG: Current Firmware Version - (%.32s)",fw_version);
	}


	retry = 5;
	while (--retry >= 0) {
		if (mcu_mipi_configuration(client, ar0234, CMD_ID_LANE_CONFIG) < 0) {
			dev_err(dev, "%s, mcu_mipi_configuration lane failure. retrying \n", __func__);
			continue;
		} else {
		  	break;
		}
	}
	if (retry < 0) {
	  	dev_err(dev, "%s,  Failed mcu_mipi_configuration  lane \n", __func__);
		return -EFAULT;
	}

	retry = 5;
	while (--retry >= 0) {
		if (mcu_mipi_configuration(client, ar0234, CMD_ID_MIPI_CLK_CONFIG) < 0) {
			dev_err(dev, "%s, mcu_mipi_configuration clk failure. retrying \n", __func__);
			continue;
		} else {
		  	break;
		}
	}
	if (retry < 0) {
	  	dev_err(dev, "%s, Failed mcu_mipi_configuration clk \n", __func__);
		return -EFAULT;
	}	

	/* Query the number of controls from MCU*/
	retry = 5;
	while (--retry >= 0) {
		if(mcu_list_ctrls(client, NULL, ar0234) < 0) {
			dev_err(dev, "%s, init controls failure. retrying \n", __func__);
			continue;
		} else {
		  	break;
		}
	}
	if (retry < 0) {
	  	dev_err(dev, "%s, Failed to init controls \n", __func__);
		return -EFAULT;
	}

	//Query the number for Formats available from MCU
	if(mcu_list_fmts(client, NULL, &frm_fmt_size,ar0234) < 0) {
		dev_err(dev, "%s, Failed to init formats \n", __func__);
		return -EFAULT;
	}
        ar0234->numfmts=frm_fmt_size; 

//memory_allocations

	ar0234->mcu_ctrl_info = devm_kzalloc(dev, sizeof(ISP_CTRL_INFO) * ar0234->num_ctrls, GFP_KERNEL);
	if(!ar0234->mcu_ctrl_info) {
		debug_printk("Unable to allocate memory \n");
		return -ENOMEM;
	}

	ar0234->ctrldb = devm_kzalloc(dev, sizeof(uint32_t) * ar0234->num_ctrls, GFP_KERNEL);
	if(!ar0234->ctrldb) {
		debug_printk("Unable to allocate memory \n");
		return -ENOMEM;
	}

	ar0234->stream_info = devm_kzalloc(dev, sizeof(ISP_STREAM_INFO) * (frm_fmt_size + 1), GFP_KERNEL);
	if(!ar0234->stream_info) {
		debug_printk("Unable to allocate memory \n");
		return -ENOMEM;
	}

	ar0234->streamdb = devm_kzalloc(dev, sizeof(int) * (frm_fmt_size + 1), GFP_KERNEL);
	if(!ar0234->streamdb) {
		debug_printk("Unable to allocate memory \n");
		return -ENOMEM;
	}

	ar0234->mcu_cam_frmfmt = devm_kzalloc(dev, sizeof(struct camera_common_frmfmt) * (frm_fmt_size), GFP_KERNEL);
	if(!ar0234->mcu_cam_frmfmt) {
		debug_printk("Unable to allocate memory \n");
		return -ENOMEM;
	}


	if (mcu_get_sensor_id(client, &sensor_id) < 0) {
		debug_printk("Unable to get MCU Sensor ID \n");
		return -EFAULT;
	}


	if (mcu_cam_init(client) < 0) {
		debug_printk("Unable to INIT ISP \n");
		return -EFAULT;
	}
        else
	{
		debug_printk(" INIT ISP Success \n");
	}

	for(loop = 0; loop < frm_fmt_size; loop++) {
		ar0234->mcu_cam_frmfmt[loop].framerates = devm_kzalloc(dev, sizeof(int) * MAX_NUM_FRATES, GFP_KERNEL);
		if(!ar0234->mcu_cam_frmfmt[loop].framerates) {
			debug_printk("Unable to allocate memory \n");
			return -ENOMEM;
		}
	}

	// Enumerate Formats 
	if (mcu_list_fmts(client, ar0234->stream_info, &frm_fmt_size,ar0234) < 0) 
	{
		debug_printk("Unable to List Fmts \n");
		return -EFAULT;
	}


	/* Initialise controls. */
        ISP_CTRL_INFO *mcu_cam_ctrls=ar0234->mcu_ctrl_info;
	if (mcu_list_ctrls(client,mcu_cam_ctrls,ar0234) < 0) {
		debug_printk(" Failed to init ctrls\n");
		return -EFAULT;
	}

	v4l2_ctrl_handler_init(&ar0234->ctrls, ar0234->num_ctrls+1);

	for (i = 0; i < ar0234->num_ctrls; i++) {

		if (mcu_cam_ctrls[i].ctrl_type == CTRL_STANDARD) {
				ar0234_try_add_ctrls(ar0234, i,
						     &mcu_cam_ctrls[i]);
		} else {
			debug_printk(" Invalid format\n");
		}
	}

return 0;
}



static const struct ar0234_mode_info *
ar0234_find_mode(struct ar0234 *sensor, enum ar0234_frame_rate fr,
		 int width, int height, bool nearest)
{
	const struct ar0234_mode_info *mode;

	mode = v4l2_find_nearest_size(ar0234_mode_info_data,
			       ARRAY_SIZE(ar0234_mode_info_data),
			       width, height,
			       width, height);
	if (!mode ||
	    (!nearest && (mode->width != width || mode->height != height)))
		return NULL;

	/* Check to see if the current mode exceeds the max frame rate */
	if (ar0234_framerates[fr] > ar0234_framerates[mode->max_fps])
		return NULL;

	return mode;
}


static int ar0234_set_power_on(struct ar0234 *ar0234)
{
	int ret;

	ret = regulator_bulk_enable(AR0234_NUM_SUPPLIES, ar0234->supplies);
	if (ret < 0)
		return ret;

	ret = clk_prepare_enable(ar0234->xclk);
	if (ret < 0) {
		dev_err(ar0234->dev, "clk prepare enable failed\n");
		regulator_bulk_disable(AR0234_NUM_SUPPLIES, ar0234->supplies);
		return ret;
	}
#if 0
	usleep_range(5000, 15000);
	gpiod_set_value_cansleep(ar0234->pwdn_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ar0234->rst_gpio, 0);
#endif
	msleep(20);

	return 0;
}

static void ar0234_set_power_off(struct ar0234 *ar0234)
{
#if 0	
	gpiod_set_value_cansleep(ar0234->rst_gpio, 1);
	gpiod_set_value_cansleep(ar0234->pwdn_gpio, 0);
#endif
	clk_disable_unprepare(ar0234->xclk);
	regulator_bulk_disable(AR0234_NUM_SUPPLIES, ar0234->supplies);
}

static int ar0234_s_power(struct v4l2_subdev *sd, int on)
{
	struct ar0234 *ar0234 = to_ar0234(sd);
	int ret = 0;
	debug_printk("DEBUG:  %s\n",__func__);

	mutex_lock(&mcu_i2c_mutex);

	/* If the power count is modified from 0 to != 0 or from != 0 to 0,
	 * update the power state.
	 */
	if (ar0234->power_count == !on) {
		if (on) {
			ret = ar0234_set_power_on(ar0234);
			if (ret < 0)
				goto exit;
			usleep_range(500, 1000);
		} else {
			ar0234_set_power_off(ar0234);
		}
	}


	/* Update the power count. */
	ar0234->power_count += on ? 1 : -1;
	WARN_ON(ar0234->power_count < 0);

exit:
	mutex_unlock(&mcu_i2c_mutex);


	return ret;
}

static int ar0234_try_frame_interval(struct ar0234 *sensor,
				     struct v4l2_fract *fi,
				     u32 width, u32 height)
{
	const struct ar0234_mode_info *mode;
	enum ar0234_frame_rate rate = AR0234_65_FPS;
	int minfps, maxfps, best_fps, fps;
	int i;

	minfps = ar0234_framerates[AR0234_60_FPS];
	maxfps = ar0234_framerates[AR0234_120_FPS];

	if (fi->numerator == 0) {
		fi->denominator = maxfps;
		fi->numerator = 1;
		rate = AR0234_60_FPS;
		goto find_mode;
	}

	fps = clamp_val(DIV_ROUND_CLOSEST(fi->denominator, fi->numerator),
			minfps, maxfps);

	best_fps = minfps;
	fps = clamp_val(DIV_ROUND_CLOSEST(fi->denominator, fi->numerator),
			minfps, maxfps);

	for (i = 0; i < ARRAY_SIZE(ar0234_framerates); i++) {
		int curr_fps = ar0234_framerates[i];

		if (abs(curr_fps - fps) < abs(best_fps - fps)) {
			best_fps = curr_fps;
			rate = i;
		}
	}

	fi->numerator = 1;
	fi->denominator = best_fps;

find_mode:
	mode = ar0234_find_mode(sensor, rate, width, height, false);
	return mode ? rate : -EINVAL;
}



static int ar0234_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ar0234 *ar0234 = container_of(ctrl->handler,
					     struct ar0234, ctrls);
	int ret=0;

	debug_printk("DEBUG:  %s\n",__func__);
	debug_printk("DEBUG: ctrl->id = 0x%x , ctrl->val = %d \n",ctrl->id,ctrl->val);

//mutex handled inside mcu_set_ctrl
//	mutex_lock(&mcu_i2c_mutex);

	if (!ar0234->power_count) {
		mutex_unlock(&mcu_i2c_mutex);
		return 0;
	}

	switch (ctrl->id) {
 	case V4L2_CID_BRIGHTNESS:
 	case V4L2_CID_CONTRAST:
	case V4L2_CID_SATURATION:
	case V4L2_CID_AUTO_WHITE_BALANCE:	
        case V4L2_CID_GAMMA:
	case V4L2_CID_GAIN:
	case V4L2_CID_HFLIP:
	case V4L2_CID_VFLIP:
        case V4L2_CID_WHITE_BALANCE_TEMPERATURE:	
        case V4L2_CID_SHARPNESS:
	case V4L2_CID_EXPOSURE_AUTO:
	case V4L2_CID_EXPOSURE_ABSOLUTE:
//	case V4L2_CID_ROI_WINDOW_SIZE:
//	case V4L2_CID_ROI_EXPOSURE:
	case V4L2_FRAME_SYNC:
	case V4L2_DENOISE:
	case EXPOSURE_COMPENSATION:
		debug_printk("DEBUG: Filtered:  ctrl->id = 0x%x , ctrl->val = %d \n",ctrl->id,ctrl->val);
		if ((ret = mcu_set_ctrl(ar0234->i2c_client, ctrl->id, CTRL_STANDARD, ctrl->val)) < 0)
		{
			dev_err(ar0234->dev,"ERR: %s (%d ) \n", __func__, __LINE__);
			return -EINVAL;
		}
		break;
	default:
		break;
	}

//	mutex_unlock(&mcu_i2c_mutex);
	return ret;
}

static const struct v4l2_ctrl_ops ar0234_ctrl_ops = {
	.s_ctrl = ar0234_s_ctrl,
};

static int ar0234_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	debug_printk("DEBUG:  %s\n",__func__);
	if (code->index > 0)
		return -EINVAL;

	code->code = MEDIA_BUS_FMT_UYVY8_2X8;

	return 0;
}

static int ar0234_enum_frame_size(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	debug_printk("DEBUG:  %s\n",__func__);
	if (fse->code != MEDIA_BUS_FMT_UYVY8_2X8)
		return -EINVAL;

	if (fse->index >= ARRAY_SIZE(ar0234_mode_info_data))
		return -EINVAL;

	fse->min_width = ar0234_mode_info_data[fse->index].width;
	fse->max_width = ar0234_mode_info_data[fse->index].width;
	fse->min_height = ar0234_mode_info_data[fse->index].height;
	fse->max_height = ar0234_mode_info_data[fse->index].height;

	return 0;
}

static int ar0234_enum_frame_interval(
	struct v4l2_subdev *sd,
	struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_frame_interval_enum *fie)
{
	struct ar0234 *sensor = to_ar0234(sd);
	struct v4l2_fract tpf;
	int ret;
	debug_printk("DEBUG:  %s\n",__func__);

	if (fie->pad != 0)
		return -EINVAL;
	if (fie->index >= AR0234_NUM_FRAMERATES)
		return -EINVAL;

	tpf.numerator = 1;
	tpf.denominator = ar0234_framerates[fie->index];

	ret = ar0234_try_frame_interval(sensor, &tpf,
					fie->width, fie->height);
	if (ret < 0)
		return -EINVAL;

	fie->interval = tpf;
	return 0;
}

static int ar0234_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct ar0234 *sensor = to_ar0234(sd);
	debug_printk("DEBUG:  %s\n",__func__);

	mutex_lock(&mcu_i2c_mutex);
	fi->interval = sensor->frame_interval;
	mutex_unlock(&mcu_i2c_mutex);

	return 0;
}

static int ar0234_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct ar0234 *sensor = to_ar0234(sd);
	const struct ar0234_mode_info *mode;
	int frame_rate, ret = 0;

	debug_printk("DEBUG:  %s\n",__func__);

	if (fi->pad != 0)
		return -EINVAL;

	mutex_lock(&mcu_i2c_mutex);

	if (sensor->streaming) {
		ret = -EBUSY;
		goto out;
	}

	mode = sensor->current_mode;

	frame_rate = ar0234_try_frame_interval(sensor, &fi->interval,
					       mode->width, mode->height);
	if (frame_rate < 0)
		frame_rate = AR0234_65_FPS;

	mode = ar0234_find_mode(sensor, frame_rate, mode->width,
				mode->height, true);
	if (!mode) {
		ret = -EINVAL;
		goto out;
	}

	if (mode != sensor->current_mode ||
	    frame_rate != sensor->current_fr) {
		sensor->current_fr = frame_rate;
		sensor->frame_interval = fi->interval;
		sensor->current_mode = mode;
	}
out:
	mutex_unlock(&mcu_i2c_mutex);
	return ret;
}

static struct v4l2_mbus_framefmt *
__ar0234_get_pad_format(struct ar0234 *ar0234,
			struct v4l2_subdev_pad_config *cfg,
			unsigned int pad,
			enum v4l2_subdev_format_whence which)
{
	switch (which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		return v4l2_subdev_get_try_format(&ar0234->sd, cfg, pad);
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		return &ar0234->fmt;
	default:
		return NULL;
	}
}

static int ar0234_get_format(struct v4l2_subdev *sd,
			     struct v4l2_subdev_pad_config *cfg,
			     struct v4l2_subdev_format *format)
{
	struct ar0234 *ar0234 = to_ar0234(sd);
	debug_printk("DEBUG:  %s\n",__func__);

	format->format = *__ar0234_get_pad_format(ar0234, cfg, format->pad,
						  format->which);
	return 0;
}

static struct v4l2_rect *
__ar0234_get_pad_crop(struct ar0234 *ar0234, struct v4l2_subdev_pad_config *cfg,
		      unsigned int pad, enum v4l2_subdev_format_whence which)
{
	switch (which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		return v4l2_subdev_get_try_crop(&ar0234->sd, cfg, pad);
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		return &ar0234->crop;
	default:
		return NULL;
	}
}

static int ar0234_set_format(struct v4l2_subdev *sd,
			     struct v4l2_subdev_pad_config *cfg,
			     struct v4l2_subdev_format *format)
{
	struct ar0234 *ar0234 = to_ar0234(sd);
	struct v4l2_mbus_framefmt *__format;
	struct v4l2_rect *__crop;
	const struct ar0234_mode_info *new_mode;
	int ret,mode_index=0;

	debug_printk("DEBUG:  %s\n",__func__);

	__crop = __ar0234_get_pad_crop(ar0234, cfg, format->pad,
			format->which);

	new_mode = v4l2_find_nearest_size(ar0234_mode_info_data,
			       ARRAY_SIZE(ar0234_mode_info_data),
			       width, height,
			       format->format.width, format->format.height);

	__crop->width = new_mode->width;
	__crop->height = new_mode->height;

	if (format->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
		ret = v4l2_ctrl_s_ctrl_int64(ar0234->pixel_clock,
					     new_mode->pixel_clock);
		if (ret < 0)
			return ret;

		ret = v4l2_ctrl_s_ctrl(ar0234->link_freq,
				       new_mode->link_freq);
		if (ret < 0)
			return ret;

		ar0234->current_mode = new_mode;
	}

	__format = __ar0234_get_pad_format(ar0234, cfg, format->pad,
			format->which);
	__format->width = __crop->width;
	__format->height = __crop->height;
	__format->code = MEDIA_BUS_FMT_UYVY8_2X8;
	__format->field = V4L2_FIELD_NONE;
	__format->colorspace = V4L2_COLORSPACE_SRGB;

	format->format = *__format;

	for(mode_index=0; mode_index<MAX_MODE_SUPPORTED; ++mode_index)
	{
		if(new_mode->height == ar0234_mode_info_data[mode_index].height)
			break;
	}
 	mcu_stream_config(ar0234->i2c_client, ar0234->format_fourcc, mode_index,0);
 
	return 0;
}

static int ar0234_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
{
	struct v4l2_subdev_format fmt = { 0 };
	debug_printk("DEBUG:  %s\n",__func__);

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 1920;
	fmt.format.height = 1080;

//	ar0234_set_format(subdev, cfg, &fmt);

	return 0;
}

static int ar0234_get_selection(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_selection *sel)
{
	struct ar0234 *ar0234 = to_ar0234(sd);

	debug_printk("DEBUG:  %s\n",__func__);
	if (sel->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;

	sel->r = *__ar0234_get_pad_crop(ar0234, cfg, sel->pad,
					sel->which);
	return 0;
}

static int ar0234_s_stream(struct v4l2_subdev *subdev, int enable)
{
	struct ar0234 *ar0234 = to_ar0234(subdev);
	int ret;
	debug_printk("DEBUG: %s\n",__func__);

	pr_info("DEBUG: width = %d height = %d \n",ar0234->fmt.width,ar0234->fmt.height);

	if (enable) {

		debug_printk("DEBUG: Stream on\n");
       		
		mcu_cam_stream_on(ar0234->i2c_client);

		ret = v4l2_ctrl_handler_setup(&ar0234->ctrls);
		if (ret < 0) {
			dev_err(ar0234->dev, "could not sync v4l2 controls\n");
			return ret;
		}

		ar0234->streaming = enable;
	} else {
		
		debug_printk("DEBUG: Stream off\n");

       		mcu_cam_stream_off(ar0234->i2c_client);

		ar0234->streaming = false;

	}

	return 0;
}



static int ar0234_try_add_ctrls(struct ar0234 *ar0234, int index,
				ISP_CTRL_INFO * mcu_ctrl)
{
	struct i2c_client *client = ar0234->i2c_client;
	struct device *dev = &client->dev;
	struct v4l2_ctrl_config custom_ctrl_config;

	ar0234->ctrls.error = 0;
	/* Try Enumerating in standard controls */
	ar0234->ctrl[index] =
	    v4l2_ctrl_new_std(&ar0234->ctrls,
			      &ar0234_ctrl_ops,
			      mcu_ctrl->ctrl_id,
			      mcu_ctrl->ctrl_data.std.ctrl_min,
			      mcu_ctrl->ctrl_data.std.ctrl_max,
			      mcu_ctrl->ctrl_data.std.ctrl_step,
			      mcu_ctrl->ctrl_data.std.ctrl_def);
	if (ar0234->ctrl[index] != NULL) {
		pr_info("%d. Initialized Control 0x%08x - %s \n",
			     index, mcu_ctrl->ctrl_id,
			     ar0234->ctrl[index]->name);
		return 0;
	}

	if(mcu_ctrl->ctrl_id == V4L2_CID_EXPOSURE_AUTO)
		goto custom;


	/* Try Enumerating in standard menu */
	ar0234->ctrls.error = 0;
	ar0234->ctrl[index] =
	    v4l2_ctrl_new_std_menu(&ar0234->ctrls,
				   &ar0234_ctrl_ops,
				   mcu_ctrl->ctrl_id,
				   mcu_ctrl->ctrl_data.std.ctrl_max,
				   0, mcu_ctrl->ctrl_data.std.ctrl_def);
	if (ar0234->ctrl[index] != NULL) {
		pr_info("%d. Initialized Control Menu 0x%08x - %s \n",
			     index, mcu_ctrl->ctrl_id,
			     ar0234->ctrl[index]->name);
		return 0;
	}


custom:
	ar0234->ctrls.error = 0;
	memset(&custom_ctrl_config, 0x0, sizeof(struct v4l2_ctrl_config));

	if (mcu_get_ctrl_ui(client, mcu_ctrl, index)!= ERRCODE_SUCCESS) {
		dev_err(dev, "Error Enumerating Control 0x%08x !! \n",
			mcu_ctrl->ctrl_id);
		return -EIO;
	}

	/* Fill in Values for Custom Ctrls */
	custom_ctrl_config.ops = &ar0234_ctrl_ops;
	custom_ctrl_config.id = mcu_ctrl->ctrl_id;
	/* Do not change the name field for the control */
	custom_ctrl_config.name = mcu_ctrl->ctrl_ui_data.ctrl_ui_info.ctrl_name;

	/* Sample Control Type and Flags */
	custom_ctrl_config.type = mcu_ctrl->ctrl_ui_data.ctrl_ui_info.ctrl_ui_type;
	custom_ctrl_config.flags = mcu_ctrl->ctrl_ui_data.ctrl_ui_info.ctrl_ui_flags;

	custom_ctrl_config.min = mcu_ctrl->ctrl_data.std.ctrl_min;
	custom_ctrl_config.max = mcu_ctrl->ctrl_data.std.ctrl_max;
	custom_ctrl_config.step = mcu_ctrl->ctrl_data.std.ctrl_step;
	custom_ctrl_config.def = mcu_ctrl->ctrl_data.std.ctrl_def;

	if (custom_ctrl_config.type == V4L2_CTRL_TYPE_MENU) {
		custom_ctrl_config.step = 0;
		custom_ctrl_config.type_ops = NULL;

		custom_ctrl_config.qmenu =
			(const char *const *)(mcu_ctrl->ctrl_ui_data.ctrl_menu_info.menu);
	}

	ar0234->ctrl[index] =
	    v4l2_ctrl_new_custom(&ar0234->ctrls,
				 &custom_ctrl_config, NULL);
	if (ar0234->ctrl[index] != NULL) {
		pr_info("%d. Initialized Custom Ctrl 0x%08x - %s \n",
			     index, mcu_ctrl->ctrl_id,
			     ar0234->ctrl[index]->name);
		return 0;
	}

	dev_err(dev,
		"%d.  default: Failed to init 0x%08x ctrl Error - %d \n",
		index, mcu_ctrl->ctrl_id, ar0234->ctrls.error);
	return -EINVAL;
}


static const struct v4l2_subdev_core_ops ar0234_core_ops = {
	.s_power = ar0234_s_power,
};

static const struct v4l2_subdev_video_ops ar0234_video_ops = {
//	.g_frame_interval = ar0234_g_frame_interval,
//	.s_frame_interval = ar0234_s_frame_interval,
	.s_stream = ar0234_s_stream,
};

static const struct v4l2_subdev_pad_ops ar0234_subdev_pad_ops = {
	.init_cfg = ar0234_entity_init_cfg,
	.enum_mbus_code = ar0234_enum_mbus_code,
	.enum_frame_size = ar0234_enum_frame_size,
	.get_fmt = ar0234_get_format,
	.set_fmt = ar0234_set_format,
//	.get_selection = ar0234_get_selection,
//	.enum_frame_interval = ar0234_enum_frame_interval,
};

static const struct v4l2_subdev_ops ar0234_subdev_ops = {
	.core = &ar0234_core_ops,
	.video = &ar0234_video_ops,
	.pad = &ar0234_subdev_pad_ops,
};

static int ar0234_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct device_node *endpoint;
	struct ar0234 *ar0234;
	u8 chip_id_high, chip_id_low;
	unsigned int i;
	u32 xclk_freq;
	int ret,mode=1;

	pr_info("DEBUG:  %s\n",__func__);

	ar0234 = devm_kzalloc(dev, sizeof(struct ar0234), GFP_KERNEL);
	if (!ar0234)
		return -ENOMEM;

	ar0234->i2c_client = client;
	ar0234->dev = dev;

	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!endpoint) {
		dev_err(dev, "endpoint node not found\n");
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(of_fwnode_handle(endpoint),
					 &ar0234->ep);

	of_node_put(endpoint);

	if (ret < 0) {
		dev_err(dev, "parsing endpoint node failed\n");
		return ret;
	}

	if (ar0234->ep.bus_type != V4L2_MBUS_CSI2_DPHY) {
		dev_err(dev, "invalid bus type, must be CSI2\n");
		return -EINVAL;
	}

	for (i = 0; i < AR0234_NUM_SUPPLIES; i++)
		ar0234->supplies[i].supply = ar0234_supply_name[i];

	ret = devm_regulator_bulk_get(dev, AR0234_NUM_SUPPLIES,
				      ar0234->supplies);
	if (ret < 0)
		return ret;


	ar0234->pwdn_gpio = devm_gpiod_get(dev, "pwdn", GPIOD_OUT_HIGH);
	if (IS_ERR(ar0234->pwdn_gpio)) {
		dev_err(dev, "cannot get pwdn-gpios \n");
		return PTR_ERR(ar0234->pwdn_gpio);
	}

	ar0234->rst_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ar0234->rst_gpio)) {
		dev_err(dev, "cannot get reset-gpios \n");
		return PTR_ERR(ar0234->rst_gpio);
	}

        camera_initialization(ar0234);


	mutex_init(&mcu_i2c_mutex);

	ar0234->pixel_clock = v4l2_ctrl_new_std(&ar0234->ctrls,
						&ar0234_ctrl_ops,
						V4L2_CID_PIXEL_RATE,
						1, INT_MAX, 1, 1);
	ar0234->link_freq = v4l2_ctrl_new_int_menu(&ar0234->ctrls,
						   &ar0234_ctrl_ops,
						   V4L2_CID_LINK_FREQ,
						   ARRAY_SIZE(link_freq) - 1,
						   0, link_freq);
	if (ar0234->link_freq)
		ar0234->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ar0234->sd.ctrl_handler = &ar0234->ctrls;

	if (ar0234->ctrls.error) {
		dev_err(dev, "%s: control initialization error %d\n",
		       __func__, ar0234->ctrls.error);
		ret = ar0234->ctrls.error;
		goto free_ctrl;
	}

	v4l2_i2c_subdev_init(&ar0234->sd, client, &ar0234_subdev_ops);
	ar0234->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	ar0234->pad.flags = MEDIA_PAD_FL_SOURCE;
	ar0234->sd.dev = &client->dev;
	ar0234->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	ret = media_entity_pads_init(&ar0234->sd.entity, 1, &ar0234->pad);
	if (ret < 0) {
		dev_err(dev, "could not register media entity\n");
		goto free_ctrl;
	}

	ret = ar0234_s_power(&ar0234->sd, true);
	if (ret < 0) {
		dev_err(dev, "could not power up AR0234\n");
		goto free_entity;
	}

	ar0234_s_power(&ar0234->sd, false);

	ret = v4l2_async_register_subdev(&ar0234->sd);
	if (ret < 0) {
		dev_err(dev, "could not register v4l2 device\n");
		goto free_entity;
	}

	ar0234_entity_init_cfg(&ar0234->sd, NULL);

       ar0234->format_fourcc = V4L2_PIX_FMT_UYVY;
       mcu_stream_config(client, ar0234->format_fourcc, mode,0);
       mcu_cam_stream_on(client);

       pr_info("Ar0234 probed successfully\n");

       return 0;

power_down:
	ar0234_s_power(&ar0234->sd, false);
free_entity:
	media_entity_cleanup(&ar0234->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&ar0234->ctrls);
	mutex_destroy(&mcu_i2c_mutex);

	return ret;
}

static int ar0234_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ar0234 *ar0234 = to_ar0234(sd);
	int loop;

	debug_printk("DEBUG:  %s\n",__func__);

       	mcu_cam_stream_off(ar0234->i2c_client);

	v4l2_async_unregister_subdev(&ar0234->sd);
	media_entity_cleanup(&ar0234->sd.entity);
	v4l2_ctrl_handler_free(&ar0234->ctrls);


	for(loop = 0; loop < ar0234->mcu_ctrl_info->ctrl_ui_data.ctrl_menu_info.num_menu_elem
			; loop++) {
		FREE_SAFE(&client->dev, ar0234->mcu_ctrl_info->ctrl_ui_data.ctrl_menu_info.menu[loop]);
	}


	for(loop = 0; loop < ar0234->numfmts; loop++ ) {
		FREE_SAFE(&client->dev, (void *)ar0234->mcu_cam_frmfmt[loop].framerates);
	}


	FREE_SAFE(&client->dev, ar0234->ctrldb);
	FREE_SAFE(&client->dev, ar0234->streamdb);
	FREE_SAFE(&client->dev, ar0234->stream_info);
	FREE_SAFE(&client->dev, ar0234->mcu_ctrl_info);
	FREE_SAFE(&client->dev, ar0234->mcu_cam_frmfmt);
	FREE_SAFE(&client->dev, ar0234);

	mutex_destroy(&mcu_i2c_mutex);

	return 0;
}

static const struct i2c_device_id ar0234_id[] = {
	{ "ar0234", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ar0234_id);

static const struct of_device_id ar0234_of_match[] = {
	{ .compatible = "ar0234" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ar0234_of_match);

static struct i2c_driver ar0234_i2c_driver = {
	.driver = {
		.of_match_table = of_match_ptr(ar0234_of_match),
		.name  = "ar0234",
	},
	.probe_new = ar0234_probe,
	.remove = ar0234_remove,
	.id_table = ar0234_id,
};

module_i2c_driver(ar0234_i2c_driver);

MODULE_DESCRIPTION("AR0234 Camera Driver");
MODULE_AUTHOR("Elango Palanisamy <elango.p@e-consystems.com>");
MODULE_LICENSE("GPL v2");

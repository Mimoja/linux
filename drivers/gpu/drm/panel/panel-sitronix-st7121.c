// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for panels based on the Sitronix ST7121 controller, such as:
 *
 * - Startek KD055HDFID040 5.5" 720x1280 MIPI-DSI panel
 *
 * Copyright (C) 2026 Mimoja <git@mimoja.de>
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct st7121_panel_desc {
	const struct drm_display_mode *mode;
	unsigned int lanes;
	unsigned long mode_flags;
	enum mipi_dsi_pixel_format format;
	void (*init_sequence)(struct mipi_dsi_multi_context *ctx);
};

struct st7121 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	const struct st7121_panel_desc *desc;

	struct regulator *vcc;
	struct regulator *iovcc;
	struct gpio_desc *enable_gpio;
	struct gpio_desc *reset_gpio;

	enum drm_panel_orientation orientation;
};

static inline struct st7121 *panel_to_st7121(struct drm_panel *panel)
{
	return container_of(panel, struct st7121, panel);
}

/* Initial code of the KD055HDFID040, as provided by Startek */
static void kd055hdfid040_init_sequence(struct mipi_dsi_multi_context *ctx)
{
	/* Startek's sequence starts with sleep in and display off */
	mipi_dsi_dcs_enter_sleep_mode_multi(ctx);
	mipi_dsi_dcs_set_display_off_multi(ctx);

	mipi_dsi_dcs_write_seq_multi(ctx, 0x60, 0x71, 0x21, 0xa2);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x60, 0x71, 0x21, 0xa3);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x60, 0x71, 0x21, 0xa4);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x78, 0x21);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x79, 0xef);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x86, 0x25, 0x19, 0xc0, 0x15, 0xf5,
				     0x01, 0x00, 0x25, 0xd9, 0x15);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xa5, 0xf0, 0x03);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xb7, 0x00, 0x00, 0x73, 0x73);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xb0, 0x22, 0x57, 0x11, 0x75, 0x25,
				     0x57, 0x57);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xbf, 0xbe, 0xcf);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xd7, 0x10, 0x0c, 0x12, 0x2a, 0x80,
				     0x80);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x90, 0x71, 0x23, 0x5a, 0x20, 0x24,
				     0x03, 0x28);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xa3, 0x80, 0x01, 0x8c, 0xff, 0x45,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00,
				     0x00, 0x1e, 0x5c, 0x1e, 0x80, 0x10, 0x00,
				     0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46,
				     0x00, 0x00, 0x1e, 0x5c, 0x1e, 0x80, 0x10,
				     0x20, 0x58, 0x00, 0x00, 0x00, 0xff);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xa6, 0x02, 0x00, 0x24, 0x55, 0x34,
				     0x00, 0x35, 0x00, 0x67, 0x67, 0x91, 0xff,
				     0x00, 0x24, 0x77, 0x2f, 0x00, 0x30, 0x00,
				     0x67, 0x67, 0x91, 0xff, 0x00, 0x24, 0x77,
				     0x32, 0x00, 0x33, 0x00, 0x67, 0x67, 0x91,
				     0xff, 0x00, 0xac, 0x51, 0x00, 0x04, 0x00,
				     0x00, 0x67, 0x67, 0xff, 0xff, 0x00, 0x08,
				     0x80, 0x08, 0x80, 0x06, 0x00, 0x00, 0x00,
				     0x00);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xa7, 0x18, 0x18, 0xc0, 0x64, 0x40,
				     0x43, 0x52, 0x41, 0x02, 0x40, 0x00, 0x67,
				     0x67, 0x91, 0xff, 0x08, 0x80, 0x64, 0x40,
				     0x61, 0x70, 0x43, 0x04, 0x00, 0x00, 0x67,
				     0x67, 0x91, 0xff, 0x08, 0x80, 0x64, 0x40,
				     0x4b, 0x5a, 0x40, 0x00, 0x02, 0x01, 0x67,
				     0x67, 0x91, 0xff, 0x08, 0x80, 0x64, 0x40,
				     0x69, 0x78, 0x00, 0x00, 0x20, 0x00, 0x67,
				     0x67, 0x91, 0xff, 0x08, 0x80, 0x53);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xac, 0x19, 0x06, 0x00, 0x1e, 0x1c,
				     0x1d, 0x18, 0x08, 0x0a, 0x13, 0x11, 0x1e,
				     0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
				     0x1e, 0x1e, 0x1e, 0x19, 0x06, 0x01, 0x1e,
				     0x1c, 0x1d, 0x18, 0x09, 0x0b, 0x12, 0x10,
				     0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
				     0x1e, 0x1e, 0x1e, 0x1e);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xad, 0x0c, 0x40, 0x46, 0x00, 0x07,
				     0x4b, 0x4b, 0xff, 0xff, 0xf0, 0x40, 0x0e,
				     0x01, 0x07, 0x42, 0x42, 0xff, 0xff, 0x01,
				     0x00, 0x00, 0xff, 0xff, 0xff, 0xff);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xae, 0xff, 0x07, 0x00, 0xff, 0x07,
				     0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xb2, 0x15, 0x19, 0x05, 0x23, 0x49,
				     0xaf, 0x03, 0x2e, 0x5c, 0xd2, 0xff, 0x10,
				     0x60, 0xfd, 0x20, 0xc0, 0x00);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xe8, 0x20, 0x67, 0x04, 0x8d, 0x8d,
				     0x3e, 0x04, 0xdc, 0xdc, 0x3e, 0x06, 0xfa,
				     0x26, 0x3e);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x75, 0x03, 0x04);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xe7, 0x4b, 0x00, 0x00, 0xbc, 0x4b,
				     0x8c, 0x20, 0x1a, 0xf0, 0x7d, 0x14, 0x7d,
				     0x14, 0x7d, 0x14, 0x7d, 0x14, 0xff, 0x00,
				     0x32, 0x30, 0x73, 0x00, 0x00, 0xc8, 0x6a,
				     0xff, 0x5a, 0x64, 0x38, 0x88, 0x15, 0xb1,
				     0x01, 0x01, 0x64, 0x01, 0x01, 0x7c, 0xff,
				     0x1a, 0x01);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xe1, 0x0c, 0x0c);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xea, 0x15, 0x00, 0x55);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xc8, 0x00, 0x00, 0x1c, 0x33, 0x5a,
				     0x00, 0x90, 0x05, 0xd9, 0x07, 0x11, 0x36,
				     0x07, 0xa2, 0x04, 0x21, 0xec, 0x02, 0x2a,
				     0x01, 0x22, 0x66, 0x0f, 0xa6, 0x0a, 0x33,
				     0x00, 0x0a, 0x4d, 0x0e, 0xf3, 0x8a, 0x0f,
				     0xb9, 0xe5, 0x03, 0xfb);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xc9, 0x00, 0x00, 0x1c, 0x33, 0x5a,
				     0x00, 0x90, 0x05, 0xd9, 0x07, 0x11, 0x36,
				     0x07, 0xa2, 0x04, 0x21, 0xec, 0x02, 0x2a,
				     0x01, 0x22, 0x66, 0x0f, 0xa6, 0x0a, 0x33,
				     0x00, 0x0a, 0x4d, 0x0e, 0xf3, 0x8a, 0x0f,
				     0xb9, 0xe5, 0x03, 0xfb);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x60, 0x71, 0x21, 0x00);
	mipi_dsi_dcs_write_seq_multi(ctx, MIPI_DCS_SET_ADDRESS_MODE, 0x00);
}

static const struct drm_display_mode kd055hdfid040_mode = {
	.clock		= 63000,

	.hdisplay	= 720,
	.hsync_start	= 720 + 40,
	.hsync_end	= 720 + 40 + 2,
	.htotal		= 720 + 40 + 2 + 40,

	.vdisplay	= 1280,
	.vsync_start	= 1280 + 150,
	.vsync_end	= 1280 + 150 + 2,
	.vtotal		= 1280 + 150 + 2 + 10,

	.width_mm	= 68,
	.height_mm	= 121,
};

static const struct st7121_panel_desc kd055hdfid040_desc = {
	.mode = &kd055hdfid040_mode,
	.lanes = 4,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
		      MIPI_DSI_MODE_LPM,
	.format = MIPI_DSI_FMT_RGB888,
	.init_sequence = kd055hdfid040_init_sequence,
};

static int st7121_prepare(struct drm_panel *panel)
{
	struct st7121 *st7121 = panel_to_st7121(panel);
	struct device *dev = &st7121->dsi->dev;
	int ret;

	gpiod_set_value_cansleep(st7121->reset_gpio, 1);

	/* IOVCC has to be up before VCI */
	ret = regulator_enable(st7121->iovcc);
	if (ret) {
		dev_err(dev, "Failed to enable iovcc supply: %d\n", ret);
		return ret;
	}

	ret = regulator_enable(st7121->vcc);
	if (ret) {
		dev_err(dev, "Failed to enable vcc supply: %d\n", ret);
		goto disable_iovcc;
	}

	gpiod_set_value_cansleep(st7121->enable_gpio, 1);
	usleep_range(10000, 11000);

	gpiod_set_value_cansleep(st7121->reset_gpio, 0);
	msleep(120);

	return 0;

disable_iovcc:
	regulator_disable(st7121->iovcc);
	return ret;
}

static int st7121_unprepare(struct drm_panel *panel)
{
	struct st7121 *st7121 = panel_to_st7121(panel);

	gpiod_set_value_cansleep(st7121->reset_gpio, 1);
	gpiod_set_value_cansleep(st7121->enable_gpio, 0);
	usleep_range(10000, 11000);

	regulator_disable(st7121->vcc);
	regulator_disable(st7121->iovcc);

	return 0;
}

static int st7121_enable(struct drm_panel *panel)
{
	struct st7121 *st7121 = panel_to_st7121(panel);
	struct mipi_dsi_multi_context ctx = { .dsi = st7121->dsi };

	st7121->desc->init_sequence(&ctx);

	mipi_dsi_dcs_set_tear_on_multi(&ctx, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	mipi_dsi_dcs_exit_sleep_mode_multi(&ctx);
	mipi_dsi_msleep(&ctx, 120);
	mipi_dsi_dcs_set_display_on_multi(&ctx);
	mipi_dsi_msleep(&ctx, 20);

	return ctx.accum_err;
}

static int st7121_disable(struct drm_panel *panel)
{
	struct st7121 *st7121 = panel_to_st7121(panel);
	struct mipi_dsi_multi_context ctx = { .dsi = st7121->dsi };

	mipi_dsi_dcs_set_display_off_multi(&ctx);
	mipi_dsi_dcs_enter_sleep_mode_multi(&ctx);
	mipi_dsi_msleep(&ctx, 50);

	return ctx.accum_err;
}

static int st7121_get_modes(struct drm_panel *panel,
			    struct drm_connector *connector)
{
	struct st7121 *st7121 = panel_to_st7121(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, st7121->desc->mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static enum drm_panel_orientation st7121_get_orientation(struct drm_panel *panel)
{
	struct st7121 *st7121 = panel_to_st7121(panel);

	return st7121->orientation;
}

static const struct drm_panel_funcs st7121_funcs = {
	.prepare = st7121_prepare,
	.unprepare = st7121_unprepare,
	.enable = st7121_enable,
	.disable = st7121_disable,
	.get_modes = st7121_get_modes,
	.get_orientation = st7121_get_orientation,
};

static int st7121_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct st7121 *st7121;
	int ret;

	st7121 = devm_drm_panel_alloc(dev, struct st7121, panel, &st7121_funcs,
				      DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(st7121))
		return PTR_ERR(st7121);

	st7121->dsi = dsi;
	st7121->desc = of_device_get_match_data(dev);
	mipi_dsi_set_drvdata(dsi, st7121);

	st7121->vcc = devm_regulator_get(dev, "vcc");
	if (IS_ERR(st7121->vcc))
		return dev_err_probe(dev, PTR_ERR(st7121->vcc),
				     "Failed to get vcc regulator\n");

	st7121->iovcc = devm_regulator_get(dev, "iovcc");
	if (IS_ERR(st7121->iovcc))
		return dev_err_probe(dev, PTR_ERR(st7121->iovcc),
				     "Failed to get iovcc regulator\n");

	st7121->enable_gpio = devm_gpiod_get_optional(dev, "enable",
						      GPIOD_OUT_LOW);
	if (IS_ERR(st7121->enable_gpio))
		return dev_err_probe(dev, PTR_ERR(st7121->enable_gpio),
				     "Failed to get enable GPIO\n");

	st7121->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(st7121->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(st7121->reset_gpio),
				     "Failed to get reset GPIO\n");

	ret = of_drm_get_panel_orientation(dev->of_node, &st7121->orientation);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get orientation\n");

	st7121->panel.prepare_prev_first = true;

	ret = drm_panel_of_backlight(&st7121->panel);
	if (ret)
		return ret;

	drm_panel_add(&st7121->panel);

	dsi->lanes = st7121->desc->lanes;
	dsi->format = st7121->desc->format;
	dsi->mode_flags = st7121->desc->mode_flags;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
		drm_panel_remove(&st7121->panel);
		return ret;
	}

	return 0;
}

static void st7121_remove(struct mipi_dsi_device *dsi)
{
	struct st7121 *st7121 = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&st7121->panel);
}

static const struct of_device_id st7121_of_match[] = {
	{ .compatible = "startek,kd055hdfid040", .data = &kd055hdfid040_desc },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, st7121_of_match);

static struct mipi_dsi_driver st7121_driver = {
	.probe = st7121_probe,
	.remove = st7121_remove,
	.driver = {
		.name = "panel-sitronix-st7121",
		.of_match_table = st7121_of_match,
	},
};
module_mipi_dsi_driver(st7121_driver);

MODULE_AUTHOR("Mimoja <git@mimoja.de>");
MODULE_DESCRIPTION("DRM driver for Sitronix ST7121 based MIPI DSI panels");
MODULE_LICENSE("GPL");

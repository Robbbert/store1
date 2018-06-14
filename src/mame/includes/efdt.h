// license:BSD-3-Clause
// copyright-holders: ElSemi, Roberto Fresca.

#include "emupal.h"


class efdt_state : public driver_device
{
public:
	efdt_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette"),
		m_audiocpu(*this, "audiocpu"),
		m_videoram(*this, "videoram", 8),
		m_vregs1(*this, "vregs1"),
		m_vregs2(*this, "vregs2")
		 { }

	required_device<cpu_device> m_maincpu;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;
	optional_device<cpu_device> m_audiocpu;

	/* memory pointers */
	required_shared_ptr<uint8_t> m_videoram;
	required_shared_ptr<uint8_t> m_vregs1;
	required_shared_ptr<uint8_t> m_vregs2;
	uint8_t m_soundlatch[4];
	uint8_t m_soundCommand;
	uint8_t m_soundControl;
	

	/* video-related */
	tilemap_t      *m_tilemap[2];
	int				m_tilebank;

	DECLARE_WRITE8_MEMBER(efdt_videoram_w);
	DECLARE_WRITE8_MEMBER(efdt_vregs1_w);
	DECLARE_WRITE8_MEMBER(efdt_vregs2_w);

	TILE_GET_INFO_MEMBER(get_tile_info_0);
	TILE_GET_INFO_MEMBER(get_tile_info_1);

	virtual void machine_start() override;
	virtual void machine_reset() override;

	DECLARE_VIDEO_START(efdt);
	DECLARE_PALETTE_INIT(efdt);

	DECLARE_READ8_MEMBER(main_soundlatch_r);
	DECLARE_WRITE8_MEMBER(main_soundlatch_w);

	DECLARE_READ8_MEMBER(soundlatch_0_r);
	DECLARE_READ8_MEMBER(soundlatch_1_r);
	DECLARE_READ8_MEMBER(soundlatch_2_r);
	DECLARE_READ8_MEMBER(soundlatch_3_r);

	DECLARE_WRITE8_MEMBER(soundlatch_0_w);
	DECLARE_WRITE8_MEMBER(soundlatch_1_w);
	DECLARE_WRITE8_MEMBER(soundlatch_2_w);
	DECLARE_WRITE8_MEMBER(soundlatch_3_w);

	uint32_t screen_update_efdt(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void efdt(machine_config &config);
	void efdt_map(address_map &map);
	void efdt_snd_map(address_map &map);
};
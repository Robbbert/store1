// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina
#ifndef MAME_SOUND_NILE_H
#define MAME_SOUND_NILE_H

#pragma once


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> nile_device

class nile_device : public device_t,
					public device_sound_interface
{
public:
	nile_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// device-level overrides
	virtual void device_start() override;

	// sound stream update overrides
	virtual void sound_stream_update(sound_stream &stream, std::vector<read_stream_view> const &inputs, std::vector<write_stream_view> &outputs) override;

public:
	void nile_snd_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t nile_snd_r(offs_t offset);
	void nile_sndctrl_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t nile_sndctrl_r();

private:
	static constexpr unsigned NILE_VOICES = 8;

	sound_stream *m_stream;
	required_region_ptr<uint8_t> m_sound_ram;
	uint16_t m_sound_regs[0x80];
	int m_vpos[NILE_VOICES];
	int m_frac[NILE_VOICES];
	int m_lponce[NILE_VOICES];
	uint16_t m_ctrl;
};

DECLARE_DEVICE_TYPE(NILE, nile_device)

#endif // MAME_SOUND_NILE_H

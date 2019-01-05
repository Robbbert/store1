// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    Microprose Games 3D hardware

    driver by Phil Bennett

    Games supported:
        * F-15 Strike Eagle [2 sets]
        * B.O.T.S.S. - Battle of the Solar System [2 sets]
        * Tank Battle (prototype)

    TODO:
        * DS1215 Phantom time chip
        * DS1267 Volume control

    Known issues:
        * Games are running too smoothly!
        * F-15 controls need tweaking
        * botss will hit a divide-by-zero error if the throttle is
          not calibrated first in service mode. (clone is ok)

****************************************************************************/

#include "emu.h"
#include "includes/micro3d.h"
#include "audio/micro3d.h"
#include "cpu/am29000/am29000.h"
#include "cpu/m68000/m68000.h"
#include "cpu/mcs51/mcs51.h"
#include "bus/rs232/rs232.h"
#include "machine/mc68681.h"
#include "machine/mc68901.h"
#include "machine/nvram.h"
#include "machine/z80scc.h"
#include "sound/ym2151.h"

#include "screen.h"
#include "speaker.h"


/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( micro3d )
	PORT_START("INPUTS_A_B")
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0000, "Shared Memory Handshake Test")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, "Dr. Math Monitor Mode")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, "Burn-in Tests" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0000, "Manufacturing Tests")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, "Host Monitor Mode")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START("VGB_SW")
	PORT_DIPNAME( 0x0008, 0x0008, "VGB Monitor Mode")
	PORT_DIPSETTING(    0x0008, DEF_STR(Off) )
	PORT_DIPSETTING(    0x0000, DEF_STR(On) )

	PORT_START("SOUND_SW")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Sound PCB Test SW") PORT_CODE(KEYCODE_F1)

	PORT_START("VOLUME")
	PORT_ADJUSTER(100, "Volume")
INPUT_PORTS_END

static INPUT_PORTS_START( f15se )
	PORT_INCLUDE( micro3d )

	PORT_MODIFY("INPUTS_A_B")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Decoy")
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("INPUTS_C_D")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Trigger")
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Missile")
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Target Select")
	PORT_BIT( 0x88ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOYSTICK_X")
	PORT_BIT( 0xfff, 0x000, IPT_AD_STICK_X ) PORT_MINMAX(0xc1, 0xc0) PORT_SENSITIVITY(25) PORT_KEYDELTA(200) PORT_REVERSE

	PORT_START("JOYSTICK_Y")
	PORT_BIT(0xfff, 0x000, IPT_AD_STICK_Y ) PORT_MINMAX(0xc1, 0x0c0) PORT_SENSITIVITY(25) PORT_KEYDELTA(200)

	PORT_START("THROTTLE")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Z ) PORT_MINMAX(0x01,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(25) PORT_CENTERDELTA(0) PORT_NAME("Throttle")
INPUT_PORTS_END

static INPUT_PORTS_START( botss )
	PORT_INCLUDE( micro3d )

	PORT_MODIFY("INPUTS_A_B")
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_CUSTOM_MEMBER(DEVICE_SELF, micro3d_state, botss_hwchk_r, nullptr)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 )  PORT_NAME("Shield")
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_BIT( 0x7800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("INPUTS_C_D")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Trigger")
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Blaster")
	PORT_BIT( 0xccff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOYSTICK_X")
	PORT_BIT( 0xfff, 0x000, IPT_AD_STICK_X ) PORT_MINMAX(0xc1, 0xc0) PORT_SENSITIVITY(25) PORT_KEYDELTA(200) PORT_REVERSE

	PORT_START("JOYSTICK_Y")
	PORT_BIT(0xfff, 0x000, IPT_AD_STICK_Y ) PORT_MINMAX(0xc1, 0x0c0) PORT_SENSITIVITY(25) PORT_KEYDELTA(200)

	PORT_START("THROTTLE")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Z ) PORT_MINMAX(0x01,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(25) PORT_CENTERDELTA(0) PORT_NAME("Throttle")
INPUT_PORTS_END

static INPUT_PORTS_START( botss11 )
	PORT_INCLUDE( micro3d )

	PORT_MODIFY("INPUTS_A_B")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 )  PORT_NAME("Shield")
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)

	PORT_START("INPUTS_C_D")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Throttle up")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Throttle down")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Trigger")
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Blaster")
	PORT_BIT( 0xccf5, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( tankbatl )
	PORT_INCLUDE( micro3d )

	PORT_MODIFY("INPUTS_A_B")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP )
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0xc000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("INPUTS_C_D")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
INPUT_PORTS_END


/*************************************
 *
 *  Host memory map
 *
 *************************************/

void micro3d_state::hostmem(address_map &map)
{
	map(0x000000, 0x143fff).rom();
	map(0x200000, 0x20ffff).ram().share("nvram");
	map(0x800000, 0x83ffff).ram().share("shared_ram");
	map(0x900000, 0x900001).w(FUNC(micro3d_state::host_drmath_int_w));
	map(0x920000, 0x920001).portr("INPUTS_C_D");
	map(0x940000, 0x940001).portr("INPUTS_A_B");
	map(0x960000, 0x960001).w(FUNC(micro3d_state::micro3d_reset_w));
	map(0x980001, 0x980001).rw("adc", FUNC(adc0844_device::read), FUNC(adc0844_device::write));
	map(0x9a0000, 0x9a0007).rw(m_vgb, FUNC(tms34010_device::host_r), FUNC(tms34010_device::host_w));
	map(0x9c0000, 0x9c0001).noprw();                 /* Lamps */
	map(0x9e0000, 0x9e002f).rw("mfp", FUNC(mc68901_device::read), FUNC(mc68901_device::write)).umask16(0xff00);
	map(0xa00000, 0xa0003f).rw(m_duart, FUNC(mc68681_device::read), FUNC(mc68681_device::write)).umask16(0xff00);
	map(0xa20000, 0xa20001).r(FUNC(micro3d_state::micro3d_encoder_h_r));
	map(0xa40002, 0xa40003).r(FUNC(micro3d_state::micro3d_encoder_l_r));
}


/*************************************
 *
 *  Video memory map
 *
 *************************************/

void micro3d_state::vgbmem(address_map &map)
{
	map(0x00000000, 0x007fffff).ram().share("sprite_vram");
	map(0x00800000, 0x00bfffff).ram();
	map(0x00c00000, 0x00c0000f).portr("VGB_SW");
	map(0x00e00000, 0x00e0000f).w(FUNC(micro3d_state::micro3d_xfer3dk_w));
	map(0x02000000, 0x0200ffff).ram().w(m_palette, FUNC(palette_device::write16)).share("palette"); // clut
	map(0x02600000, 0x0260000f).w(FUNC(micro3d_state::micro3d_creg_w));
	map(0x02c00000, 0x02c0003f).r(FUNC(micro3d_state::vgb_uart_r)).umask16(0x00ff);
	map(0x02e00000, 0x02e0003f).w(FUNC(micro3d_state::vgb_uart_w)).umask16(0x00ff);
	map(0x03800000, 0x03dfffff).rom().region("tms_gfx", 0);
	map(0x03e00000, 0x03ffffff).rom().region("tms34010", 0);
	map(0xc0000000, 0xc00001ff).rw(m_vgb, FUNC(tms34010_device::io_register_r), FUNC(tms34010_device::io_register_w));
	map(0xffe00000, 0xffffffff).rom().region("tms34010", 0);
}


/*************************************
 *
 *  Dr. Math memory map
 *
 *************************************/

void micro3d_state::drmath_prg(address_map &map)
{
	map(0x00000000, 0x000fffff).rom();
}

void micro3d_state::drmath_data(address_map &map)
{
	map(0x00000000, 0x000fffff).rom().region("drmath", 0);
	map(0x00800000, 0x0083ffff).rw(FUNC(micro3d_state::micro3d_shared_r), FUNC(micro3d_state::micro3d_shared_w));
	map(0x00400000, 0x004fffff).ram();
	map(0x00500000, 0x005fffff).ram();
	map(0x00a00000, 0x00a00003).w(FUNC(micro3d_state::drmath_int_w));
	map(0x01000000, 0x01000003).w(FUNC(micro3d_state::micro3d_mac1_w));
	map(0x01000004, 0x01000007).rw(FUNC(micro3d_state::micro3d_mac2_r), FUNC(micro3d_state::micro3d_mac2_w));
	map(0x01200000, 0x01203fff).ram().share("mac_sram");
	map(0x01400000, 0x01400003).rw(FUNC(micro3d_state::micro3d_pipe_r), FUNC(micro3d_state::micro3d_fifo_w));
	map(0x01600000, 0x01600003).w(FUNC(micro3d_state::drmath_intr2_ack));
	map(0x01800000, 0x01800003).w(FUNC(micro3d_state::micro3d_alt_fifo_w));
	map(0x03fffff0, 0x03ffffff).rw("scc", FUNC(z80scc_device::ba_cd_inv_r), FUNC(z80scc_device::ba_cd_inv_w)).umask32(0x000000ff);
}

/*************************************
 *
 *  Sound memory map
 *
 *************************************/

void micro3d_state::soundmem_prg(address_map &map)
{
	map(0x0000, 0x7fff).rom();
}

void micro3d_state::soundmem_io(address_map &map)
{
	map(0x0000, 0x07ff).ram();
	map(0xfd00, 0xfd01).rw("ym2151", FUNC(ym2151_device::read), FUNC(ym2151_device::write));
	map(0xfe00, 0xfe00).w(FUNC(micro3d_state::micro3d_upd7759_w));
	map(0xff00, 0xff00).w(FUNC(micro3d_state::micro3d_snd_dac_a));
	map(0xff01, 0xff01).w(FUNC(micro3d_state::micro3d_snd_dac_b));
}


/*************************************
 *
 *  Machine driver
 *
 *************************************/

MACHINE_CONFIG_START(micro3d_state::micro3d)

	MCFG_DEVICE_ADD("maincpu", M68000, 32_MHz_XTAL / 2)
	MCFG_DEVICE_PROGRAM_MAP(hostmem)
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", micro3d_state,  micro3d_vblank)

	TMS34010(config, m_vgb, 40_MHz_XTAL);
	m_vgb->set_addrmap(AS_PROGRAM, &micro3d_state::vgbmem);
	m_vgb->set_halt_on_reset(false);
	m_vgb->set_pixel_clock(40_MHz_XTAL / 8);
	m_vgb->set_pixels_per_clock(4);
	m_vgb->set_scanline_ind16_callback(FUNC(micro3d_state::scanline_update));
	m_vgb->output_int().set(FUNC(micro3d_state::tms_interrupt));
	m_vgb->set_screen("screen");

	MCFG_DEVICE_ADD("drmath", AM29000, 32_MHz_XTAL / 2)
	MCFG_DEVICE_PROGRAM_MAP(drmath_prg)
	MCFG_DEVICE_DATA_MAP(drmath_data)

	scc8530_device &scc(SCC8530N(config, "scc", 32_MHz_XTAL / 2 / 2));
	scc.out_txdb_callback().set("monitor_drmath", FUNC(rs232_port_device::write_txd));

	I8051(config, m_audiocpu, 11.0592_MHz_XTAL);
	m_audiocpu->set_addrmap(AS_PROGRAM, &micro3d_state::soundmem_prg);
	m_audiocpu->set_addrmap(AS_IO, &micro3d_state::soundmem_io);
	m_audiocpu->port_in_cb<1>().set(FUNC(micro3d_state::micro3d_sound_p1_r));
	m_audiocpu->port_out_cb<1>().set(FUNC(micro3d_state::micro3d_sound_p1_w));
	m_audiocpu->port_in_cb<3>().set(FUNC(micro3d_state::micro3d_sound_p3_r));
	m_audiocpu->port_out_cb<3>().set(FUNC(micro3d_state::micro3d_sound_p3_w));
	m_audiocpu->serial_tx_cb().set(FUNC(micro3d_state::data_from_i8031));
	m_audiocpu->serial_rx_cb().set(FUNC(micro3d_state::data_to_i8031));

	MCFG_DEVICE_ADD("duart", MC68681, 3.6864_MHz_XTAL)
	MCFG_MC68681_IRQ_CALLBACK(WRITELINE(*this, micro3d_state, duart_irq_handler))
	MCFG_MC68681_A_TX_CALLBACK(WRITELINE("monitor_host", rs232_port_device, write_txd))
	MCFG_MC68681_B_TX_CALLBACK(WRITELINE(*this, micro3d_state, duart_txb))
	MCFG_MC68681_INPORT_CALLBACK(READ8(*this, micro3d_state, duart_input_r))
	MCFG_MC68681_OUTPORT_CALLBACK(WRITE8(*this, micro3d_state, duart_output_w))

	mc68901_device &mfp(MC68901(config, "mfp", 4000000));
	mfp.set_timer_clock(4000000);
	mfp.set_rx_clock(0);
	mfp.set_tx_clock(0);
	mfp.out_irq_cb().set_inputline("maincpu", M68K_IRQ_4);
	//mfp.out_tao_cb().set("mfp", FUNC(mc68901_device::rc_w));
	//mfp.out_tao_cb().append("mfp", FUNC(mc68901_device::tc_w));
	mfp.out_tco_cb().set("mfp", FUNC(mc68901_device::tbi_w));

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);
	MCFG_QUANTUM_TIME(attotime::from_hz(3000))

	PALETTE(config, m_palette).set_format(palette_device::BRGx_555, 4096);

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_RAW_PARAMS(40_MHz_XTAL/8*4, 192*4, 0, 144*4, 434, 0, 400)
	MCFG_SCREEN_UPDATE_DEVICE("vgb", tms34010_device, tms340x0_ind16)
	MCFG_SCREEN_PALETTE(m_palette)

	MC2661(config, m_vgb_uart, 40_MHz_XTAL / 8); // actually SCN2651
	m_vgb_uart->txd_handler().set("monitor_vgb", FUNC(rs232_port_device::write_txd));

	rs232_port_device &monitor_host(RS232_PORT(config, "monitor_host", default_rs232_devices, nullptr)); // J2 (4-pin molex)
	monitor_host.rxd_handler().set("duart", FUNC(mc68681_device::rx_a_w));

	rs232_port_device &monitor_drmath(RS232_PORT(config, "monitor_drmath", default_rs232_devices, nullptr)); // J4 (4-pin molex)
	monitor_drmath.rxd_handler().set("scc", FUNC(z80scc_device::rxb_w));
	monitor_drmath.dcd_handler().set("scc", FUNC(z80scc_device::dcdb_w));

	rs232_port_device &monitor_vgb(RS232_PORT(config, "monitor_vgb", default_rs232_devices, nullptr)); // J3 (4-pin molex)
	monitor_vgb.rxd_handler().set(m_vgb_uart, FUNC(mc2661_device::rx_w));
	monitor_vgb.dsr_handler().set(m_vgb_uart, FUNC(mc2661_device::dsr_w));

	ADC0844(config, m_adc);
	m_adc->intr_callback().set("mfp", FUNC(mc68901_device::i3_w));
	m_adc->ch1_callback().set_ioport("THROTTLE");
	m_adc->ch2_callback().set(FUNC(micro3d_state::adc_volume_r));

	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();

	UPD7759(config, m_upd7759);
	m_upd7759->add_route(ALL_OUTPUTS, "lspeaker", 0.35);
	m_upd7759->add_route(ALL_OUTPUTS, "rspeaker", 0.35);

	ym2151_device &ym2151(YM2151(config, "ym2151", 3.579545_MHz_XTAL));
	ym2151.add_route(0, "lspeaker", 0.35);
	ym2151.add_route(1, "rspeaker", 0.35);

	MICRO3D_SOUND(config, m_noise_1);
	m_noise_1->add_route(0, "lspeaker", 1.0);
	m_noise_1->add_route(1, "rspeaker", 1.0);

	MICRO3D_SOUND(config, m_noise_2);
	m_noise_2->add_route(0, "lspeaker", 1.0);
	m_noise_2->add_route(1, "rspeaker", 1.0);
MACHINE_CONFIG_END

void micro3d_state::botss11(machine_config &config)
{
	micro3d(config);
	m_adc->ch1_callback().set_constant(0);
}


/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( f15se )
	/* Host PCB (MPG DW-00011C-0011-01) */
	ROM_REGION( 0x180000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "host.u67", 0x000001, 0x20000, CRC(8f495ceb) SHA1(90998ad67e76928ed1a6cae56038b98d1aa2e7b0) )
	ROM_LOAD16_BYTE( "host.u91", 0x000000, 0x20000, CRC(dfae5ec3) SHA1(29306eed5047e39a0a2350e61ab7126a84cb710b) )
	ROM_LOAD16_BYTE( "host.u68", 0x040001, 0x20000, CRC(685fc355) SHA1(5bfe015a8deccb66e3317154d715f490f00ace74) )
	ROM_LOAD16_BYTE( "host.u92", 0x040000, 0x20000, CRC(8f7bb2eb) SHA1(1923d55d66da0fbc158b4f90bcc98c88955953ea) )

	ROM_LOAD16_BYTE( "004.hst",  0x080001, 0x20000, CRC(81671ce1) SHA1(51ff641ccbc9dea640a62944910abe73d796b062) )
	ROM_LOAD16_BYTE( "005.hst",  0x080000, 0x20000, CRC(bdaa7db5) SHA1(52cd832cdd44e609e8cd269469b806e2cd27d63d) )
	ROM_LOAD16_BYTE( "host.u70", 0x0c0001, 0x20000, CRC(251e92d2) SHA1(a20279089af1f738ba912f90a4d048d4e58795fe) )
	ROM_LOAD16_BYTE( "007.hst",  0x0c0000, 0x20000, CRC(36e06cba) SHA1(5ffee5da6f475978be10fa5e1a2c24f00497ea5f) )
	ROM_LOAD16_BYTE( "008.hst",  0x100001, 0x20000, CRC(d96fd4e2) SHA1(001af758da437e955b4ee914eabeb9739ebc4454) )
	ROM_LOAD16_BYTE( "009.hst",  0x100000, 0x20000, CRC(33e3b473) SHA1(66deda79ba94f0ed722b399b3fc6062dcdd1a6c9) )
	ROM_FILL(                    0x140000, 0x40000, 0xff )

	/* Dr Math PCB (MPG 010-00002-001) */
	ROM_REGION32_BE( 0x100000, "drmath", 0 )
	ROMX_LOAD( "122.dth", 0x00000, 0x08000, CRC(9d2032cf) SHA1(8430816756ea92bbe86b94eaa24a6071bf0ef879), ROM_SKIP(7) )
	ROMX_LOAD( "125.dth", 0x00001, 0x08000, CRC(7718487c) SHA1(609106f55601f84095b64ce2484107779da89149), ROM_SKIP(7) )
	ROMX_LOAD( "123.dth", 0x00002, 0x08000, CRC(54d5544f) SHA1(d039ee39991b947a7483111359ab245fc104e060), ROM_SKIP(7) )
	ROMX_LOAD( "124.dth", 0x00003, 0x08000, CRC(7be96646) SHA1(a6733f75c0404282d71e8c1a287546ef4d9d42ad), ROM_SKIP(7) )
	ROMX_LOAD( "118.dth", 0x00004, 0x08000, CRC(cc895c20) SHA1(140ef47536914fe1441778e759894c2cdd893276), ROM_SKIP(7) )
	ROMX_LOAD( "121.dth", 0x00005, 0x08000, CRC(392e5c43) SHA1(455cf3bb3c16217e58d6eea51d8f49a5bed1955e), ROM_SKIP(7) )
	ROMX_LOAD( "119.dth", 0x00006, 0x08000, CRC(b1c966e5) SHA1(9703bb1f9bdf6a779b59daebb39df2926727fa76), ROM_SKIP(7) )
	ROMX_LOAD( "120.dth", 0x00007, 0x08000, CRC(5fb9836d) SHA1(d511aa9f02972a7f475c82c6f57d1f3fd4f118fa), ROM_SKIP(7) )

	ROM_REGION16_BE( 0x80000, "vertex", 0 )
	ROM_LOAD16_BYTE( "014.dth", 0x00001, 0x20000, CRC(5ca7713f) SHA1(ac7b9629684b99ecfb1945176b06eb6be284ba93) )
	ROM_LOAD16_BYTE( "015.dth", 0x00000, 0x20000, CRC(beae31bb) SHA1(1ab80a6b99eea6d5bf9b1bce58ecca13042c77a6) )
	ROM_LOAD16_BYTE( "016.dth", 0x40001, 0x20000, CRC(5db4f677) SHA1(25a6fe4c562e4fa4225aa4687dd41920b614e591) )
	ROM_LOAD16_BYTE( "017.dth", 0x40000, 0x20000, CRC(47f9a868) SHA1(7c8a9355893e4a3f3846fd05e0237ffd1404ffee) )

	/* Video Graphics PCB (MPG DW-010-00003-001) */
	ROM_REGION16_LE( 0x40000, "tms34010", 0 )
	ROM_LOAD16_BYTE( "vgb_u101.bin", 0x00000, 0x20000, CRC(e99fac71) SHA1(98d1d2134fabc1bad637cbe42cbe9cdc20b32126) )
	ROM_LOAD16_BYTE( "vgb_u097.bin", 0x00001, 0x20000, CRC(78b9b7c7) SHA1(4bce993dd3aea126e3a9d42ee8c68b8ab47fdba7) )

	ROM_REGION16_LE( 0xc0000, "tms_gfx", 0 )
	ROM_LOAD16_BYTE( "005.vgb", 0x00000, 0x20000, CRC(7b1852f0) SHA1(d21525e59b3112313ea9783ac3dd988a4c1d5f87) )
	ROM_LOAD16_BYTE( "006.vgb", 0x00001, 0x20000, CRC(9d031636) SHA1(b7c7b57d547f2ce2eeb97126e961f3b5f35823f7) )
	ROM_LOAD16_BYTE( "007.vgb", 0x40000, 0x20000, CRC(15326070) SHA1(ec4484d4515694742d3fd3b944f342f052463988) )
	ROM_LOAD16_BYTE( "008.vgb", 0x40001, 0x20000, CRC(ca0e86d8) SHA1(a7b4b02d100a7875d5a184cdb76d507e926d1ca3) )
	ROM_LOAD16_BYTE( "003.vgb", 0x80000, 0x20000, CRC(4d8e8f54) SHA1(d8a23b5fd00ab919dc6d63fc72824d1293073813) )
	ROM_LOAD16_BYTE( "002.vgb", 0x80001, 0x20000, CRC(f6488e31) SHA1(d2f9304cc59f5523007592ae76ddd56107cc29e8) )

	/* Sound PCB  (MPG 010-00018-002) */
	ROM_REGION( 0x08000, "audiocpu", 0 )
	ROM_LOAD( "4-001.snd", 0x000000, 0x08000, CRC(705685a9) SHA1(311f7cac126a19e8bd555ebf31ff4ec4680ddfa4) )

	ROM_REGION( 0x40000, "upd7759", 0 )
	ROM_LOAD( "3-001.snd", 0x000000, 0x40000, CRC(af84b635) SHA1(844e5987a66e9e3ab2d2fe05b93a4da3512776bb) )
ROM_END

ROM_START( f15se21 )
	/* Host PCB (MPG DW-00011C-0011-01) */
	ROM_REGION( 0x180000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "500.hst", 0x000001, 0x20000, CRC(6c26806d) SHA1(7cfd2b3b92b0fc6627c92a2013a317ca5abc66a0) )
	ROM_LOAD16_BYTE( "501.hst", 0x000000, 0x20000, CRC(81f02bf7) SHA1(09976746fe4d9c88bd8840f6e7addb09226aa54b) )
	ROM_LOAD16_BYTE( "502.hst", 0x040001, 0x20000, CRC(1eb945e5) SHA1(aba3ff038f2ca0f1200be5710073825ce80e3656) )
	ROM_LOAD16_BYTE( "503.hst", 0x040000, 0x20000, CRC(21fcb974) SHA1(56f78ce652e2bf432fbba8cda8c800f02dad84bb) )

	ROM_LOAD16_BYTE( "004.hst", 0x080001, 0x20000, CRC(81671ce1) SHA1(51ff641ccbc9dea640a62944910abe73d796b062) )
	ROM_LOAD16_BYTE( "005.hst", 0x080000, 0x20000, CRC(bdaa7db5) SHA1(52cd832cdd44e609e8cd269469b806e2cd27d63d) )
	ROM_LOAD16_BYTE( "host.u70",0x0c0001, 0x20000, CRC(251e92d2) SHA1(a20279089af1f738ba912f90a4d048d4e58795fe) )
	ROM_LOAD16_BYTE( "007.hst", 0x0c0000, 0x20000, CRC(36e06cba) SHA1(5ffee5da6f475978be10fa5e1a2c24f00497ea5f) )
	ROM_LOAD16_BYTE( "008.hst", 0x100001, 0x20000, CRC(d96fd4e2) SHA1(001af758da437e955b4ee914eabeb9739ebc4454) )
	ROM_LOAD16_BYTE( "009.hst", 0x100000, 0x20000, CRC(33e3b473) SHA1(66deda79ba94f0ed722b399b3fc6062dcdd1a6c9) )
	ROM_FILL(                   0x140000, 0x40000, 0xff )

	/* Video Graphics PCB (MPG DW-010-00003-001) */
	ROM_REGION16_LE( 0x40000, "tms34010", 0 )
	ROM_LOAD16_BYTE( "001.vgb", 0x000000, 0x20000, CRC(810c142d) SHA1(d37e5ecd716dda65d43cec7bca524c59d3dc9803) )
	ROM_LOAD16_BYTE( "004.vgb", 0x000001, 0x20000, CRC(b69e1260) SHA1(1a2b69ea7c96b0293b24d87ea46bd4b1d4c56a66) )

	ROM_REGION16_LE( 0xC0000, "tms_gfx", 0 )
	ROM_LOAD16_BYTE( "005.vgb", 0x000000, 0x20000, CRC(7b1852f0) SHA1(d21525e59b3112313ea9783ac3dd988a4c1d5f87) )
	ROM_LOAD16_BYTE( "006.vgb", 0x000001, 0x20000, CRC(9d031636) SHA1(b7c7b57d547f2ce2eeb97126e961f3b5f35823f7) )
	ROM_LOAD16_BYTE( "007.vgb", 0x040000, 0x20000, CRC(15326070) SHA1(ec4484d4515694742d3fd3b944f342f052463988) )
	ROM_LOAD16_BYTE( "008.vgb", 0x040001, 0x20000, CRC(ca0e86d8) SHA1(a7b4b02d100a7875d5a184cdb76d507e926d1ca3) )
	ROM_LOAD16_BYTE( "003.vgb", 0x080000, 0x20000, CRC(4d8e8f54) SHA1(d8a23b5fd00ab919dc6d63fc72824d1293073813) )
	ROM_LOAD16_BYTE( "002.vgb", 0x080001, 0x20000, CRC(f6488e31) SHA1(d2f9304cc59f5523007592ae76ddd56107cc29e8) )

	/* Dr Math PCB (MPG 010-00002-001) */
	ROM_REGION32_BE( 0x100000, "drmath", 0 )
	ROMX_LOAD( "122.dth", 0x00000, 0x08000, CRC(9d2032cf) SHA1(8430816756ea92bbe86b94eaa24a6071bf0ef879), ROM_SKIP(7) )
	ROMX_LOAD( "125.dth", 0x00001, 0x08000, CRC(7718487c) SHA1(609106f55601f84095b64ce2484107779da89149), ROM_SKIP(7) )
	ROMX_LOAD( "123.dth", 0x00002, 0x08000, CRC(54d5544f) SHA1(d039ee39991b947a7483111359ab245fc104e060), ROM_SKIP(7) )
	ROMX_LOAD( "124.dth", 0x00003, 0x08000, CRC(7be96646) SHA1(a6733f75c0404282d71e8c1a287546ef4d9d42ad), ROM_SKIP(7) )
	ROMX_LOAD( "118.dth", 0x00004, 0x08000, CRC(cc895c20) SHA1(140ef47536914fe1441778e759894c2cdd893276), ROM_SKIP(7) )
	ROMX_LOAD( "121.dth", 0x00005, 0x08000, CRC(392e5c43) SHA1(455cf3bb3c16217e58d6eea51d8f49a5bed1955e), ROM_SKIP(7) )
	ROMX_LOAD( "119.dth", 0x00006, 0x08000, CRC(b1c966e5) SHA1(9703bb1f9bdf6a779b59daebb39df2926727fa76), ROM_SKIP(7) )
	ROMX_LOAD( "120.dth", 0x00007, 0x08000, CRC(5fb9836d) SHA1(d511aa9f02972a7f475c82c6f57d1f3fd4f118fa), ROM_SKIP(7) )

	ROM_REGION16_BE( 0x80000, "vertex", 0 )
	ROM_LOAD16_BYTE( "014.dth", 0x00001, 0x20000, CRC(5ca7713f) SHA1(ac7b9629684b99ecfb1945176b06eb6be284ba93) )
	ROM_LOAD16_BYTE( "015.dth", 0x00000, 0x20000, CRC(beae31bb) SHA1(1ab80a6b99eea6d5bf9b1bce58ecca13042c77a6) )
	ROM_LOAD16_BYTE( "016.dth", 0x40001, 0x20000, CRC(5db4f677) SHA1(25a6fe4c562e4fa4225aa4687dd41920b614e591) )
	ROM_LOAD16_BYTE( "017.dth", 0x40000, 0x20000, CRC(47f9a868) SHA1(7c8a9355893e4a3f3846fd05e0237ffd1404ffee) )

	/* Sound PCB (MPG 010-00018-002) */
	ROM_REGION( 0x08000, "audiocpu", 0 )
	ROM_LOAD( "110-00004-001.u2", 0x000000, 0x08000, CRC(705685a9) SHA1(311f7cac126a19e8bd555ebf31ff4ec4680ddfa4) )

	ROM_REGION( 0x40000, "upd7759", 0 )
	ROM_LOAD( "110-00003-001.u17", 0x000000, 0x40000, CRC(af84b635) SHA1(844e5987a66e9e3ab2d2fe05b93a4da3512776bb) )
ROM_END

ROM_START( botss )
	/* Host PCB (MPG DW-00011C-0011-01) */
	ROM_REGION( 0x180000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "110-00153-100.u67", 0x000001, 0x20000, CRC(338aa9c3) SHA1(3d10329a5df80ab1761fd3953eb3872a72f26bef) )
	ROM_LOAD16_BYTE( "110-00153-101.u91", 0x000000, 0x20000, CRC(3278279e) SHA1(570935988c776283cdcd5aa13d71a75f0a466099) )
	ROM_LOAD16_BYTE( "110-00153-102.u68", 0x040001, 0x20000, CRC(258d2687) SHA1(5b765d14a0a8dc4ef58453bbb7780068c168d268) )
	ROM_LOAD16_BYTE( "110-00153-103.u92", 0x040000, 0x20000, CRC(1dde7ffa) SHA1(5363af00c8c896eccfad38ace5dfeb3bf96a73a1) )
	ROM_LOAD16_BYTE( "110-00013-004.u69", 0x080001, 0x20000, CRC(72a607ca) SHA1(1afc85380be12c429808c48f1502736a4c8b98e5) )
	ROM_LOAD16_BYTE( "110-00013-005.u93", 0x080000, 0x20000, CRC(f37680ae) SHA1(51f1ee805b7d1b2b078c612c572e12846de623b9) )
	ROM_LOAD16_BYTE( "110-00013-006.u70", 0x0c0001, 0x20000, CRC(57a1c728) SHA1(2bdc831be739ada0f4f4adec7974da453878db0e) )
	ROM_LOAD16_BYTE( "110-00013-007.u94", 0x0c0000, 0x20000, CRC(4c9e16af) SHA1(1f8acc9bb85fe1bf459b4358b9bf9cf9847e6a36) )
	ROM_LOAD16_BYTE( "110-00013-008.u71", 0x100001, 0x20000, CRC(cfc0333e) SHA1(9f290769129a61189870faef45c3f061eb7b5c07) )
	ROM_LOAD16_BYTE( "110-00013-009.u95", 0x100000, 0x20000, CRC(6c595d1e) SHA1(89fdc30166ba1e9706798547195bdf6875a02e96) )
	ROM_FILL(                     0x140000, 0x40000, 0xff )

	/* Dr Math PCB (MPG 010-00002-001) */
	ROM_REGION32_BE( 0x100000, "drmath", 0 )
	ROMX_LOAD( "110-00013-122.u134", 0x000000, 0x08000, CRC(bf60c487) SHA1(5ce80e89d9a24b627b0e97bf36a4e71c2eff4324), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-125.u126", 0x000001, 0x08000, CRC(b0dccf4a) SHA1(e8bfd622c006985b724cdbd3ad14c33e9ed27c6c), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-123.u114", 0x000002, 0x08000, CRC(04ba6ed1) SHA1(012be71c6b955beda2bd0ff376dcaab51b226723), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-224.u107", 0x000003, 0x08000, CRC(220db5d3) SHA1(3bfbe0eb97282c4ce449fd44e8e141de74f08eb0), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-118.u135", 0x000004, 0x08000, CRC(2903e682) SHA1(027ed6524e9d4490632f10aeb22150c2fbc4eec2), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-121.u127", 0x000005, 0x08000, CRC(198a636b) SHA1(356b8948aafb98cb5e6ee7b5ad6ea9e5998265e5), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-219.u115", 0x000006, 0x08000, CRC(9c9dbac1) SHA1(4c66971884190598e128684ece2e15a1c80b94ed), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-120.u108", 0x000007, 0x08000, CRC(dafa173a) SHA1(a19980b92a5e74ebe395be36313701fdb527a46a), ROM_SKIP(7) )

	ROM_REGION16_BE( 0x80000, "vertex", 0 )
	ROM_LOAD16_BYTE( "110-00013-014.u153", 0x00001, 0x20000, CRC(0eee0557) SHA1(8abe52cad31e59cf814fd9f64f4e42ddb4aa8c93) )
	ROM_LOAD16_BYTE( "110-00013-015.u154", 0x00000, 0x20000, CRC(68564122) SHA1(410d2db74e574774b2eadd7fdf891feef5d8a93f) )
	ROM_LOAD16_BYTE( "110-00013-016.u167", 0x40001, 0x20000, CRC(60c6cb26) SHA1(0e2bf65793715e12d8fd7f87fd3336a9d00ee7e6) )
	ROM_LOAD16_BYTE( "110-00013-017.u160", 0x40000, 0x20000, CRC(d8b89379) SHA1(aa08e111c1505a4ad55b14659f8e21fd39cfcb16) )

	ROM_REGION16_LE( 0x40000, "tms34010", 0 )
	ROM_LOAD16_BYTE( "110-00023-201.u101", 0x000000, 0x20000, CRC(7dc05f7d) SHA1(4d202b229cf4690d92491311e9ff14034b19c35c) )
	ROM_LOAD16_BYTE( "110-00023-204.u97",  0x000001, 0x20000, CRC(925fd08a) SHA1(fb06413debbffcd63b018f374f25b0d8e419c739) )

	/* Video Graphics PCB (MPG DW-010-00002-002) */
	ROM_REGION16_LE( 0xc0000, "tms_gfx", 0 )
	ROM_LOAD16_BYTE( "110-00023-205.u124", 0x000000, 0x20000, CRC(5482e0c4) SHA1(492afac1862f2899cd734d1e57ca978ed6a906d5) )
	ROM_LOAD16_BYTE( "110-00023-206.u121", 0x000001, 0x20000, CRC(a55e5d19) SHA1(86fbcb425103ae9fff381357339af349848fc3f2) )
	ROM_LOAD16_BYTE( "110-00023-207.u130", 0x040000, 0x20000, CRC(0d8cf60f) SHA1(d8021c6bc15beb5a0e6c86b91f8ed0389b1311a5) )
	ROM_LOAD16_BYTE( "110-00023-208.u133", 0x040001, 0x20000, CRC(a4db3137) SHA1(ef266cc17e33a2c63cda3332e266bf943e464e7f) )
	ROM_LOAD16_BYTE( "110-00023-203.u114", 0x080000, 0x20000, CRC(b1dacbb1) SHA1(323531b6919eed4a963d6aad871f1fd34203e698) )
	ROM_LOAD16_BYTE( "110-00023-202.u108", 0x080001, 0x20000, CRC(ac0d3179) SHA1(f4c67d59d913ead0f8a6d42e2ca66857ebf01602) )

	/* Sound PCB (MPG 010-00018-002) */
	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "110-00014-001.u2", 0x000000, 0x08000, CRC(307fcb6d) SHA1(0cf63a39ac8920be6532974311804529d7218545) )

	ROM_REGION( 0x40000, "upd7759", 0 )
	ROM_LOAD( "110-00013-001.u17", 0x000000, 0x40000, CRC(015a0b17) SHA1(f229c9aa59f0e6b25b818f9513997a8685e33982) )

	ROM_REGION( 0x10000, "plds", 0 )
	/* Host */
	ROM_LOAD( "120-00001-306.u77", 0x000000, 0x00310, CRC(60282a45) SHA1(8621b64fa00fa556c09d7d1566480cd442a8e655) ) /* AmPAL23S8-20 */

	/* Dr. Math */
	ROM_LOAD( "mac1_u173_a.bin", 0x000000, 0x014c7, CRC(78040232) SHA1(c9adc1db76b4ee5ee08f4a11caae77993b23cc30) ) /* EPS448 */
	ROM_LOAD( "mac2_u166_a.bin", 0x000000, 0x014c7, CRC(c85c4c66) SHA1(fab07ad0611de7d2c2af9b6fa262d574e238bd9f) ) /* EPS448 */
ROM_END

ROM_START( botss11 )
	/* Host PCB (MPG DW-00011C-0011-02) */
	ROM_REGION( 0x180000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "110-00013-300.u67", 0x000001, 0x20000, CRC(7f74362a) SHA1(41611ba8e6eb5d6b3dfe88e1cede7d9fb5472e40) )
	ROM_LOAD16_BYTE( "110-00013-301.u91", 0x000000, 0x20000, CRC(a8100d1e) SHA1(69d3cac6f67563c0796560f7b874d7660720027d) )
	ROM_LOAD16_BYTE( "110-00013-302.u68", 0x040001, 0x20000, CRC(af865ee4) SHA1(f00bce49401431bc749208399329d9f92457186b) )
	ROM_LOAD16_BYTE( "110-00013-303.u92", 0x040000, 0x20000, CRC(15182619) SHA1(e95dcce11c0651c8e85fc0c658029f48eea35fb8) )
	ROM_LOAD16_BYTE( "110-00013-104.u69", 0x080001, 0x20000, CRC(72a607ca) SHA1(1afc85380be12c429808c48f1502736a4c8b98e5) )
	ROM_LOAD16_BYTE( "110-00013-105.u93", 0x080000, 0x20000, CRC(f37680ae) SHA1(51f1ee805b7d1b2b078c612c572e12846de623b9) )
	ROM_LOAD16_BYTE( "110-00013-106.u70", 0x0c0001, 0x20000, CRC(57a1c728) SHA1(2bdc831be739ada0f4f4adec7974da453878db0e) )
	ROM_LOAD16_BYTE( "110-00013-107.u94", 0x0c0000, 0x20000, CRC(4c9e16af) SHA1(1f8acc9bb85fe1bf459b4358b9bf9cf9847e6a36) )
	ROM_LOAD16_BYTE( "110-00013-108.u71", 0x100001, 0x20000, CRC(cfc0333e) SHA1(9f290769129a61189870faef45c3f061eb7b5c07) )
	ROM_LOAD16_BYTE( "110-00013-109.u95", 0x100000, 0x20000, CRC(6c595d1e) SHA1(89fdc30166ba1e9706798547195bdf6875a02e96) )
	ROM_FILL(                     0x140000, 0x40000, 0xff )

	/* Dr Math PCB (MPG 010-00002-001) */
	ROM_REGION32_BE( 0x100000, "drmath", 0 )
	ROMX_LOAD( "110-00013-122.u134", 0x000000, 0x08000, CRC(bf60c487) SHA1(5ce80e89d9a24b627b0e97bf36a4e71c2eff4324), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-125.u126", 0x000001, 0x08000, CRC(b0dccf4a) SHA1(e8bfd622c006985b724cdbd3ad14c33e9ed27c6c), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-123.u114", 0x000002, 0x08000, CRC(04ba6ed1) SHA1(012be71c6b955beda2bd0ff376dcaab51b226723), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-124.u107", 0x000003, 0x08000, CRC(220db5d3) SHA1(3bfbe0eb97282c4ce449fd44e8e141de74f08eb0), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-018.u135", 0x000004, 0x08000, CRC(2903e682) SHA1(027ed6524e9d4490632f10aeb22150c2fbc4eec2), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-121.u127", 0x000005, 0x08000, CRC(198a636b) SHA1(356b8948aafb98cb5e6ee7b5ad6ea9e5998265e5), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-119.u115", 0x000006, 0x08000, CRC(9c9dbac1) SHA1(4c66971884190598e128684ece2e15a1c80b94ed), ROM_SKIP(7) )
	ROMX_LOAD( "110-00013-120.u108", 0x000007, 0x08000, CRC(dafa173a) SHA1(a19980b92a5e74ebe395be36313701fdb527a46a), ROM_SKIP(7) )

	ROM_REGION16_BE( 0x80000, "vertex", 0 )
	ROM_LOAD16_BYTE( "110-00013-014.u153", 0x00001, 0x20000, CRC(0eee0557) SHA1(8abe52cad31e59cf814fd9f64f4e42ddb4aa8c93) )
	ROM_LOAD16_BYTE( "110-00013-015.u154", 0x00000, 0x20000, CRC(68564122) SHA1(410d2db74e574774b2eadd7fdf891feef5d8a93f) )
	ROM_LOAD16_BYTE( "110-00013-016.u167", 0x40001, 0x20000, CRC(60c6cb26) SHA1(0e2bf65793715e12d8fd7f87fd3336a9d00ee7e6) )
	ROM_LOAD16_BYTE( "110-00013-017.u160", 0x40000, 0x20000, CRC(d8b89379) SHA1(aa08e111c1505a4ad55b14659f8e21fd39cfcb16) )

	ROM_REGION16_LE( 0x40000, "tms34010", 0 )
	ROM_LOAD16_BYTE( "110-00023-101.u101", 0x000000, 0x20000, CRC(6aada23d) SHA1(85dbf9b20e4f17cb21922637763654d6cae80dfd) )
	ROM_LOAD16_BYTE( "110-00023-104.u97",  0x000001, 0x20000, CRC(715cac9d) SHA1(2aa0c563dc1fe4d02fa1ecbaed16f720f899fdc4) )

	/* Video Graphics PCB (MPG DW-010-00002-002) */
	ROM_REGION16_LE( 0xc0000, "tms_gfx", 0 )
	ROM_LOAD16_BYTE( "110-00023-105.u124", 0x000000, 0x20000, CRC(5482e0c4) SHA1(492afac1862f2899cd734d1e57ca978ed6a906d5) )
	ROM_LOAD16_BYTE( "110-00023-106.u121", 0x000001, 0x20000, CRC(a55e5d19) SHA1(86fbcb425103ae9fff381357339af349848fc3f2) )
	ROM_LOAD16_BYTE( "110-00023-107.u130", 0x040000, 0x20000, CRC(006487b6) SHA1(f8bc6abad13df099da1708bd22f239703e407b21) )
	ROM_LOAD16_BYTE( "110-00023-108.u133", 0x040001, 0x20000, CRC(e4587ba1) SHA1(1323b4be5a526ae182ee38e96fccd263a4cecc37) )
	ROM_LOAD16_BYTE( "110-00023-103.u114", 0x080000, 0x20000, CRC(4e486e70) SHA1(04ee16cfadd43dbe9ed5bd8330c21a718d63a8f4) )
	ROM_LOAD16_BYTE( "110-00023-102.u108", 0x080001, 0x20000, CRC(441e8490) SHA1(6cfe30cea3fa297b71e881fbddad6d65a96e4386) )

	/* Sound PCB (MPG 010-00018-002) */
	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "110-00014-001.u2", 0x000000, 0x08000, CRC(307fcb6d) SHA1(0cf63a39ac8920be6532974311804529d7218545) )

	ROM_REGION( 0x40000, "upd7759", 0 )
	ROM_LOAD( "110-00013-001.u17", 0x000000, 0x40000, CRC(015a0b17) SHA1(f229c9aa59f0e6b25b818f9513997a8685e33982) )
ROM_END

ROM_START( tankbatl )
	ROM_REGION( 0x180000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "lo_u67",    0x000001, 0x20000, CRC(97aabac0) SHA1(12a0719d3332a63e912161200b0a942c27c1f5da) )
	ROM_LOAD16_BYTE( "le_u91",    0x000000, 0x20000, CRC(977f90d9) SHA1(530fa5c32b1f28e2b90d20d98cc453cb290c0ad2) )
	ROM_LOAD16_BYTE( "ho_u68",    0x040001, 0x20000, CRC(8f76f4ac) SHA1(f6c1d4c933a373b153eee7d9f3016c985acaa281) )
	ROM_LOAD16_BYTE( "he_u92",    0x040000, 0x20000, CRC(1ea1db7c) SHA1(ecaa1bd3d70489a5ba0d96c6935c2959f57467b2) )
	ROM_LOAD16_BYTE( "b00_o.u69", 0x080001, 0x20000, CRC(393718e5) SHA1(f956f8bd946f53a032af16011dc69f66fb3f095c) )
	ROM_LOAD16_BYTE( "b00_e.u93", 0x080000, 0x20000, CRC(aedea0ef) SHA1(a81c3518c7a1e21f2fa2ad29c30346f727069257) )
	ROM_LOAD16_BYTE( "b01_o.u70", 0x0c0001, 0x20000, CRC(e895167d) SHA1(677cbf1be32c1f0c76a0e1527db66eb037d7e9df) )
	ROM_LOAD16_BYTE( "b01_e.u94", 0x0c0000, 0x20000, CRC(823bba4d) SHA1(6668e972b1435aac43f9b21cc40fc3adec0d285f) )
	ROM_LOAD16_BYTE( "host.71",   0x100001, 0x20000, CRC(cfc0333e) SHA1(9f290769129a61189870faef45c3f061eb7b5c07) )
	ROM_LOAD16_BYTE( "host.95",   0x100000, 0x20000, CRC(6c595d1e) SHA1(89fdc30166ba1e9706798547195bdf6875a02e96) )
	ROM_FILL(                     0x140000, 0x40000, 0xff )

	ROM_REGION32_BE( 0x100000, "drmath", 0 )
	ROMX_LOAD( "s24e_u.134", 0x00000, 0x08000, CRC(0a41756b) SHA1(8681aaf8eeda7acdff967a773290c4b2c17cbe30), ROM_SKIP(7) )
	ROMX_LOAD( "s16e_u.126", 0x00001, 0x08000, CRC(d24654cd) SHA1(88d3624f23c669dc902136c822b1f4732104c9c1), ROM_SKIP(7) )
	ROMX_LOAD( "s08e_u.114", 0x00002, 0x08000, CRC(765da5d7) SHA1(d489581bd12d7fca42570ee7a12d922be2528c1e), ROM_SKIP(7) )
	ROMX_LOAD( "s00e_u.107", 0x00003, 0x08000, CRC(558918cc) SHA1(7e61639ab4af88f888f4aa481dd01db7de3829da), ROM_SKIP(7) )
	ROMX_LOAD( "s24o_u.135", 0x00004, 0x08000, CRC(f89bab5f) SHA1(e79e71d0a5e7ba933952c5d41f6afb633da06e8a), ROM_SKIP(7) )
	ROMX_LOAD( "s16o_u.127", 0x00005, 0x08000, CRC(53ba1a3f) SHA1(333734fff41b98abfa7b2904692cb128ab1f90a3), ROM_SKIP(7) )
	ROMX_LOAD( "s08o_u.115", 0x00006, 0x08000, CRC(af1eae4a) SHA1(44f272b472f546ffff7d8f82e29c5d80b472b1c3), ROM_SKIP(7) )
	ROMX_LOAD( "s00o_u.108", 0x00007, 0x08000, CRC(9cadc977) SHA1(e95f60d9df422511bae6a6c4a20f813d77a894a4), ROM_SKIP(7) )

	ROM_REGION16_BE( 0x80000, "vertex", 0 )
	ROM_LOAD16_BYTE( "pb0o_u.153", 0x00001, 0x20000, CRC(bcd7ddad) SHA1(3982756b6f0821df77918dd0d00807a90dbfb595) )
	ROM_LOAD16_BYTE( "pb0e_u.154", 0x00000, 0x20000, CRC(d84e7c71) SHA1(2edb13c1f96f35c7934dad380e06035335ccbb48) )
	ROM_LOAD16_BYTE( "pb1o_u.167", 0x40001, 0x20000, CRC(e4a65313) SHA1(f2df5cc87aa388d3273705562ab2d7c937a0a866) )
	ROM_LOAD16_BYTE( "pb1e_u.160", 0x40000, 0x20000, CRC(9d9d1395) SHA1(9d937eac8d7e7bea40a69b596ba2c01753b97565) )

	ROM_REGION16_LE( 0x40000, "tms34010", 0 )
	ROM_LOAD16_BYTE( "3el_u101", 0x000000, 0x20000, CRC(130e1a18) SHA1(c31af5c5a403da588142ccbea79d3aa253ac6519) )
	ROM_LOAD16_BYTE( "3eh_u97",  0x000001, 0x20000, CRC(0fdcab16) SHA1(afc21747e1624f3ab87b289b5f4a498141062445) )

	ROM_REGION16_LE( 0xC0000, "tms_gfx", 0 )
	ROM_LOAD16_BYTE( "38l_u124", 0x000000, 0x20000, CRC(4e084daa) SHA1(f65f51d8d7c6b46aa844b37b212dab11c786d856) )
	ROM_LOAD16_BYTE( "38h_u121", 0x000001, 0x20000, CRC(3628c8c1) SHA1(760eda076ec46af5b954548036da5230a5c86371) )
	ROM_LOAD16_BYTE( "3al_u130", 0x040000, 0x20000, CRC(8a5386e3) SHA1(361f6abdb88cf51d5ec5ce6882986296dd274d3b) )
	ROM_LOAD16_BYTE( "3ah_u133", 0x040001, 0x20000, CRC(7e674ac1) SHA1(81f1d87e62faf94a44aca7e41a32edf5c7c145ec) )
	ROM_LOAD16_BYTE( "3cl_u114", 0x080000, 0x20000, CRC(bc04b0e6) SHA1(d08fddd52f2c1a565a80f5d4ff8b07f1c5f01a01) )
	ROM_LOAD16_BYTE( "3ch_u108", 0x080001, 0x20000, CRC(7cb688af) SHA1(6be495ae0ed74739f62de65386810864c9ffaaee) )

	ROM_REGION( 0x08000, "audiocpu", 0 )
	ROM_LOAD( "sound.u2", 0x000000, 0x08000, CRC(77190a90) SHA1(a36a5a8457cc1c325e6318b083e5e271e163f7cb) )

	ROM_REGION( 0x40000, "upd7759", 0 )
	ROM_LOAD( "sound.u17", 0x000000, 0x40000, CRC(d033ef6c) SHA1(0404473c87b5b52e39ab3824b159a2d98159bbea) )
ROM_END


/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1991, f15se,    0,     micro3d, f15se,    micro3d_state, init_micro3d, ROT0, "Microprose Games Inc.", "F-15 Strike Eagle (rev. 2.2 02/25/91)",          MACHINE_IMPERFECT_SOUND )
GAME( 1991, f15se21,  f15se, micro3d, f15se,    micro3d_state, init_micro3d, ROT0, "Microprose Games Inc.", "F-15 Strike Eagle (rev. 2.1 02/04/91)",          MACHINE_IMPERFECT_SOUND )
GAME( 1992, botss,    0,     micro3d, botss,    micro3d_state, init_botss,   ROT0, "Microprose Games Inc.", "Battle of the Solar System (rev. 1.1a 7/23/92)", MACHINE_IMPERFECT_SOUND )
GAME( 1992, botss11,  botss, botss11, botss11,  micro3d_state, init_micro3d, ROT0, "Microprose Games Inc.", "Battle of the Solar System (rev. 1.1 3/24/92)",  MACHINE_IMPERFECT_SOUND )
GAME( 1992, tankbatl, 0,     botss11, tankbatl, micro3d_state, init_micro3d, ROT0, "Microprose Games Inc.", "Tank Battle (prototype rev. 4/21/92)",           MACHINE_IMPERFECT_SOUND )

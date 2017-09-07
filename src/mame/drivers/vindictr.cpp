// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    Atari Vindicators hardware

    driver by Aaron Giles

    Games supported:
        * Vindicators (1988) [8 sets]

    Known bugs:
        * none at this time

****************************************************************************

    Memory map (TBA)

***************************************************************************/


#include "emu.h"
#include "includes/vindictr.h"

#include "cpu/m68000/m68000.h"
#include "machine/eeprompar.h"
#include "machine/watchdog.h"
#include "speaker.h"



/*************************************
 *
 *  Initialization
 *
 *************************************/

void vindictr_state::update_interrupts()
{
	m_maincpu->set_input_line(4, m_scanline_int_state ? ASSERT_LINE : CLEAR_LINE);
	m_maincpu->set_input_line(6, m_sound_int_state ? ASSERT_LINE : CLEAR_LINE);
}


MACHINE_RESET_MEMBER(vindictr_state,vindictr)
{
	atarigen_state::machine_reset();
	scanline_timer_reset(*m_screen, 8);
}



/*************************************
 *
 *  I/O handling
 *
 *************************************/

READ16_MEMBER(vindictr_state::port1_r)
{
	int result = ioport("260010")->read();
	result ^= 0x0010;
	return result;
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, AS_PROGRAM, 16, vindictr_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x3fffff)
	AM_RANGE(0x000000, 0x05ffff) AM_ROM
	AM_RANGE(0x0e0000, 0x0e0fff) AM_DEVREADWRITE8("eeprom", eeprom_parallel_28xx_device, read, write, 0x00ff)
	AM_RANGE(0x1f0000, 0x1fffff) AM_DEVWRITE("eeprom", eeprom_parallel_28xx_device, unlock_write)
	AM_RANGE(0x260000, 0x26000f) AM_READ_PORT("260000")
	AM_RANGE(0x260010, 0x26001f) AM_READ(port1_r)
	AM_RANGE(0x260020, 0x26002f) AM_READ_PORT("260020")
	AM_RANGE(0x260030, 0x260031) AM_DEVREAD8("jsa", atari_jsa_i_device, main_response_r, 0x00ff)
	AM_RANGE(0x2e0000, 0x2e0001) AM_DEVWRITE("watchdog", watchdog_timer_device, reset16_w)
	AM_RANGE(0x360000, 0x360001) AM_WRITE(scanline_int_ack_w)
	AM_RANGE(0x360010, 0x360011) AM_WRITENOP
	AM_RANGE(0x360020, 0x360021) AM_DEVWRITE("jsa", atari_jsa_i_device, sound_reset_w)
	AM_RANGE(0x360030, 0x360031) AM_DEVWRITE8("jsa", atari_jsa_i_device, main_command_w, 0x00ff)
	AM_RANGE(0x3e0000, 0x3e0fff) AM_RAM_WRITE(vindictr_paletteram_w) AM_SHARE("paletteram")
	AM_RANGE(0x3f0000, 0x3f1fff) AM_MIRROR(0x8000) AM_RAM_DEVWRITE("playfield", tilemap_device, write) AM_SHARE("playfield")
	AM_RANGE(0x3f2000, 0x3f3fff) AM_MIRROR(0x8000) AM_RAM AM_SHARE("mob")
	AM_RANGE(0x3f4000, 0x3f4f7f) AM_MIRROR(0x8000) AM_RAM_DEVWRITE("alpha", tilemap_device, write) AM_SHARE("alpha")
	AM_RANGE(0x3f4f80, 0x3f4fff) AM_MIRROR(0x8000) AM_RAM AM_SHARE("mob:slip")
	AM_RANGE(0x3f5000, 0x3f7fff) AM_MIRROR(0x8000) AM_RAM
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( vindictr )
	PORT_START("260000")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Left Stick Fire")
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 Right Stick Fire")
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Left Stick Thumb")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("P1 Right Stick Thumb")
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN ) PORT_2WAY PORT_PLAYER(1)

	PORT_START("260010")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_CUSTOM ) PORT_VBLANK("screen")
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_CUSTOM ) PORT_ATARI_JSA_SOUND_TO_MAIN_READY("jsa")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_CUSTOM ) PORT_ATARI_JSA_MAIN_TO_SOUND_READY("jsa")
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED ) // TODO: what's this?
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Left Stick Fire")
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("P2 Right Stick Fire")
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Left Stick Thumb")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("P2 Right Stick Thumb")
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN ) PORT_2WAY PORT_PLAYER(2)

	PORT_START("260020")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout anlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};


static const gfx_layout pfmolayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static GFXDECODE_START( vindictr )
	GFXDECODE_ENTRY( "gfx1", 0, pfmolayout,  256, 32 )      /* sprites & playfield */
	GFXDECODE_ENTRY( "gfx2", 0, anlayout,      0, 64 )      /* characters 8x8 */
GFXDECODE_END



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_CONFIG_START( vindictr )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68010, ATARI_CLOCK_14MHz/2)
	MCFG_CPU_PROGRAM_MAP(main_map)

	MCFG_MACHINE_RESET_OVERRIDE(vindictr_state,vindictr)

	MCFG_EEPROM_2804_ADD("eeprom")
	MCFG_EEPROM_28XX_LOCK_AFTER_WRITE(true)

	MCFG_WATCHDOG_ADD("watchdog")

	/* video hardware */
	MCFG_GFXDECODE_ADD("gfxdecode", "palette", vindictr)
	MCFG_PALETTE_ADD("palette", 2048*8)

	MCFG_TILEMAP_ADD_STANDARD("playfield", "gfxdecode", 2, vindictr_state, get_playfield_tile_info, 8,8, SCAN_COLS, 64,64)
	MCFG_TILEMAP_ADD_STANDARD_TRANSPEN("alpha", "gfxdecode", 2, vindictr_state, get_alpha_tile_info, 8,8, SCAN_ROWS, 64,32, 0)
	MCFG_ATARI_MOTION_OBJECTS_ADD("mob", "screen", vindictr_state::s_mob_config)
	MCFG_ATARI_MOTION_OBJECTS_GFXDECODE("gfxdecode")

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	/* note: these parameters are from published specs, not derived */
	/* the board uses a SYNGEN chip to generate video signals */
	MCFG_SCREEN_RAW_PARAMS(ATARI_CLOCK_14MHz/2, 456, 0, 336, 262, 0, 240)
	MCFG_SCREEN_UPDATE_DRIVER(vindictr_state, screen_update_vindictr)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_VIDEO_START_OVERRIDE(vindictr_state,vindictr)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MCFG_ATARI_JSA_I_ADD("jsa", WRITELINE(atarigen_state, sound_int_write_line))
	MCFG_ATARI_JSA_TEST_PORT("260010", 1)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "lspeaker", 1.0)
	MCFG_DEVICE_REMOVE("jsa:tms")
MACHINE_CONFIG_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( vindictr )
	ROM_REGION( 0x60000, "maincpu", 0 ) /* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "136059-5117.d1",  0x000000, 0x010000, CRC(2e5135e4) SHA1(804b3ba201088ac2c35cfcbd530acbd73548ea8c) )
	ROM_LOAD16_BYTE( "136059-5118.d3",  0x000001, 0x010000, CRC(e357fa79) SHA1(220a10287f4bf9d981fd412c8dd0a9c106eaf342) )
	ROM_LOAD16_BYTE( "136059-5119.f1",  0x020000, 0x010000, CRC(0deb7330) SHA1(e9fb311e96bcf57f2136fff87a973a5a3b5208b3) )
	ROM_LOAD16_BYTE( "136059-5120.f3",  0x020001, 0x010000, CRC(a6ae4753) SHA1(e69067ba0f1e5a4e446356e2fee3763dd4bcdd5a) )
	ROM_LOAD16_BYTE( "136059-5121.k1",  0x040000, 0x010000, CRC(96b150c5) SHA1(405c848f7990c981fefd355ca635bfb0ac24eb26) )
	ROM_LOAD16_BYTE( "136059-5122.k3",  0x040001, 0x010000, CRC(6415d312) SHA1(0115e32c1c42421cb3d978cc8642f7f88d492043) )

	ROM_REGION( 0x10000, "jsa:cpu", 0 ) /* 64k for 6502 code */
	ROM_LOAD( "136059-1124.2k",  0x00000, 0x10000, CRC(d2212c0a) SHA1(df11fe76d74abc0cea23f18264cef4b0f33b1ffd) )

	ROM_REGION( 0x100000, "gfx1", ROMREGION_INVERT )
	ROM_LOAD( "136059-1104.12p", 0x000000, 0x020000, CRC(062f8e52) SHA1(0968b8c822d8fee1cf7ddcf9c3b1bf059e446417) )
	ROM_LOAD( "136059-1116.19p", 0x020000, 0x010000, CRC(0e4366fa) SHA1(1891f6b818f7b0e447e8a83ad0c12aade0b776ee) )
	ROM_RELOAD(                  0x030000, 0x010000 )
	ROM_LOAD( "136059-1103.8p",  0x040000, 0x020000, CRC(09123b57) SHA1(ddd5a4033b5f5ed45f639909364fe5ccd7a0cb53) )
	ROM_LOAD( "136059-1115.2p",  0x060000, 0x010000, CRC(6b757bca) SHA1(2d615b1b42f554bbfebc34928c106c3dd93dc7b2) )
	ROM_RELOAD(                  0x070000, 0x010000 )
	ROM_LOAD( "136059-1102.12r", 0x080000, 0x020000, CRC(a5268c4f) SHA1(99f1f1f2e88f8b2f235070e525aaed9aff6e91c6) )
	ROM_LOAD( "136059-1114.19r", 0x0a0000, 0x010000, CRC(609f619e) SHA1(64b5c2b0f5da07a9dd148aa19bb87e2b2cb1c395) )
	ROM_RELOAD(                  0x0b0000, 0x010000 )
	ROM_LOAD( "136059-1101.8r",  0x0c0000, 0x020000, CRC(2d07fdaa) SHA1(b6772fd764ddc1d2fa1c44c931b269aab9ad5e2b) )
	ROM_LOAD( "136059-1113.2r",  0x0e0000, 0x010000, CRC(0a2aba63) SHA1(e4780c790278034f0332697d5f06e6ed6b57d273) )
	ROM_RELOAD(                  0x0f0000, 0x010000 )

	ROM_REGION( 0x04000, "gfx2", 0 )
	ROM_LOAD( "136059-1123.16n", 0x000000, 0x004000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0x00800, "plds", 0 )
	ROM_LOAD( "pal16l8a-136059-1150.c3",  0x0000, 0x0104, CRC(09d02b00) SHA1(3851f0c0958db983ab907f64ac370a1051c2b76a) )
	ROM_LOAD( "pal16l8a-136059-1151.d17", 0x0200, 0x0104, CRC(797dcde7) SHA1(0c9db6610c40d3bf58117aa9bc8826d33f063dff) )
	ROM_LOAD( "pal16l8a-136059-1152.e17", 0x0400, 0x0104, CRC(56634c58) SHA1(c52db58572d0d8f8eeab6abf891455115b6ed146) )
	ROM_LOAD( "pal16r6a-136059-1153.n7",  0x0600, 0x0104, CRC(61076033) SHA1(c860835a8fa48e141f3d24732395ac35a4b908a4) )
ROM_END


ROM_START( vindictre )
	ROM_REGION( 0x60000, "maincpu", 0 ) /* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "136059-5717.d1",  0x000000, 0x010000, CRC(af5ba4a8) SHA1(fdb6e7f0707af94b39368cc39ae45c53209ce32e) )
	ROM_LOAD16_BYTE( "136059-5718.d3",  0x000001, 0x010000, CRC(c87b0581) SHA1(f33c72e83e8c811d3405deb470573327c7b68ea6) )
	ROM_LOAD16_BYTE( "136059-5719.f1",  0x020000, 0x010000, CRC(1e5f94e1) SHA1(bf14e4d3c26507ad3a78ad28b6b54e4ea0939ceb) )
	ROM_LOAD16_BYTE( "136059-5720.f3",  0x020001, 0x010000, CRC(cace40d7) SHA1(e897c56aa6134f39fc8e96f5ff96ca9c71623a32) )
	ROM_LOAD16_BYTE( "136059-5721.k1",  0x040000, 0x010000, CRC(96b150c5) SHA1(405c848f7990c981fefd355ca635bfb0ac24eb26) )
	ROM_LOAD16_BYTE( "136059-5722.k3",  0x040001, 0x010000, CRC(6415d312) SHA1(0115e32c1c42421cb3d978cc8642f7f88d492043) )

	ROM_REGION( 0x10000, "jsa:cpu", 0 ) /* 64k for 6502 code */
	ROM_LOAD( "136059-1124.2k",  0x00000, 0x10000, CRC(d2212c0a) SHA1(df11fe76d74abc0cea23f18264cef4b0f33b1ffd) )

	ROM_REGION( 0x100000, "gfx1", ROMREGION_INVERT )
	ROM_LOAD( "136059-1104.12p", 0x000000, 0x020000, CRC(062f8e52) SHA1(0968b8c822d8fee1cf7ddcf9c3b1bf059e446417) )
	ROM_LOAD( "136059-1116.19p", 0x020000, 0x010000, CRC(0e4366fa) SHA1(1891f6b818f7b0e447e8a83ad0c12aade0b776ee) )
	ROM_RELOAD(                  0x030000, 0x010000 )
	ROM_LOAD( "136059-1103.8p",  0x040000, 0x020000, CRC(09123b57) SHA1(ddd5a4033b5f5ed45f639909364fe5ccd7a0cb53) )
	ROM_LOAD( "136059-1115.2p",  0x060000, 0x010000, CRC(6b757bca) SHA1(2d615b1b42f554bbfebc34928c106c3dd93dc7b2) )
	ROM_RELOAD(                  0x070000, 0x010000 )
	ROM_LOAD( "136059-1102.12r", 0x080000, 0x020000, CRC(a5268c4f) SHA1(99f1f1f2e88f8b2f235070e525aaed9aff6e91c6) )
	ROM_LOAD( "136059-1114.19r", 0x0a0000, 0x010000, CRC(609f619e) SHA1(64b5c2b0f5da07a9dd148aa19bb87e2b2cb1c395) )
	ROM_RELOAD(                  0x0b0000, 0x010000 )
	ROM_LOAD( "136059-1101.8r",  0x0c0000, 0x020000, CRC(2d07fdaa) SHA1(b6772fd764ddc1d2fa1c44c931b269aab9ad5e2b) )
	ROM_LOAD( "136059-1113.2r",  0x0e0000, 0x010000, CRC(0a2aba63) SHA1(e4780c790278034f0332697d5f06e6ed6b57d273) )
	ROM_RELOAD(                  0x0f0000, 0x010000 )

	ROM_REGION( 0x04000, "gfx2", 0 )
	ROM_LOAD( "136059-1123.16n", 0x000000, 0x004000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0x00800, "plds", 0 )
	ROM_LOAD( "pal16l8a-136059-1150.c3",  0x0000, 0x0104, CRC(09d02b00) SHA1(3851f0c0958db983ab907f64ac370a1051c2b76a) )
	ROM_LOAD( "pal16l8a-136059-1151.d17", 0x0200, 0x0104, CRC(797dcde7) SHA1(0c9db6610c40d3bf58117aa9bc8826d33f063dff) )
	ROM_LOAD( "pal16l8a-136059-1152.e17", 0x0400, 0x0104, CRC(56634c58) SHA1(c52db58572d0d8f8eeab6abf891455115b6ed146) )
	ROM_LOAD( "pal16r6a-136059-1153.n7",  0x0600, 0x0104, CRC(61076033) SHA1(c860835a8fa48e141f3d24732395ac35a4b908a4) )
ROM_END


ROM_START( vindictrg )
	ROM_REGION( 0x60000, "maincpu", 0 ) /* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "136059-1217.d1",  0x000000, 0x010000, CRC(0a589e9a) SHA1(6770212b57599cd9bcdeb126aec30d9815608005) )
	ROM_LOAD16_BYTE( "136059-1218.d3",  0x000001, 0x010000, CRC(e8b7959a) SHA1(b63747934b188f44a5e59a54f52d15b33f9d676b) )
	ROM_LOAD16_BYTE( "136059-1219.f1",  0x020000, 0x010000, CRC(2534fcbc) SHA1(d8a2121de88efabf99a153fd477c7bf2fddc88c9) )
	ROM_LOAD16_BYTE( "136059-1220.f3",  0x020001, 0x010000, CRC(d0947780) SHA1(5dc0f510f809eb2f75792cfdcfd35087d3aa28a6) )
	ROM_LOAD16_BYTE( "136059-1221.k1",  0x040000, 0x010000, CRC(ee1b1014) SHA1(ddfe01cdec4654a42c9e49660e3532e5c865a9b7) )
	ROM_LOAD16_BYTE( "136059-1222.k3",  0x040001, 0x010000, CRC(517b33f0) SHA1(f6430862bb00e11a68e964c89adcad1f05bc021b) )

	ROM_REGION( 0x10000, "jsa:cpu", 0 ) /* 64k for 6502 code */
	ROM_LOAD( "136059-1124.2k",  0x00000, 0x10000, CRC(d2212c0a) SHA1(df11fe76d74abc0cea23f18264cef4b0f33b1ffd) )

	ROM_REGION( 0x100000, "gfx1", ROMREGION_INVERT )
	ROM_LOAD( "136059-1104.12p", 0x000000, 0x020000, CRC(062f8e52) SHA1(0968b8c822d8fee1cf7ddcf9c3b1bf059e446417) )
	ROM_LOAD( "136059-1116.19p", 0x020000, 0x010000, CRC(0e4366fa) SHA1(1891f6b818f7b0e447e8a83ad0c12aade0b776ee) )
	ROM_RELOAD(                  0x030000, 0x010000 )
	ROM_LOAD( "136059-1103.8p",  0x040000, 0x020000, CRC(09123b57) SHA1(ddd5a4033b5f5ed45f639909364fe5ccd7a0cb53) )
	ROM_LOAD( "136059-1115.2p",  0x060000, 0x010000, CRC(6b757bca) SHA1(2d615b1b42f554bbfebc34928c106c3dd93dc7b2) )
	ROM_RELOAD(                  0x070000, 0x010000 )
	ROM_LOAD( "136059-1102.12r", 0x080000, 0x020000, CRC(a5268c4f) SHA1(99f1f1f2e88f8b2f235070e525aaed9aff6e91c6) )
	ROM_LOAD( "136059-1114.19r", 0x0a0000, 0x010000, CRC(609f619e) SHA1(64b5c2b0f5da07a9dd148aa19bb87e2b2cb1c395) )
	ROM_RELOAD(                  0x0b0000, 0x010000 )
	ROM_LOAD( "136059-1101.8r",  0x0c0000, 0x020000, CRC(2d07fdaa) SHA1(b6772fd764ddc1d2fa1c44c931b269aab9ad5e2b) )
	ROM_LOAD( "136059-1113.2r",  0x0e0000, 0x010000, CRC(0a2aba63) SHA1(e4780c790278034f0332697d5f06e6ed6b57d273) )
	ROM_RELOAD(                  0x0f0000, 0x010000 )

	ROM_REGION( 0x04000, "gfx2", 0 )
	ROM_LOAD( "136059-1223.16n", 0x000000, 0x004000, CRC(d27975bb) SHA1(a8ab8bdbd9fbcbcf73e8621b2a4447d25bf612b8) )

	ROM_REGION( 0x00800, "plds", 0 )
	ROM_LOAD( "pal16l8a-136059-1150.c3",  0x0000, 0x0104, CRC(09d02b00) SHA1(3851f0c0958db983ab907f64ac370a1051c2b76a) )
	ROM_LOAD( "pal16l8a-136059-1151.d17", 0x0200, 0x0104, CRC(797dcde7) SHA1(0c9db6610c40d3bf58117aa9bc8826d33f063dff) )
	ROM_LOAD( "pal16l8a-136059-1152.e17", 0x0400, 0x0104, CRC(56634c58) SHA1(c52db58572d0d8f8eeab6abf891455115b6ed146) )
	ROM_LOAD( "pal16r6a-136059-1153.n7",  0x0600, 0x0104, CRC(61076033) SHA1(c860835a8fa48e141f3d24732395ac35a4b908a4) )
ROM_END


ROM_START( vindictre4 )
	ROM_REGION( 0x60000, "maincpu", 0 ) /* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "136059-1117.d1",  0x000000, 0x010000, CRC(2e5135e4) SHA1(804b3ba201088ac2c35cfcbd530acbd73548ea8c) )
	ROM_LOAD16_BYTE( "136059-1118.d3",  0x000001, 0x010000, CRC(e357fa79) SHA1(220a10287f4bf9d981fd412c8dd0a9c106eaf342) )
	ROM_LOAD16_BYTE( "136059-4719.f1",  0x020000, 0x010000, CRC(3b27ab80) SHA1(330a6fe0e0265cce40c913aa5c3607429afe510b) )
	ROM_LOAD16_BYTE( "136059-4720.f3",  0x020001, 0x010000, CRC(e5ac9933) SHA1(6c9b617219d27678fae0af83f6eaa6bd95a02d35) )
	ROM_LOAD16_BYTE( "136059-4121.k1",  0x040000, 0x010000, CRC(9a0444ee) SHA1(211be931a8b6ca42dd140baf3e165ce23f75431f) )
	ROM_LOAD16_BYTE( "136059-4122.k3",  0x040001, 0x010000, CRC(d5022d78) SHA1(eeb6876ee6994f5736114a786c5c4ba97f26ef01) )

	ROM_REGION( 0x10000, "jsa:cpu", 0 ) /* 64k for 6502 code */
	ROM_LOAD( "136059-1124.2k",  0x00000, 0x10000, CRC(d2212c0a) SHA1(df11fe76d74abc0cea23f18264cef4b0f33b1ffd) )

	ROM_REGION( 0x100000, "gfx1", ROMREGION_INVERT )
	ROM_LOAD( "136059-1104.12p", 0x000000, 0x020000, CRC(062f8e52) SHA1(0968b8c822d8fee1cf7ddcf9c3b1bf059e446417) )
	ROM_LOAD( "136059-1116.19p", 0x020000, 0x010000, CRC(0e4366fa) SHA1(1891f6b818f7b0e447e8a83ad0c12aade0b776ee) )
	ROM_RELOAD(                  0x030000, 0x010000 )
	ROM_LOAD( "136059-1103.8p",  0x040000, 0x020000, CRC(09123b57) SHA1(ddd5a4033b5f5ed45f639909364fe5ccd7a0cb53) )
	ROM_LOAD( "136059-1115.2p",  0x060000, 0x010000, CRC(6b757bca) SHA1(2d615b1b42f554bbfebc34928c106c3dd93dc7b2) )
	ROM_RELOAD(                  0x070000, 0x010000 )
	ROM_LOAD( "136059-1102.12r", 0x080000, 0x020000, CRC(a5268c4f) SHA1(99f1f1f2e88f8b2f235070e525aaed9aff6e91c6) )
	ROM_LOAD( "136059-1114.19r", 0x0a0000, 0x010000, CRC(609f619e) SHA1(64b5c2b0f5da07a9dd148aa19bb87e2b2cb1c395) )
	ROM_RELOAD(                  0x0b0000, 0x010000 )
	ROM_LOAD( "136059-1101.8r",  0x0c0000, 0x020000, CRC(2d07fdaa) SHA1(b6772fd764ddc1d2fa1c44c931b269aab9ad5e2b) )
	ROM_LOAD( "136059-1113.2r",  0x0e0000, 0x010000, CRC(0a2aba63) SHA1(e4780c790278034f0332697d5f06e6ed6b57d273) )
	ROM_RELOAD(                  0x0f0000, 0x010000 )

	ROM_REGION( 0x04000, "gfx2", 0 )
	ROM_LOAD( "136059-1123.16n", 0x000000, 0x004000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0x00800, "plds", 0 )
	ROM_LOAD( "pal16l8a-136059-1150.c3",  0x0000, 0x0104, CRC(09d02b00) SHA1(3851f0c0958db983ab907f64ac370a1051c2b76a) )
	ROM_LOAD( "pal16l8a-136059-1151.d17", 0x0200, 0x0104, CRC(797dcde7) SHA1(0c9db6610c40d3bf58117aa9bc8826d33f063dff) )
	ROM_LOAD( "pal16l8a-136059-1152.e17", 0x0400, 0x0104, CRC(56634c58) SHA1(c52db58572d0d8f8eeab6abf891455115b6ed146) )
	ROM_LOAD( "pal16r6a-136059-1153.n7",  0x0600, 0x0104, CRC(61076033) SHA1(c860835a8fa48e141f3d24732395ac35a4b908a4) )
ROM_END


ROM_START( vindictr4 )
	ROM_REGION( 0x60000, "maincpu", 0 ) /* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "136059-1117.d1",  0x000000, 0x010000, CRC(2e5135e4) SHA1(804b3ba201088ac2c35cfcbd530acbd73548ea8c) )
	ROM_LOAD16_BYTE( "136059-1118.d3",  0x000001, 0x010000, CRC(e357fa79) SHA1(220a10287f4bf9d981fd412c8dd0a9c106eaf342) )
	ROM_LOAD16_BYTE( "136059-4119.f1",  0x020000, 0x010000, CRC(44c77ee0) SHA1(f47307126a4960d59d19d1783497971f76ee00a5) )
	ROM_LOAD16_BYTE( "136059-4120.f3",  0x020001, 0x010000, CRC(4deaa77f) SHA1(1c582186d07f39dadf81e90a65928ff1520a60cc) )
	ROM_LOAD16_BYTE( "136059-4121.k1",  0x040000, 0x010000, CRC(9a0444ee) SHA1(211be931a8b6ca42dd140baf3e165ce23f75431f) )
	ROM_LOAD16_BYTE( "136059-4122.k3",  0x040001, 0x010000, CRC(d5022d78) SHA1(eeb6876ee6994f5736114a786c5c4ba97f26ef01) )

	ROM_REGION( 0x10000, "jsa:cpu", 0 ) /* 64k for 6502 code */
	ROM_LOAD( "136059-1124.2k",  0x00000, 0x10000, CRC(d2212c0a) SHA1(df11fe76d74abc0cea23f18264cef4b0f33b1ffd) )

	ROM_REGION( 0x100000, "gfx1", ROMREGION_INVERT )
	ROM_LOAD( "136059-1104.12p", 0x000000, 0x020000, CRC(062f8e52) SHA1(0968b8c822d8fee1cf7ddcf9c3b1bf059e446417) )
	ROM_LOAD( "136059-1116.19p", 0x020000, 0x010000, CRC(0e4366fa) SHA1(1891f6b818f7b0e447e8a83ad0c12aade0b776ee) )
	ROM_RELOAD(                  0x030000, 0x010000 )
	ROM_LOAD( "136059-1103.8p",  0x040000, 0x020000, CRC(09123b57) SHA1(ddd5a4033b5f5ed45f639909364fe5ccd7a0cb53) )
	ROM_LOAD( "136059-1115.2p",  0x060000, 0x010000, CRC(6b757bca) SHA1(2d615b1b42f554bbfebc34928c106c3dd93dc7b2) )
	ROM_RELOAD(                  0x070000, 0x010000 )
	ROM_LOAD( "136059-1102.12r", 0x080000, 0x020000, CRC(a5268c4f) SHA1(99f1f1f2e88f8b2f235070e525aaed9aff6e91c6) )
	ROM_LOAD( "136059-1114.19r", 0x0a0000, 0x010000, CRC(609f619e) SHA1(64b5c2b0f5da07a9dd148aa19bb87e2b2cb1c395) )
	ROM_RELOAD(                  0x0b0000, 0x010000 )
	ROM_LOAD( "136059-1101.8r",  0x0c0000, 0x020000, CRC(2d07fdaa) SHA1(b6772fd764ddc1d2fa1c44c931b269aab9ad5e2b) )
	ROM_LOAD( "136059-1113.2r",  0x0e0000, 0x010000, CRC(0a2aba63) SHA1(e4780c790278034f0332697d5f06e6ed6b57d273) )
	ROM_RELOAD(                  0x0f0000, 0x010000 )

	ROM_REGION( 0x04000, "gfx2", 0 )
	ROM_LOAD( "136059-1123.16n", 0x000000, 0x004000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0x00800, "plds", 0 )
	ROM_LOAD( "pal16l8a-136059-1150.c3",  0x0000, 0x0104, CRC(09d02b00) SHA1(3851f0c0958db983ab907f64ac370a1051c2b76a) )
	ROM_LOAD( "pal16l8a-136059-1151.d17", 0x0200, 0x0104, CRC(797dcde7) SHA1(0c9db6610c40d3bf58117aa9bc8826d33f063dff) )
	ROM_LOAD( "pal16l8a-136059-1152.e17", 0x0400, 0x0104, CRC(56634c58) SHA1(c52db58572d0d8f8eeab6abf891455115b6ed146) )
	ROM_LOAD( "pal16r6a-136059-1153.n7",  0x0600, 0x0104, CRC(61076033) SHA1(c860835a8fa48e141f3d24732395ac35a4b908a4) )
ROM_END


ROM_START( vindictre3 )
	ROM_REGION( 0x60000, "maincpu", 0 ) /* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "136059-3117.d1",  0x000000, 0x010000, CRC(af5ba4a8) SHA1(fdb6e7f0707af94b39368cc39ae45c53209ce32e) )
	ROM_LOAD16_BYTE( "136059-3118.d3",  0x000001, 0x010000, CRC(c87b0581) SHA1(f33c72e83e8c811d3405deb470573327c7b68ea6) )
	ROM_LOAD16_BYTE( "136059-3119.f1",  0x020000, 0x010000, CRC(f0516142) SHA1(16f23a9a8939cead728108fc23fccebf2529d553) )
	ROM_LOAD16_BYTE( "136059-3120.f3",  0x020001, 0x010000, CRC(32a3729f) SHA1(cbddef0c4993e2d8cb6e70890dd5192de2cd56e0) )
	ROM_LOAD16_BYTE( "136059-2121.k1",  0x040000, 0x010000, CRC(9b6111e0) SHA1(427197b21a5db2a06751ab281fde7a2f63818db8) )
	ROM_LOAD16_BYTE( "136059-2122.k3",  0x040001, 0x010000, CRC(8d029a28) SHA1(a166d2a767f70050397f0f12add44ad1f5bc9fde) )

	ROM_REGION( 0x10000, "jsa:cpu", 0 ) /* 64k for 6502 code */
	ROM_LOAD( "136059-1124.2k",  0x00000, 0x10000, CRC(d2212c0a) SHA1(df11fe76d74abc0cea23f18264cef4b0f33b1ffd) )

	ROM_REGION( 0x100000, "gfx1", ROMREGION_INVERT )
	ROM_LOAD( "136059-1104.12p", 0x000000, 0x020000, CRC(062f8e52) SHA1(0968b8c822d8fee1cf7ddcf9c3b1bf059e446417) )
	ROM_LOAD( "136059-1116.19p", 0x020000, 0x010000, CRC(0e4366fa) SHA1(1891f6b818f7b0e447e8a83ad0c12aade0b776ee) )
	ROM_RELOAD(                  0x030000, 0x010000 )
	ROM_LOAD( "136059-1103.8p",  0x040000, 0x020000, CRC(09123b57) SHA1(ddd5a4033b5f5ed45f639909364fe5ccd7a0cb53) )
	ROM_LOAD( "136059-1115.2p",  0x060000, 0x010000, CRC(6b757bca) SHA1(2d615b1b42f554bbfebc34928c106c3dd93dc7b2) )
	ROM_RELOAD(                  0x070000, 0x010000 )
	ROM_LOAD( "136059-1102.12r", 0x080000, 0x020000, CRC(a5268c4f) SHA1(99f1f1f2e88f8b2f235070e525aaed9aff6e91c6) )
	ROM_LOAD( "136059-1114.19r", 0x0a0000, 0x010000, CRC(609f619e) SHA1(64b5c2b0f5da07a9dd148aa19bb87e2b2cb1c395) )
	ROM_RELOAD(                  0x0b0000, 0x010000 )
	ROM_LOAD( "136059-1101.8r",  0x0c0000, 0x020000, CRC(2d07fdaa) SHA1(b6772fd764ddc1d2fa1c44c931b269aab9ad5e2b) )
	ROM_LOAD( "136059-1113.2r",  0x0e0000, 0x010000, CRC(0a2aba63) SHA1(e4780c790278034f0332697d5f06e6ed6b57d273) )
	ROM_RELOAD(                  0x0f0000, 0x010000 )

	ROM_REGION( 0x04000, "gfx2", 0 )
	ROM_LOAD( "136059-1123.16n", 0x000000, 0x004000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0x00800, "plds", 0 )
	ROM_LOAD( "pal16l8a-136059-1150.c3",  0x0000, 0x0104, CRC(09d02b00) SHA1(3851f0c0958db983ab907f64ac370a1051c2b76a) )
	ROM_LOAD( "pal16l8a-136059-1151.d17", 0x0200, 0x0104, CRC(797dcde7) SHA1(0c9db6610c40d3bf58117aa9bc8826d33f063dff) )
	ROM_LOAD( "pal16l8a-136059-1152.e17", 0x0400, 0x0104, CRC(56634c58) SHA1(c52db58572d0d8f8eeab6abf891455115b6ed146) )
	ROM_LOAD( "pal16r6a-136059-1153.n7",  0x0600, 0x0104, CRC(61076033) SHA1(c860835a8fa48e141f3d24732395ac35a4b908a4) )
ROM_END


ROM_START( vindictr2 )
	ROM_REGION( 0x60000, "maincpu", 0 ) /* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "136059-1117.d1",  0x000000, 0x010000, CRC(2e5135e4) SHA1(804b3ba201088ac2c35cfcbd530acbd73548ea8c) )
	ROM_LOAD16_BYTE( "136059-1118.d3",  0x000001, 0x010000, CRC(e357fa79) SHA1(220a10287f4bf9d981fd412c8dd0a9c106eaf342) )
	ROM_LOAD16_BYTE( "136059-2119.f1",  0x020000, 0x010000, CRC(7f8c044e) SHA1(56cd047ff12ff2968bf403b38b86fdceb9c2b83d) )
	ROM_LOAD16_BYTE( "136059-2120.f3",  0x020001, 0x010000, CRC(4260cd3b) SHA1(54fe16202e32ea6cf89da1837ff68b32eaf20dfc) )
	ROM_LOAD16_BYTE( "136059-2121.k1",  0x040000, 0x010000, CRC(9b6111e0) SHA1(427197b21a5db2a06751ab281fde7a2f63818db8) )
	ROM_LOAD16_BYTE( "136059-2122.k3",  0x040001, 0x010000, CRC(8d029a28) SHA1(a166d2a767f70050397f0f12add44ad1f5bc9fde) )

	ROM_REGION( 0x10000, "jsa:cpu", 0 ) /* 64k for 6502 code */
	ROM_LOAD( "136059-1124.2k",  0x00000, 0x10000, CRC(d2212c0a) SHA1(df11fe76d74abc0cea23f18264cef4b0f33b1ffd) )

	ROM_REGION( 0x100000, "gfx1", ROMREGION_INVERT )
	ROM_LOAD( "136059-1104.12p", 0x000000, 0x020000, CRC(062f8e52) SHA1(0968b8c822d8fee1cf7ddcf9c3b1bf059e446417) )
	ROM_LOAD( "136059-1116.19p", 0x020000, 0x010000, CRC(0e4366fa) SHA1(1891f6b818f7b0e447e8a83ad0c12aade0b776ee) )
	ROM_RELOAD(                  0x030000, 0x010000 )
	ROM_LOAD( "136059-1103.8p",  0x040000, 0x020000, CRC(09123b57) SHA1(ddd5a4033b5f5ed45f639909364fe5ccd7a0cb53) )
	ROM_LOAD( "136059-1115.2p",  0x060000, 0x010000, CRC(6b757bca) SHA1(2d615b1b42f554bbfebc34928c106c3dd93dc7b2) )
	ROM_RELOAD(                  0x070000, 0x010000 )
	ROM_LOAD( "136059-1102.12r", 0x080000, 0x020000, CRC(a5268c4f) SHA1(99f1f1f2e88f8b2f235070e525aaed9aff6e91c6) )
	ROM_LOAD( "136059-1114.19r", 0x0a0000, 0x010000, CRC(609f619e) SHA1(64b5c2b0f5da07a9dd148aa19bb87e2b2cb1c395) )
	ROM_RELOAD(                  0x0b0000, 0x010000 )
	ROM_LOAD( "136059-1101.8r",  0x0c0000, 0x020000, CRC(2d07fdaa) SHA1(b6772fd764ddc1d2fa1c44c931b269aab9ad5e2b) )
	ROM_LOAD( "136059-1113.2r",  0x0e0000, 0x010000, CRC(0a2aba63) SHA1(e4780c790278034f0332697d5f06e6ed6b57d273) )
	ROM_RELOAD(                  0x0f0000, 0x010000 )

	ROM_REGION( 0x04000, "gfx2", 0 )
	ROM_LOAD( "136059-1123.16n", 0x000000, 0x004000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0x00800, "plds", 0 )
	ROM_LOAD( "pal16l8a-136059-1150.c3",  0x0000, 0x0104, CRC(09d02b00) SHA1(3851f0c0958db983ab907f64ac370a1051c2b76a) )
	ROM_LOAD( "pal16l8a-136059-1151.d17", 0x0200, 0x0104, CRC(797dcde7) SHA1(0c9db6610c40d3bf58117aa9bc8826d33f063dff) )
	ROM_LOAD( "pal16l8a-136059-1152.e17", 0x0400, 0x0104, CRC(56634c58) SHA1(c52db58572d0d8f8eeab6abf891455115b6ed146) )
	ROM_LOAD( "pal16r6a-136059-1153.n7",  0x0600, 0x0104, CRC(61076033) SHA1(c860835a8fa48e141f3d24732395ac35a4b908a4) )
ROM_END


ROM_START( vindictr1 )
	ROM_REGION( 0x60000, "maincpu", 0 ) /* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "136059-1117.d1",  0x000000, 0x010000, CRC(2e5135e4) SHA1(804b3ba201088ac2c35cfcbd530acbd73548ea8c) )
	ROM_LOAD16_BYTE( "136059-1118.d3",  0x000001, 0x010000, CRC(e357fa79) SHA1(220a10287f4bf9d981fd412c8dd0a9c106eaf342) )
	ROM_LOAD16_BYTE( "136059-1119.f1",  0x020000, 0x010000, CRC(48938c95) SHA1(061771b074135b945621d781fbde7ec1260f31a1) )
	ROM_LOAD16_BYTE( "136059-1120.f3",  0x020001, 0x010000, CRC(ed1de5e3) SHA1(3bf4faba019c63523d3fbd347075a2fdd5353345) )
	ROM_LOAD16_BYTE( "136059-1121.k1",  0x040000, 0x010000, CRC(9b6111e0) SHA1(427197b21a5db2a06751ab281fde7a2f63818db8) )
	ROM_LOAD16_BYTE( "136059-1122.k3",  0x040001, 0x010000, CRC(a94773f1) SHA1(2be841ab755d4ce319f3d562e9990918923384ee) )

	ROM_REGION( 0x10000, "jsa:cpu", 0 ) /* 64k for 6502 code */
	ROM_LOAD( "136059-1124.2k",  0x00000, 0x10000, CRC(d2212c0a) SHA1(df11fe76d74abc0cea23f18264cef4b0f33b1ffd) )

	ROM_REGION( 0x100000, "gfx1", ROMREGION_INVERT )
	ROM_LOAD( "136059-1104.12p", 0x000000, 0x020000, CRC(062f8e52) SHA1(0968b8c822d8fee1cf7ddcf9c3b1bf059e446417) )
	ROM_LOAD( "136059-1116.19p", 0x020000, 0x010000, CRC(0e4366fa) SHA1(1891f6b818f7b0e447e8a83ad0c12aade0b776ee) )
	ROM_RELOAD(                  0x030000, 0x010000 )
	ROM_LOAD( "136059-1103.8p",  0x040000, 0x020000, CRC(09123b57) SHA1(ddd5a4033b5f5ed45f639909364fe5ccd7a0cb53) )
	ROM_LOAD( "136059-1115.2p",  0x060000, 0x010000, CRC(6b757bca) SHA1(2d615b1b42f554bbfebc34928c106c3dd93dc7b2) )
	ROM_RELOAD(                  0x070000, 0x010000 )
	ROM_LOAD( "136059-1102.12r", 0x080000, 0x020000, CRC(a5268c4f) SHA1(99f1f1f2e88f8b2f235070e525aaed9aff6e91c6) )
	ROM_LOAD( "136059-1114.19r", 0x0a0000, 0x010000, CRC(609f619e) SHA1(64b5c2b0f5da07a9dd148aa19bb87e2b2cb1c395) )
	ROM_RELOAD(                  0x0b0000, 0x010000 )
	ROM_LOAD( "136059-1101.8r",  0x0c0000, 0x020000, CRC(2d07fdaa) SHA1(b6772fd764ddc1d2fa1c44c931b269aab9ad5e2b) )
	ROM_LOAD( "136059-1113.2r",  0x0e0000, 0x010000, CRC(0a2aba63) SHA1(e4780c790278034f0332697d5f06e6ed6b57d273) )
	ROM_RELOAD(                  0x0f0000, 0x010000 )

	ROM_REGION( 0x04000, "gfx2", 0 )
	ROM_LOAD( "136059-1123.16n", 0x000000, 0x004000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0x00800, "plds", 0 )
	ROM_LOAD( "pal16l8a-136059-1150.c3",  0x0000, 0x0104, CRC(09d02b00) SHA1(3851f0c0958db983ab907f64ac370a1051c2b76a) )
	ROM_LOAD( "pal16l8a-136059-1151.d17", 0x0200, 0x0104, CRC(797dcde7) SHA1(0c9db6610c40d3bf58117aa9bc8826d33f063dff) )
	ROM_LOAD( "pal16l8a-136059-1152.e17", 0x0400, 0x0104, CRC(56634c58) SHA1(c52db58572d0d8f8eeab6abf891455115b6ed146) )
	ROM_LOAD( "pal16r6a-136059-1153.n7",  0x0600, 0x0104, CRC(61076033) SHA1(c860835a8fa48e141f3d24732395ac35a4b908a4) )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

DRIVER_INIT_MEMBER(vindictr_state,vindictr)
{
}



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1988, vindictr,  0,        vindictr, vindictr, vindictr_state, vindictr, ROT0, "Atari Games", "Vindicators (rev 5)", 0 )
GAME( 1988, vindictre, vindictr, vindictr, vindictr, vindictr_state, vindictr, ROT0, "Atari Games", "Vindicators (Europe, rev 5)", 0 )
GAME( 1988, vindictrg, vindictr, vindictr, vindictr, vindictr_state, vindictr, ROT0, "Atari Games", "Vindicators (German, rev 1)", 0 )
GAME( 1988, vindictre4,vindictr, vindictr, vindictr, vindictr_state, vindictr, ROT0, "Atari Games", "Vindicators (Europe, rev 4)", 0 )
GAME( 1988, vindictr4, vindictr, vindictr, vindictr, vindictr_state, vindictr, ROT0, "Atari Games", "Vindicators (rev 4)", 0 )
GAME( 1988, vindictre3,vindictr, vindictr, vindictr, vindictr_state, vindictr, ROT0, "Atari Games", "Vindicators (Europe, rev 3)", 0 )
GAME( 1988, vindictr2, vindictr, vindictr, vindictr, vindictr_state, vindictr, ROT0, "Atari Games", "Vindicators (rev 2)", 0 )
GAME( 1988, vindictr1, vindictr, vindictr, vindictr, vindictr_state, vindictr, ROT0, "Atari Games", "Vindicators (rev 1)", 0 )

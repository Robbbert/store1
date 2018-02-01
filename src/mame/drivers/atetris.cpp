// license:BSD-3-Clause
// copyright-holders:Zsolt Vasvari, Aaron Giles
/***************************************************************************

    Atari Tetris hardware

    driver by Zsolt Vasvari

    Games supported:
        * Tetris

    Known bugs:
        * the bootlegs don't actually have the slapstic. The additional
          hardware needs to be emulated.

****************************************************************************

    Memory map

****************************************************************************

    ========================================================================
    CPU #1
    ========================================================================
    0000-0FFF   R/W   xxxxxxxx    Program RAM
    1000-1FFF   R/W   xxxxxxxx    Playfield RAM
                      xxxxxxxx       (byte 0: LSB of character code)
                      -----xxx       (byte 1: MSB of character code)
                      xxxx----       (byte 1: palette index)
    2000-20FF   R/W   xxxxxxxx    Palette RAM
                      xxx----        (red component)
                      ---xxx--       (green component)
                      ------xx       (blue component)
    2400-25FF   R/W   xxxxxxxx    EEPROM
    2800-280F   R/W   xxxxxxxx    POKEY #1
    2810-281F   R/W   xxxxxxxx    POKEY #2
    3000          W   --------    Watchdog
    3400          W   --------    EEPROM write enable
    3800          W   --------    IRQ acknowledge
    3C00          W   --xx----    Coin counters
                  W   --x-----       (right coin counter)
                  W   ---x----       (left coin counter)
    4000-7FFF   R     xxxxxxxx    Banked program ROM
    8000-FFFF   R     xxxxxxxx    Program ROM
    ========================================================================
    Interrupts:
        IRQ generated by 32V
    ========================================================================

***************************************************************************/


#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "includes/atetris.h"
#include "sound/sn76496.h"
#include "sound/pokey.h"
#include "machine/nvram.h"
#include "machine/watchdog.h"
#include "speaker.h"


#define MASTER_CLOCK        XTAL(14'318'181)
#define BOOTLEG_CLOCK       XTAL(14'745'600)


/*************************************
 *
 *  Interrupt generation
 *
 *************************************/

TIMER_CALLBACK_MEMBER(atetris_state::interrupt_gen)
{
	int scanline = param;

	/* assert/deassert the interrupt */
	m_maincpu->set_input_line(0, (scanline & 32) ? ASSERT_LINE : CLEAR_LINE);

	/* set the next timer */
	scanline += 32;
	if (scanline >= 256)
		scanline -= 256;
	m_interrupt_timer->adjust(m_screen->time_until_pos(scanline), scanline);
}


WRITE8_MEMBER(atetris_state::irq_ack_w)
{
	m_maincpu->set_input_line(0, CLEAR_LINE);
}



/*************************************
 *
 *  Machine init
 *
 *************************************/

void atetris_state::reset_bank()
{
	memcpy(m_slapstic_base, &m_slapstic_source[m_current_bank * 0x4000], 0x4000);
}


void atetris_state::machine_start()
{
	/* Allocate interrupt timer */
	m_interrupt_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(atetris_state::interrupt_gen),this));

	/* Set up save state */
	save_item(NAME(m_current_bank));
	save_item(NAME(m_nvram_write_enable));
	machine().save().register_postload(save_prepost_delegate(FUNC(atetris_state::reset_bank), this));
}


void atetris_state::machine_reset()
{
	/* reset the slapstic */
	m_slapstic_device->slapstic_reset();
	m_current_bank = m_slapstic_device->slapstic_bank() & 1;
	reset_bank();

	/* start interrupts going (32V clocked by 16V) */
	m_interrupt_timer->adjust(m_screen->time_until_pos(48), 48);
}



/*************************************
 *
 *  Slapstic handler
 *
 *************************************/

READ8_MEMBER(atetris_state::slapstic_r)
{
	int result = m_slapstic_base[0x2000 + offset];
	int new_bank = m_slapstic_device->slapstic_tweak(space, offset) & 1;

	/* update for the new bank */
	if (new_bank != m_current_bank)
	{
		m_current_bank = new_bank;
		memcpy(m_slapstic_base, &m_slapstic_source[m_current_bank * 0x4000], 0x4000);
	}
	return result;
}



/*************************************
 *
 *  Coin counters
 *
 *************************************/

WRITE8_MEMBER(atetris_state::coincount_w)
{
	machine().bookkeeping().coin_counter_w(0, (data >> 5) & 1);
	machine().bookkeeping().coin_counter_w(1, (data >> 4) & 1);
}



/*************************************
 *
 *  NVRAM handlers
 *
 *************************************/

WRITE8_MEMBER(atetris_state::nvram_w)
{
	if (m_nvram_write_enable)
		m_nvram[offset] = data;
	m_nvram_write_enable = 0;
}


WRITE8_MEMBER(atetris_state::nvram_enable_w)
{
	m_nvram_write_enable = 1;
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

/* full address map derived from schematics */
ADDRESS_MAP_START(atetris_state::main_map)
	AM_RANGE(0x0000, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x1fff) AM_RAM_WRITE(videoram_w) AM_SHARE("videoram")
	AM_RANGE(0x2000, 0x20ff) AM_MIRROR(0x0300) AM_RAM_DEVWRITE("palette", palette_device, write8) AM_SHARE("palette")
	AM_RANGE(0x2400, 0x25ff) AM_MIRROR(0x0200) AM_RAM_WRITE(nvram_w) AM_SHARE("nvram")
	AM_RANGE(0x2800, 0x280f) AM_MIRROR(0x03e0) AM_DEVREADWRITE("pokey1", pokey_device, read, write)
	AM_RANGE(0x2810, 0x281f) AM_MIRROR(0x03e0) AM_DEVREADWRITE("pokey2", pokey_device, read, write)
	AM_RANGE(0x3000, 0x3000) AM_MIRROR(0x03ff) AM_DEVWRITE("watchdog", watchdog_timer_device, reset_w)
	AM_RANGE(0x3400, 0x3400) AM_MIRROR(0x03ff) AM_WRITE(nvram_enable_w)
	AM_RANGE(0x3800, 0x3800) AM_MIRROR(0x03ff) AM_WRITE(irq_ack_w)
	AM_RANGE(0x3c00, 0x3c00) AM_MIRROR(0x03ff) AM_WRITE(coincount_w)
	AM_RANGE(0x4000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x7fff) AM_READ(slapstic_r)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END


ADDRESS_MAP_START(atetris_state::atetrisb2_map)
	AM_RANGE(0x0000, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x1fff) AM_RAM_WRITE(videoram_w) AM_SHARE("videoram")
	AM_RANGE(0x2000, 0x20ff) AM_RAM_DEVWRITE("palette", palette_device, write8) AM_SHARE("palette")
	AM_RANGE(0x2400, 0x25ff) AM_RAM_WRITE(nvram_w) AM_SHARE("nvram")
	AM_RANGE(0x2802, 0x2802) AM_DEVWRITE("sn1", sn76496_device, write)
	AM_RANGE(0x2804, 0x2804) AM_DEVWRITE("sn2", sn76496_device, write)
	AM_RANGE(0x2806, 0x2806) AM_DEVWRITE("sn3", sn76496_device, write)
	AM_RANGE(0x2808, 0x2808) AM_READ_PORT("IN0")
	AM_RANGE(0x2818, 0x2818) AM_READ_PORT("IN1")
	AM_RANGE(0x3000, 0x3000) AM_DEVWRITE("watchdog", watchdog_timer_device, reset_w)
	AM_RANGE(0x3400, 0x3400) AM_WRITE(nvram_enable_w)
	AM_RANGE(0x3800, 0x3800) AM_WRITE(irq_ack_w)
	AM_RANGE(0x3c00, 0x3c00) AM_WRITE(coincount_w)
	AM_RANGE(0x4000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x7fff) AM_READ(slapstic_r)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END


ADDRESS_MAP_START(atetris_state::atetrisb3_map)
	AM_RANGE(0x0000, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x1fff) AM_RAM_WRITE(videoram_w) AM_SHARE("videoram")
	AM_RANGE(0x2000, 0x20ff) AM_RAM_DEVWRITE("palette", palette_device, write8) AM_SHARE("palette")
	AM_RANGE(0x2400, 0x25ff) AM_RAM_WRITE(nvram_w) AM_SHARE("nvram")
	//AM_RANGE(0x2802, 0x2802) AM_DEVWRITE("sn1", sn76489_device, write)
	//AM_RANGE(0x2804, 0x2804) AM_DEVWRITE("sn2", sn76489_device, write)
	//AM_RANGE(0x2806, 0x2806) AM_DEVWRITE("sn3", sn76489_device, write)
	AM_RANGE(0x2808, 0x2808) AM_READ_PORT("IN0")
	AM_RANGE(0x2818, 0x2818) AM_READ_PORT("IN1")
	AM_RANGE(0x3000, 0x3000) AM_DEVWRITE("watchdog", watchdog_timer_device, reset_w)
	AM_RANGE(0x3400, 0x3400) AM_WRITE(nvram_enable_w)
	AM_RANGE(0x3800, 0x3800) AM_WRITE(irq_ack_w)
	AM_RANGE(0x3c00, 0x3c00) AM_WRITE(coincount_w)
	AM_RANGE(0x4000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x7fff) AM_READ(slapstic_r)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( atetris )
	// These ports are read via the Pokeys
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_DIPNAME( 0x04, 0x00, "Freeze" )            PORT_DIPLOCATION("50H:!4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Freeze Step" )       PORT_DIPLOCATION("50H:!3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPUNUSED_DIPLOC( 0x10, 0x00, "50H:!2" )   /* Listed As "SPARE2 (Unused)" */
	PORT_DIPUNUSED_DIPLOC( 0x20, 0x00, "50H:!1" )   /* Listed As "SPARE1 (Unused)" */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_CUSTOM ) PORT_VBLANK("screen")
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
INPUT_PORTS_END


// Same as the regular one except they added a Flip Controls switch
static INPUT_PORTS_START( atetrisc )
	PORT_INCLUDE( atetris )

	PORT_MODIFY("IN0")
	PORT_DIPNAME( 0x20, 0x00, "Flip Controls" )     PORT_DIPLOCATION("50H:!1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics layouts
 *
 *************************************/

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4},
	{ 0*4*8, 1*4*8, 2*4*8, 3*4*8, 4*4*8, 5*4*8, 6*4*8, 7*4*8},
	8*8*4
};


static GFXDECODE_START( atetris )
	GFXDECODE_ENTRY( "gfx1", 0, charlayout, 0, 16 )
GFXDECODE_END


/*************************************
 *
 *  Machine driver
 *
 *************************************/

MACHINE_CONFIG_START(atetris_state::atetris)

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6502,MASTER_CLOCK/8)
	MCFG_CPU_PROGRAM_MAP(main_map)

	MCFG_SLAPSTIC_ADD("slapstic", 101)

	MCFG_NVRAM_ADD_1FILL("nvram")

	MCFG_WATCHDOG_ADD("watchdog")

	/* video hardware */
	MCFG_GFXDECODE_ADD("gfxdecode", "palette", atetris)

	MCFG_PALETTE_ADD("palette", 256)
	MCFG_PALETTE_FORMAT(RRRGGGBB)

	MCFG_SCREEN_ADD("screen", RASTER)
	/* note: these parameters are from published specs, not derived */
	/* the board uses an SOS-2 chip to generate video signals */
	MCFG_SCREEN_RAW_PARAMS(MASTER_CLOCK/2, 456, 0, 336, 262, 0, 240)
	MCFG_SCREEN_UPDATE_DRIVER(atetris_state, screen_update)
	MCFG_SCREEN_PALETTE("palette")


	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("pokey1", POKEY, MASTER_CLOCK/8)
	MCFG_POKEY_ALLPOT_R_CB(IOPORT("IN0"))
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_SOUND_ADD("pokey2", POKEY, MASTER_CLOCK/8)
	MCFG_POKEY_ALLPOT_R_CB(IOPORT("IN1"))
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END


MACHINE_CONFIG_START(atetris_state::atetrisb2)

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6502,BOOTLEG_CLOCK/8)
	MCFG_CPU_PROGRAM_MAP(atetrisb2_map)

	MCFG_SLAPSTIC_ADD("slapstic", 101)

	MCFG_NVRAM_ADD_1FILL("nvram")

	MCFG_WATCHDOG_ADD("watchdog")

	/* video hardware */
	MCFG_GFXDECODE_ADD("gfxdecode", "palette", atetris)

	MCFG_PALETTE_ADD("palette", 256)
	MCFG_PALETTE_FORMAT(RRRGGGBB)

	MCFG_SCREEN_ADD("screen", RASTER)
	/* note: these parameters are from published specs, not derived */
	/* the board uses an SOS-2 chip to generate video signals */
	MCFG_SCREEN_RAW_PARAMS(MASTER_CLOCK/2, 456, 0, 336, 262, 0, 240)
	MCFG_SCREEN_UPDATE_DRIVER(atetris_state, screen_update)
	MCFG_SCREEN_PALETTE("palette")


	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("sn1", SN76496, BOOTLEG_CLOCK/8)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_SOUND_ADD("sn2", SN76496, BOOTLEG_CLOCK/8)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_SOUND_ADD("sn3", SN76496, BOOTLEG_CLOCK/8)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END


MACHINE_CONFIG_DERIVED(atetris_state::atetrisb3, atetrisb2)

	MCFG_CPU_REPLACE("maincpu", M6502, MASTER_CLOCK/8)
	MCFG_CPU_PROGRAM_MAP(atetrisb3_map)

	//8749 at 10 MHz instead of slapstic

	MCFG_SOUND_REPLACE("sn1", SN76489, 4000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_SOUND_REPLACE("sn2", SN76489, 4000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_SOUND_REPLACE("sn3", SN76489, 4000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( atetris )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "136066-1100.45f", 0x10000, 0x8000, CRC(2acbdb09) SHA1(5e1189227f26563fd3e5372121ea5c915620f892) )
	ROM_CONTINUE(                0x08000, 0x8000 )

	ROM_REGION( 0x10000, "gfx1", 0 )
	ROM_LOAD( "136066-1101.35a", 0x0000, 0x10000, CRC(84a1939f) SHA1(d8577985fc8ed4e74f74c68b7c00c4855b7c3270) )
ROM_END


ROM_START( atetrisa )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "d1",           0x10000, 0x8000, CRC(2bcab107) SHA1(3cfb8df8cd3782f3ff7f6b32ff15c461352061ee) )
	ROM_CONTINUE(             0x08000, 0x8000 )

	ROM_REGION( 0x10000, "gfx1", 0 )
	ROM_LOAD( "136066-1101.35a",     0x0000, 0x10000, CRC(84a1939f) SHA1(d8577985fc8ed4e74f74c68b7c00c4855b7c3270) )
ROM_END


ROM_START( atetrisb )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "tetris.01",    0x10000, 0x8000, CRC(944d15f6) SHA1(926fa5cb26b6e6a50bea455eec1f6d3fb92aa95c) )
	ROM_CONTINUE(             0x08000, 0x8000 )

	ROM_REGION( 0x10000, "gfx1", 0 )
	ROM_LOAD( "tetris.02",    0x0000, 0x10000, CRC(5c4e7258) SHA1(58060681a728e74d69b2b6f5d02faa597ca6c226) )

	/* there's an extra EEPROM, maybe used for protection crack, which */
	/* however doesn't seem to be required to run the game in this driver. */
	ROM_REGION( 0x0800, "user1", 0 )
	ROM_LOAD( "tetris.03",    0x0000, 0x0800, CRC(26618c0b) SHA1(4d6470bf3a79be3b0766e246abe00582d4c85a97) )
ROM_END


ROM_START( atetrisb2 )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "k1-01",    0x10000, 0x8000, CRC(fa056809) SHA1(e4ccccdf9b04b68127c7b03ae263519cf00f94cb) )
	ROM_CONTINUE(         0x08000, 0x8000 )

	ROM_REGION( 0x10000, "gfx1", 0 )
	ROM_LOAD( "136066-1101.35a", 0x0000, 0x10000, CRC(84a1939f) SHA1(d8577985fc8ed4e74f74c68b7c00c4855b7c3270) )
ROM_END


/*
Tetris (Korean bootleg of atetrisa set)

PCB Layout
----------

RC-1108
|---------------------------------------------------|
|                                        14.31818MHz|
| PAL                                               |
|                                                   |
|     P8749H   6116                                 |
|J                                                  |
|A          10MHz                     27512         |
|M              PAL                                 |
|M                                62256             |
|A                                                  |
|                27512                              |
|                                              PAL  |
|                                      PAL     PAL  |
|76489 76489  4MHz                  82S123          |
|76489              6502                            |
|VOL MB3713    PAL                                  |
|---------------------------------------------------|
*/

ROM_START( atetrisb3 )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "prg.bin",           0x10000, 0x8000, CRC(2bcab107) SHA1(3cfb8df8cd3782f3ff7f6b32ff15c461352061ee) )
	ROM_CONTINUE(             0x08000, 0x8000 )

	ROM_REGION( 0x10000, "gfx1", 0 )
	ROM_LOAD( "gfx.bin",     0x0000, 0x10000, CRC(84a1939f) SHA1(d8577985fc8ed4e74f74c68b7c00c4855b7c3270) )

	// 8749 (10 MHz OSC) instead of the slapstic, needs to be hooked up.
	ROM_REGION( 0x0800, "user1", 0 )
	ROM_LOAD( "8749h.bin",    0x0000, 0x0800, CRC(a66a9c47) SHA1(fbebd755a5e826c7d94ebcafdff2f9a01c9fd1a5) )

	// currently unused
	ROM_REGION( 0x0020, "proms", 0 )
	ROM_LOAD( "82s123.bin", 0x00000, 0x0020, CRC(79656af3) SHA1(bf55f100806520b291157c03999606367dd14ecc) )
ROM_END


ROM_START( atetrisc )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "tetcktl1.rom", 0x10000, 0x8000, CRC(9afd1f4a) SHA1(323d1576d92c905e8e95108b39cabf6fa0c10db6) )
	ROM_CONTINUE(             0x08000, 0x8000 )

	ROM_REGION( 0x10000, "gfx1", 0 )
	ROM_LOAD( "136066-1103.35a", 0x0000, 0x10000, CRC(ec2a7f93) SHA1(cb850141ffd1504f940fa156a39e71a4146d7fea) )
ROM_END


ROM_START( atetrisc2 )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "136066-1102.45f", 0x10000, 0x8000, CRC(1bd28902) SHA1(ae8c34f082bce1f827bf60830f207c46cb282421) )
	ROM_CONTINUE(                0x08000, 0x8000 )

	ROM_REGION( 0x10000, "gfx1", 0 )
	ROM_LOAD( "136066-1103.35a", 0x0000, 0x10000, CRC(ec2a7f93) SHA1(cb850141ffd1504f940fa156a39e71a4146d7fea) )
ROM_END



/*************************************
 *
 *  Driver init
 *
 *************************************/

DRIVER_INIT_MEMBER(atetris_state,atetris)
{
	uint8_t *rgn = memregion("maincpu")->base();

	m_slapstic_device->slapstic_init();
	m_slapstic_source = &rgn[0x10000];
	m_slapstic_base = &rgn[0x04000];
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1988, atetris,  0,       atetris,   atetris,  atetris_state, atetris, ROT0,   "Atari Games", "Tetris (set 1)", MACHINE_SUPPORTS_SAVE )
GAME( 1988, atetrisa, atetris, atetris,   atetris,  atetris_state, atetris, ROT0,   "Atari Games", "Tetris (set 2)", MACHINE_SUPPORTS_SAVE )
GAME( 1988, atetrisb, atetris, atetris,   atetris,  atetris_state, atetris, ROT0,   "bootleg",     "Tetris (bootleg set 1)", MACHINE_SUPPORTS_SAVE )
GAME( 1988, atetrisb2,atetris, atetrisb2, atetris,  atetris_state, atetris, ROT0,   "bootleg",     "Tetris (bootleg set 2)", MACHINE_SUPPORTS_SAVE )
GAME( 1988, atetrisb3,atetris, atetrisb3, atetris,  atetris_state, atetris, ROT0,   "bootleg",     "Tetris (bootleg set 3)", MACHINE_NO_SOUND | MACHINE_SUPPORTS_SAVE )
GAME( 1989, atetrisc, atetris, atetris,   atetrisc, atetris_state, atetris, ROT270, "Atari Games", "Tetris (cocktail set 1)", MACHINE_SUPPORTS_SAVE )
GAME( 1989, atetrisc2,atetris, atetris,   atetrisc, atetris_state, atetris, ROT270, "Atari Games", "Tetris (cocktail set 2)", MACHINE_SUPPORTS_SAVE )

/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

/*
 * Incrementing EDITLEVEL can be used to force invalidation of old bones
 * and save files.
 */
#define EDITLEVEL	0

#define COPYRIGHT_BANNER_A \
"DynaMoHack Copyright 2017 Paweł Sęk and Elijah Stone"

#define COPYRIGHT_BANNER_B \
"DynaHack Copyright 2012-2013 Tung Nguyen"

#define COPYRIGHT_BANNER_C \
"NitroHack Copyright 2011-2012 Daniel Thaler"

#define COPYRIGHT_BANNER_D \
"NetHack Copyright 1985-2003 Stichting Mathematisch Centrum and M. Stephenson"

/*
 * If two or more successive releases have compatible data files, define
 * this with the version number of the oldest such release so that the
 * new release will accept old save and bones files.  The format is
 *	0xMMmmPPeeL
 * 0x = literal prefix "0x", MM = major version, mm = minor version,
 * PP = patch level, ee = edit level, L = literal suffix "L",
 * with all four numbers specified as two hexadecimal digits.
 */
#define VERSION_COMPATIBILITY 0x00000100L	/* 0.0.1-0 */


/*patchlevel.h*/

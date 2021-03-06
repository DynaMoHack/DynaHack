/* Copyright (c) Dean Luick, with acknowledgements to Kevin Darcy */
/* and Dave Cohrs, 1990.					  */
/* DynaMoHack may be freely redistributed.  See license for details. */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "vision.h"
#include "mondata.h"	/* for mindless() */

#ifndef INVISIBLE_OBJECTS
#define vobj_at(x,y) (level->objects[x][y])
#endif


#define dbuf_monid(mon) (what_mon(mon->mnum) + 1)
#define dbuf_objid(obj) (what_obj(obj->otyp) + 1)
#define dbuf_effect(type, id) ((type << 16) | (id + 1))
#define dbuf_explosion(etype, id) ((E_EXPLOSION << 16) | (etype * NUMEXPCHARS + id + 1))


/*
 * sensemon()
 *
 * Returns true if the hero can sense the given monster.  This includes
 * monsters that are hiding or mimicing other monsters.
 */
#define tp_sensemon(mon) (	/* The hero can always sense a monster IF:  */\
    (!mindless(mon->data) &&	/* 1. the monster has a brain to sense AND  */\
     (!which_armor(mon, W_ARMH) || which_armor(mon, W_ARMH)->otyp != TINFOIL_HAT)) && \
      ((Blind && Blind_telepat) ||	/* 2a. hero is blind and telepathic OR	    */\
				/* 2b. hero is using a telepathy inducing   */\
				/*	 object and in range		    */\
				/* FALSE if monster is wearing a tinfoil hat*/\
      (Unblind_telepat &&					      \
	(distu(mon->mx, mon->my) <= (BOLT_LIM * BOLT_LIM))))		      \
)

#define sensemon(mon) (tp_sensemon(mon) || Detect_monsters || match_warn_of_mon(mon))

/*
 * mon_warning() is used to warn of any dangerous monsters in your
 * vicinity, and a glyph representing the warning level is displayed.
 */

#define mon_warning(mon) (Warning && !(mon)->mpeaceful && 				\
			 (distu((mon)->mx, (mon)->my) < 100))

/*
 * mon_visible()
 *
 * Returns true if the hero can see the monster.  It is assumed that the
 * hero can physically see the location of the monster.  The function
 * vobj_at() returns a pointer to an object that the hero can see there.
 * Infravision is not taken into account.
 */
#define mon_visible(mon) (		/* The hero can see the monster     */\
					/* IF the monster		    */\
    (!mon->minvis || See_invisible) &&	/* 1. is not invisible AND	    */\
    (!mon->mundetected) &&		/* 2. not an undetected hider	    */\
    (!(mon->mburied || u.uburied))	/* 3. neither you or it is buried   */\
)

/*
 * see_with_infrared()
 *
 * This function is true if the player can see a monster using infravision.
 * The caller must check for invisibility (invisible monsters are also
 * invisible to infravision), because this is usually called from within
 * canseemon() or canspotmon() which already check that.
 */
#define see_with_infrared(mon) (!Blind && Infravision && infravisible(mon->data) && couldsee(mon->mx, mon->my))


/*
 * canseemon()
 *
 * This is the globally used canseemon().  It is not called within the display
 * routines.  Like mon_visible(), but it checks to see if the hero sees the
 * location instead of assuming it.  (And also considers worms.)
 */
#define canseemon(lev, mon) ((mon->wormno ? worm_known(lev, mon) : \
	    (cansee(mon->mx, mon->my) || see_with_infrared(mon))) \
	&& mon_visible(mon))


/*
 * canspotmon(mon)
 *
 * This function checks whether you can either see a monster or sense it by
 * telepathy, and is what you usually call for monsters about which nothing is
 * known.
 */
#define canspotmon(lev, mon) \
	(canseemon(lev, mon) || sensemon(mon))

/* knowninvisible(mon)
 * This one checks to see if you know a monster is both there and invisible.
 * 1) If you can see the monster and have see invisible, it is assumed the
 * monster is transparent, but visible in some manner.
 * 2) If you can't see the monster, but can see its location and you have
 * telepathy that works when you can see, you can tell that there is a
 * creature in an apparently empty spot.
 * Infravision is not relevant; we assume that invisible monsters are also
 * invisible to infravision.
 */
#define knowninvisible(mon) \
	(mtmp->minvis && \
	    ((cansee(mon->mx, mon->my) && (See_invisible || Detect_monsters)) || \
		(!Blind && (HTelepat & ~INTRINSIC) && \
		    distu(mon->mx, mon->my) <= (BOLT_LIM * BOLT_LIM) \
		) \
	    ) \
	)

/*
 * is_safepet(mon)
 *
 * A special case check used in attack() and domove().	Placing the
 * definition here is convenient.
 */
#define is_safepet(lev, mon) \
	(mon && mon->mtame && canspotmon(lev, mon) && flags.safe_dog \
		&& !Confusion && !Hallucination && !Stunned)


/*
 * canseeself()
 * senseself()
 *
 * This returns true if the hero can see her/himself.
 *
 * The u.uswallow check assumes that you can see yourself even if you are
 * invisible.  If not, then we don't need the check.
 */
#define canseeself()	(Blind || u.uswallow || (!Invisible && !u.uundetected))
#define senseself()	(canseeself() || Unblind_telepat || Detect_monsters)

/*
 * random_monster()
 * random_object()
 * random_trap()
 *
 * Respectively return a random monster, object, or trap number.
 */
#define random_monster() (display_rng(NUMMONS))
#define random_object()  (display_rng(NUM_OBJECTS-1) + 1)
#define random_trap()	 (display_rng(TRAPNUM-1) + 1)

/*
 * what_obj()
 * what_mon()
 * what_trap()
 *
 * If hallucinating, choose a random object/monster, otherwise, use the one
 * given.
 */
#define what_obj(obj)	(Hallucination ? random_object()  : obj)
#define what_mon(mon)	(Hallucination ? random_monster() : mon)
#define what_trap(trp)	(Hallucination ? random_trap()	  : trp)

/*
 * covers_objects()
 * covers_traps()
 *
 * These routines are true if what is really at the given location will
 * "cover" any objects or traps that might be there.
 */
#define covers_objects(lev,xx,yy)						      \
    ((is_pool(lev,xx,yy) && !Underwater) || (lev->locations[xx][yy].typ == LAVAPOOL))

#define covers_traps(lev,xx,yy)	covers_objects(lev,xx,yy)


/*
 * tmp_at() control calls.
 */
#define DISP_BEAM    (-1)  /* Keep all symbols showing & clean up at end. */
#define DISP_FLASH   (-2)  /* Clean up each symbol before displaying new one. */
#define DISP_ALWAYS  (-3)  /* Like flash, but still displayed if not visible. */
#define DISP_OBJECT  (-4)  /* Like flash, but shows an object instead of an effect symbol */
#define DISP_CHANGE  (-5)  /* Change symbol. */
#define DISP_END     (-6)  /* Clean up. */
#define DISP_FREEMEM (-7)  /* Free all memory during exit only. */


/* Total number of cmap indices in the shield_static[] array. */
#define SHIELD_COUNT 21
#define NUM_ZAP 8	/* number of zap beam types */

#endif /* DISPLAY_H */

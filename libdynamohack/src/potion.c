/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaMoHack may be freely redistributed.  See license for details. */

#include "hack.h"

boolean notonhead = FALSE;

static int nothing, unkn;
static const char beverages[] = { POTION_CLASS, 0 };
static const char beverages_and_fountains[] =
	{ ALLOW_NONE, NONE_ON_COMMA, POTION_CLASS, 0 };

static long itimeout(long);
static long itimeout_incr(long,int);
static void ghost_from_bottle(void);
static void alchemy_init(void);
static short mixtype(struct obj *,struct obj *);
static int dip(struct obj *,struct obj *);

/* force `val' to be within valid range for intrinsic timeout value */
static long itimeout(long val)
{
    if (val >= TIMEOUT) val = TIMEOUT;
    else if (val < 1) val = 0;

    return val;
}

/* increment `old' by `incr' and force result to be valid intrinsic timeout */
static long itimeout_incr(long old, int incr)
{
    return itimeout((old & TIMEOUT) + (long)incr);
}

/* set the timeout field of intrinsic `which' */
void set_itimeout(unsigned int *which, long val)
{
    *which &= ~TIMEOUT;
    *which |= itimeout(val);
}

/* increment the timeout field of intrinsic `which' */
void incr_itimeout(unsigned int *which, long incr)
{
    set_itimeout(which, itimeout_incr(*which, incr));
}


void make_confused(long xtime, boolean talk)
{
	long old = HConfusion;

	if (!xtime && old) {
		if (talk)
		    pline("You feel less %s now.",
			Hallucination ? "trippy" : "confused");
	}
	if ((xtime && !old) || (!xtime && old)) iflags.botl = TRUE;

	set_itimeout(&HConfusion, xtime);
}


void make_stunned(long xtime, boolean talk)
{
	long old = HStun;

	if (!xtime && old) {
		if (talk)
		    pline("You feel %s now.",
			Hallucination ? "less wobbly" : "a bit steadier");
	}
	if (xtime && !old) {
		if (talk) {
			if (u.usteed)
				pline("You wobble in the saddle.");
			else
				pline("You %s...", stagger(youmonst.data, "stagger"));
		}
	}
	if ((!xtime && old) || (xtime && !old)) iflags.botl = TRUE;

	set_itimeout(&HStun, xtime);
}


void make_sick(long xtime, const char *cause, boolean talk, int type)
{
	long old = Sick;

	if (xtime > 0L) {
	    if (Sick_resistance) return;
	    if (!old) {
		/* newly sick */
		pline("You feel deathly sick.");
	    } else {
		/* already sick */
		if (talk) pline("You feel %s worse.",
			      xtime <= Sick/2L ? "much" : "even");
	    }
	    set_itimeout(&Sick, xtime);
	    u.usick_type |= type;
	    iflags.botl = TRUE;
	} else if (old && (type & u.usick_type)) {
	    /* was sick, now not */
	    u.usick_type &= ~type;
	    if (u.usick_type) { /* only partly cured */
		if (talk) pline("You feel somewhat better.");
		set_itimeout(&Sick, Sick * 2); /* approximation */
	    } else {
		if (talk) pline("What a relief!");
		Sick = 0L;		/* set_itimeout(&Sick, 0L) */
	    }
	    iflags.botl = TRUE;
	}

	if (Sick) {
	    exercise(A_CON, FALSE);
	    if (cause) {
		strncpy(u.usick_cause, cause, sizeof(u.usick_cause));
		u.usick_cause[sizeof(u.usick_cause)-1] = 0;
		}
	    else
		u.usick_cause[0] = 0;
	} else
	    u.usick_cause[0] = 0;
}


void make_vomiting(long xtime, boolean talk)
{
	long old = Vomiting;

	if (!xtime && old)
	    if (talk) pline("You feel much less nauseated now.");

	set_itimeout(&Vomiting, xtime);
}

static const char vismsg[] = "Your vision seems to %s for a moment but is %s now.";
static const char eyemsg[] = "Your %s momentarily %s.";

void make_blinded(long xtime, boolean talk)
{
	long old = Blinded;
	boolean u_could_see, can_see_now;
	int eyecnt;
	char buf[BUFSZ];

	/* we need to probe ahead in case the Eyes of the Overworld
	   are or will be overriding blindness */
	u_could_see = !Blind;
	Blinded = xtime ? 1L : 0L;
	can_see_now = !Blind;
	Blinded = old;		/* restore */

	if (u.usleep) talk = FALSE;

	if (can_see_now && !u_could_see) {	/* regaining sight */
	    if (talk) {
		if (Hallucination)
		    pline("Far out!  Everything is all cosmic again!");
		else
		    pline("You can see again.");
	    }
	} else if (old && !xtime) {
	    /* clearing temporary blindness without toggling blindness */
	    if (talk) {
		if (!haseyes(youmonst.data)) {
		    strange_feeling(NULL, NULL);
		} else if (Blindfolded) {
		    strcpy(buf, body_part(EYE));
		    eyecnt = eyecount(youmonst.data);
		    pline(eyemsg, (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "itches" : "itch");
		} else {	/* Eyes of the Overworld */
		    pline(vismsg, "brighten",
			 Hallucination ? "sadder" : "normal");
		}
	    }
	}

	if (u_could_see && !can_see_now) {	/* losing sight */
	    if (talk) {
		if (Hallucination)
		    pline("Oh, bummer!  Everything is dark!  Help!");
		else
		    pline("A cloud of darkness falls upon you.");
	    }
	    /* Before the hero goes blind, set the ball&chain variables. */
	    if (Punished) set_bc(0);
	} else if (!old && xtime) {
	    /* setting temporary blindness without toggling blindness */
	    if (talk) {
		if (!haseyes(youmonst.data)) {
		    strange_feeling(NULL, NULL);
		} else if (Blindfolded) {
		    strcpy(buf, body_part(EYE));
		    eyecnt = eyecount(youmonst.data);
		    pline(eyemsg, (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "twitches" : "twitch");
		} else {	/* Eyes of the Overworld */
		    pline(vismsg, "dim",
			 Hallucination ? "happier" : "normal");
		}
	    }
	}

	set_itimeout(&Blinded, xtime);

	if (u_could_see ^ can_see_now) {  /* one or the other but not both */
	    iflags.botl = 1;
	    vision_full_recalc = 1;	/* blindness just got toggled */
	    if (Blind_telepat || Infravision) see_monsters();
	}
}


boolean make_hallucinated(long xtime, /* nonzero if this is an attempt to turn on hallucination */
			  boolean talk,
			  long mask)  /* nonzero if resistance status should change by mask */
{
	long old = HHallucination;
	boolean changed = 0;
	const char *message, *verb;

	message = (!xtime) ? "Everything %s SO boring now." :
			     "Oh wow!  Everything %s so cosmic!";
	verb = (!Blind) ? "looks" : "feels";

	if (mask) {
	    if (HHallucination) changed = TRUE;

	    if (!xtime) EHalluc_resistance |= mask;
	    else EHalluc_resistance &= ~mask;
	} else {
	    if (!EHalluc_resistance && (!!HHallucination != !!xtime))
		changed = TRUE;
	    set_itimeout(&HHallucination, xtime);

	    /* clearing temporary hallucination without toggling vision */
	    if (!changed && !HHallucination && old && talk) {
		if (!haseyes(youmonst.data)) {
		    strange_feeling(NULL, NULL);
		} else if (Blind) {
		    char buf[BUFSZ];
		    int eyecnt = eyecount(youmonst.data);

		    strcpy(buf, body_part(EYE));
		    pline(eyemsg, (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "itches" : "itch");
		} else {	/* Grayswandir */
		    pline(vismsg, "flatten", "normal");
		}
	    }
	}

	if (changed) {
	    if (u.uswallow) {
		swallowed(0);	/* redraw swallow display */
	    } else {
		/* The see_* routines should be called *before* the pline. */
		see_monsters();
		see_objects();
		see_traps();
	    }

	    /* for perm_inv and anything similar
	    (eg. Qt windowport's equipped items display) */
	    update_inventory();

	    iflags.botl = 1;
	    if (talk) pline(message, verb);
	}
	return changed;
}


static void ghost_from_bottle(void)
{
	struct monst *mtmp = makemon(&mons[PM_GHOST], level, u.ux, u.uy, NO_MM_FLAGS);

	if (!mtmp) {
		pline("This bottle turns out to be empty.");
		return;
	}
	if (Blind) {
		pline("As you open the bottle, something emerges.");
		return;
	}
	pline("As you open the bottle, an enormous %s emerges!",
		Hallucination ? rndmonnam() : "ghost");
	if (flags.verbose)
	    pline("You are frightened to death, and unable to move.");
	nomul(-3, "being frightened to death");
	nomovemsg = "You regain your composure.";
}


/* "Quaffing is like drinking, except you spill more."  -- Terry Pratchett
 */
int dodrink(struct obj *potion)
{
	const char *potion_descr;
	void (*terrain)(void) = NULL;
	const char *drink_choices = NULL;

	if (Strangled) {
		pline("If you can't breathe air, how can you drink liquid?");
		return 0;
	}
	/* Is there a fountain to drink from here? */
	if (IS_FOUNTAIN(level->locations[u.ux][u.uy].typ) && !Levitation) {
		terrain = drinkfountain;
	}
	/* Or a kitchen sink? */
	if (IS_SINK(level->locations[u.ux][u.uy].typ)) {
		terrain = drinksink;
	}

	/* Or are you surrounded by water? */
	if (Underwater) {
		if (yn("Drink the water around you?") == 'y') {
			/* Indiana Jones 3 */
			if (Role_if(PM_ARCHEOLOGIST))
			    pline("No thank you, fish make love in it!");
			else
			    pline("Do you know what lives in this water!?");
			return 1;
		}
	}

	drink_choices = terrain ? beverages_and_fountains : beverages;
	if (potion && !validate_object(potion, drink_choices, "drink"))
		return 0;
	else if (!potion)
		potion = getobj(drink_choices, "drink", NULL);
	if (!potion) return 0;

	/* Quaff from fountains and sinks with "q," */
	if (potion == &zeroobj) {
	    if (!terrain) {
		warning("dodrink: zeroobj selected without terrain");
		return 0;
	    }
	    terrain();
	    return 1;
	}

	potion->in_use = TRUE;		/* you've opened the stopper */

#define POTION_OCCUPANT_CHANCE(n) (13 + 2*(n))	/* also in muse.c */

	potion_descr = OBJ_DESCR(objects[potion->otyp]);
	if (potion_descr) {
	    if (!strcmp(potion_descr, "milky") &&
		    flags.ghost_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.ghost_count))) {
		ghost_from_bottle();
		useup(potion);
		return 1;
	    } else if (!strcmp(potion_descr, "smoky") &&
		    flags.djinni_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.djinni_count))) {
		djinni_from_bottle(potion);
		useup(potion);
		return 1;
	    }
	}
	return dopotion(potion);
}


int dopotion(struct obj *otmp)
{
	int retval;

	if (otmp == uwep) {
	    /* Unwield the potion to avoid a crash if its effect
	     * causes the player to drop it. We don't print a message
	     * here; setuwep doesn't either. */
	    setuwep(0);
	}

	otmp->in_use = TRUE;
	nothing = unkn = 0;
	if ((retval = peffects(otmp)) >= 0) return retval;

	if (nothing) {
	    unkn++;
	    pline("You have a %s feeling for a moment, then it passes.",
		  Hallucination ? "normal" : "peculiar");
	}
	if (otmp->dknown && !objects[otmp->otyp].oc_name_known) {
		if (!unkn) {
			makeknown(otmp->otyp);
			more_experienced(0, 0, 10);
		} else if (!objects[otmp->otyp].oc_uname)
			docall(otmp);
	}
	useup(otmp);
	return 1;
}


int peffects(struct obj *otmp)
{
	int i, ii, lim;

	switch(otmp->otyp){
	case POT_RESTORE_ABILITY:
	case SPE_RESTORE_ABILITY:
		unkn++;
		if (otmp->cursed) {
		    pline("Ulch!  This makes you feel mediocre!");
		    break;
		} else {
		    pline("Wow!  This makes you feel %s!",
			  (otmp->blessed) ?
				(unfixable_trouble_count(FALSE) ? "better" : "great")
			  : "good");
		    i = rn2(A_MAX);		/* start at a random point */
		    for (ii = 0; ii < A_MAX; ii++) {
			lim = AMAX(i);
			if (i == A_STR && u.uhs >= 3) --lim;	/* WEAK */
			if (ABASE(i) < lim) {
			    ABASE(i) = lim;
			    iflags.botl = 1;
			    /* only first found if not blessed */
			    if (!otmp->blessed) break;
			}
			if (++i >= A_MAX) i = 0;
		    }
		}
		break;
	case POT_HALLUCINATION:
		if (Hallucination || Halluc_resistance) nothing++;
		make_hallucinated(itimeout_incr(HHallucination,
					   rn1(200, 600 - 300 * bcsign(otmp))),
				  TRUE, 0L);
		break;
	case POT_WATER:
		if (!otmp->blessed && !otmp->cursed) {
		    pline("This tastes like water.");
		    u.uhunger += rnd(10);
		    newuhs(FALSE);
		    break;
		}
		unkn++;
		if (is_undead(youmonst.data) || is_demon(youmonst.data) ||
				u.ualign.type == A_CHAOTIC) {
		    if (otmp->blessed) {
			pline("This burns like acid!");
			exercise(A_CON, FALSE);
			if (u.ulycn >= LOW_PM) {
			    pline("Your affinity to %s disappears!",
				 makeplural(mons_mname(&mons[u.ulycn])));
			    if (youmonst.data == &mons[u.ulycn])
				you_unwere(FALSE);
			    u.ulycn = NON_PM;	/* cure lycanthropy */
			}
			losehp(dice(2,6), "potion of holy water", KILLED_BY_AN);
		    } else if (otmp->cursed) {
			pline("You feel quite proud of yourself.");
			healup(dice(2,6),0,0,0);
			if (u.ulycn >= LOW_PM && !Upolyd) you_were();
			exercise(A_CON, TRUE);
		    }
		} else {
		    if (otmp->blessed) {
			pline("You feel full of awe.");
			make_sick(0L, NULL, TRUE, SICK_ALL);
			exercise(A_WIS, TRUE);
			exercise(A_CON, TRUE);
			if (u.ulycn >= LOW_PM)
			    you_unwere(TRUE);	/* "Purified" */
			/* make_confused(0L,TRUE); */
		    } else {
			if (u.ualign.type == A_LAWFUL) {
			    pline("This burns like acid!");
			    losehp(dice(2,6), "potion of unholy water",
				KILLED_BY_AN);
			} else
			    pline("You feel full of dread.");
			if (u.ulycn >= LOW_PM && !Upolyd) you_were();
			exercise(A_CON, FALSE);
		    }
		}
		break;
	case POT_BOOZE:
		unkn++;
		pline("Ooph!  This tastes like %s%s!",
		      otmp->odiluted ? "watered down " : "",
		      Hallucination ? "dandelion wine" : "liquid fire");
		if (!otmp->blessed)
		    make_confused(itimeout_incr(HConfusion, dice(3,8)), FALSE);
		/* the whiskey makes us feel better */
		if (!otmp->odiluted) healup(1, 0, FALSE, FALSE);
		u.uhunger += 10 * (2 + bcsign(otmp));
		newuhs(FALSE);
		exercise(A_WIS, FALSE);
		if (otmp->cursed) {
			pline("You pass out.");
			multi = -rnd(15);
			nomovemsg = "You awake with a headache.";
		}
		break;
	case POT_ENLIGHTENMENT:
		if (otmp->cursed) {
			unkn++;
			pline("You have an uneasy feeling...");
			exercise(A_WIS, FALSE);
		} else {
			if (otmp->blessed) {
				adjattrib(A_INT, 1, FALSE, 0);
				adjattrib(A_WIS, 1, FALSE, 0);
			}
			pline("You feel self-knowledgeable...");
			if (Upolyd) {
			    discover_randomized_dragon(youmonst.data,
				    "You discover that you are %s.");
			}
			win_pause_output(P_MESSAGE);
			enlightenment(0);
			pline("The feeling subsides.");
			exercise(A_WIS, TRUE);
		}
		break;
	case SPE_INVISIBILITY:
		/* spell cannot penetrate mummy wrapping */
		if (BInvis && uarmc->otyp == MUMMY_WRAPPING) {
			pline("You feel rather itchy under your %s.", xname(uarmc));
			break;
		}
		/* FALLTHRU */
	case POT_INVISIBILITY:
		if (Invis || Blind || BInvis) {
		    nothing++;
		} else {
		    self_invis_message();
		}
		if (otmp->blessed) HInvis |= FROMOUTSIDE;
		else incr_itimeout(&HInvis, rn1(15,31));
		newsym(u.ux,u.uy);	/* update position */
		if (otmp->cursed) {
		    pline("For some reason, you feel your presence is known.");
		    aggravate();
		}
		break;
	case POT_SEE_INVISIBLE:
		/* tastes like fruit juice in Rogue */
	case POT_FRUIT_JUICE:
	    {
		int msg = Invisible && !Blind;

		unkn++;
		if (otmp->cursed)
		    pline("Yecch!  This tastes %s.",
			  Hallucination ? "overripe" : "rotten");
		else
		    pline(Hallucination ?
		      "This tastes like 10%% real %s%s all-natural beverage." :
				"This tastes like %s%s.",
			  otmp->odiluted ? "reconstituted " : "",
			  fruitname(TRUE));
		if (otmp->otyp == POT_FRUIT_JUICE) {
		    u.uhunger += (otmp->odiluted ? 5 : 10) * (2 + bcsign(otmp));
		    newuhs(FALSE);
		    break;
		}
		if (!otmp->cursed) {
			/* Tell them they can see again immediately, which
			 * will help them identify the potion...
			 */
			make_blinded(0L,TRUE);
		}
		if (otmp->blessed)
			HSee_invisible |= FROMOUTSIDE;
		else
			incr_itimeout(&HSee_invisible, rn1(100,750));
		set_mimic_blocking(); /* do special mimic handling */
		see_monsters();	/* see invisible monsters */
		newsym(u.ux,u.uy); /* see yourself! */
		if (msg && !Blind) { /* Blind possible if polymorphed */
		    pline("You can see through yourself, but you are visible!");
		    unkn--;
		}
		break;
	    }
	case POT_PARALYSIS:
		if (Free_action)
		    pline("You stiffen momentarily.");
		else {
		    if (Levitation || Is_airlevel(&u.uz)||Is_waterlevel(&u.uz))
			pline("You are motionlessly suspended.");
		    else if (u.usteed)
			pline("You are frozen in place!");
		    else
			pline("Your %s are frozen to the %s!",
			     makeplural(body_part(FOOT)), surface(u.ux, u.uy));
		    nomul(-(rn1(10, 25 - 12*bcsign(otmp))), "frozen by a potion");
		    nomovemsg = "You can move again.";
		    exercise(A_DEX, FALSE);
		}
		break;
	case POT_SLEEPING:
		if (FSleep_resistance) {
		    pline("You yawn.");
		} else {
		    int duration = rn1(10, 25 - 12 * bcsign(otmp));
		    if (PSleep_resistance)
			duration = (duration + 1) / 2;
		    pline("You suddenly fall asleep!");
		    fall_asleep(-duration, TRUE);
		}
		break;
	case POT_MONSTER_DETECTION:
	case SPE_DETECT_MONSTERS:
		if (otmp->blessed) {
		    int x, y;

		    if (Detect_monsters) nothing++;
		    unkn++;
		    /* after a while, repeated uses become less effective */
		    if (HDetect_monsters >= 300L)
			i = 1;
		    else
			i = rn1(40,21);
		    incr_itimeout(&HDetect_monsters, i);
		    for (x = 1; x < COLNO; x++) {
			for (y = 0; y < ROWNO; y++) {
			    if (level->locations[x][y].mem_invis) {
				unmap_object(x, y);
				newsym(x,y);
			    }
			    if (MON_AT(level, x,y)) unkn = 0;
			}
		    }
		    see_monsters();
		    if (unkn) pline("You feel lonely.");
		    break;
		}
		if (monster_detect(otmp, 0))
			return 1;		/* nothing detected */
		exercise(A_WIS, TRUE);
		break;
	case POT_OBJECT_DETECTION:
	case SPE_DETECT_TREASURE:
		if (object_detect(otmp, 0, FALSE))
			return 1;		/* nothing detected */
		exercise(A_WIS, TRUE);
		break;
	case POT_SICKNESS:
		pline("Yecch!  This stuff tastes like poison.");
		if (otmp->blessed) {
		    pline("(But in fact it was mildly stale %s.)",
			  fruitname(TRUE));
		    if (!Role_if (PM_HEALER)) {
			/* NB: blessed otmp->fromsink is not possible */
			losehp(1, "mildly contaminated potion", KILLED_BY_AN);
		    }
		} else {
		    if (FPoison_resistance)
			pline(
			  "(But in fact it was biologically contaminated %s.)",
			      fruitname(TRUE));
		    if (Role_if (PM_HEALER))
			pline("Fortunately, you have been immunized.");
		    else {
			int typ = rn2(A_MAX);

			if (!Fixed_abil) {
			    poisontell(typ);
			    adjattrib(typ,
				      Poison_resistance ? -1 : -rn1(4,3),
				      FALSE,
				      1);
			}
			if (!FPoison_resistance) {
			    int contam_dmg = rnd(10) + 5 * !!otmp->cursed;
			    if (PPoison_resistance)
				contam_dmg = (contam_dmg + 1) / 2;
			    losehp(contam_dmg,
				   otmp->fromsink ? "contaminated tap water" :
						    "contaminated potion",
				   KILLED_BY);
			}
			exercise(A_CON, FALSE);
		    }
		}
		if (Hallucination) {
			pline("You are shocked back to your senses!");
			make_hallucinated(0L,FALSE,0L);
		}
		break;
	case POT_CONFUSION:
		if (!Confusion)
		    if (Hallucination) {
			pline("What a trippy feeling!");
			unkn++;
		    } else
			pline("Huh, What?  Where am I?");
		else	nothing++;
		make_confused(itimeout_incr(HConfusion,
					    rn1(7, 16 - 8 * bcsign(otmp))),
			      FALSE);
		break;
	case POT_GAIN_ABILITY:
		if (otmp->cursed) {
		    pline("Ulch!  That potion tasted foul!");
		    unkn++;
		} else if (Fixed_abil) {
		    nothing++;
		} else {      /* If blessed, try very hard to find an ability */
		              /* that can be increased; if not, try up to */
		    int itmp; /* 3 times to find one which can be increased. */
		    i = -1;		/* increment to 0 */
		    for (ii = (otmp->blessed ? 1000 : A_MAX / 2); ii > 0; ii--) {
			i = rn2(A_MAX);
			/* only give "your X is already as high as it can get"
			   message on last attempt */
			itmp = (ii == 1) ? 0 : -1;
			if (adjattrib(i, 1, FALSE, itmp))
			    break;
		    }
		}
		break;
	case POT_SPEED:
		if (Wounded_legs && !otmp->cursed
		   && !u.usteed	/* heal_legs() would heal steeds legs */) {
			heal_legs();
			unkn++;
			break;
		} /* and fall through */
	case SPE_HASTE_SELF:
		if (!Very_fast) /* wwf@doe.carleton.ca */
			pline("You are suddenly moving %sfaster.",
				Fast ? "" : "much ");
		else {
			pline("Your %s get new energy.",
				makeplural(body_part(LEG)));
			unkn++;
		}
		exercise(A_DEX, TRUE);
		incr_itimeout(&HFast, rn1(10, 100 + 60 * bcsign(otmp)));
		break;
	case POT_BLINDNESS:
		if (Blind) nothing++;
		make_blinded(itimeout_incr(Blinded,
					   rn1(200, 250 - 125 * bcsign(otmp))),
			     (boolean)!Blind);
		break;
	case POT_GAIN_LEVEL:
		if (otmp->cursed) {
			unkn++;
			/* they went up a level */
			if ((ledger_no(&u.uz) == 1 && u.uhave.amulet) ||
				Can_rise_up(level, u.ux, u.uy)) {
			    const char *riseup ="You rise up, through the %s!";
			    if (ledger_no(&u.uz) == 1) {
				s_level *first_plane = get_first_elemental_plane();
				pline(riseup, ceiling(u.ux,u.uy));
				goto_level(first_plane ? &first_plane->dlevel : NULL,
					   FALSE, FALSE, FALSE);
			    } else {
			        int newlev = depth(&u.uz)-1;
				d_level newlevel;

				get_level(&newlevel, newlev);
				if (on_level(&newlevel, &u.uz)) {
				    pline("It tasted bad.");
				    break;
				} else pline(riseup, ceiling(u.ux,u.uy));
				goto_level(&newlevel, FALSE, FALSE, FALSE);
			    }
			}
			else pline("You have an uneasy feeling.");
			break;
		}
		pluslvl(FALSE);
		if (otmp->blessed)
			/* blessed potions place you at a random spot in the
			 * middle of the new level instead of the low point
			 */
			u.uexp = rndexp(TRUE);
		break;
	case POT_HEALING:
		pline("You feel better.");
		healup(dice(6 + 2 * bcsign(otmp), 4),
		       !otmp->cursed ? 1 : 0, !!otmp->blessed, !otmp->cursed);
		exercise(A_CON, TRUE);
		break;
	case POT_EXTRA_HEALING:
		pline("You feel much better.");
		healup(dice(6 + 2 * bcsign(otmp), 8),
		       otmp->blessed ? 5 : !otmp->cursed ? 2 : 0,
		       !otmp->cursed, TRUE);
		make_hallucinated(0L,TRUE,0L);
		exercise(A_CON, TRUE);
		exercise(A_STR, TRUE);
		break;
	case POT_FULL_HEALING:
		pline("You feel completely healed.");
		healup(400, 4+4*bcsign(otmp), !otmp->cursed, TRUE);
		/* Restore one lost level if blessed */
		if (otmp->blessed && u.ulevel < u.ulevelmax) {
		    /* when multiple levels have been lost, drinking
		       multiple potions will only get half of them back */
		    u.ulevelmax -= 1;
		    pluslvl(FALSE);
		}
		make_hallucinated(0L,TRUE,0L);
		exercise(A_STR, TRUE);
		exercise(A_CON, TRUE);
		break;
	case POT_LEVITATION:
	case SPE_LEVITATION:
		if (otmp->cursed) HLevitation &= ~I_SPECIAL;
		if (!Levitation) {
			/* kludge to ensure proper operation of float_up() */
			HLevitation = 1;
			float_up();
			/* reverse kludge */
			HLevitation = 0;
			if (otmp->cursed && !Is_waterlevel(&u.uz)) {
	if ((u.ux != level->upstair.sx || u.uy != level->upstair.sy)
	   && (u.ux != level->sstairs.sx || u.uy != level->sstairs.sy || !level->sstairs.up)
	   && (!level->upladder.sx || u.ux != level->upladder.sx || u.uy != level->upladder.sy)
	) {
					pline("You hit your %s on the %s.",
						body_part(HEAD),
						ceiling(u.ux,u.uy));
					losehp(uarmh ? 1 : rnd(10),
						"colliding with the ceiling",
						KILLED_BY);
				} else doup();
			}
		} else
			nothing++;
		if (otmp->blessed) {
		    incr_itimeout(&HLevitation, rn1(50,250));
		    HLevitation |= I_SPECIAL;
		} else incr_itimeout(&HLevitation, rn1(140,10));
		spoteffects(FALSE);	/* for sinks */
		break;
	case POT_GAIN_ENERGY:			/* M. Stephenson */
		{	int num;
			if (otmp->cursed)
			    pline("You feel lackluster.");
			else
			    pline("Magical energies course through your body.");
			num = rnd(5) + 5 * otmp->blessed + 1;
			u.uenmax += (otmp->cursed) ? -num : num;
			u.uen += (otmp->cursed) ? -num : num;
			if (u.uenmax <= 0) u.uenmax = 0;
			if (u.uen <= 0) u.uen = 0;
			iflags.botl = 1;
			exercise(A_WIS, TRUE);
		}
		break;
	case POT_OIL:				/* P. Winner */
		{
			boolean good_for_you = FALSE;

			if (otmp->lamplit) {
			    if (likes_fire(youmonst.data)) {
				pline("Ahh, a refreshing drink.");
				good_for_you = TRUE;
			    } else {
				pline("You burn your %s.", body_part(FACE));
				losehp(dice(FFire_resistance ? 1 : 3,
					    PFire_resistance ? 2 : 4),
				       "burning potion of oil", KILLED_BY_AN);
			    }
			} else if (otmp->cursed)
			    pline("This tastes like castor oil.");
			else
			    pline("That was smooth!");
			exercise(A_WIS, good_for_you);
		}
		break;
	case POT_ACID:
		if (Acid_resistance)
			/* Not necessarily a creature who _likes_ acid */
			pline("This tastes %s.", Hallucination ? "tangy" : "sour");
		else {
			pline("This burns%s!", otmp->blessed ? " a little" :
					otmp->cursed ? " a lot" : " like acid");
			losehp(dice(otmp->cursed ? 2 : 1, otmp->blessed ? 4 : 8),
					"potion of acid", KILLED_BY_AN);
			exercise(A_CON, FALSE);
		}
		if (Stoned) fix_petrification();
		unkn++; /* holy/unholy water can burn like acid too */
		break;
	case POT_POLYMORPH:
		pline("You feel a little %s.", Hallucination ? "normal" : "strange");
		if (!Unchanging) polyself(FALSE);
		break;
	case POT_BLOOD:
	case POT_VAMPIRE_BLOOD:
		unkn++;
		u.uconduct.unvegan++;
		if (maybe_polyd(is_vampire(youmonst.data), Race_if(PM_VAMPIRE))) {
		    violated(CONDUCT_VEGETARIAN);
		    if (otmp->cursed) {
			pline("Yecch!  This %s.", Hallucination ?
				"liquid could do with a good stir" :
				"blood has congealed");
		    } else {
			pline(Hallucination ?
			      "The %s liquid stirs memories of home." :
			      "The %s blood tastes delicious.",
			      otmp->odiluted ? "watery" : "thick");
		    }
		    if (!otmp->cursed) {
			lesshungry((otmp->odiluted ? 1 : 2) *
				   (otmp->otyp == POT_VAMPIRE_BLOOD ? 400 :
				    otmp->blessed ? 15 : 10));
		    }
		    if (otmp->otyp == POT_VAMPIRE_BLOOD && otmp->blessed) {
			int num = newhp();
			if (Upolyd) {
			    u.mhmax += num;
			    u.mh += num;
			} else {
			    u.uhpmax += num;
			    u.uhp += num;
			}
		    }
		} else if (otmp->otyp == POT_VAMPIRE_BLOOD) {
		    /* [CWC] fix conducts for potions of (vampire) blood -
		       doesn't use violated(CONDUCT_VEGETARIAN) to prevent
		       duplicated "you feel guilty" messages */
		    u.uconduct.unvegetarian++;
		    if (u.ualign.type == A_LAWFUL || Role_if(PM_MONK)) {
			pline("You feel %sguilty about drinking such a vile liquid.",
			      Role_if(PM_MONK) ? "especially " : "");
			u.ugangr++;
			adjalign(-15);
		    } else if (u.ualign.type == A_NEUTRAL) {
			adjalign(-3);
		    }
		    exercise(A_CON, FALSE);
		    if (!Unchanging) {
			int successful_polymorph = 0;
			if (otmp->blessed)
			    successful_polymorph = polymon(PM_VAMPIRE_LORD);
			else if (otmp->cursed)
			    successful_polymorph = polymon(PM_VAMPIRE_BAT);
			else
			    successful_polymorph = polymon(PM_VAMPIRE);
			if (successful_polymorph)
			    u.mtimedone = 0;	/* "permament" change */
		    }
		} else {
		    violated(CONDUCT_VEGETARIAN);
		    pline("Ugh.  That was vile.");
		    make_vomiting(Vomiting + dice(10,8), TRUE);
		}
		break;
	default:
		impossible("What a funny potion! (%u)", otmp->otyp);
		return 0;
	}
	return -1;
}

void healup(int nhp, int nxtra, boolean curesick, boolean cureblind)
{
	if (nhp) {
		if (Upolyd) {
			u.mh += nhp;
			if (u.mh > u.mhmax) u.mh = (u.mhmax += nxtra);
		} else {
			u.uhp += nhp;
			if (u.uhp > u.uhpmax) u.uhp = (u.uhpmax += nxtra);
		}
	}
	if (cureblind)	make_blinded(0L,TRUE);
	if (curesick)	make_sick(0L, NULL, TRUE, SICK_ALL);
	iflags.botl = 1;
	return;
}

void strange_feeling(struct obj *obj, const char *txt)
{
	if (flags.beginner || !txt)
		pline("You have a %s feeling for a moment, then it passes.",
		Hallucination ? "normal" : "strange");
	else
		pline(txt);

	if (!obj)	/* e.g., crystal ball finds no traps */
		return;

	if (obj->dknown && !objects[obj->otyp].oc_name_known &&
						!objects[obj->otyp].oc_uname)
		docall(obj);
	useup(obj);
}

const char *const bottlenames[] = {
	"bottle", "phial", "flagon", "carafe", "flask", "jar", "vial"
};


const char *bottlename(void)
{
	return bottlenames[rn2(SIZE(bottlenames))];
}


void potionhit(struct monst *mon, struct obj *obj, boolean your_fault)
{
	const char *botlnam = bottlename();
	boolean isyou = (mon == &youmonst);
	int distance;
	boolean disint = (touch_disintegrates(mon->data) &&
			  !oresist_disintegration(obj) &&
			  !mon->mcan &&
			  mon->mhp > 6);

	if (isyou) {
		distance = 0;
		pline("The %s crashes on your %s and breaks into shards.",
			botlnam, body_part(HEAD));
		losehp(rnd(2), "thrown potion", KILLED_BY_AN);
	} else {
		distance = distu(mon->mx,mon->my);
		if (!cansee(mon->mx,mon->my)) {
		    pline(disint ? "Vip!" : "Crash!");
		} else {
		    char *mnam = mon_nam(mon);
		    char buf[BUFSZ];

		    if (has_head(mon->data)) {
			sprintf(buf, "%s %s",
				s_suffix(mnam),
				(notonhead ? "body" : "head"));
		    } else {
			strcpy(buf, mnam);
		    }
		    pline("The %s crashes on %s and %s.",
			  botlnam, buf,
			  disint ? "disintegrates" : "breaks into shards");
		}
		if (rn2(5) && mon->mhp > 1)
			mon->mhp--;
	}

	/* oil doesn't instantly evaporate */
	if (obj->otyp != POT_OIL && cansee(mon->mx,mon->my) && !disint)
		pline("%s.", Tobjnam(obj, "evaporate"));

    if (isyou) {
	switch (obj->otyp) {
	case POT_OIL:
		if (obj->lamplit)
		    splatter_burning_oil(u.ux, u.uy);
		break;
	case POT_POLYMORPH:
		pline("You feel a little %s.", Hallucination ? "normal" : "strange");
		if (!Unchanging && !Antimagic) polyself(FALSE);
		break;
	case POT_ACID:
		if (!Acid_resistance) {
		    pline("This burns%s!", obj->blessed ? " a little" :
				    obj->cursed ? " a lot" : "");
		    losehp(dice(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8),
				    "potion of acid", KILLED_BY_AN);
		}
		break;
	}
    } else {
	boolean angermon = TRUE;

	if (!your_fault) angermon = FALSE;

	if (!disint) {
	    switch (obj->otyp) {
	    case POT_HEALING:
	    case POT_EXTRA_HEALING:
	    case POT_FULL_HEALING:
		    if (mon->data == &mons[PM_PESTILENCE]) goto do_illness;
		    /*FALLTHRU*/
	    case POT_RESTORE_ABILITY:
	    case POT_GAIN_ABILITY:
 do_healing:
		    angermon = FALSE;
		    if (mon->mhp < mon->mhpmax) {
			mon->mhp = mon->mhpmax;
			if (canseemon(level, mon))
			    pline("%s looks sound and hale again.", Monnam(mon));
		    }
		    break;
	    case POT_SICKNESS:
		    if (mon->data == &mons[PM_PESTILENCE]) goto do_healing;
		    if (dmgtype(mon->data, AD_DISE) ||
			       dmgtype(mon->data, AD_PEST) || /* won't happen, see prior goto */
			       resists_poison(mon)) {
			if (canseemon(level, mon))
			    pline("%s looks unharmed.", Monnam(mon));
			break;
		    }
 do_illness:
		    if ((mon->mhpmax > 3) && !resist(mon, POTION_CLASS, 0, NOTELL))
			    mon->mhpmax /= 2;
		    if ((mon->mhp > 2) && !resist(mon, POTION_CLASS, 0, NOTELL))
			    mon->mhp /= 2;
		    if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
		    if (canseemon(level, mon))
			pline("%s looks rather ill.", Monnam(mon));
		    break;
	    case POT_CONFUSION:
	    case POT_BOOZE:
		    if (!resist(mon, POTION_CLASS, 0, NOTELL))  mon->mconf = TRUE;
		    break;
	    case POT_INVISIBILITY:
		    angermon = FALSE;
		    mon_set_minvis(mon);
		    break;
	    case POT_SLEEPING:
		    /* wakeup() doesn't rouse victims of temporary sleep */
		    if (sleep_monst(mon, rnd(12), POTION_CLASS)) {
			pline("%s falls asleep.", Monnam(mon));
			slept_monst(mon);
		    }
		    break;
	    case POT_PARALYSIS:
		    if (mon->mcanmove) {
			    mon->mcanmove = 0;
			    /* really should be rnd(5) for consistency with players
			     * breathing potions, but...
			     */
			    mon->mfrozen = rnd(25);
		    }
		    break;
	    case POT_SPEED:
		    angermon = FALSE;
		    mon_adjust_speed(mon, 1, obj);
		    break;
	    case POT_BLINDNESS:
		    if (haseyes(mon->data)) {
			int btmp = 64 + rn2(32) +
			    rn2(32) * !resist(mon, POTION_CLASS, 0, NOTELL);
			btmp += mon->mblinded;
			mon->mblinded = min(btmp,127);
			mon->mcansee = 0;
		    }
		    break;
	    case POT_WATER:
		    if (is_undead(mon->data) || is_demon(mon->data) ||
			    is_were(mon->data)) {
			if (obj->blessed) {
			    pline("%s %s in pain!", Monnam(mon),
				  is_silent(mon->data) ? "writhes" : "shrieks");
			    mon->mhp -= dice(2,6);
			    /* should only be by you */
			    if (mon->mhp < 1) killed(mon);
			    else if (is_were(mon->data) && !is_human(mon->data))
				new_were(mon);	/* revert to human */
			} else if (obj->cursed) {
			    angermon = FALSE;
			    if (canseemon(level, mon))
				pline("%s looks healthier.", Monnam(mon));
			    mon->mhp += dice(2,6);
			    if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
			    if (is_were(mon->data) && is_human(mon->data) &&
				    !Protection_from_shape_changers)
				new_were(mon);	/* transform into beast */
			}
		    } else if (mon->data == &mons[PM_GREMLIN]) {
			angermon = FALSE;
			split_mon(mon, NULL);
		    } else if (mon->data == &mons[PM_IRON_GOLEM]) {
			if (canseemon(level, mon))
			    pline("%s rusts.", Monnam(mon));
			mon->mhp -= dice(1,6);
			/* should only be by you */
			if (mon->mhp < 1) killed(mon);
		    }
		    break;
	    case POT_OIL:
		    if (obj->lamplit)
			    splatter_burning_oil(mon->mx, mon->my);
		    break;
	    case POT_ACID:
		    if (!resists_acid(mon) && !resist(mon, POTION_CLASS, 0, NOTELL)) {
			pline("%s %s in pain!", Monnam(mon),
			      is_silent(mon->data) ? "writhes" : "shrieks");
			mon->mhp -= dice(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8);
			if (mon->mhp < 1) {
			    if (your_fault)
				killed(mon);
			    else
				monkilled(mon, "", AD_ACID);
			}
		    }
		    break;
	    case POT_POLYMORPH:
		    bhitm(mon, obj);
		    break;
/*
	    case POT_GAIN_LEVEL:
	    case POT_LEVITATION:
	    case POT_FRUIT_JUICE:
	    case POT_MONSTER_DETECTION:
	    case POT_OBJECT_DETECTION:
		    break;
*/
	    }
	}
	if (angermon)
	    wakeup(mon);
	else
	    mon->msleeping = 0;
    }

	if (!disint) {
	    /* Note: potionbreathe() does its own docall() */
	    if ((distance==0 || ((distance < 3) && rn2(5))) &&
		(!breathless(youmonst.data) || haseyes(youmonst.data)))
		    potionbreathe(obj);
	    else if (obj->dknown && !objects[obj->otyp].oc_name_known &&
		       !objects[obj->otyp].oc_uname && cansee(mon->mx,mon->my))
		    docall(obj);
	}

	if (*u.ushops && obj->unpaid) {
	        struct monst *shkp =
			shop_keeper(level, *in_rooms(level, u.ux, u.uy, SHOPBASE));

		if (!shkp)
		    obj->unpaid = 0;
		else {
		    stolen_value(obj, u.ux, u.uy,
				 (boolean)shkp->mpeaceful, FALSE);
		    subfrombill(obj, shkp);
		}
	}
	obfree(obj, NULL);
}


/* vapors are inhaled or get in your eyes */
void potionbreathe(struct obj *obj)
{
	int i, ii, kn = 0;

	switch(obj->otyp) {
	case POT_RESTORE_ABILITY:
	case POT_GAIN_ABILITY:
		if (obj->cursed) {
		    if (!breathless(youmonst.data))
			pline("Ulch!  That potion smells terrible!");
		    else if (haseyes(youmonst.data)) {
			int numeyes = eyecount(youmonst.data);
			pline("Your %s sting%s!",
			     (numeyes == 1) ? body_part(EYE) : makeplural(body_part(EYE)),
			     (numeyes == 1) ? "s" : "");
		    }
		    break;
		} else {
		    i = rn2(A_MAX);		/* start at a random point */
		    for (ii = 0; ii < A_MAX; ii++) {
			if (ABASE(i) < AMAX(i)) {
			    ABASE(i)++;
			    iflags.botl = 1;
			}
			if (++i >= A_MAX) i = 0;
		    }
		}
		break;
	case POT_FULL_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, iflags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, iflags.botl = 1;
		/*FALL THROUGH*/
	case POT_EXTRA_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, iflags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, iflags.botl = 1;
		/*FALL THROUGH*/
	case POT_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, iflags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, iflags.botl = 1;
		exercise(A_CON, TRUE);
		break;
	case POT_SICKNESS:
		if (!Role_if (PM_HEALER)) {
			if (Upolyd) {
			    if (u.mh <= 5) u.mh = 1; else u.mh -= 5;
			} else {
			    if (u.uhp <= 5) u.uhp = 1; else u.uhp -= 5;
			}
			iflags.botl = 1;
			exercise(A_CON, FALSE);
		}
		break;
	case POT_HALLUCINATION:
		pline("You have a momentary vision.");
		break;
	case POT_CONFUSION:
	case POT_BOOZE:
		if (!Confusion)
			pline("You feel somewhat dizzy.");
		make_confused(itimeout_incr(HConfusion, rnd(5)), FALSE);
		break;
	case POT_INVISIBILITY:
		if (!Blind && !Invis) {
		    kn++;
		    pline("For an instant you %s!",
			See_invisible ? "could see right through yourself"
			: "couldn't see yourself");
		}
		break;
	case POT_PARALYSIS:
		kn++;
		if (!Free_action) {
		    pline("Something seems to be holding you.");
		    nomul(-rnd(5), "frozen by a potion");
		    nomovemsg = "You can move again.";
		    exercise(A_DEX, FALSE);
		} else pline("You stiffen momentarily.");
		break;
	case POT_SLEEPING:
		kn++;
		if (!FSleep_resistance) {
		    pline("You feel rather tired.");
		    nomul(-rnd(PSleep_resistance ? 2 : 5),
			  "sleeping off a magical draught");
		    nomovemsg = "You can move again.";
		    exercise(A_DEX, FALSE);
		} else pline("You yawn.");
		break;
	case POT_SPEED:
		if (!Fast) pline("Your knees seem more flexible now.");
		incr_itimeout(&HFast, rnd(5));
		exercise(A_DEX, TRUE);
		break;
	case POT_BLINDNESS:
		if (!Blind && !u.usleep) {
		    kn++;
		    pline("It suddenly gets dark.");
		}
		make_blinded(itimeout_incr(Blinded, rnd(5)), FALSE);
		if (!Blind && !u.usleep) pline("Your vision quickly clears.");
		break;
	case POT_WATER:
		if (u.umonnum == PM_GREMLIN) {
		    split_mon(&youmonst, NULL);
		} else if (u.ulycn >= LOW_PM) {
		    /* vapor from [un]holy water will trigger
		       transformation but won't cure lycanthropy */
		    if (obj->blessed && youmonst.data == &mons[u.ulycn])
			you_unwere(FALSE);
		    else if (obj->cursed && !Upolyd)
			you_were();
		}
		break;
	case POT_ACID:
	case POT_POLYMORPH:
		exercise(A_CON, FALSE);
		break;
	case POT_BLOOD:
	case POT_VAMPIRE_BLOOD:
		if (maybe_polyd(is_vampire(youmonst.data), Race_if(PM_VAMPIRE))) {
		    exercise(A_WIS, FALSE);
		    pline("You feel a %ssense of loss.",
			  obj->otyp == POT_VAMPIRE_BLOOD ? "terrible " : "");
		} else {
		    exercise(A_CON, FALSE);
		}
		break;
/*
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
	case POT_OIL:
		break;
*/
	}
	/* note: no obfree() */
	if (obj->dknown) {
	    if (kn)
		makeknown(obj->otyp);
	    else if (!objects[obj->otyp].oc_name_known &&
						!objects[obj->otyp].oc_uname)
		docall(obj);
	}
}


/* New alchemy scheme based on color mixing.
 * YANI by Graham Cox <aca00gac@shef.ac.uk>
 * Implemented by Nephi Allred <zindorsky@hotmail.com> on 15 Apr 2003
 *
 *      Alchemical tables are based on 4 bits describing
 *      dark/light level, yellow, blue and red
 *
 *      DYBR
 *      0000    white
 *      0001    pink
 *      0010    sky-blue
 *      0011    puce
 *      0100    yellow
 *      0101    orange
 *      0110    emerald
 *      0111    ochre
 *      1000    black
 *      1001    ruby
 *      1010    indigo
 *      1011    magenta
 *      1100    golden
 *      1101    amber
 *      1110    dark green
 *      1111    brown
 */

/* Assumes gain ability is first potion and vampire blood is last. */
char alchemy_table1[POT_VAMPIRE_BLOOD - POT_GAIN_ABILITY];
short alchemy_table2[17];

#define ALCHEMY_WHITE		0
#define ALCHEMY_BLACK		8
#define ALCHEMY_GRAY		(alchemy_table2[16])
#define IS_PRIMARY_COLOR(x)	(((x)&7)==1 || ((x)&7)==2 || ((x)&7)==4)
#define IS_SECONDARY_COLOR(x)	(((x)&7)==3 || ((x)&7)==5 || ((x)&7)==6)
#define IS_LIGHT_COLOR(x)	(((x)&8)==0)
#define IS_DARK_COLOR(x)	((x)&8)

/* Does a one-time set up of alchemical tables. */
static void alchemy_init(void)
{
	if (!alchemy_init_done) {
	    short i;
	    const char *potion_desc;

#define	    ALCH_PREP(i, n) \
	    do { \
		alchemy_table1[i - POT_GAIN_ABILITY] = n; \
		alchemy_table2[n] = i; \
	    } while (0)

	    for (i = POT_GAIN_ABILITY; i <= POT_WATER; i++) {
		potion_desc = OBJ_DESCR(objects[i]);
		if      (!strcmp(potion_desc,"white"))		ALCH_PREP(i,  0);
		else if (!strcmp(potion_desc,"pink"))		ALCH_PREP(i,  1);
		else if (!strcmp(potion_desc,"sky blue"))	ALCH_PREP(i,  2);
		else if (!strcmp(potion_desc,"puce"))		ALCH_PREP(i,  3);
		else if (!strcmp(potion_desc,"yellow"))		ALCH_PREP(i,  4);
		else if (!strcmp(potion_desc,"orange"))		ALCH_PREP(i,  5);
		else if (!strcmp(potion_desc,"emerald"))	ALCH_PREP(i,  6);
		else if (!strcmp(potion_desc,"ochre"))		ALCH_PREP(i,  7);
		else if (!strcmp(potion_desc,"black"))		ALCH_PREP(i,  8);
		else if (!strcmp(potion_desc,"ruby"))		ALCH_PREP(i,  9);
		else if (!strcmp(potion_desc,"indigo"))		ALCH_PREP(i, 10);
		else if (!strcmp(potion_desc,"magenta"))	ALCH_PREP(i, 11);
		else if (!strcmp(potion_desc,"golden"))		ALCH_PREP(i, 12);
		else if (!strcmp(potion_desc,"amber"))		ALCH_PREP(i, 13);
		else if (!strcmp(potion_desc,"dark green"))	ALCH_PREP(i, 14);
		else if (!strcmp(potion_desc,"brown"))		ALCH_PREP(i, 15);
		else if (!strcmp(potion_desc,"silver")) {
		    alchemy_table1[i - POT_GAIN_ABILITY] = -1;
		    alchemy_table2[16] = i;
		} else {
		    alchemy_table1[i - POT_GAIN_ABILITY] = -1;
		}
	    }
	    alchemy_init_done = TRUE;

#undef	    ALCH_PREP
	}
}


/* returns the potion type when o1 is dipped in o2 */
static short mixtype(struct obj *o1, struct obj *o2)
{
	/* cut down on the number of cases below */
	if (o1->oclass == POTION_CLASS && o2->otyp == POT_FRUIT_JUICE) {
		struct obj *swp;

		swp = o1; o1 = o2; o2 = swp;
	}

	switch (o1->otyp) {
	    case POT_FRUIT_JUICE:
		switch (o2->otyp) {
		    case POT_BLOOD:
			return POT_BLOOD;
		    case POT_VAMPIRE_BLOOD:
			return POT_VAMPIRE_BLOOD;
		}
		break;
	}

	if (o1->oclass == POTION_CLASS) {
	    int i1, i2, result;

	    alchemy_init();
	    i1 = alchemy_table1[o1->otyp - POT_GAIN_ABILITY];
	    i2 = alchemy_table1[o2->otyp - POT_GAIN_ABILITY];

	    /* check that both potions are of mixable types */
	    if (i1 < 0 || i2 < 0)
		return 0;

	    /* swap for simplified checks */
	    if (i2 == ALCHEMY_WHITE || (i2 == ALCHEMY_BLACK && i1 != ALCHEMY_WHITE)) {
		result = i1;
		i1 = i2;
		i2 = result;
	    }

	    if (i1 == ALCHEMY_WHITE && i2 == ALCHEMY_BLACK) {
		return ALCHEMY_GRAY;
	    } else if ((IS_PRIMARY_COLOR(i1) && IS_PRIMARY_COLOR(i2)) ||
		       (IS_SECONDARY_COLOR(i1) && IS_SECONDARY_COLOR(i2))) {
		/* bitwise OR simulates pigment addition */
		result = i1 | i2;
		/* adjust light/dark level if necessary */
		if ((i1 ^ i2) & 8) {
		    if (o1->odiluted == o2->odiluted) {
			/* same dilution level, randomly toggle */
			result ^= (rn2(2) << 3);
		    } else {
			/* use dark/light level of undiluted potion */
			result ^= (o1->odiluted ? i1 : i2) & 8;
		    }
		}
	    } else if ((i1 == ALCHEMY_WHITE && IS_DARK_COLOR(i2)) ||
		       (i1 == ALCHEMY_BLACK && IS_LIGHT_COLOR(i2))) {
		/* toggle light/dark bit */
		result = i2 ^ 8;
	    } else {
		return 0;
	    }

	    /* don't create non-existing potions */
	    if (OBJ_NAME(objects[alchemy_table2[result]]) == 0)
		return 0;

	    return alchemy_table2[result];
	} else {
	    switch (o1->otyp) {
		case UNICORN_HORN:
		    switch (o2->otyp) {
			case POT_SICKNESS:
			    return POT_FRUIT_JUICE;
			case POT_HALLUCINATION:
			case POT_BLINDNESS:
			case POT_CONFUSION:
			case POT_BLOOD:
			case POT_VAMPIRE_BLOOD:
			    return POT_WATER;
		    }
		    break;
		case AMETHYST:	/* "a-methyst" == "not intoxicated" */
		    if (o2->otyp == POT_BOOZE)
			return POT_FRUIT_JUICE;
		    break;
		}
	}

	return 0;
}


boolean get_wet(struct obj *obj)
/* returns TRUE if something happened (potion should be used up) */
{
	char Your_buf[BUFSZ];

	if (snuff_lit(obj)) return TRUE;

	if (obj->greased) {
		grease_protect(obj,NULL,&youmonst);
		return FALSE;
	}
	Shk_Your(Your_buf, obj);
	/* (Rusting shop goods ought to be charged for.) */
	switch (obj->oclass) {
	    case POTION_CLASS:
		if (obj->otyp == POT_WATER) return FALSE;
		/* KMH -- Water into acid causes an explosion */
		if (obj->otyp == POT_ACID) {
			pline("It boils vigorously!");
			pline("You are caught in the explosion!");
			losehp(Acid_resistance ? rnd(5) : rnd(10),
			       "elementary chemistry", KILLED_BY);
			makeknown(obj->otyp);
			update_inventory();
			return TRUE;
		}
		pline("%s %s%s.", Your_buf, aobjnam(obj,"dilute"),
		      obj->odiluted ? " further" : "");
		if (obj->unpaid && costly_spot(u.ux, u.uy)) {
		    pline("You dilute it, you pay for it.");
		    bill_dummy_object(obj);
		}
		if (obj->odiluted) {
			obj->odiluted = 0;
			obj->blessed = obj->cursed = FALSE;
			obj->otyp = POT_WATER;
		} else obj->odiluted++;
		update_inventory();
		return TRUE;
	    case SCROLL_CLASS:
		if (obj->otyp != SCR_BLANK_PAPER
		    ) {
			if (!Blind) {
				boolean oq1 = obj->quan == 1L;
				pline("The scroll%s %s.",
					  oq1 ? "" : "s", otense(obj, "fade"));
			}
			if (obj->unpaid && costly_spot(u.ux, u.uy)) {
			    pline("You erase it, you pay for it.");
			    bill_dummy_object(obj);
			}
			obj->otyp = SCR_BLANK_PAPER;
			obj->spe = 0;
			update_inventory();
			return TRUE;
		} else break;
	    case SPBOOK_CLASS:
		if (obj->otyp != SPE_BLANK_PAPER) {

			if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
	pline("%s suddenly heats up; steam rises and it remains dry.",
				The(xname(obj)));
			} else {
			    if (!Blind) {
				    boolean oq1 = obj->quan == 1L;
				    pline("The spellbook%s %s.",
					oq1 ? "" : "s", otense(obj, "fade"));
			    }
			    if (obj->unpaid && costly_spot(u.ux, u.uy)) {
			        pline("You erase it, you pay for it.");
			        bill_dummy_object(obj);
			    }
			    obj->otyp = SPE_BLANK_PAPER;
			    update_inventory();
			}
			return TRUE;
		}
		break;
	    case WEAPON_CLASS:
	    /* Just "fall through" to generic rustprone check for now. */
	    /* fall through */
	    default:
		if (!obj->oerodeproof && is_rustprone(obj) &&
		    (obj->oeroded < MAX_ERODE) && !rn2(2)) {
			pline("%s %s some%s.",
			      Your_buf, aobjnam(obj, "rust"),
			      obj->oeroded ? " more" : "what");
			obj->oeroded++;
			update_inventory();
			return TRUE;
		} else break;
	}
	pline("%s %s wet.", Your_buf, aobjnam(obj,"get"));
	return FALSE;
}


/* User command for dipping objects. */
int dodip(struct obj *potion)
{
	struct obj *obj;
	const char *tmp;
	uchar here;
	char allowall[2] = {ALL_CLASSES, 0};
	char qbuf[QBUFSZ];

	if (potion && !validate_object(potion, beverages, "dip into"))
		return 0;
	
	obj = getobj(allowall, "dip", NULL);
	if (!obj) return 0;

	if (!potion) {
		here = level->locations[u.ux][u.uy].typ;
		/* Is there a fountain to dip into here? */
		if (IS_FOUNTAIN(here)) {
			sprintf(qbuf, "Dip %s into the fountain?",
				safe_qbuf("", sizeof("Dip  into the fountain?"),
					  the(xname(obj)),
					  the(simple_typename(obj->otyp)),
					  "this item"));
			if (yn(qbuf) == 'y') {
				dipfountain(obj);
				return 1;
			}
		} else if (is_pool(level, u.ux,u.uy) || is_swamp(level, u.ux,u.uy)) {
			tmp = waterbody_name(u.ux,u.uy);
			sprintf(qbuf, "Dip %s into the %s?",
				safe_qbuf("",
					  sizeof("Dip  into the pool of water?"),
					  the(xname(obj)),
					  the(simple_typename(obj->otyp)),
					  "this item"),
				tmp);
			if (yn(qbuf) == 'y') {
			    if (Levitation) {
				floating_above(tmp);
			    } else if (u.usteed && !is_swimmer(u.usteed->data) &&
				    P_SKILL(P_RIDING) < P_BASIC) {
				rider_cant_reach(); /* not skilled enough to reach */
			    } else {
				get_wet(obj);
				if (obj->otyp == POT_ACID) useup(obj);
			    }
			    return 1;
			}
		}
		sprintf(qbuf, "dip %s into",
			safe_qbuf("", sizeof("dip  into"),
				  the(xname(obj)),
				  the(simple_typename(obj->otyp)),
				  "this item"));
		potion = getobj(beverages, qbuf, NULL);
	}

	if (!potion) return 0;
	if (potion == obj && potion->quan == 1L) {
		pline("That is a potion bottle, not a Klein bottle!");
		return 0;
	}

	return dip(potion, obj);
}


/* Dip an arbitrary object into a potion. */
int dip(struct obj *potion, struct obj *obj)
{
	struct obj *singlepotion;
	const char *tmp;
	short mixture;
	char Your_buf[BUFSZ];

	potion->in_use = TRUE;		/* assume it will be used up */
	if (potion->otyp == POT_WATER) {
		boolean useeit = !Blind;
		if (useeit) Shk_Your(Your_buf, obj);
		if (potion->blessed) {
			if (obj->cursed) {
				if (useeit)
				    pline("%s %s %s.",
					  Your_buf,
					  aobjnam(obj, "softly glow"),
					  hcolor("amber"));
				uncurse(obj);
				obj->bknown=1;
poof:
				if (!(objects[potion->otyp].oc_name_known) &&
				   !(objects[potion->otyp].oc_uname))
					docall(potion);
				useup(potion);
				return 1;
			} else if (!obj->blessed) {
				if (useeit) {
				    tmp = hcolor("light blue");
				    pline("%s %s with a%s %s aura.",
					  Your_buf,
					  aobjnam(obj, "softly glow"),
					  strchr(vowels, *tmp) ? "n" : "", tmp);
				}
				bless(obj);
				obj->bknown=1;
				goto poof;
			}
		} else if (potion->cursed) {
			if (obj->blessed) {
				if (useeit)
				    pline("%s %s %s.",
					  Your_buf,
					  aobjnam(obj, "glow"),
					  hcolor((const char *)"brown"));
				unbless(obj);
				obj->bknown=1;
				goto poof;
			} else if (!obj->cursed) {
				if (useeit) {
				    tmp = hcolor("black");
				    pline("%s %s with a%s %s aura.",
					  Your_buf,
					  aobjnam(obj, "glow"),
					  strchr(vowels, *tmp) ? "n" : "", tmp);
				}
				curse(obj);
				obj->bknown=1;
				goto poof;
			}
		} else
			if (get_wet(obj))
			    goto poof;
	} else if (obj->otyp == POT_POLYMORPH ||
		potion->otyp == POT_POLYMORPH) {
	    if (dip_diluted_same_potions(&potion, &obj)) {
		potion->in_use = FALSE;
		prinv(NULL, potion, 0L);
		return 1;
	    } else if (obj->otyp == potion->otyp ||	/* both POT_POLY */
		    obj->otyp == WAN_POLYMORPH ||
		    obj->otyp == SPE_POLYMORPH ||
		    obj->otyp == AMULET_OF_UNCHANGING ||
		    obj == uball || obj == uskin ||
		    obj_resists(obj->otyp == POT_POLYMORPH ?
				potion : obj, 5, 95)) {
		/* some objects can't be polymorphed */
		pline("Nothing happens.");
	    } else {
	    	boolean was_wep = FALSE, was_swapwep = FALSE, was_quiver = FALSE;
		short save_otyp = obj->otyp;
		/* KMH, conduct */
		u.uconduct.polypiles++;

		if (obj == uwep) was_wep = TRUE;
		else if (obj == uswapwep) was_swapwep = TRUE;
		else if (obj == uquiver) was_quiver = TRUE;

		obj = poly_obj(obj, STRANGE_OBJECT);

		if (was_wep) setuwep(obj);
		else if (was_swapwep) setuswapwep(obj);
		else if (was_quiver) setuqwep(obj);

		if (obj->otyp != save_otyp) {
			makeknown(POT_POLYMORPH);
			useup(potion);
			prinv(NULL, obj, 0L);
			return 1;
		} else {
			pline("Nothing seems to happen.");
			goto poof;
		}
	    }
	    potion->in_use = FALSE;	/* didn't go poof */
	    return 1;
	} else if (obj->oclass == POTION_CLASS && obj->otyp != potion->otyp) {
		/* Mixing potions is dangerous... */
		/* Give a clue to what's going on ... */
		if (potion->dknown && obj->dknown) {
		    pline("You mix the %s potion with the %s one ...",
			  OBJ_DESCR(objects[potion->otyp]),
			  OBJ_DESCR(objects[obj->otyp]));
		} else {
		    pline("The potions mix...");
		}
		/* KMH, balance patch -- acid is particularly unstable */
		if (obj->cursed || obj->otyp == POT_ACID ||
		    potion->otyp == POT_ACID || !rn2(10)) {
			pline("BOOM!  They explode!");
			exercise(A_STR, FALSE);
			if (!breathless(youmonst.data) || haseyes(youmonst.data))
				potionbreathe(obj);
			useup(obj);
			useup(potion);
			losehp(Acid_resistance ? rnd(5) : rnd(10),
			       "alchemic blast", KILLED_BY_AN);
			return 1;
		}

		obj->blessed = obj->cursed = obj->bknown = 0;
		if (Blind || Hallucination) obj->dknown = 0;

		if ((mixture = mixtype(obj, potion)) != 0) {
			obj->otyp = mixture;
		} else {
		    switch (obj->odiluted ? 1 : rnd(8)) {
			case 1:
				obj->otyp = POT_WATER;
				break;
			case 2:
			case 3:
				obj->otyp = POT_SICKNESS;
				break;
			case 4:
				{
				  struct obj *otmp;
				  otmp = mkobj(level, POTION_CLASS,FALSE);
				  obj->otyp = otmp->otyp;
				  obfree(otmp, NULL);
				}
				break;
			default:
				if (!Blind)
			  pline("The mixture glows brightly and evaporates.");
				useup(obj);
				useup(potion);
				return 1;
		    }
		}

		obj->odiluted = (obj->otyp != POT_WATER);

		if (obj->otyp == POT_WATER && !Hallucination) {
			pline("The mixture bubbles%s.",
				Blind ? "" : ", then clears");
		} else if (!Blind) {
			pline("The mixture looks %s.",
				hcolor(OBJ_DESCR(objects[obj->otyp])));
		}

		useup(potion);
		/* Down with the Alchemy Tax! */
		obj->quan++;
		obj->owt = weight(obj);
		return 1;
	} else if (dip_diluted_same_potions(&potion, &obj)) {
		potion->in_use = FALSE;
		prinv(NULL, potion, 0L);
		return 1;
	}

#ifdef INVISIBLE_OBJECTS
	if (potion->otyp == POT_INVISIBILITY && !obj->oinvis) {
		obj->oinvis = TRUE;
		if (!Blind) {
		    if (!See_invisible) pline("Where did %s go?",
		    		the(xname(obj)));
		    else pline("You notice a little haziness around %s.",
		    		the(xname(obj)));
		}
		goto poof;
	} else if (potion->otyp == POT_SEE_INVISIBLE && obj->oinvis) {
		obj->oinvis = FALSE;
		if (!Blind) {
		    if (!See_invisible) pline("So that's where %s went!",
		    		the(xname(obj)));
		    else pline("The haziness around %s disappears.",
		    		the(xname(obj)));
		}
		goto poof;
	}
#endif

	if (potion->otyp == POT_ACID && obj->otyp == CORPSE &&
	    obj->corpsenm == PM_LICHEN && !Blind) {
	    pline("%s %s %s around the edges.", The(cxname(obj)),
		  otense(obj, "turn"), potion->odiluted ? "orange" : "red");
	    potion->in_use = FALSE;	/* didn't go poof */
	    return 1;
	}

	if (is_poisonable(obj)) {
	    if (potion->otyp == POT_SICKNESS && !obj->opoisoned) {
		char buf[BUFSZ];
		if (potion->quan > 1L)
		    sprintf(buf, "One of %s", the(xname(potion)));
		else
		    strcpy(buf, The(xname(potion)));
		pline("%s forms a coating on %s.",
		      buf, the(xname(obj)));
		obj->opoisoned = TRUE;
		goto poof;
	    } else if (obj->opoisoned &&
		      (potion->otyp == POT_HEALING ||
		       potion->otyp == POT_EXTRA_HEALING ||
		       potion->otyp == POT_FULL_HEALING)) {
		pline("A coating wears off %s.", the(xname(obj)));
		obj->opoisoned = 0;
		goto poof;
	    }
	}

	if (potion->otyp == POT_OIL) {
	    boolean wisx = FALSE;
	    if (potion->lamplit) {	/* burning */
		int omat = objects[obj->otyp].oc_material;
		/* the code here should be merged with fire_damage */
		if (catch_lit(obj)) {
		    /* catch_lit does all the work if true */
		} else if (obj->oerodeproof || obj_resists(obj, 5, 95) ||
			   !is_flammable(obj) || obj->oclass == FOOD_CLASS) {
		    pline("%s %s to burn for a moment.",
			  Yname2(obj), otense(obj, "seem"));
		} else {
		    if ((omat == PLASTIC || omat == PAPER) && !obj->oartifact)
			obj->oeroded = MAX_ERODE;
		    pline("The burning oil %s %s.",
			    obj->oeroded == MAX_ERODE ? "destroys" : "damages",
			    yname(obj));
		    if (obj->oeroded == MAX_ERODE) {
			obj_extract_self(obj);
			obfree(obj, NULL);
			obj = NULL;
		    } else {
			/* we know it's carried */
			if (obj->unpaid) {
			    /* create a dummy duplicate to put on bill */
			    verbalize("You burnt it, you bought it!");
			    bill_dummy_object(obj);
			}
			obj->oeroded++;
		    }
		}
	    } else if (potion->cursed) {
		pline("The potion spills and covers your %s with oil.",
			  makeplural(body_part(FINGER)));
		incr_itimeout(&Glib, dice(2,10));
	    } else if (obj->oclass != WEAPON_CLASS && !is_weptool(obj)) {
		/* the following cases apply only to weapons */
		goto more_dips;
	    /* Oil removes rust and corrosion, but doesn't unburn.
	     * Arrows, etc are classed as metallic due to arrowhead
	     * material, but dipping in oil shouldn't repair them.
	     */
	    } else if ((!is_rustprone(obj) && !is_corrodeable(obj)) ||
			is_ammo(obj) || (!obj->oeroded && !obj->oeroded2)) {
		/* uses up potion, doesn't set obj->greased */
		pline("%s %s with an oily sheen.",
		      Yname2(obj), otense(obj, "gleam"));
	    } else {
		pline("%s %s less %s.",
		      Yname2(obj), otense(obj, "are"),
		      (obj->oeroded && obj->oeroded2) ? "corroded and rusty" :
			obj->oeroded ? "rusty" : "corroded");
		if (obj->oeroded > 0) obj->oeroded--;
		if (obj->oeroded2 > 0) obj->oeroded2--;
		wisx = TRUE;
	    }
	    exercise(A_WIS, wisx);
	    makeknown(potion->otyp);
	    useup(potion);
	    return 1;
	}
    more_dips:

	/* Allow filling of MAGIC_LAMPs to prevent identification by player */
	if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP) &&
	   (potion->otyp == POT_OIL)) {
	    /* Turn off engine before fueling, turn off fuel too :-)  */
	    if (obj->lamplit || potion->lamplit) {
		useup(potion);
		explode(u.ux, u.uy, 11, dice(6,6), 0, EXPL_FIERY);
		exercise(A_WIS, FALSE);
		return 1;
	    }
	    /* Adding oil to an empty magic lamp renders it into an oil lamp */
	    if ((obj->otyp == MAGIC_LAMP) && obj->spe == 0) {
		obj->otyp = OIL_LAMP;
		obj->age = 0;
	    }
	    if (obj->age > 1000L) {
		pline("%s %s full.", Yname2(obj), otense(obj, "are"));
		potion->in_use = FALSE;	/* didn't go poof */
	    } else {
		pline("You fill %s with oil.", yname(obj));
		check_unpaid(potion);	/* Yendorian Fuel Tax */
		obj->age += 2*potion->age;	/* burns more efficiently */
		if (obj->age > 1500L) obj->age = 1500L;
		useup(potion);
		exercise(A_WIS, TRUE);
	    }
	    makeknown(POT_OIL);
	    obj->spe = 1;
	    update_inventory();
	    return 1;
	}

	potion->in_use = FALSE;		/* didn't go poof */
	if ((obj->otyp == UNICORN_HORN || obj->otyp == AMETHYST) &&
	    (mixture = mixtype(obj, potion)) != 0) {
		char oldbuf[BUFSZ], newbuf[BUFSZ];
		short old_otyp = potion->otyp;
		boolean old_dknown = FALSE;
		boolean more_than_one = potion->quan > 1;

		oldbuf[0] = '\0';
		if (potion->dknown) {
		    old_dknown = TRUE;
		    sprintf(oldbuf, "%s ",
			    hcolor(OBJ_DESCR(objects[potion->otyp])));
		}
		/* with multiple merged potions, split off one and
		   just clear it */
		if (potion->quan > 1L) {
		    singlepotion = splitobj(potion, 1L);
		} else singlepotion = potion;
		
		if (singlepotion->unpaid && costly_spot(u.ux, u.uy)) {
		    pline("You use it, you pay for it.");
		    bill_dummy_object(singlepotion);
		}
		singlepotion->otyp = mixture;
		singlepotion->blessed = 0;
		if (mixture == POT_WATER)
		    singlepotion->cursed = singlepotion->odiluted = 0;
		else
		    singlepotion->cursed = obj->cursed;  /* odiluted left as-is */
		singlepotion->bknown = FALSE;
		if (Blind) {
		    singlepotion->dknown = FALSE;
		} else {
		    singlepotion->dknown = !Hallucination;
		    if (mixture == POT_WATER && singlepotion->dknown)
			sprintf(newbuf, "clears");
		    else
			sprintf(newbuf, "turns %s",
				hcolor(OBJ_DESCR(objects[mixture])));
		    pline("The %spotion%s %s.", oldbuf,
			      more_than_one ? " that you dipped into" : "",
			      newbuf);
		    if (!objects[old_otyp].oc_uname &&
			!objects[old_otyp].oc_name_known && old_dknown) {
			struct obj fakeobj;
			fakeobj = zeroobj;
			fakeobj.dknown = 1;
			fakeobj.otyp = old_otyp;
			fakeobj.oclass = POTION_CLASS;
			docall(&fakeobj);
		    }
		}
		obj_extract_self(singlepotion);
		hold_another_object(singlepotion, "You juggle and drop %s!",
				    doname(singlepotion), NULL);
		update_inventory();
		return 1;
	}

	pline("Interesting...");
	return 1;
}


void djinni_from_bottle(struct obj *obj)
{
	struct monst *mtmp;
	int chance;

	if (!(mtmp = makemon(&mons[PM_DJINNI], level, u.ux, u.uy, NO_MM_FLAGS))){
		pline("It turns out to be empty.");
		return;
	}

	if (!Blind) {
		pline("In a cloud of smoke, %s emerges!", a_monnam(mtmp));
		pline("%s speaks.", Monnam(mtmp));
	} else {
		pline("You smell acrid fumes.");
		pline("Something speaks.");
	}

	chance = rn2(5);
	if (obj->blessed) chance = (chance == 4) ? rnd(4) : 0;
	else if (obj->cursed) chance = (chance == 0) ? rn2(4) : 4;
	/* 0,1,2,3,4:  b=80%,5,5,5,5; nc=20%,20,20,20,20; c=5%,5,5,5,80 */

	switch (chance) {
	case 0 : verbalize("I am in your debt.  I will grant one wish!");
		makewish(FALSE);
		mongone(mtmp);
		break;
	case 1 : verbalize("Thank you for freeing me!");
		tamedog(mtmp, NULL);
		break;
	case 2 : verbalize("You freed me!");
		mtmp->mpeaceful = TRUE;
		set_malign(mtmp);
		break;
	case 3 : verbalize("It is about time!");
		pline("%s vanishes.", Monnam(mtmp));
		mongone(mtmp);
		break;
	default: verbalize("You disturbed me, fool!");
		break;
	}
}

/* clone a gremlin or mold (2nd arg non-null implies heat as the trigger);
   hit points are cut in half (odd HP stays with original) */
struct monst *split_mon(struct monst *mon,	/* monster being split */
			struct monst *mtmp)	/* optional attacker whose heat triggered it */
{
	struct monst *mtmp2;
	char reason[BUFSZ];

	reason[0] = '\0';
	if (mtmp) sprintf(reason, " from %s heat",
			  (mtmp == &youmonst) ? (const char *)"your" :
			      (const char *)s_suffix(mon_nam(mtmp)));

	if (mon == &youmonst) {
	    mtmp2 = cloneu();
	    if (mtmp2) {
		mtmp2->mhpmax = u.mhmax / 2;
		u.mhmax -= mtmp2->mhpmax;
		iflags.botl = 1;
		pline("You multiply%s!", reason);
	    }
	} else {
	    mtmp2 = clone_mon(mon, 0, 0);
	    if (mtmp2) {
		mtmp2->mhpmax = mon->mhpmax / 2;
		mon->mhpmax -= mtmp2->mhpmax;
		if (canspotmon(level, mon))
		    pline("%s multiplies%s!", Monnam(mon), reason);
	    }
	}
	return mtmp2;
}

/*potion.c*/

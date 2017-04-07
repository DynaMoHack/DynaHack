/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaMoHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "eshk.h"
#include "dlb.h"

	/* these probably ought to be generated by makedefs, like LAST_GEM */
#define FIRST_GEM    DILITHIUM_CRYSTAL
#define FIRST_AMULET AMULET_OF_ESP
#define LAST_AMULET  AMULET_OF_YENDOR
 
struct valuable_data { long count; int typ; };

static struct valuable_data
	gems[LAST_GEM+1 - FIRST_GEM + 1], /* 1 extra for glass */
	amulets[LAST_AMULET+1 - FIRST_AMULET];

static const struct val_list { struct valuable_data *list; int size; } valuables[] = {
	{ gems,    sizeof gems / sizeof *gems },
	{ amulets, sizeof amulets / sizeof *amulets },
	{ 0, 0 }
};

static void disclose(int,boolean);
static void dump_disclose(int);
static void get_valuables(struct obj *);
static void sort_valuables(struct valuable_data *,int);
static void artifact_score(struct obj *,boolean,struct menulist *);
static void savelife(int);
static boolean check_survival(int how, char *kilbuf);
static long calc_score(int how);
static boolean should_query_disclose_options(char *defquery);
static void container_contents(struct obj *,boolean,boolean);

#define done_stopprint program_state.stopprint

/*
 * The order of these needs to match the macros in hack.h.
 */
static const char *const deaths[] = {		/* the array of death */
	"died", "choked", "poisoned", "starvation", "drowning",
	"burning", "dissolving under the heat and pressure",
	"crushed", "turned to stone", "turned into slime",
	"genocided", "disintegrated", "panic", "trickery",
	"quit", "escaped", "defied the gods and escaped", "ascended"
};

static const char *const ends[] = {		/* "when you..." */
	"died", "choked", "were poisoned", "starved", "drowned",
	"burned", "dissolved in the lava",
	"were crushed", "turned to stone", "turned into slime",
	"were genocided", "were disintegrated", "panicked", "were tricked",
	"quit", "escaped", "defied the gods and escaped", "ascended"
};

static char killbuf[BUFSZ];

extern const char * const killed_by_prefix[];	/* from topten.c */


/* "#quit" command or keyboard interrupt */
int done2(void)
{
	if (paranoid_yn("Really quit?", iflags.paranoid_quit) == 'n') {
		flush_screen();
		if (multi > 0) nomul(0, NULL);
		if (multi == 0) {
		    u.uinvulnerable = FALSE;	/* avoid ctrl-C bug -dlc */
		    u.usleep = 0;
		}
		return 0;
	}

	killer = 0;
	done(QUIT);
	return 0;
}


/* stores a killer string fitting the monster in buf and returns killer format */
int done_in_by_format(const struct monst *mtmp, char *buf)
{
	int kfmt;
	boolean distorted = (boolean)(Hallucination && canspotmon(level, mtmp));

	buf[0] = '\0';
	kfmt = KILLED_BY_AN;
	/* "killed by the high priest of Crom" is okay, "killed by the high
	   priest" alone isn't */
	if ((mtmp->data->geno & G_UNIQ) != 0 && !(mtmp->data == &mons[PM_HIGH_PRIEST] && !mtmp->ispriest)) {
	    if (!type_is_pname(mtmp->data))
		strcat(buf, "the ");
	    kfmt = KILLED_BY;
	}
	/* _the_ <invisible> <distorted> ghost of Dudley */
	if (mtmp->data == &mons[PM_GHOST] && mtmp->mnamelth) {
		strcat(buf, "the ");
		kfmt = KILLED_BY;
	}
	if (mtmp->minvis)
		strcat(buf, "invisible ");
	if (distorted)
		strcat(buf, "hallucinogen-distorted ");

	if (mtmp->data == &mons[PM_GHOST]) {
		strcat(buf, "ghost");
		if (mtmp->mnamelth) sprintf(eos(buf), " of %s", NAME(mtmp));
	} else if (mtmp->isshk) {
		sprintf(eos(buf), "%s %s, the shopkeeper",
			(mtmp->female ? "Ms." : "Mr."), shkname(mtmp));
		kfmt = KILLED_BY;
	} else if (mtmp->ispriest || mtmp->isminion) {
		/* m_monnam() suppresses "the" prefix plus "invisible", and
		   it overrides the effect of Hallucination on priestname() */
		const char *killer_mon = m_monnam(mtmp);
		strcat(buf, killer_mon);
	} else {
		strcat(buf, mons_mname(mtmp->data));
		if (mtmp->mnamelth)
		    sprintf(eos(buf), " called %s", NAME(mtmp));
	}

	if (multi) {
	    if (*multi_txt)
		sprintf(eos(buf), ", while %s", multi_txt);
	    else
		strcat(buf, ", while helpless");
	}

	return kfmt;
}


void done_in_by(const struct monst *mtmp)
{
	char buf[BUFSZ];
	killer_format = done_in_by_format(mtmp, buf);
	killer = buf;

	if (mtmp->data->mlet == S_WRAITH)
		u.ugrave_arise = PM_WRAITH;
	else if (mtmp->data->mlet == S_MUMMY && urace.mummynum != NON_PM)
		u.ugrave_arise = urace.mummynum;
	else if (is_vampire(mtmp->data) && Race_if (PM_HUMAN))
		u.ugrave_arise = PM_VAMPIRE;
	else if (mtmp->data == &mons[PM_GHOUL])
		u.ugrave_arise = PM_GHOUL;
	if (u.ugrave_arise >= LOW_PM &&
				(mvitals[u.ugrave_arise].mvflags & G_GENOD))
		u.ugrave_arise = NON_PM;

	pline("You die...");
	if (touch_petrifies(mtmp->data))
		done(STONING);
	else
		done(DIED);
}


/*VARARGS1*/
void panic(const char *str, ...)
{
	char buf[BUFSZ];
	va_list the_args;
	va_start(the_args, str);

	if (program_state.panicking++)
	    terminate(); /* avoid loops - this should never happen*/

	raw_print(program_state.gameover ?
		  "Postgame wrapup disrupted.\n" :
		  !program_state.something_worth_saving ?
		  "Program initialization has failed.\n" :
		  "Suddenly, the dungeon collapses.\n");
	if (!wizard)
	    raw_printf("Report error to \"%s\"%s.\n", WIZARD,
			!program_state.something_worth_saving ? "" :
			" and it may be possible to rebuild.");
	if (program_state.something_worth_saving)
	    dosave0(TRUE);
	
	vsprintf(buf,str,the_args);
	raw_print(buf);
	paniclog("panic", buf);

	va_end(the_args);
	done(PANICKED);
}

static boolean should_query_disclose_options(char *defquery)
{
    if (!defquery) {
	impossible("should_query_disclose_option: null defquery");
	return TRUE;
    }
	
    switch (flags.end_disclose) {
	default: /* fall through */
	case DISCLOSE_PROMPT_DEFAULT_YES: *defquery = 'y'; return TRUE;
	case DISCLOSE_YES_WITHOUT_PROMPT: *defquery = 'y'; return FALSE;
	case DISCLOSE_PROMPT_DEFAULT_NO: *defquery = 'n'; return TRUE;
	case DISCLOSE_NO_WITHOUT_PROMPT: *defquery = 'n'; return FALSE;
    }
}


static void disclose(int how, boolean taken)
{
	char	c = 0, defquery;
	char	qbuf[QBUFSZ];
	boolean ask = should_query_disclose_options(&defquery);

	if (invent) {
	    if (taken)
		sprintf(qbuf,"Do you want to see what you had when you %s?",
			(how == QUIT) ? "quit" : "died");
	    else
		strcpy(qbuf,"Do you want your possessions identified?");

	    if (!done_stopprint) {
		c = ask ? yn_function(qbuf, ynqchars, defquery) : defquery;
		if (c == 'y') {
		    display_inventory(NULL, TRUE);
		    container_contents(invent, TRUE, TRUE);
		}
		if (c == 'q')  done_stopprint++;
	    }
	}

	if (!done_stopprint) {
	    c = ask ? yn_function("Do you want to see your attributes?",
				  ynqchars, defquery) : defquery;
	    if (c == 'y')
		enlightenment(how >= PANICKED ? 1 : 2); /* final */
	    if (c == 'q') done_stopprint++;
	}

	if (!done_stopprint)
	    list_vanquished(defquery, ask);

	if (!done_stopprint)
	    list_genocided(defquery, ask);

	if (!done_stopprint) {
	    c = ask ? yn_function("Do you want to see your conduct?",
				  ynqchars, defquery) : defquery;
	    if (c == 'y')
		show_conduct(how >= PANICKED ? 1 : 2);
	    if (c == 'q') done_stopprint++;
	}
}


/* like disclose, but don't ask any questions */
static void dump_disclose(int how)
{
	/* temporarily redirect menu window output into the dumpfile */
	dump_catch_menus(TRUE);
	
	/* re-"display" all the disclosure menus */
	display_inventory(NULL, TRUE);
	container_contents(invent, TRUE, FALSE);
	dump_spells();
	dump_skills();
	enlightenment(how >= PANICKED ? 1 : 2); /* final */
	list_vanquished('y', FALSE);
	list_genocided('y', FALSE);
	show_conduct(how >= PANICKED ? 1 : 2);
	dooverview();
	dohistory();
	
	/* make menus work normally again */
	dump_catch_menus(FALSE);
}


/* try to get the player back in a viable state after being killed */
static void savelife(int how)
{
	u.uswldtim = 0;
	u.uhp = u.uhpmax;
	if (u.uhunger < 500) {
	    u.uhunger = 500;
	    newuhs(FALSE);
	}
	/* cure impending doom of sickness hero won't have time to fix */
	if ((Sick & TIMEOUT) == 1) {
	    u.usick_type = 0;
	    Sick = 0;
	}
	if (how == CHOKING) init_uhunger();
	nomovemsg = "You survived that attempt on your life.";
	flags.move = 0;
	if (multi > 0) multi = 0; else multi = -1;
	if (u.utrap && u.utraptype == TT_LAVA) u.utrap = 0;
	/* drowning monster still grabs, so at least give back the grace turn */
	u.uwilldrown = 0;

	iflags.botl = 1;
	u.ugrave_arise = NON_PM;
	HUnchanging = 0L;
	flush_screen();
}

/*
 * Get valuables from the given list.  Revised code: the list always remains
 * intact.
 */
static void get_valuables(struct obj *list)
{
    struct obj *obj;
    int i;

    /* find amulets and gems, ignoring all artifacts */
    for (obj = list; obj; obj = obj->nobj)
	if (Has_contents(obj)) {
	    get_valuables(obj->cobj);
	} else if (obj->oartifact) {
	    continue;
	} else if (obj->oclass == AMULET_CLASS) {
	    i = obj->otyp - FIRST_AMULET;
	    if (!amulets[i].count) {
		amulets[i].count = obj->quan;
		amulets[i].typ = obj->otyp;
	    } else amulets[i].count += obj->quan; /* always adds one */
	} else if (obj->oclass == GEM_CLASS && obj->otyp < LUCKSTONE) {
	    i = min(obj->otyp, LAST_GEM + 1) - FIRST_GEM;
	    if (!gems[i].count) {
		gems[i].count = obj->quan;
		gems[i].typ = obj->otyp;
	    } else gems[i].count += obj->quan;
	}
    return;
}

/*
 *  Sort collected valuables, most frequent to least.  We could just
 *  as easily use qsort, but we don't care about efficiency here.
 */
static void sort_valuables(struct valuable_data list[],
			   int size /* max value is less than 20 */)
{
    int i, j;
    struct valuable_data ltmp;

    /* move greater quantities to the front of the list */
    for (i = 1; i < size; i++) {
	if (list[i].count == 0) continue;	/* empty slot */
	ltmp = list[i]; /* structure copy */
	for (j = i; j > 0; --j)
	    if (list[j-1].count >= ltmp.count) break;
	    else {
		list[j] = list[j-1];
	    }
	list[j] = ltmp;
    }
    return;
}

/* called twice; first to calculate total, then to list relevant items */
static void artifact_score(struct obj *list,
			   boolean counting, /* true => add up points; false => display them */
			   struct menulist *menu)
{
    char pbuf[BUFSZ];
    struct obj *otmp;
    long value, points;
    short dummy;	/* object type returned by artifact_name() */

    for (otmp = list; otmp; otmp = otmp->nobj) {
	if (otmp->oartifact ||
			otmp->otyp == BELL_OF_OPENING ||
			otmp->otyp == SPE_BOOK_OF_THE_DEAD ||
			otmp->otyp == CANDELABRUM_OF_INVOCATION) {
	    value = arti_cost(otmp);	/* zorkmid value */
	    points = value * 5 / 2;	/* score value */
	    if (counting) {
		u.urscore += points;
	    } else {
		makeknown(otmp->otyp);
		otmp->known = otmp->dknown = otmp->bknown = otmp->rknown = 1;
		/* assumes artifacts don't have quan > 1 */
		sprintf(pbuf, "%s%s (worth %ld %s and %ld points)",
			the_unique_obj(otmp) ? "The " : "",
			otmp->oartifact ? artifact_name(xname(otmp), &dummy) :
				OBJ_NAME(objects[otmp->otyp]),
			value, currency(value), points);
		add_menutext(menu, pbuf);
	    }
	}
	if (Has_contents(otmp))
	    artifact_score(otmp->cobj, counting, menu);
    }
}


static boolean check_survival(int how, char *kilbuf)
{
	if (how == TRICKED) {
	    if (killer) {
		paniclog("trickery", killer);
		killer = 0;
	    }
	    if (wizard) {
		pline("You are a very tricky wizard, it seems.");
		return TRUE;
	    }
	}

	/* kilbuf: used to copy killer in case it comes from something like
	 *	xname(), which would otherwise get overwritten when we call
	 *	xname() when listing possessions
	 * pbuf: holds sprintf'd output for raw_print and putstr
	 */
	if (how == ASCENDED || (!killer && how == GENOCIDED))
		killer_format = NO_KILLER_PREFIX;
	/* Avoid killed by "a" burning or "a" starvation */
	if (!killer && (how == STARVING || how == BURNING))
		killer_format = KILLED_BY;
	/* Ignore some killer-strings, but use them for QUIT and ASCENDED */
	strcpy(kilbuf, how == PANICKED || how == TRICKED || how == ESCAPED ||
		       !killer ? deaths[how] : killer);
	killer = kilbuf;

	if (how < PANICKED) u.umortality++;
	if (Lifesaved && (how <= MAX_SURVIVABLE_DEATH)) {
		pline("But wait...");
		makeknown(AMULET_OF_LIFE_SAVING);
		pline("Your medallion %s!",
		      !Blind ? "begins to glow" : "feels warm");
		/* Keep it blessed, or this might happen! */
		if (uamul && uamul->cursed && rnf(1,4)) {
			pline("But... the chain on your medallion breaks and it "
			      "falls to the %s!", surface(u.ux,u.uy));
			You_hear("homeric laughter!");
			/* It already started to work.
			 * Too bad you couldn't hold onto it.
			 */
			useup(uamul);
		} else  {
			if (how == CHOKING) pline("You vomit ...");
			if (how == DISINTEGRATED) pline("You reconstitute!");
			else pline("You feel much better!");
			pline("The medallion crumbles to dust!");
			if (uamul) useup(uamul);

			adjattrib(A_CON, -1, FALSE, 1);
			if (u.uhpmax <= 0) u.uhpmax = 10;	/* arbitrary */
			savelife(how);
			if (how == GENOCIDED && is_playermon_genocided())
				pline("Unfortunately you are still genocided...");
			else {
				killer = 0;
				killer_format = 0;
				historic_event(FALSE, "were saved from death by "
						      "your amulet of life saving!");
				return TRUE;
			}
		}
	}
	if ((wizard || discover) && (how <= MAX_SURVIVABLE_DEATH)) {
		if (yn("Die?") == 'y')
		    return FALSE;
		pline("OK, so you don't %s.",
			(how == CHOKING) ? "choke" :
			(how == DISINTEGRATED) ? "disintegrate" :
			"die");
		if (u.uhpmax <= 0)
		    u.uhpmax = u.ulevel * 8;	/* arbitrary */
		savelife(how);
		killer = 0;
		killer_format = 0;
		historic_event(FALSE, "were saved from death by your wizard powers!");
		return TRUE;
	}
	
	return FALSE;
}


static long calc_score(int how)
{
	long umoney;
	long tmp;
	int deepest = deepest_lev_reached(FALSE);

	umoney = money_cnt(invent);
	tmp = u.umoney0;
	umoney += hidden_gold();	/* accumulate gold from containers */
	tmp = umoney - tmp;		/* net gain */

	if (tmp < 0L)
	    tmp = 0L;
	if (how < PANICKED)
	    tmp -= tmp / 10L;
	u.urscore += tmp;
	u.urscore += 50L * (long)(deepest - 1);
	if (deepest > 20)
	    u.urscore += 1000L * (long)((deepest > 30) ? 10 : deepest - 20);
	if (how == ASCENDED || how == DEFIED) u.urscore *= 2L;

	return umoney;
}


void display_rip(int how, char *kilbuf, char *pbuf, long umoney)
{
	char outrip_buf[BUFSZ];
	boolean show_endwin = FALSE;
	struct menulist menu;
	int saved_stopprint = done_stopprint;

	/* always show this under normal circumstances */
	done_stopprint = 0;

	init_menulist(&menu);

	/* clean up unneeded windows */
	if (program_state.game_running) {
	    win_pause_output(P_MESSAGE);

	    if (!done_stopprint || flags.tombstone)
		show_endwin = TRUE;

	    if ((how <= GENOCIDED || how == DISINTEGRATED) &&
		flags.tombstone && show_endwin) {
		/* Put together death description */
		switch (killer_format) {
		    default: warning("bad killer format?");
		    case KILLED_BY_AN:
			strcpy(outrip_buf, killed_by_prefix[how]);
			strcat(outrip_buf, an(killer));
			break;
		    case KILLED_BY:
			strcpy(outrip_buf, killed_by_prefix[how]);
			strcat(outrip_buf, killer);
			break;
		    case NO_KILLER_PREFIX:
			strcpy(outrip_buf, killer);
			break;
		}
	    }
	} else {
	    done_stopprint = 1;
	}

/* changing kilbuf really changes killer. we do it this way because
   killer is declared a (const char *)
*/
	if (u.uhave.amulet) strcat(kilbuf, " (with the Amulet)");
	else if (how == ESCAPED) {
	    if (Is_astralevel(&u.uz))	/* offered Amulet to wrong deity */
		strcat(kilbuf, " (in celestial disgrace)");
	    else if (carrying(FAKE_AMULET_OF_YENDOR))
		strcat(kilbuf, " (with a fake Amulet)");
		/* don't bother counting to see whether it should be plural */
	}

	if (!done_stopprint) {
	    sprintf(pbuf, "%s %s the %s...", Goodbye(), plname,
		   how != ASCENDED ?
		      (const char *) ((flags.female && urole.name.f) ?
		         urole.name.f : urole.name.m) :
		      (const char *) (flags.female ? "Demigoddess" : "Demigod"));
	    add_menutext(&menu, pbuf);
	    add_menutext(&menu, "");
	}

	if (how == ESCAPED || how == DEFIED || how == ASCENDED) {
	    struct monst *mtmp;
	    struct obj *otmp;
	    const struct val_list *val;
	    int i;

	    for (val = valuables; val->list; val++)
		for (i = 0; i < val->size; i++) {
		    val->list[i].count = 0L;
		}
	    get_valuables(invent);

	    /* add points for collected valuables */
	    for (val = valuables; val->list; val++)
		for (i = 0; i < val->size; i++)
		    if (val->list[i].count != 0L)
			u.urscore += val->list[i].count
				  * (long)objects[val->list[i].typ].oc_cost;

	    /* count the points for artifacts */
	    artifact_score(invent, TRUE, &menu);

	    keepdogs(1, NULL);	/* only pets follow you */
	    viz_array[0][0] |= IN_SIGHT; /* need visibility for naming */
	    mtmp = mydogs;
	    if (!done_stopprint) strcpy(pbuf, "You");
	    if (mtmp) {
		while (mtmp) {
		    if (!done_stopprint)
			sprintf(eos(pbuf), " and %s", mon_nam(mtmp));
		    if (mtmp->mtame)
			u.urscore += mtmp->mhp;
		    mtmp = mtmp->nmon;
		}
		if (!done_stopprint) add_menutext(&menu, pbuf);
		pbuf[0] = '\0';
	    } else {
		if (!done_stopprint) strcat(pbuf, " ");
	    }
	    if (!done_stopprint) {
		sprintf(eos(pbuf), "%s with %d point%s,",
			how==ASCENDED ? "went to your reward" :
			how==DEFIED   ? "defied the gods and escaped" :
					"escaped from the dungeon",
			u.urscore, plur(u.urscore));
		add_menutext(&menu, pbuf);
	    }

	    if (!done_stopprint)
		artifact_score(invent, FALSE, &menu);	/* list artifacts */

	    /* list valuables here */
	    for (val = valuables; val->list; val++) {
		sort_valuables(val->list, val->size);
		for (i = 0; i < val->size && !done_stopprint; i++) {
		    int typ = val->list[i].typ;
		    long count = val->list[i].count;

		    if (count == 0L) continue;
		    if (objects[typ].oc_class != GEM_CLASS || typ <= LAST_GEM) {
			otmp = mksobj(level, typ, FALSE, FALSE);
			makeknown(otmp->otyp);
			otmp->known = 1;	/* for fake amulets */
			otmp->dknown = 1;	/* seen it (blindness fix) */
			otmp->onamelth = 0;
			otmp->quan = count;
			sprintf(pbuf, "%8ld %s (worth %ld %s),",
				count, xname(otmp),
				count * (long)objects[typ].oc_cost, currency(2L));
			obfree(otmp, NULL);
		    } else {
			sprintf(pbuf,
				"%8ld worthless piece%s of colored glass,",
				count, plur(count));
		    }
		    add_menutext(&menu, pbuf);
		}
	    }

	} else if (!done_stopprint) {
	    /* did not escape or ascend */
	    if (u.uz.dnum == 0 && u.uz.dlevel <= 0) {
		/* level teleported out of the dungeon; `how' is DIED,
		   due to falling or to "arriving at heaven prematurely" */
		sprintf(pbuf, "You %s beyond the confines of the dungeon",
			(u.uz.dlevel < 0) ? "passed away" : ends[how]);
	    } else {
		/* more conventional demise */
		const char *where = dungeons[u.uz.dnum].dname;

		if (Is_astralevel(&u.uz)) where = "The Astral Plane";
		sprintf(pbuf, "You %s in %s", ends[how], where);
		if (!In_endgame(&u.uz) && !Is_knox(&u.uz))
		    sprintf(eos(pbuf), " on dungeon level %d",
			    In_quest(&u.uz) ? dunlev(&u.uz) : depth(&u.uz));
	    }

	    sprintf(eos(pbuf), " with %d point%s,",
		    u.urscore, plur(u.urscore));
	    add_menutext(&menu, pbuf);
	}

	if (!done_stopprint) {
	    sprintf(pbuf, "and %ld piece%s of gold, after %u move%s.",
		    umoney, plur(umoney), moves, plur(moves));
	    add_menutext(&menu, pbuf);
	}
	if (!done_stopprint) {
	    sprintf(pbuf,
	     "You were level %d with a maximum of %d hit point%s when you %s.",
		    u.ulevel, u.uhpmax, plur(u.uhpmax), ends[how]);
	    add_menutext(&menu, pbuf);
	    add_menutext(&menu, "");
	}
	if (!done_stopprint)
	    outrip(menu.items, menu.icount,
		   (how <= GENOCIDED || how == DISINTEGRATED), plname, umoney,
		   outrip_buf, how, getyear());

	free(menu.items);

	done_stopprint = saved_stopprint;
}


/* Be careful not to call panic from here! */
void done(int how)
{
	int fd;
	boolean taken;
	char pbuf[BUFSZ];
	boolean bones_ok;
	struct obj *corpse = NULL;
	long umoney;
	const char *dumpname;

	if (check_survival(how, killbuf))
	    return;
	
	/* replays are done here: no dumping or high-score calculation required */
	if (program_state.viewing)
	    terminate();
	
	/*
	 *	The game is now over...
	 */
	program_state.gameover = 1;
	
	/* don't do the whole post-game dance if the game exploded */
	if (how == PANICKED)
	    terminate();
	
	log_command_result();
	/* render vision subsystem inoperative */
	iflags.vision_inited = 0;
	/* might have been killed while using a disposable item, so make sure
	   it's gone prior to inventory disclosure and creation of bones data */
	inven_inuse(TRUE);

	/* Sometimes you die on the first move.  Life's not fair.
	 * On those rare occasions you get hosed immediately, go out
	 * smiling... :-)  -3.
	 */
	if (moves <= 1 && how < PANICKED)	/* You die... --More-- */
	    pline("Do not pass go.  Do not collect 200 %s.", currency(200L));

	bones_ok = (how < GENOCIDED) && can_make_bones(&u.uz);

	if (how == TURNED_SLIME)
	    u.ugrave_arise = PM_GREEN_SLIME;

	if (bones_ok && u.ugrave_arise < LOW_PM) {
	    /* corpse gets burnt up too */
	    if (how == BURNING)
		u.ugrave_arise = (NON_PM - 2);	/* leave no corpse */
	    else if (how == STONING)
		u.ugrave_arise = (NON_PM - 1);	/* statue instead of corpse */
	    else if (u.ugrave_arise == NON_PM &&
		     !(mvitals[u.umonnum].mvflags & G_NOCORPSE)) {
		int mnum = u.umonnum;

		if (!Upolyd) {
		    /* Base corpse on race when not poly'd since original
		     * u.umonnum is based on role, and all role monsters
		     * are human.
		     */
		    mnum = (flags.female && urace.femalenum != NON_PM) ?
			urace.femalenum : urace.malenum;
		}
		corpse = mk_named_object(CORPSE, &mons[mnum],
				       u.ux, u.uy, plname);
		sprintf(pbuf, "%s, %s%s", plname,
			killer_format == NO_KILLER_PREFIX ? "" :
			killed_by_prefix[how],
			killer_format == KILLED_BY_AN ? an(killer) : killer);
		make_grave(level, u.ux, u.uy, pbuf);
	    }
	}

	if (how == QUIT) {
		killer_format = NO_KILLER_PREFIX;
		if (u.uhp < 1) {
			how = DIED;
			u.umortality++;	/* skipped above when how==QUIT */
			/* note that killer is pointing at killbuf */
			strcpy(killbuf, "quit while already on Charon's boat");
		}
	}
	if (how == ESCAPED || how == DEFIED)
		killer_format = NO_KILLER_PREFIX;
	
	fd = logfile;
	log_finish(LS_DONE);
	/* write_log_toptenentry needs killer_format */
	write_log_toptenentry(fd, how);
	/* in case of a subsequent panic(), there's no point trying to save */
	program_state.something_worth_saving = 0;

	/* these affect score and/or bones, but avoid them during panic */
	taken = paybill((how == ESCAPED) ? -1 : (how != QUIT));
	paygd();
	clearpriests();

	win_pause_output(P_MESSAGE);

	if (how < PANICKED)
	    check_tutorial_message(QT_T_DEATH);

	if (flags.end_disclose != DISCLOSE_NO_WITHOUT_PROMPT)
	    disclose(how, taken);

	dumpname = begin_dump(how);
	dump_disclose(how);
	
	/* finish_paybill should be called after disclosure but before bones */
	if (bones_ok && taken) finish_paybill();

	/* calculate score, before creating bones [container gold] */
	umoney = calc_score(how);

	if (bones_ok) {
	    if (!wizard || (!done_stopprint && yn("Save bones?") == 'y'))
		savebones(corpse);
	    /* corpse may be invalid pointer now so
		ensure that it isn't used again */
	    corpse = NULL;
	}

	/* update gold for the rip output, which can't use hidden_gold()
	   (containers will be gone by then if bones just got saved...) */
	done_money = umoney;

	end_dump(how, killbuf, pbuf, umoney);
	display_rip(how, killbuf, pbuf, umoney);

	/* generate a topten entry for this game.
	   update_topten does not display anything. */
	update_topten(how, dumpname ? dumpname : "");

	terminate();
}


static void container_contents(struct obj *list, boolean all_containers,
			       boolean show_empty)
{
	struct obj *box, *obj;
	char buf[BUFSZ];
	int i, nr_items = 10, icount = 0;
	struct nh_objitem *items = malloc(nr_items * sizeof(struct nh_objitem));
	struct obj **objlist;

	for (box = list; box; box = box->nobj) {
	    unsigned saveknown = objects[box->otyp].oc_name_known;
	    objects[box->otyp].oc_name_known = 1;
	    if (Is_container(box) || box->otyp == STATUE) {
		if (box->otyp == BAG_OF_TRICKS && box->spe) {
		    continue;	/* charged bag of tricks can't contain anything */
		} else if (box->cobj) {
		    /* count contained objects */
		    icount = 0;
		    for (obj = box->cobj; obj; obj = obj->nobj) icount++;
		    objlist = malloc(icount * sizeof(struct obj*));
		    
		    /* add the objects to a list */
		    icount = 0;
		    for (obj = box->cobj; obj; obj = obj->nobj) {
			objlist[icount++] = obj;
		    }
		    
		    /* sort the list */
		    qsort(objlist, icount, sizeof(struct obj*), obj_compare);
		    
		    /* add the sorted objects to the menu */
		    for (i = 0; i < icount; i++)
			add_objitem(&items, &nr_items, MI_NORMAL, i, i+1,
			            doname(objlist[i]), objlist[i], FALSE);
		    
		    free(objlist);
		    sprintf(buf, "Contents of %s:", the(xname(box)));
		    display_objects(items, icount, buf, PICK_NONE, NULL);
		    if (all_containers)
			container_contents(box->cobj, TRUE, show_empty);
		} else if (!done_stopprint && show_empty) {
		    pline("%s empty.", Tobjnam(box, "are"));
		    win_pause_output(P_MESSAGE);
		}
	    }
	    objects[box->otyp].oc_name_known = saveknown;
	    if (!all_containers)
		break;
	}
	
	free(items);
}


void terminate(void)
{
	/* don't bother to try to release memory if we're in panic mode, to
	   avoid trouble in case that happens to be due to memory problems */
	if (!program_state.panicking && !program_state.viewing) {
	    freedynamicdata();
	    dlb_cleanup();
	}
	
	program_state.game_running = 0;
	if (!program_state.panicking) /* logging could be in disorder, ex: RO logfile */
	    log_finish(LS_IN_PROGRESS); /* didn't necessarily get here via done() */
	
	/* try to leave gracefully - this should return control to the ui code */
	if (exit_jmp_buf_valid) {
	    exit_jmp_buf_valid = 0;
	    nh_longjmp(exit_jmp_buf, 1);
	}

	/* no jmp_buf.
	 * This can only happen when an unguarded api function calls panic()
	 * This should not happen. */
	exit(1);
}

void list_vanquished(char defquery, boolean ask)
{
    int i, lev;
    int ntypes = 0, max_lev = 0, nkilled;
    long total_killed = 0L;
    char c;
    char buf[BUFSZ];
    struct menulist menu;

    /* get totals first */
    for (i = LOW_PM; i < NUMMONS; i++) {
	if (mvitals[i].died) ntypes++;
	total_killed += (long)mvitals[i].died;
	if (mons[i].mlevel > max_lev) max_lev = mons[i].mlevel;
    }

    /* vanquished creatures list;
     * includes all dead monsters, not just those killed by the player
     */
    if (ntypes != 0) {
	c = ask ? yn_function("Do you want an account of creatures vanquished?",
			      ynqchars, defquery) : defquery;
	if (c == 'q') done_stopprint++;
	if (c == 'y') {
	    init_menulist(&menu);

	    /* countdown by monster "toughness" */
	    for (lev = max_lev; lev >= 0; lev--)
	      for (i = LOW_PM; i < NUMMONS; i++)
		if (mons[i].mlevel == lev && (nkilled = mvitals[i].died) > 0) {
		    if ((mons[i].geno & G_UNIQ) && i != PM_HIGH_PRIEST) {
			sprintf(buf, "%s%s",
				!type_is_pname(&mons[i]) ? "The " : "",
				mons_mname(&mons[i]));
			if (nkilled > 1) {
			    switch (nkilled) {
				case 2:  sprintf(eos(buf)," (twice)");  break;
				case 3:  sprintf(eos(buf)," (thrice)");  break;
				default: sprintf(eos(buf)," (%d time%s)",
						 nkilled, plur(nkilled));
					 break;
			    }
			}
		    } else {
			/* trolls or undead might have come back,
			   but we don't keep track of that */
			if (nkilled == 1)
			    strcpy(buf, an(mons_mname(&mons[i])));
			else
			    sprintf(buf, "%d %s",
				    nkilled, makeplural(mons_mname(&mons[i])));
		    }
		    add_menutext(&menu,  buf);
		}

	    if (Hallucination)
		add_menutext(&menu, "and a partridge in a pear tree");

	    if (ntypes > 1) {
		add_menutext(&menu,  "");
		sprintf(buf, "%ld creatures vanquished.", total_killed);
		add_menutext(&menu,  buf);
	    }
	    display_menu(menu.items, menu.icount, "Vanquished creatures:", PICK_NONE, NULL);
	    free(menu.items);
	}
    }
}

/* number of monster species which have been genocided */
int num_genocides(void)
{
    int i, n = 0;

    for (i = LOW_PM; i < NUMMONS; ++i)
	if (mvitals[i].mvflags & G_GENOD) ++n;

    return n;
}

/* number of monster species which have been extincted */
int num_extinctions(void)
{
    int i, n = 0;

    for (i = LOW_PM; i < NUMMONS; i++)
	if ((mvitals[i].mvflags & G_GONE) && !(mons[i].geno & G_UNIQ))
	    n++;

    return n;
}

/* number of monster species which have been vanquished */
int num_vanquished(void)
{
    int i, n = 0;

    for (i = LOW_PM; i < NUMMONS; i++)
	if (mvitals[i].died)
	    n++;

    return n;
}

void list_genocided(char defquery, boolean ask)
{
    int i;
    int ngenocided, nextincted;
    char c, *query, *title;
    char buf[BUFSZ];
    struct menulist menu;

    ngenocided = nextincted = 0;
    for (i = LOW_PM; i < NUMMONS; ++i) {
	if (mvitals[i].mvflags & G_GENOD)
	    ngenocided++;
	if ((mvitals[i].mvflags & G_GONE) && !(mons[i].geno & G_UNIQ))
	    nextincted++;
    }

    /* genocided species list */
    if (ngenocided != 0 || nextincted != 0) {
	query = nextincted ? "Do you want a list of species genocided or extinct?" :
	                     "Do you want a list of species genocided?";
	c = ask ? yn_function(query, ynqchars, defquery) : defquery;
	if (c == 'q') done_stopprint++;
	if (c == 'y') {
	    init_menulist(&menu);
	    for (i = LOW_PM; i < NUMMONS; i++)
		if ((mvitals[i].mvflags & G_GENOD) ||
		    ((mvitals[i].mvflags & G_EXTINCT) && !(mons[i].geno & G_UNIQ))) {
		    if ((mons[i].geno & G_UNIQ) && i != PM_HIGH_PRIEST)
			sprintf(buf, "%s%s",
				!type_is_pname(&mons[i]) ? "" : "the ",
				mons_mname(&mons[i]));
		    else
			strcpy(buf, makeplural(mons_mname(&mons[i])));
		
		    if( !(mvitals[i].mvflags & G_GENOD) )
			strcat(buf, " (extinct)");
		    add_menutext(&menu, buf);
		}

	    if (nextincted > 0 && aprilfoolsday()) {
		add_menutext(&menu, "ammonites (extinct)");
		nextincted++;
	    }

	    add_menutext(&menu, "");
	    sprintf(buf, "%d species genocided.", ngenocided);
	    if (ngenocided)
		add_menutext(&menu, buf);
	    sprintf(buf, "%d species extinct.", nextincted);
	    if (nextincted)
		add_menutext(&menu, buf);

	    title = nextincted ? "Genocided or extinct species:" :
	                         "Genocided species:";
	    display_menu(menu.items, menu.icount, title, PICK_NONE, NULL);
	    free(menu.items);
	}
    }
}

/*end.c*/
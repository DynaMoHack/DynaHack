/* Copyright (c) D. Cohrs, 1993. */
/* NitroHack may be freely redistributed.  See license for details. */

#include "hack.h"

struct nh_window_procs windowprocs;

int getpos(coord *cc, boolean force, const char *goal)
{
	int rv = -1, x, y;
	
	x = cc->x;
	y = cc->y;
	
	flush_screen();
	
	do {
	    rv = (*windowprocs.win_getpos)(&x, &y, force, goal);
	} while (force && (rv == -1 || x < 1 || y < 1 || x > COLNO || y > ROWNO));
	log_getpos(rv, x, y);
	
	cc->x = x;
	cc->y = y;
	
	return rv;
}


int getdir(const char *s, schar *dx, schar *dy, schar *dz)
{
	const char *query = s ? s : "In what direction?";
	boolean restricted = u.umonnum == PM_GRID_BUG;
	enum nh_direction dir = (*windowprocs.win_getdir)(query, restricted);
	log_getdir(dir);
	
	*dz = 0;
	if (!dir_to_delta(dir, dx, dy, dz))
		return 0;
	
	if (*dx && *dy && u.umonnum == PM_GRID_BUG) {
		*dx = *dy = 0;
		return 0;
	}
	
	if (!*dz && (Stunned || (Confusion && !rn2(5))))
		confdir(dx, dy);
	
	return 1;
}


char query_key(const char *query, int *count)
{
	char key;
	key = (*windowprocs.win_query_key)(query, count);
	log_query_key(key, count);
	return key;
}


void getlin(const char *query, char *bufp)
{
	(*windowprocs.win_getlin)(query, bufp);
	log_getlin(bufp);
}


/*
 *   Parameter validator for generic yes/no function to prevent
 *   the core from sending too long a prompt string to the
 *   window port causing a buffer overflow there.
 */
char yn_function(const char *query,const char *resp, char def)
{
	char qbuf[QBUFSZ], key;
	unsigned truncspot, reduction = sizeof(" [N]  ?") + 1;

	if (resp)
	    reduction += strlen(resp) + sizeof(" () ");
	
	if (strlen(query) >= (QBUFSZ - reduction)) {
	    paniclog("Query truncated: ", query);
	    reduction += sizeof("...");
	    truncspot = QBUFSZ - reduction;
	    strncpy(qbuf, query, (int)truncspot);
	    qbuf[truncspot] = '\0';
	    strcat(qbuf,"...");
	} else
	    strcpy(qbuf, query);
	
	key = (*windowprocs.win_yn_function)(qbuf, resp, def);
	log_yn_function(key);
	return key;
}


/*
 *   Ask the player a yes/no question if paranoid is true.
 *   This calls getlin or yn, so this needs no special log support.
 *   Returns 'y' or 'n'.
 */
char paranoid_yn(const char *query, boolean paranoid)
{
	if (paranoid) {
	    char buf[BUFSZ];
	    char query_yesno[2 * BUFSZ];
	    /* put [yes/no] between question and question mark? */
	    sprintf(query_yesno, "%s [yes/no]", query);
	    getlin(query_yesno, buf);
	    lcase(buf);
	    return !(strcmp (buf, "yes")) ? 'y' : 'n';
	} else
	    return yn(query);
}


int display_menu(struct nh_menuitem *items, int icount, const char *title,
		 int how, int *results)
{
	int n;
	n = (*windowprocs.win_display_menu)(items, icount, title, how, results);
	if (how != PICK_NONE)
	    log_menu(n, results);
	return n;
}


int display_objects(struct nh_objitem *items, int icount, const char *title,
		    int how, struct nh_objresult *pick_list)
{
	int n;
	n = (*windowprocs.win_display_objects)(items, icount, title, how, pick_list);
	if (how != PICK_NONE)
	    log_objmenu(n, pick_list);
	return n;
}


boolean win_list_items(struct nh_objitem *items, int icount, boolean is_invent)
{
    if (!windowprocs.win_list_items)
	return FALSE;
    
    return (*windowprocs.win_list_items)(items, icount, is_invent);
}


/*windows.c*/

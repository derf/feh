/* feh_init.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.
Copyright (C) 2012      Christopher Hrabak        LastUpdt Jan 8,2013  cah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


June 2012  Hrabak rewrote the keyevents.c logic.
See newkeyev.h, feh_initpre.c, feh_init.c and feh_init.h for more detail.

* This is the feh_init.c module, which compiles into a small standalone routine
* that is called from within feh each time the external "keys" file changes.
* feh_init.c pre-parses the keybindings, saving the result to a binary file
* called feh_bindings.map.  feh reads that .map file, thus avoiding ALL key
* parsing at runtime.  feh_init also creates an HTML help file, displayable
* within feh, that shows the keybindings (including any remapped bindings
* from the external "keys" file) with descriptions.  See the feh_init.h header
* for all the gory details.
*
* feh_init.c depends on the stubptr.c module which is created by (another)
* standalone (even smaller) routine called feh_initpre.c.  feh_initpre.c
* runs whenever the default keybindings (stored in feh_init.h) changes.
* Makefile takes care of all these dependancies - running feh_initpre to gen
* stubptr.c B4 compiling feh_init.c.
* Originally Hrabak used a bash script to do this (pre) feh_init stuff, but
* moved it all into feh_initpre.c, encapsulating the logic in one place.
*
*/


#include "feh.h"
#include "feh_init.h"

/* ***************** *
 * Global variables  *
 * ***************** */

/* save all the binding array states here, bs is short for bindings_state */
struct _binding_state bs = { .ss_last=ACT_SUBSET_SIZE-1, .acnt[3]=ACT_SUBSET_SIZE } ;

SubsetAction act_subset[ ACT_SUBSET_SIZE  ];                  /* subset of feh_actions */
Key2Action *menu_actions, *feh_actions, *move_actions;        /* will be malloc'd later */

/* Needed JUST for bind_path in find_config_files() */
feh_global_vars fgv;              /* 'fgv' stands for FehGlobalVars */
extern SimplePtrmap fname[];
extern StubPtr funcptr[];
extern int fname_cnt;             /* from stub_ptr.c                */

/* ***************** *
 * Begin functions   *
 * ***************** */

int write_map2disc() {
    /* called from main().  Takes the four arrays of structs that hold menu_, move_, feh_
     * and act_subset binding lists, and fwrites() them out to disk in binary fmt.
     * This allows the feh runtime to fread() them without any further processing.
     * The four global bs.acnt[]  values hold the size of the arrays to fwrite()
     * so no need to shrink those array before the fwrite() (as I did in the orig code).
     * The external file is in binary fmt, named feh_bindings.map and stored either in
     * the ~/.config/feh dir, or the /etc/feh/, where ever the external keys binding file
     * was located.  If no keys file, then store in ~/.config/feh/.
     */

  int iErr=-1;
  size_t size, len;
  FILE *fs=NULL;
  char *home = NULL;
  int map_info[4];
  char *buf = mobs(1);
  struct stat st;

  /* find the .map file, else keep drilling till you find SOMETHING. */
  if ( find_config_files( FEH_BINDINGS_MAP , buf, &st ) ) {
      if ( find_config_files( FEH_EXTERNAL_KEYS_FILE , buf, &st ) ) {
          if ( find_config_files(".", buf, &st ) ) {
              /* not found, so try making a new .config dir? */
              home = getenv("HOME");
              if (home) {       /* atleast we have a home dir defined */
                  buf[0]=buf[1]='\0';
                  strcat(buf , home);
                  strcat(buf , "/.config/feh/" );
                  if ( mkdir(buf, S_IRWXU) ==0 )
                      return (iErr);     /* let the caller call again */
                  else  eprintf("Not able to create ", buf ,".\n");
              } else eprintf("No HOME in environment\n");
          }
      }
  }

  /* buf[] has the full pathname of where to store the .map file */
  home=strrchr( buf, '/' );
  if (strcmp(home+1, FEH_BINDINGS_MAP ) !=0 ) {
      home[1]=home[2]='\0';
      strcat(buf , FEH_BINDINGS_MAP );
  }

  fs=fopen( buf, "w" );
  if (! fs)  eprintf("Could not open " , &buf ,"\n");

  /* create the map_info header first; an array of four (int) in enum() order */
  size=1;
  map_info[0]=bs.acnt[MENU_PTR];
  map_info[1]=bs.acnt[MOVE_PTR];
  map_info[2]=bs.acnt[FEH_PTR];
  map_info[3]=bs.acnt[SUB_PTR];
  len=fwrite(map_info, sizeof(map_info), size, fs);
  if (len != size ) return (iErr*2);

  /* menu_actions[] */
  size=bs.acnt[MENU_PTR] ;
  len=fwrite(menu_actions, sizeof( Key2Action ), size , fs);
  if (len != size ) return (iErr*3);

  /* move_actions[] */
  size=bs.acnt[MOVE_PTR] ;
  len=fwrite(move_actions, sizeof( Key2Action ), size , fs);
  if (len != size ) return (iErr*4);

  /* feh_actions[] */
  size=bs.acnt[FEH_PTR] ;
  len=fwrite(feh_actions, sizeof( Key2Action ), size , fs);
  if (len != size ) return (iErr*4);

  /* act_subset[] */
  size=bs.acnt[SUB_PTR] ;
  len=fwrite(act_subset, sizeof( SubsetAction ), size , fs);
  if (len != size ) return (iErr*5);

  fclose (fs);

  return ( 0 );

}   /* end of write_map2disc() */


void make_subset( void ) {
    /* called from main(). Loads the act_subset[] with a sampling of elements
     * from feh_actions[]; consisting of 1st:10%-90%:last.  For small sets (under
     * 20(?), a linear search is faster than a bsearch().  so act_subset[]
     * gets searched first to figure out which 10% section if the full
     * feh_actions[] to search. */

  int i=(bs.acnt[FEH_PTR]/bs.ss_last)+1;      /* calc tenths of feh_actions, actually 1/subset_size */
  int j;                                      /* index for act_subset[] */
  int k;                                      /* index for feh_actions */

  /* this would be the place to strip out any NULL actions caused by the keys file
   * passing in a "DISABLE_ME" action.*/

  /* sort feh_ in symstate order */
  qsort( feh_actions ,bs.acnt[FEH_PTR],sizeof(feh_actions[0]),  cmp_symstate );

  /* save first element and one element every 10% of the total */
  for (k=0, j=0; k<bs.acnt[FEH_PTR]; k+=i, j++ ) {
    act_subset[j].symstate=feh_actions[k].symstate;
    act_subset[j].ptr=feh_actions[k].ptr;
    act_subset[j].which=k;
  }

  /* and catch the last one */
  act_subset[bs.ss_last].symstate=feh_actions[bs.acnt[FEH_PTR]-1].symstate;
  act_subset[bs.ss_last].ptr=feh_actions[bs.acnt[FEH_PTR]-1].ptr;
  act_subset[bs.ss_last].which=bs.acnt[FEH_PTR]-1;


  /* sort menu_ in symstate order */
  qsort( menu_actions,bs.acnt[MENU_PTR],sizeof(menu_actions[0]), cmp_symstate );

  /* sort move_ in symstate order */
  qsort( move_actions,bs.acnt[MOVE_PTR],sizeof(move_actions[0]), cmp_symstate );

}   /* end of make_subset() */


int cmp_symstate( const void *left, const void *right ){
    /* called by qsort to put the feh_actions[] in symstate order. */
    const Key2Action *l= ( const Key2Action *) left;
    const Key2Action *r= ( const Key2Action *) right;
    return (int)( l->symstate - r->symstate );
}

int cmp_acttok( const void *left, const void *right ){
    /* called by qsort to put the hlp.list in action name order */
    return (int)( strcmp( *(char **)left, *(char **)right ));
}

StubPtr find_stubptr( char * action ){
    /* called by parse_bindings() to resolve the function ptr associated with
     * this action. Actually I cannot resolve the function ptr at this time
     * cause those are in the real feh program.  This is just the feh_init
     * bindings load.  So what I do is maintain a parallel array of all the
     * action names (in fname[]) and the addresses of the function ptrs
     * (in funcptr[]).  Looking up the action name in fname[] and assigning
     * that INDEX number in the .ptr member allows init_bindings() logic in
     * feh to simply grab the correct function ptr with a simple ...
     *     feh_actions[i].ptr=functptr[feh_actions[i].ptr];
     * This requires me to CHEAT in the feh_init load; storing an (int) where
     * a StubPtr should be stored - and doing the reverse in the
     * init_bindings() code.  Hense the cast.
     */

  int i=0, j=0, slen=strlen( action );

  /* exhaustively search the fname[] array to find this action string
   * All searches should be successfull else somebody goofed on input.
   */

  for ( ;i < fname_cnt; i++){
    /* check string length B4 a compare() */
    if ( fname[i].len != slen ) continue;

    /* manually compare char-by-char */
    for ( j=0; ; j++ ) {
      if ( j==slen )
        return ( (StubPtr) i );    /* complete match, but I am just saving the index */
      if ( fname[i].action[j]==action[j] )
        continue ;                 /* still matching */
      else break;
    }
  }

  /* nothing should get here */
  weprintf("fname[]: no function found for %s", action );
  eprintf("BUG!\n");
  return (StubPtr)999;

}   /* end of find_stubptr()  */



int parse_bindings( char *action, char *ks ) {
    /* called by feh_init::make_feh_bindings().
     * returns 0 if a new action was added to bindings.
     * hlp.list saves the ACTUAL bindings for a help display.
     */

  /* code pulled from old keyevents.c::feh_set_parse_kb_partial() */
	char *cur = ks;
	int state = 0, keysym=0 ,  which ;
	unsigned int symstate;
  Key2Action *iptr;        /* to loop thru menu_ and feh_actions  */


	if (!*ks) { return -1; } /* should I allow a null keybinding to KILL this action? */

  while (cur[1] == '-') {
		switch (cur[0]) {
			case 'A':
				state |= AltMask;
				break;
			case 'C':
				state |= ControlMask;
				break;
			case 'S':
				state |= ShiftMask;
				break;
			default:
				weprintf("keys: invalid modifier %c in %s", cur[0], ks);
				break;
		}
		cur += 2;
	}

  /* what is leftover is the actual keysym */
	keysym = XStringToKeysym(cur);

	if (keysym == NoSymbol) {
		weprintf("keys: Invalid keysym: %s ", cur);
		/*weprintf("keys: Given a hex key code like 0x25 Try %s", XKeysymToString( 0x25 ));*/
    return -1;
  }

  /* no errors get past this point */

  symstate=(keysym << 8) + state;             /* this is HRABAK's combo symstate code */

  /* menu_ stuff 'must' begin with 'menu_'.  Same with 'move_' stuff
   * here is where I would trap the "menu_DISABLE_ME" action codes.
   */
  if ( action[0]=='m' && action[1]=='e' && action[2]=='n' && action[3]=='u' && action[4]=='_' ){
    which=MENU_PTR; iptr=menu_actions;
  } else if ( action[0]=='m' && action[1]=='o' && action[2]=='v' && action[3]=='e' && action[4]=='_' ){
    which=MOVE_PTR; iptr=move_actions;
  } else {
    which=FEH_PTR;  iptr=feh_actions;
  }

  /* external "keys" bindings are first, so check each successive entry for a dupe */
  for ( ;iptr<bs.last[which]; iptr++)
    if ( iptr->symstate==symstate) {
        /* ignore any duplicate bindings but warn */
        weprintf("Attempt to bind \"%s\" twice. <%s> ignored. Fix this! ",  ks, action  );
        return -1;
    }

  /* symstate not found, so OK to add a new one.  Should I be checking action?*/
  bs.last[which]->symstate=symstate;
  bs.last[which]->ptr=find_stubptr( action );
  bs.last[which]++;                             /* get ready for the next one */

  return 0;

} /* end of parse_bindings() */

int make_feh_bindings( void ) {
    /* called by feh_init::main() to load all the keybindings.
     * Combines external "keys" and default keybindings (both share the
     * same format), mashed all together.  The external bindings load first,
     * superceeding any defaults of the same name (FIFO).
     * This was originally called init_bindings(), and the code is
     * inherited from that function, but heavily hacked.
     */

  /* code pulled from old keyevents.c */
  char action[128], k1[32];
	char *line = mobs(2);               /* SIZE_256 */
	FILE *fs = NULL;

  int i = 0, menu_cnt=0, move_cnt=0, feh_cnt=0, read;
  int dflt_size = 0 ;
  char *tmp, *tmp_ptr, *key;
  struct stat st;

  /* count how many menu_, move_ and feh_ bindings we have */
  hlp.dflt_cnt = DFLT_KB_CNT ;

  for ( i=0; i < hlp.dflt_cnt; i++ ){
    tmp  = dflt_kb[i];
    dflt_size += strlen( tmp );        /* total of all bindings */
    if ( tmp[0]=='m' && tmp[1]=='o' && tmp[2]=='v' && tmp[3]=='e' && tmp[4]=='_' )
        move_cnt++;
    else if ( tmp[0]=='m' && tmp[1]=='e' && tmp[2]=='n' && tmp[3]=='u' && tmp[4]=='_' )
        menu_cnt++;
    else
        feh_cnt++;
  }

  /* save the ACTUAL bindings to the hlpname[], assuming 2 * dflt_size */
  read = dflt_size * 2;
  hlp.names = emalloc( sizeof(char) * read );
  memset(hlp.names,'\0', read );
  hlp.ptr = hlp.names;                /* will follow the trailing '\0' char */
  hlp.cnt = 0;

  /* do we have an external "keys" file? */
	if ( find_config_files( FEH_EXTERNAL_KEYS_FILE , line, &st ) == 0 ) {
    if ( ( fs=fopen( line, "r" )) == NULL )
      weprintf("%s to load %s.", ERR_FAILED,line );
  }


  /* if we have an external keys file, double the array sizes */
  if (fs ) i = 2; else i = 1;

  read = sizeof(Key2Action) * menu_cnt * i;
  menu_actions=emalloc( read );
  memset(menu_actions,'\0', read );
  read = sizeof(Key2Action) * move_cnt * i;
  move_actions=emalloc( read );
  memset(move_actions,'\0', read );
  read = sizeof(Key2Action) * feh_cnt  * i;
  feh_actions=emalloc( read );
  memset(feh_actions,'\0', read );

  /* position the ptrs in bs.last[] to the start of _actions[] arrays */
  bs.last[MENU_PTR] = menu_actions;
  bs.last[FEH_PTR]  = feh_actions;
  bs.last[MOVE_PTR] = move_actions;

  /* run thru the external bindings BEFORE the dflt_kb[] (FIFO) */
  if ( fs ) {
    *k1 = '\0';
    while (fgets(line, SIZE_256, fs)) {
      read = sscanf(line, "%s : %s \n",(char *) &action, (char *) &k1);
      if ((read == EOF) || (read == 0) || (line[0] == '#'))
        continue;
      if ( parse_bindings( action, k1 ) == 0 ) {
          /* add this to the hlp.list, where each entry is action'\0'k1'\0'*/
          hlp.list[ hlp.cnt ++ ] = hlp.ptr ;
          hlp.ptr = stpcpy( hlp.ptr, action);
          hlp.ptr++ ;                /* embed the NULL inside the pair */
          hlp.ptr = stpcpy( hlp.ptr, k1 );
          hlp.ptr++ ;                /* move beyond the last '\0' */
      }
    }
    fclose(fs);
  }

  /* then tack on the dflt_kb[] bindings */
  for ( i=0; i<hlp.dflt_cnt; i++ ){
      strcpy(action, dflt_kb[i]);
      if ( ( key = strchr(action, ':')) )
          *key = '\0';                /* truncates action */
      else {
          weprintf("Oops!  dflt_kb[ %d ] looks bad (%s)", i, dflt_kb[i]);
          continue;
      }
      key++;                                /* move past the '\0' */
      /* check if we have a reMapped funcptr */
      if ( (tmp_ptr = strchr( key, ':' )) )
          *tmp_ptr = '\0';                  /* truncates key */

      if ( parse_bindings( action, key ) == 0 ) {
          /* add this to the hlp.list, where each entry is action'\0'key'\0'*/
          hlp.list[ hlp.cnt ++ ] = hlp.ptr ;
          hlp.ptr = stpcpy( hlp.ptr, action );
          hlp.ptr++ ;                /* embed the NULL inside the pair */
          hlp.ptr = stpcpy( hlp.ptr,key );
          hlp.ptr++ ;                /* move beyond the last '\0' */
      }
  }

  /* Save the final counts for each _actions[] array */
  bs.acnt[MENU_PTR]=(bs.last[MENU_PTR]-menu_actions);
  bs.acnt[FEH_PTR] =(bs.last[FEH_PTR] -feh_actions);
  bs.acnt[MOVE_PTR]=(bs.last[MOVE_PTR]-move_actions);

  /* sort the hlp.list in acttok:keytok order */
  qsort( hlp.list ,hlp.cnt,sizeof(char*),  cmp_acttok );

  return 0;

}  /* end of make_feh_bindings() */

int make_feh_help( void ){
    /* create the feh_bindings.htm help file by merging content from
     * feh_bindings.desc (which holds descriptions for ALL possible feh
     * bindings), with the feh_bindings.htm.orig file which holds general
     * help (user editable).  The final file holds this general help plus
     * JUST the list of bindings in this feh build, including any remapped
     * keys (as found in the feh_bindings.conf file).
     */
  FILE *fs_org=NULL, *fs_hlp=NULL, *fs_desc=NULL;
  bhs_node **aHash;
  int i, aflag = 80;              /* bhs_hash does not grow, so guess well :) */
  char *hpath, *action, *desc, *ptr, *stop, *trail_ptr, *key_ptr, *line = mobs(2);
  char *big_buf = emalloc( sizeof(char) * 10240);

  ptr = big_buf;                  /* ptr to move along big_buf[] */

  /* open the .orig file */
  hpath = big_buf;
  STRCAT_2ITEMS(hpath, fgv.bind_path, FEH_BINDINGS_HELP_ORIG);
  if ( !( fs_org=fopen( hpath, "r" )) ){
      weprintf("%sload %s:",ERR_CANNOT, hpath );    /* this is a show stopper */
      return -1;
  }

  /* make an empty .htm file */
  STRCAT_2ITEMS(hpath, fgv.bind_path, FEH_BINDINGS_HELP);
  if ( !( fs_hlp=fopen( hpath, "w" )) ) {
      weprintf("%sload %s:",ERR_CANNOT, hpath );    /* this is a show stopper */
      return -1;
  }

  /* Load the entire .desc (ALL binding descriptions) file w/o comments.
   * If they are missing, I can still make a (minimal) help file.
   * NB: every line in big_buf is '\0' terminated.
   */
  STRCAT_2ITEMS(hpath, fgv.bind_path, FEH_BINDINGS_DESC );
  if (  ( fs_desc=fopen( hpath, "r" )) ) {
      big_buf[0]=big_buf[1]='\0';              /* load everything in here */
      while (fgets(ptr, SIZE_256, fs_desc)) {
        if ( ptr[0]=='#' ) continue;           /* ignore comment lines    */
        ptr = strrchr(ptr, '\0') +1 ;          /* move ptr PAST the '\0'  */
      }
      fclose(fs_desc);
  } else
      big_buf[0]='\0';

  /* if we have anything loaded from the .desc file, loop thru big_buf
   * and load the action:desc pairs into hash table.
   */
  if (big_buf[0] != '\0' ){
      /* got data, so parse it and hashit */
      aHash = bhs_hash_init( aflag );
      stop = ptr;                           /* where we ended in b_buf */
      trail_ptr = ptr = big_buf;            /* start both ptrs at beg  */
      aflag=0;                              /* flag when action starts */
      action = desc = NULL;
      for ( ; ptr < stop; ptr++ ){
        if ( ptr[0] == '\\' ){
            ptr[0] = '\0';
            if ( desc ) {                   /* closing out a desc    */
              /* hash the previous one */
              *++trail_ptr = ptr[0];        /* this is the real tail */
              if ( bhs_hash_get( aHash , action, desc, ADDIT_YES ) == -1 )
                weprintf("Oops. Could not hash %s : %s.", action, desc );
              ptr++;                        /* move ahead of '\0'    */
              action = ptr;                 /* begin new action      */
              desc = NULL;                  /* ready for new desc    */
              aflag = 1;                    /* currently on action   */
            } else if ( !aflag ) {          /* first time thru       */
              ptr++;                        /* move ahead of '\0'    */
              action = ptr;                 /* begin new action      */
              desc = NULL;                  /* ready for new desc    */
              aflag = 1;                    /* currently on action   */
            } else aflag = 0;               /* closing out an action */
        } else if ( aflag ) {
            continue ;                      /* actions should be clean */
        } else if ( ! desc ) {
            /* can ignore all these preceeding a new desc */
            if ( ptr[0]=='\n' || ptr[0]=='\0' || ptr[0]=='\t' || ptr[0]==' ' ) continue;
            trail_ptr = desc = ptr;         /* starts a new desc     */
        } else {                            /* we are in a desc      */
            if ( ptr[0]=='\n' || ptr[0]=='\0' || ptr[0]=='\t' ) continue;
            *++trail_ptr = ptr[0];          /* close any gaps        */
        }
      }   /* end of looping on ptr */
  }       /* end of big_buf test   */

  /* I would much rather this be an inline function */
#define GET_DESCRIPTION       if ( bhs_hash_get( aHash , action, NULL, ADDIT_NO ) == 0 ){ \
                                /* found it */  \
                                fprintf(fs_hlp, "</ol> <li> %s : %s </li><ol>\n", \
                                " Description", /* was action */ \
                                aHash[aHash[0]->h0.which]->h1.data); \
                              }


  /* write out the new .htm file WITH all the real bindings'
   * Reuse the *action ptr to hold the PREVIOUS action.
   */
  action = hlp.list[0];                     /* load the first one */
  while ( fgets(line, SIZE_256, fs_org) ) {
    if ( line[0] == '#' ){
        /* is this the trigger to merge in the hlp.list[] ? */
        i = strlen( TRIGGER_TO_INCLUDE_HELP );
        if ( ( strncmp( line, TRIGGER_TO_INCLUDE_HELP, i  ) == 0 ) ){
            /* yes.  So loop thru hlp.list and match these with
             * those descs in the hash table.  */
            fprintf(fs_hlp, "<ol>\n");          /* numbered, use <ul> for bullet list */
            for ( i=0; i<hlp.cnt; i++ ){
                if ( strcmp( action , hlp.list[i] ) != 0 ){
                    /* get descs only when action changes */
                    GET_DESCRIPTION             /* inline function */
                    action = hlp.list[i];       /* reset it        */
                }
                key_ptr = strrchr( hlp.list[i], '\0' );
                fprintf(fs_hlp, " <li> %s : %s </li>\n", hlp.list[i], key_ptr +1 );
            }
            GET_DESCRIPTION                     /* catch the last one */
            fprintf(fs_hlp, "</ol>\n");
        } else
            fprintf(fs_hlp, "%s\n", line);      /* just copy it       */

    } else {
        /* just repeat whatever else is in the .orig file */
        fprintf(fs_hlp, "%s\n", line);
    }
  }

  /* all done */
  fclose(fs_hlp);
  fclose(fs_org);

  return 0;

}   /* end of make_feh_help() */

int main( int argc, char **argv) {
    /* feh shells out to call this to rebuild a feh_keybindings.map file,
     * and passes a single arguement to indicate "NoTest" meaning don't
     * run the test_all_funcptr().  Running feh_init from the command line
     * would not have this flag, so the test runs (default mode ).
     */

  /* load external "keys" and default bindings */
  make_feh_bindings();

  /* making a subset of the feh_actions[] array for faster runtime access */
  make_subset();

  /* write the finished bindings (in binary fmt) to feh_bindings.map */
  if ( write_map2disc() != 0 ) {
      /* write_map2disc() tried to make a new .config dir, so try again to see if it worked */
      if ( write_map2disc() != 0 )
          eprintf("Pls create a $HOME/.config/feh/ directory!!!\n");
  }

  /* read it back again just to be sure it is OK */
  if (read_map_from_disc() !=0 ){
      /* bad news.  feh cannot run without this map */
      eprintf("The pre-built '%s' %sbe loaded.\n", FEH_BINDINGS_MAP ,ERR_CANNOT);
  }

  /* create a new feh_help.htm file that reflects the real keybindings.
   * Both the default AND any that were remapped (or disabled).
   */
  if ( make_feh_help() == 0 )
      printf( "'%s' created.\n",FEH_BINDINGS_HELP  );
  else
      weprintf("Oops!  The '%s' %sbe created.", FEH_BINDINGS_HELP, ERR_CANNOT );

  /* for testing run thru (and call) all the funcptr[]s
   * Note:  I call this with NoTest arguement to suppress
   * this output when called from feh. */
  if (argc < 2 && *argv) test_all_funcptr();


  return 0;

}     /* end of main() */


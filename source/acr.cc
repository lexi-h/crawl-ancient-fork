/*
 *  File:       acr.cc
 *  Summary:    Main entry point, event loop, and some initialization functions
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 * <18> 7/29/00         JDJ             values.h isn't included on Macs
 * <17> 19jun2000       GDL             added Windows console support
 * <16> 06mar2000       bwr             changes to berserk
 * <15> 09jan2000       BCR             new Wiz command: blink
 * <14> 10/13/99        BCR             Added auto door opening,
 *                                       move "you have no
 *                                       religion" to describe.cc
 * <13> 10/11/99        BCR             Added Daniel's wizard patch
 * <12> 10/9/99         BCR             swapped 'v' and 'V' commands,
 *                                       added wizard help command
 * <11> 10/7/99         BCR             haste and slow now take amulet of
 *                                       resist slow into account
 * <10> 9/25/99         CDL             Changes to Linux input
 *                                       switch on command enums
 * <9>  6/12/99         BWR             New init code, restructured
 *                                       wiz commands, added equipment
 *                                       listing commands
 * <8>   6/7/99         DML             Autopickup
 * <7>  5/30/99         JDJ             Added game_has_started.
 * <6>  5/25/99         BWR             Changed move() to move_player()
 * <5>  5/20/99         BWR             New berserk code, checking
 *                                       scan_randarts for NO_TELEPORT
 *                                       and NO_SPELLCASTING.
 * <4>  5/12/99         BWR             Solaris support.
 * <3>  5/09/99         JDJ             look_around no longer prints a prompt.
 * <2>  5/08/99         JDJ             you and env are no longer arrays.
 * <1>  -/--/--         LRH             Created
 */

#include "AppHdr.h"

#include <string>

// I don't seem to need values.h for VACPP..
#if !defined(__IBMCPP__) && !defined(MAC)
#include <values.h>
#endif

#ifdef DEBUG
  // this contains the DBL_MAX constant
  #include <float.h>
#endif

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#ifdef DOS
#include <conio.h>
#include <file.h>
#endif

#ifdef USE_UNIX_SIGNALS
#include <signal.h>
#endif

#ifdef USE_EMX
#include <sys/types.h>
#endif

#ifdef MAC
#include <stat.h>
#else
#include <sys/stat.h>
#endif

#include "externs.h"

#include "abl-show.h"
#include "abyss.h"
#include "chardump.h"
#include "command.h"
#include "debug.h"
#include "describe.h"
#include "direct.h"
#include "effects.h"
#include "fight.h"
#include "files.h"
#include "food.h"
#include "hiscores.h"
#include "initfile.h"
#include "invent.h"
#include "it_use3.h"
#include "item_use.h"
#include "itemname.h"
#include "items.h"
#include "lev-pand.h"
#include "misc.h"
#include "monplace.h"
#include "monstuff.h"
#include "mon-util.h"
#include "mutation.h"
#include "newgame.h"
#include "ouch.h"
#include "output.h"
#include "overmap.h"
#include "player.h"
#include "religion.h"
#include "skills2.h"
#include "spell.h"
#include "spells.h"
#include "spells1.h"
#include "spells3.h"
#include "spells4.h"
#include "spl-book.h"
#include "spl-util.h"
#include "stuff.h"
#include "tags.h"
#include "transfor.h"
#include "view.h"
#include "wpn-misc.h"

#ifdef MACROS
#include "macro.h"
#endif

// This macro converts capital letters into the mystic numbers.
// Used in switch statements so it has to be a macro. -- bwr
#define Control(xxx)    (xxx - 'A' + 1)

struct environ env;
struct player you;
struct system_environment SysEnv;
char info[200];                 // messaging queue extern'd everywhere {dlb}
char st_prn[20];
char str_pass[80];
int stealth;                    // externed in view.h     // no it is not {dlb}
char use_colour = 1;
FixedVector < char, 10 > visible;

// set to true once a new game starts or an old game loads
bool game_has_started = false;

/*

   Functions needed:
   New:
   int player_speed(player you);
   hit_player(damage, flavour, then last two ouch values, env);


   Old:
   wield(player you);
   show_map
   noisy()
   losight

*/

void (*viewwindow) (char, bool);

/* these are all defined in view.cc: */
extern unsigned char (*mapch) (unsigned char);
extern unsigned char (*mapch2) (unsigned char);
unsigned char mapchar(unsigned char ldfk);
unsigned char mapchar2(unsigned char ldfk);
unsigned char mapchar3(unsigned char ldfk);
unsigned char mapchar4(unsigned char ldfk);

/*
   Function pointers are used to make switching between Linux and DOS char sets
   possible as a runtime option (command-line -c)
*/

// these two are defined in view.cc. What does the player look like?
// (changed for shapechanging)
extern unsigned char your_sign;
extern unsigned char your_colour;

extern bool wield_change;       // defined in output.cc

// Functions in main module
static void close_door(char move_x, char move_y);
static void do_berserk_no_combat_penalty(void);
static bool initialise(void);
static void input(void);
static void move_player(char move_x, char move_y);
static void open_door(char move_x, char move_y);

/*
   It all starts here. Some initialisations are run first, then straight to
   new_game and then input.
*/
int main(int argc, char *argv[])
{
#ifdef USE_ASCII_CHARACTERS
    // Default to the non-ibm set when it makes sense.
    viewwindow = &viewwindow3;
    mapch = &mapchar3;
    mapch2 = &mapchar4;
#else
    // Use the standard ibm default
    viewwindow = &viewwindow2;
    mapch = &mapchar;
    mapch2 = &mapchar2;
#endif

    // Load in the system environment variables
    get_system_environment();

    // parse command line args -- look only for initfile & crawl_dir entries
    if (!parse_args(argc, argv, true))
    {
        // print help
        puts("Command line options:");
        puts("  -scores [N]         highscore list");
        puts("  -name <string>      character name");
        puts("  -race <letter>      preselect race");
        puts("  -class <letter>     preselect class");
        puts("  -pizza <string>     crawl pizza");
        puts("  -plain              don't use IBM extended characters");
        puts("  -dir <path>         crawl directory");
        puts("  -rc <file>          init file name");
        puts("");
        puts("Command line options override init file options, which override");
        puts("environment options (CRAWL_NAME, CRAWL_PIZZA, CRAWL_DIR, CRAWL_RC).");
        exit(1);
    }

    // Read the init file
    read_init_file();

    // now parse the args again, looking for everything else.
    parse_args(argc, argv, false);

#ifdef LINUX
    lincurses_startup();
#endif

#ifdef MAC
    init_mac();
#endif

    // check for highscore list - should be done BEFORE libw32c init
    if (Options.sc_entries > 0)
    {
        cprintf(" Best Crawlers -"EOL);
        hiscores_print_list();
        end(0);
    }

#ifdef WIN32CONSOLE
    init_libw32c();
#endif

#ifdef MACROS
    // Load macros
    macro_init();
#endif

    init_overmap();             // in overmap.cc (duh?)
    clear_ids();                // in itemname.cc

    bool game_start = initialise();
    if (game_start || Options.always_greet)
    {
        string welcome_msg = "Welcome, ";
        welcome_msg += you.your_name;
        welcome_msg += " the ";
        welcome_msg += species_name( you.species );
        welcome_msg += " ";
        welcome_msg += you.class_name;
        welcome_msg += ".";

        mpr( welcome_msg.c_str() );

        // Starting messages can go here as this should only happen
        // at the start of a new game -- bwr
        // This message isn't appropriate for Options.always_greet
        if (you.char_class == JOB_WANDERER && game_start)
        {
            int skill_levels = 0;
            for (int i = 0; i <= NUM_SKILLS; i++)
                skill_levels += you.skills[ i ];

            if (skill_levels <= 2)
            {
                // Demigods and Demonspawn wanderers stand to not be
                // able to see any of their skills at the start of
                // the game (one or two skills should be easily guessed
                // from starting equipment)... Anyways, we'll give the
                // player a message to warn them (and give a reason why). -- bwr
                mpr("You wake up in a daze, and can't recall much.");
            }
        }

        // These need some work -- should make sure that the god's
        // name is metioned, else the message might be confusing.
        switch (you.religion)
        {
        case GOD_ZIN:
            simple_god_message( " says: Spread the light, my child." );
            break;
        case GOD_SHINING_ONE:
            simple_god_message( " says: Smite the infidels!" );
            break;
        case GOD_KIKUBAAQUDGHA:
        case GOD_YREDELEMNUL:
        case GOD_NEMELEX_XOBEH:
            simple_god_message( " says: Welcome..." );
            break;
        case GOD_XOM:
            if (game_start)
                simple_god_message( " says: A new plaything!" );
            break;
        case GOD_VEHUMET:
            god_speaks( you.religion, "Let it end in hellfire!");
            break;
        case GOD_OKAWARU:
            simple_god_message(" says: Welcome,  disciple.");
            break;
        case GOD_MAKHLEB:
            god_speaks( you.religion, "Blood and souls for Makhleb!" );
            break;
        case GOD_SIF_MUNA:
            simple_god_message( " whispers: I know many secrets...");
            break;
        case GOD_TROG:
            simple_god_message( " says: Kill them all!" );
            break;
        case GOD_ELYVILON:
            simple_god_message( " says: Go forth and aid the weak!" );
            break;
        default:
            break;
        }
        // warn player about their weapon, if unsuitable
        wield_warning(false);
    }

    while (true)
    {
        input();
        //      cprintf("x");
    }

    // Should never reach this stage, right?

#ifdef LINUX
    lincurses_shutdown();
#endif

#ifdef MAC
    deinit_mac();
#endif

#ifdef USE_EMX
    deinit_emx();
#endif

    return 0;

}                               // end main()



/*
   This function handles the player's input. It's called from main(), from
   inside an endless loop.
 */
static void input(void)
{

    bool its_quiet;             //jmf: for silence messages
    FixedVector < int, 2 > plox;
    char move_x = 0;
    char move_y = 0;

    int keyin = 0;
    char str_pass[50];

#ifdef LINUX
    // Stuff for the Unix keypad kludge
    bool running = false;
    bool opening = false;
#endif

    you.shield_blocks = 0;

    you.time_taken = player_speed();

#ifdef USE_CURSES
    // yes this isn't pretty, but it avoids an evil namespace collision
    // between <string> and <curses.h>.
    window(1, 1, 80, 25);
#else
    window(1, 1, 80, NUMBER_OF_LINES);
#endif

    textcolor(LIGHTGREY);

    print_stats();

    if (!you.paralysis)
    {
        if (you.delay_t > 1)
        {
            you.delay_t--;
        }
        else if (you.delay_t == 1)
        {
            if (you.delay_doing != DELAY_AUTOPICKUP
                && you.delay_doing != DELAY_WEAPON_SWAP) // no point in msg
            {
                strcpy(info, "You finish ");
                strcat(info,
                        (you.delay_doing == DELAY_EAT)       ? "eating" :
                        (you.delay_doing == DELAY_ARMOR_ON)  ? "putting on your armour" :
                        (you.delay_doing == DELAY_ARMOR_OFF) ? "taking off your armour" :
                        (you.delay_doing == DELAY_MEMORIZE)  ? "memorising" :
                        (you.delay_doing == DELAY_PASSWALL)  ? "merging with the rock"
                                                             : "");

                if (you.delay_doing == DELAY_BUTCHER)
                {
                    strcat(info, (you.species == SP_TROLL
                                    || you.species == SP_GHOUL) ? "ripping"
                                                                : "chopping");

                    strcat( info, " the corpse into pieces" );
                }

                strcat( info, "." );
                mpr(info);

                if (you.delay_doing == DELAY_BUTCHER
                        && you.berserker
                        && you.berserk_penalty != NO_BERSERK_PENALTY)
                {
                    mpr("You enjoyed that.");
                    you.berserk_penalty = 0;
                }

                if (you.delay_doing == DELAY_PASSWALL
                    && you.passwall_x != 0 && you.passwall_y != 0)
                {
                    switch (grd[ you.passwall_x ][ you.passwall_y ])
                    {
                    case DNGN_ROCK_WALL:
                    case DNGN_STONE_WALL:
                    case DNGN_METAL_WALL:
                    case DNGN_GREEN_CRYSTAL_WALL:
                    case DNGN_WAX_WALL:
                    case DNGN_SILVER_STATUE:
                    case DNGN_ORANGE_CRYSTAL_STATUE:
                        ouch(1 + you.hp, 0, KILLED_BY_PETRIFICATION);
                        break;

                    case DNGN_SECRET_DOOR:      // oughtn't happen
                    case DNGN_CLOSED_DOOR:      // open the door
                        grd[you.passwall_x][you.passwall_y] = DNGN_OPEN_DOOR;
                        break;

                    default:
                        break;
                    }

                //jmf: hmm, what to do. kill the monster? (seems too powerful)
                //     displace the monster? randomly teleport the monster?
                //     This seems fair: try to move the monster, but if not
                //     able to, then kill it.
                    int mon = mgrd[you.passwall_x][you.passwall_y];
                    if (mon != NON_MONSTER)
                    {
                        monster_blink( &menv[ mon ] );

                        // recheck square for monster
                        mon = mgrd[you.passwall_x][you.passwall_y];
                        if (mon != NON_MONSTER)
                            monster_die( &menv[ mon ], KILL_YOU, 0 );
                    }

                    you.x_pos = you.passwall_x;
                    you.y_pos = you.passwall_y;
                    redraw_screen();

                    const unsigned char grid = grd[ you.x_pos ][ you.y_pos ];
                    if ((grid == DNGN_LAVA || grid == DNGN_DEEP_WATER)
                        && !you.levitation)
                    {
                        if (you.species == SP_MERFOLK && grid == DNGN_DEEP_WATER)
                        {
                            mpr("You fall into the water and return "
                                "to your normal form.");
                            merfolk_start_swimming();
                        }

                        // XXX: problem with this and the fact we've
                        // already moved?
                        fall_into_a_pool( false, grid );
                    }

                    // don't need this, but I'm going for safety first.
                    you.passwall_x = 0;
                    you.passwall_y = 0;
                }
            }
            you.delay_t = 0;
        }

        gotoxy(18, 9);

        if (you.delay_t == 0)
        {
            if (you.running > 0)
            {
                keyin = 125;    // a closed curly brace

                move_x = you.run_x;
                move_y = you.run_y;

                if (kbhit())
                {
                    you.running = 0;
                    goto gutch;
                }

                if (you.run_x == 0 && you.run_y == 0)
                {
                    you.running--;
                    keyin = '.';
                }
            }
            else
            {
              gutch:
                keyin = getch();
            }

            mesclr();

#ifdef LINUX
            // Kludging running and opening as two character sequences
            // for Unix systems.  This is an easy way out... all the
            // player has to do is find a termcap and numlock setting
            // that will get curses the numbers from the keypad.  This
            // will hopefully be easy.

            if (keyin == '*')
            {
                opening = true;
                keyin = getch();
            }
            else if (keyin == '/')
            {
                running = true;
                keyin = getch();
            }

            // Translate keypad codes into command enums
            keyin = key_to_command(keyin);
#else
            // Old DOS keypad support
            if (keyin == 0)     // ALT also works - see ..\KEYTEST.CPP
            {
                keyin = getch();
                switch (keyin)
                {
                case 'O': move_x = -1; move_y =  1; break;
                case 'P': move_x =  0; move_y =  1; break;
                case 'I': move_x =  1; move_y = -1; break;
                case 'H': move_x =  0; move_y = -1; break;
                case 'G': move_x = -1; move_y = -1; break;
                case 'K': move_x = -1; move_y =  0; break;
                case 'Q': move_x =  1; move_y =  1; break;
                case 'M': move_x =  1; move_y =  0; break;

                case 119: open_door(-1, -1); move_x = 0; move_y = 0; break;
                case 141: open_door( 0, -1); move_x = 0; move_y = 0; break;
                case 132: open_door( 1, -1); move_x = 0; move_y = 0; break;
                case 116: open_door( 1,  0); move_x = 0; move_y = 0; break;
                case 118: open_door( 1,  1); move_x = 0; move_y = 0; break;
                case 145: open_door( 0,  1); move_x = 0; move_y = 0; break;
                case 117: open_door(-1,  1); move_x = 0; move_y = 0; break;
                case 115: open_door(-1,  0); move_x = 0; move_y = 0; break;

                case 76:
                case 'S':
                    keyin = '.';
                    goto get_keyin_again;
                }
                keyin = 125;
            }
#endif
        }
        else
            keyin = '.';
    }
    else
    {
        keyin = '.';            // end of if you.paralysis == 0
    }

    if (keyin != 125)
    {
        move_x = 0;
        move_y = 0;
        you.turn_is_over = 0;
    }

#ifndef LINUX
  get_keyin_again:
#endif //jmf: just stops an annoying gcc warning
    //putch(keyin);
    //getch();

#ifdef WIZARD
    int wiz_command, i, j;

#endif

    switch (keyin)
    {
    case Control('Y'):
    case CMD_OPEN_DOOR_UP_RIGHT:
        open_door(-1, -1); move_x = 0; move_y = 0; break;

    case Control('K'):
    case CMD_OPEN_DOOR_UP:
        open_door( 0, -1); move_x = 0; move_y = 0; break;

    case Control('U'):
    case CMD_OPEN_DOOR_UP_LEFT:
        open_door( 1, -1); move_x = 0; move_y = 0; break;

    case Control('L'):
    case CMD_OPEN_DOOR_RIGHT:
        open_door( 1,  0); move_x = 0; move_y = 0; break;

    case Control('N'):
    case CMD_OPEN_DOOR_DOWN_RIGHT:
        open_door( 1,  1); move_x = 0; move_y = 0; break;

    case Control('J'):
    case CMD_OPEN_DOOR_DOWN:
        open_door( 0,  1); move_x = 0; move_y = 0; break;

    case Control('B'):
    case CMD_OPEN_DOOR_DOWN_LEFT:
        open_door(-1,  1); move_x = 0; move_y = 0; break;

    case Control('H'):
    case CMD_OPEN_DOOR_LEFT:
        open_door(-1,  0); move_x = 0; move_y = 0; break;

    case 'b': case CMD_MOVE_DOWN_LEFT:  move_x = -1; move_y =  1; break;
    case 'j': case CMD_MOVE_DOWN:       move_y =  1; move_x =  0; break;
    case 'u': case CMD_MOVE_UP_RIGHT:   move_x =  1; move_y = -1; break;
    case 'k': case CMD_MOVE_UP:         move_y = -1; move_x =  0; break;
    case 'y': case CMD_MOVE_UP_LEFT:    move_y = -1; move_x = -1; break;
    case 'h': case CMD_MOVE_LEFT:       move_x = -1; move_y =  0; break;
    case 'n': case CMD_MOVE_DOWN_RIGHT: move_y =  1; move_x =  1; break;
    case 'l': case CMD_MOVE_RIGHT:      move_x =  1; move_y =  0; break;

    case CMD_REST:
        // Yes this is backwards, but everyone here is used to using
        // straight 5s for long rests... might need to a roguelike
        // rest key and get people switched over. -- bwr

#ifdef LINUX
        if (!running && !opening)
        {
            you.run_x = 0;
            you.run_y = 0;
            you.running = 100;
        }
        else
        {
            search_around();
            move_x = 0;
            move_y = 0;
            you.turn_is_over = 1;
        }
#endif
        break;

    case 'B': case CMD_RUN_DOWN_LEFT:
        you.run_x = -1; you.run_y =  1; you.running = 2; break;

    case 'J': case CMD_RUN_DOWN:
        you.run_y =  1; you.run_x =  0; you.running = 2; break;

    case 'U': case CMD_RUN_UP_RIGHT:
        you.run_x =  1; you.run_y = -1; you.running = 2; break;

    case 'K': case CMD_RUN_UP:
        you.run_y = -1; you.run_x =  0; you.running = 2; break;

    case 'Y': case CMD_RUN_UP_LEFT:
        you.run_y = -1; you.run_x = -1; you.running = 2; break;

    case 'H': case CMD_RUN_LEFT:
        you.run_x = -1; you.run_y =  0; you.running = 2; break;

    case 'N': case CMD_RUN_DOWN_RIGHT:
        you.run_y =  1; you.run_x =  1; you.running = 2; break;

    case 'L': case CMD_RUN_RIGHT:
        you.run_x =  1; you.run_y =  0; you.running = 2; break;

#ifdef LINUX
        // taken care of via key -> command mapping
#else
        // Old DOS keypad support
    case '1': you.run_x = -1; you.run_y =  1; you.running =   2; break;
    case '2': you.run_y =  1; you.run_x =  0; you.running =   2; break;
    case '9': you.run_x =  1; you.run_y = -1; you.running =   2; break;
    case '8': you.run_y = -1; you.run_x =  0; you.running =   2; break;
    case '7': you.run_y = -1; you.run_x = -1; you.running =   2; break;
    case '4': you.run_x = -1; you.run_y =  0; you.running =   2; break;
    case '3': you.run_y =  1; you.run_x =  1; you.running =   2; break;
    case '6': you.run_x =  1; you.run_y =  0; you.running =   2; break;
    case '5': you.run_x =  0; you.run_y =  0; you.running = 100; break;

#endif

    case Control('A'):
    case CMD_TOGGLE_AUTOPICKUP:
        autopickup_on = !autopickup_on;
        strcpy(info, "Autopickup is now ");
        strcat(info, (autopickup_on) ? "on" : "off");
        strcat(info, ".");
        mpr(info);
        break;

    case '<':
    case CMD_GO_UPSTAIRS:
        up_stairs();
        break;
    case '>':
    case CMD_GO_DOWNSTAIRS:
        down_stairs(false, you.your_level);
        break;

    case 'O':
    case CMD_DISPLAY_OVERMAP:
        display_overmap();
        break;

    case 'o':
    case CMD_OPEN_DOOR:
        open_door(0, 0);
        break;
    case 'c':
    case CMD_CLOSE_DOOR:
        close_door(0, 0);
        break;

    case 'd':
    case CMD_DROP:
        drop();
        break;
    case 'D':
    case CMD_BUTCHER:
        butchery();
        break;
    case 'i':
    case CMD_DISPLAY_INVENTORY:
        get_invent(-1);
        break;
    case 'I':
    case CMD_INVOKE:
        invoke_wielded();
        break;

    case 'g':
    case ',':
    case CMD_PICKUP:
        pickup();
        break;

    case ';':
    case CMD_INSPECT_FLOOR:
        item_check(';');
        break;
    case 'w':
    case CMD_WIELD_WEAPON:
        wield_weapon(false);
        break;
    case 't':
    case CMD_THROW:
        throw_anything();
        break;
    case 'f':
    case CMD_FIRE:
        shoot_thing();
        break;

    case 'W':
    case CMD_WEAR_ARMOUR:
        wear_armour();
        break;
    case 'T':
    case CMD_REMOVE_ARMOUR:
        {
            int index;

            if (armour_prompt("Take off which item?", &index))
                takeoff_armour(index);
        }
        break;

    case 'R':
    case CMD_REMOVE_JEWELLERY:
        remove_ring();
        break;
    case 'P':
    case CMD_WEAR_JEWELLERY:
        puton_ring();
        break;

    case '=':
    case CMD_ADJUST_INVENTORY:
        adjust();
        return;
    case 'M':
    case CMD_MEMORISE_SPELL:
        which_spell();
        break;
    case 'z':
    case CMD_ZAP_WAND:
        zap_wand();
        break;
    case 'e':
    case CMD_EAT:
        eat_food();
        break;
    case 'a':
    case CMD_USE_ABILITY:
        activate_ability();
        break;

    case 'A':
    case CMD_DISPLAY_MUTATIONS:
        display_mutations();
        redraw_screen();
        break;

    case 'v':
    case CMD_EXAMINE_OBJECT:
        original_name();
        break;
    case 'p':
    case CMD_PRAY:
        pray();
        break;

    case '^':
    case CMD_DISPLAY_RELIGION:
        describe_god(you.religion);
        redraw_screen();
        break;

    case '.':
    case CMD_MOVE_NOWHERE:
        search_around();
        move_x = 0;
        move_y = 0;
        you.turn_is_over = 1;
        break;

    case 'q':
    case CMD_QUAFF:
        drink();
        break;
    case 'r':
    case CMD_READ:
        read_scroll();
        break;

    case 'x':
    case CMD_LOOK_AROUND:
        mpr("Move the cursor around to observe a square.", MSGCH_PROMPT);
        mpr("Press '?' for a monster description.", MSGCH_PROMPT);

        struct dist lmove;
        look_around(lmove, true);
        break;

    case 's':
    case CMD_SEARCH:
        search_around();
        you.turn_is_over = 1;
        break;

    case 'Z':
    case CMD_CAST_SPELL:
        /* randart wpns */
        if (scan_randarts(RAP_PREVENT_SPELLCASTING))
        {
            mpr("Something interferes with your magic!");
            break;
        }
        cast_a_spell();
        break;

    case '\'':
    case CMD_WEAPON_SWAP:
        wield_weapon(true);
        break;

    case 'X':
    case CMD_DISPLAY_MAP:
        if (you.level_type == LEVEL_LABYRINTH || you.level_type == LEVEL_ABYSS)
        {
            mpr("It would help if you knew where you were, first.");
            break;
        }
        plox[0] = 0;
        show_map(plox);
        redraw_screen();
        break;

    case '\\':
    case CMD_DISPLAY_KNOWN_OBJECTS:
        check_item_knowledge(); //nothing = check_item_knowledge();
        redraw_screen();
        break;

    case Control('P'):
    case CMD_REPLAY_MESSAGES:
        replay_messages();
        redraw_screen();
        break;

    case Control('R'):
    case CMD_REDRAW_SCREEN:
        redraw_screen();
        break;

    case Control('X'):
    case CMD_SAVE_GAME_NOW:
        mpr("Saving game... please wait.");
        save_game(true);
        break;

#ifdef USE_UNIX_SIGNALS
    case Control('Z'):
    case CMD_SUSPEND_GAME:
        // CTRL-Z suspend behaviour is implemented here.
        // because we want to have CTRL-Y available...
        // we have to stop both from sending the signals,
        // so we reimplement here
        clrscr();
        lincurses_shutdown();
        kill(0, SIGTSTP);
        lincurses_startup();
        redraw_screen();
        break;
#endif

    case '?':
    case CMD_DISPLAY_COMMANDS:
        list_commands(false);
        redraw_screen();
        break;

    case 'C':
    case CMD_EXPERIENCE_CHECK:
        sprintf(info, "You are a level %d %s %s.", you.experience_level,
            species_name(you.species), you.class_name);
        mpr(info);

        if (you.experience_level < 27)
        {
            sprintf(info, "Level %d requires %d experience (%d points to go!)",
                you.experience_level + 1, exp_needed(you.experience_level+2)+1,
                (exp_needed(you.experience_level+2) - you.experience) + 1);
            mpr(info);
        }
        break;


    case '!':
    case CMD_SHOUT:
        yell();                 /* in effects.cc */
        break;
    case '@':
    case CMD_DISPLAY_CHARACTER_STATUS:
        display_char_status();
        break;

    case 'm':
    case CMD_DISPLAY_SKILLS:
        show_skills();
        redraw_screen();
        break;

    case '#':
    case CMD_CHARACTER_DUMP:
        char name_your[kNameLen+1];

        strncpy(name_your, you.your_name, kNameLen);
        name_your[kNameLen] = '\0';
        if (dump_char(0, name_your) == 1)
            strcpy(info, "Char dumped successfully.");
        else
            strcat(info, "Char dump unsuccessful! Sorry about that.");
        mpr(info);
        break;

#ifdef MACROS
    case '`':
    case CMD_MACRO_ADD:
        macro_add_query();
        break;
    case '~':
    case CMD_MACRO_SAVE:
        mpr("Saving macros.");
        macro_save();
        break;
#endif

    case ')':
    case CMD_LIST_WEAPONS:
        list_weapons();
        return;

    case ']':
    case CMD_LIST_ARMOUR:
        list_armour();
        return;

    case '"':
    case CMD_LIST_JEWELLERY:
        list_jewellery();
        return;

#ifdef WIZARD
    case Control('W'):
    case CMD_WIZARD:
    case '&':
        mpr("Enter Wizard Command: ");
        wiz_command = getch();

        switch (wiz_command)
        {
        case '?':
            list_commands(true);        // tell it to list wizard commands
            redraw_screen();
            break;
        case 'x':
            gain_exp(5000);
            break;
        case 's':
            you.exp_available = 20000;
            redraw_screen();
            break;
        case '$':
            you.gold += 1000;
            you.redraw_gold = 1;
            break;
        case 'a':
            acquirement(OBJ_RANDOM);
            break;
        case 'b':
            banished(DNGN_ENTER_ABYSS);
            break;
        case 'g':
            debug_add_skills();
            break;
        case 'h':
            you.rotting = 0;
            you.poison = 0;
            you.disease = 0;
            set_hp(10 + abs(you.hp_max), true);
            set_hunger(5000 + abs(you.hunger), true);
            break;
        case 'k':
            blink();            // wizards can always blink
            break;
        case '\"':
        case '~':
            level_travel();
            break;
        case '%':
        case 'o':
            create_spec_object2();
            break;
        case '+':
        case 'm':
            create_spec_monster();
            break;
        case 'M':
            create_spec_monster_name();
            break;
        case '*':
        case 'd':
            grd[you.x_pos][you.y_pos] = DNGN_STONE_STAIRS_DOWN_I;
            break;
        case 'p':
            grd[you.x_pos][you.y_pos] = DNGN_ENTER_PANDEMONIUM;
            break;
        case 'l':
            grd[you.x_pos][you.y_pos] = DNGN_ENTER_LABYRINTH;
            break;
        case 'u':
            grd[you.x_pos][you.y_pos] = DNGN_ROCK_STAIRS_UP;
            break;
        case 'i':
            identify(0);
            break;
        case '_':
            j = 0;

            for (i = 0; i < 50; i++)
                j += you.skill_points[i];

            strcpy(info, "You have a total of ");
            itoa(j, st_prn, 10);
            strcat(info, st_prn);
            strcat(info, " skill points.");
            mpr(info);
            break;
        case 'X':
            Xom_acts(true, 20, true);
            break;
        case 'z':
            cast_spec_spell();
            break;              /* cast spell by number */
        case 'Z':
            cast_spec_spell_name();
            break;              /* cast spell by name */
        case '(':
            char specs[3];

            mpr("Create which feature? ", MSGCH_PROMPT);
            specs[0] = getche();
            specs[1] = getche();
            specs[2] = getche();
            grd[you.x_pos][you.y_pos] = atoi(specs);
            break;
        case ']':
            char specx[2];

            mpr("Gain which mutation? ", MSGCH_PROMPT);
            specx[0] = getche();
            specx[1] = getche();
            mutate(atoi(specx));
            break;

        case ':':
            int i, j;

            j = 0;
            for (i = 0; i < 20; i++)
            {
                if (you.branch_stairs[i] == 0)
                    continue;
                strcpy(info, "Branch ");
                itoa(i, st_prn, 10);
                strcat(info, st_prn);
                strcat(info, " is on level ");
                itoa(you.branch_stairs[i] + 1, st_prn, 10); //jmf: off-by-one
                strcat(info, st_prn);
                strcat(info, ".");
                mpr(info);
            }
            break;

        case '{':
            magic_mapping(99, 100);
            break;

        case '^':
            {
                int old_piety = you.piety;

                gain_piety(50);
                sprintf(info,
                        "Congratulations, your piety went from %d to %d!",
                        old_piety, you.piety);
                mpr(info);
            }
            break;

        case '\'':
            for (i = 0; i < MAX_ITEMS; i++)
            {
                if (mitm.link[i] == NON_ITEM)
                    continue;
                itoa(i, st_prn, 10);
                strcpy(info, st_prn);
                strcat(info, " is linked to ");
                itoa(mitm.link[i], st_prn, 10);
                strcat(info, st_prn);
                strcat(info, " c:");
                itoa(mitm.base_type[i], st_prn, 10);
                strcat(info, st_prn);
                strcat(info, " t:");
                itoa(mitm.sub_type[i], st_prn, 10);
                strcat(info, st_prn);
                strcat(info, " p:");
                itoa(mitm.pluses[i], st_prn, 10);
                strcat(info, st_prn);
                strcat(info, " p2:");
                itoa(mitm.pluses2[i], st_prn, 10);
                strcat(info, st_prn);
                strcat(info, " d:");
                itoa(mitm.special[i], st_prn, 10);
                strcat(info, st_prn);
                strcat(info, " q: ");
                itoa(mitm.quantity[i], st_prn, 10);
                strcat(info, st_prn);
                mpr(info);
            }
            strcpy(info, "igrid:");
            mpr(info);

            for (i = 0; i < GXM; i++)
                for (j = 0; j < GYM; j++)
                {
                    if (igrd[i][j] != NON_ITEM)
                    {
                        itoa(igrd[i][j], st_prn, 10);
                        strcpy(info, st_prn);
                        strcat(info, " at ");
                        itoa(i, st_prn, 10);
                        strcat(info, st_prn);
                        strcat(info, ", ");
                        itoa(j, st_prn, 10);
                        strcat(info, st_prn);
                        strcat(info, " c:");
                        itoa(mitm.base_type[igrd[i][j]], st_prn, 10);
                        strcat(info, st_prn);
                        strcat(info, " t:");
                        itoa(mitm.sub_type[igrd[i][j]], st_prn, 10);
                        strcat(info, st_prn);
                        strcat(info, " p:");
                        itoa(mitm.pluses[igrd[i][j]], st_prn, 10);
                        strcat(info, st_prn);
                        strcat(info, " p2:");
                        itoa(mitm.pluses2[igrd[i][j]], st_prn, 10);
                        strcat(info, st_prn);
                        strcat(info, " d:");
                        itoa(mitm.special[igrd[i][j]], st_prn, 10);
                        strcat(info, st_prn);
                        strcat(info, " q: ");
                        itoa(mitm.quantity[igrd[i][j]], st_prn, 10);
                        strcat(info, st_prn);
                        mpr(info);
                    }
                }
            break;

        default:
            mpr("Not a Wizard Command.");
            break;
        }

        break;
        // end of WIZARD block
#endif

    case 'S':
    case CMD_SAVE_GAME:
        if (yesno("Save game and exit?", false))
            save_game(true);
        break;

    case 'Q':
    case CMD_QUIT:
        quit_game();
        break;

    case 'V':
    case CMD_GET_VERSION:
        version();
        break;

    case '}':
        //        Can't use this char -- it's the special value 125
        break;

    default:
    case CMD_NO_CMD:
        mpr("Unknown command.");
        break;

    }

#ifdef LINUX
    // New Unix keypad stuff
    if (running)
    {
        you.run_x = move_x;
        you.run_y = move_y;
        you.running = 2;
        move_x = 0;
        move_y = 0;
    }
    else if (opening)
    {
        open_door(move_x, move_y);
        move_x = 0;
        move_y = 0;
    }
#endif

    if (move_x != 0 || move_y != 0)
        move_player(move_x, move_y);
    else if (you.turn_is_over)      // we did something other than move/attack
        do_berserk_no_combat_penalty();

    if (!you.turn_is_over)
    {
        viewwindow(1, false);
        return;
    }

    //if (random2(10) < you.skills [SK_TRAPS_DOORS] + 2) search_around();

    stealth = check_stealth();

    if (you.special_wield != SPWLD_NONE)
        special_wielded();

    if (one_chance_in(10)
        && ((player_teleport() > 0 && one_chance_in(100 / player_teleport()))
            || (you.level_type == LEVEL_ABYSS && one_chance_in(30))))
    {
        you_teleport2(true);    // this is instantaneous
    }

    if (env.cgrid[you.x_pos][you.y_pos] != EMPTY_CLOUD)
        in_a_cloud();

    // paradox: it both lasts longer & does more damage overall if you're
    //          moving slower.
    // rationalisation: I guess it gets rubbed off/falls off/etc if you
    //          move around more.
    if (you.duration[DUR_LIQUID_FLAMES] > 0)
        you.duration[DUR_LIQUID_FLAMES]--;

    if (you.duration[DUR_LIQUID_FLAMES] != 0)
    {
        mpr("You are covered in liquid flames!", MSGCH_WARN);
        scrolls_burn(8, OBJ_SCROLLS);

        if (player_res_fire() > 100)
        {
            ouch( (((random2avg(9, 2) + 1) /
                    (1 + (player_res_fire() - 100) * (player_res_fire() - 100)))
                    * you.time_taken) / 10, 0, KILLED_BY_BURNING );
        }

        if (player_res_fire() <= 100)
        {
            ouch(((random2avg(9, 2) + 1) * you.time_taken) / 10, 0,
                                                KILLED_BY_BURNING);
        }

        if (player_res_fire() < 100)
        {
            ouch(((random2avg(9, 2) + 1) * you.time_taken) / 10, 0,
                                                KILLED_BY_BURNING);
        }
    }

    if (you.duration[DUR_ICY_ARMOUR] > 1)
    {
        you.duration[DUR_ICY_ARMOUR]--;
        //scrolls_burn(4, OBJ_POTIONS);
    }
    else if (you.duration[DUR_ICY_ARMOUR] == 1)
    {
        mpr("Your icy armour evaporates.", MSGCH_DURATION);
        you.redraw_armor_class = 1;     // is this needed? 2apr2000 {dlb}
        you.duration[DUR_ICY_ARMOUR] = 0;
    }

    if (you.duration[DUR_REPEL_MISSILES] > 1)
    {
        you.duration[DUR_REPEL_MISSILES]--;
        if (you.duration[DUR_REPEL_MISSILES] == 6)
        {
            mpr("Your repel missiles spell is about to expire...", MSGCH_DURATION);
            if (coinflip())
                you.duration[DUR_REPEL_MISSILES]--;
        }
    }
    else if (you.duration[DUR_REPEL_MISSILES] == 1)
    {
        mpr("You feel less protected from missiles.", MSGCH_DURATION);
        you.duration[DUR_REPEL_MISSILES] = 0;
    }

    if (you.duration[DUR_DEFLECT_MISSILES] > 1)
    {
        you.duration[DUR_DEFLECT_MISSILES]--;
        if (you.duration[DUR_DEFLECT_MISSILES] == 6)
        {
            mpr("Your deflect missiles spell is about to expire...", MSGCH_DURATION);
            if (coinflip())
                you.duration[DUR_DEFLECT_MISSILES]--;
        }
    }
    else if (you.duration[DUR_DEFLECT_MISSILES] == 1)
    {
        mpr("You feel less protected from missiles.", MSGCH_DURATION);
        you.duration[DUR_DEFLECT_MISSILES] = 0;
    }

    if (you.duration[DUR_REGENERATION] > 1)
    {
        you.duration[DUR_REGENERATION]--;

        if (you.duration[DUR_REGENERATION] == 6)
        {
            mpr("Your skin is crawling a little less now.", MSGCH_DURATION);
            if (coinflip())
                you.duration[DUR_REGENERATION]--;
        }
    }
    else if (you.duration[DUR_REGENERATION] == 1)
    {
        mpr("Your skin stops crawling.", MSGCH_DURATION);
        you.duration[DUR_REGENERATION] = 0;
    }

    if (you.duration[DUR_PRAYER] > 1)
        you.duration[DUR_PRAYER]--;
    else if (you.duration[DUR_PRAYER] == 1)
    {
        god_speaks(you.religion, "Your prayer is over.");
        you.duration[DUR_PRAYER] = 0;
    }


    //jmf: more flexible weapon branding code
    if (you.duration[DUR_WEAPON_BRAND] > 1)
        you.duration[DUR_WEAPON_BRAND]--;
    else if (you.duration[DUR_WEAPON_BRAND] == 1)
    {
        int temp_effect = you.inv_dam[you.equip[EQ_WEAPON]] % 30;

        you.duration[DUR_WEAPON_BRAND] = 0;
        you.inv_dam[you.equip[EQ_WEAPON]] -= temp_effect;
        in_name(you.equip[EQ_WEAPON], 4, str_pass);
        strcpy(info, str_pass);

        switch (temp_effect)
        {
        case SPWPN_VORPAL:
            if (damage_type(you.inv_class[you.equip[EQ_WEAPON]],
                     you.inv_type[you.equip[EQ_WEAPON]]) != DVORP_CRUSHING)
            {
                strcat(info, " seems blunter.");
            }
            else
            {
                //jmf: for Maxwell's Silver Hammer
                strcat(info, " feels lighter.");
            }
            break;

        case SPWPN_FLAMING:
            strcat(info, " goes out.");
            break;
        case SPWPN_FREEZING:
            strcat(info, " stops glowing.");
            break;
        case SPWPN_VENOM:
            strcat(info, " stops dripping with poison.");
            break;
        case SPWPN_DRAINING:
            strcat(info, " stops crackling.");
            break;
        case SPWPN_DISTORTION:
            strcat(info, " seems straighter.");
            miscast_effect(SPWPN_DISTORTION, 9, 90, 100);
            break;
        default:
            strcat(info, " seems inexplicably less special.");
            break;
        }

        //you.attribute[ATTR_WEAPON_BRAND] = 0;
        mpr(info, MSGCH_DURATION);
        wield_change = true;
    }

    if (you.duration[DUR_BREATH_WEAPON] > 1)
        you.duration[DUR_BREATH_WEAPON]--;
    else if (you.duration[DUR_BREATH_WEAPON] == 1)
    {
        mpr("You have got your breath back.", MSGCH_RECOVERY);
        you.duration[DUR_BREATH_WEAPON] = 0;
    }

    if (you.duration[DUR_TRANSFORMATION] > 1)
    {
        you.duration[DUR_TRANSFORMATION]--;

        if (you.duration[DUR_TRANSFORMATION] == 10)
        {
            mpr("Your transformation is almost over.", MSGCH_DURATION);
            you.duration[DUR_TRANSFORMATION] -= random2(3);
        }
    }
    else if (you.duration[DUR_TRANSFORMATION] == 1)
    {
        untransform();
        you.duration[DUR_BREATH_WEAPON] = 0;
    }

    if (you.duration[DUR_SWIFTNESS] > 1)
    {
        you.duration[DUR_SWIFTNESS]--;
        if (you.duration[DUR_SWIFTNESS] == 6)
        {
            mpr("You start to feel a little slower.", MSGCH_DURATION);
            if (coinflip())
                you.duration[DUR_SWIFTNESS]--;
        }
    }
    else if (you.duration[DUR_SWIFTNESS] == 1)
    {
        mpr("You feel sluggish.", MSGCH_DURATION);
        you.duration[DUR_SWIFTNESS] = 0;
    }

    if (you.duration[DUR_INSULATION] > 1)
    {
        you.duration[DUR_INSULATION]--;
        if (you.duration[DUR_INSULATION] == 6)
        {
            mpr("You start to feel a little less insulated.", MSGCH_DURATION);
            if (coinflip())
                you.duration[DUR_INSULATION]--;
        }
    }
    else if (you.duration[DUR_INSULATION] == 1)
    {
        mpr("You feel conductive.", MSGCH_DURATION);
        you.duration[DUR_INSULATION] = 0;
    }

    if (you.duration[DUR_STONEMAIL] > 1)
    {
        you.duration[DUR_STONEMAIL]--;
        if (you.duration[DUR_STONEMAIL] == 6)
        {
            mpr("Your scaley stone armour is starting to flake away.", MSGCH_DURATION);
            if (coinflip())
                you.duration[DUR_STONEMAIL]--;
        }
    }
    else if (you.duration[DUR_STONEMAIL] == 1)
    {
        mpr("Your scaley stone armour disappears.", MSGCH_DURATION);
        you.duration[DUR_STONEMAIL] = 0;
        burden_change();
    }

    if (you.duration[DUR_FORESCRY] > 1) //jmf: added
        you.duration[DUR_FORESCRY]--;
    else if (you.duration[DUR_FORESCRY] == 1)
    {
        mpr("You feel firmly rooted in the present.", MSGCH_DURATION);
        you.duration[DUR_FORESCRY] = 0;
    }

    if (you.duration[DUR_SEE_INVISIBLE] > 1)    //jmf: added
        you.duration[DUR_SEE_INVISIBLE]--;
    else if (you.duration[DUR_SEE_INVISIBLE] == 1)
    {
        if (!player_see_invis())
            mpr("Your eyesight blurs momentarily.", MSGCH_DURATION);
        you.duration[DUR_SEE_INVISIBLE] = 0;
    }

    if (you.duration[DUR_SILENCE] > 0)  //jmf: cute message handled elsewhere
        you.duration[DUR_SILENCE]--;

    if (you.duration[DUR_CONDENSATION_SHIELD] > 1)
        you.duration[DUR_CONDENSATION_SHIELD]--;
    else if (you.duration[DUR_CONDENSATION_SHIELD] == 1)
    {
        you.duration[DUR_CONDENSATION_SHIELD] = 0;
        mpr("Your icy shield evaporates.", MSGCH_DURATION);
        you.redraw_armor_class = 1;
    }

    if (you.duration[DUR_STONESKIN] > 1)
        you.duration[DUR_STONESKIN]--;
    else if (you.duration[DUR_STONESKIN] == 1)
    {
        mpr("Your skin feels tender.", MSGCH_DURATION);
        you.redraw_armor_class = 1;
        you.duration[DUR_STONESKIN] = 0;
    }

#ifdef USE_ELVISH_GLAMOUR_ABILITY
    if (you.duration[DUR_GLAMOUR] > 1)  //jmf: actually GLAMOUR_RELOAD, like
        you.duration[DUR_GLAMOUR]--;    //     the breath weapon delay
    else if (you.duration[DUR_GLAMOUR] == 1)
    {
        you.duration[DUR_GLAMOUR] = 0;
        //FIXME: cute message or not?
    }
#endif

    if (you.duration[DUR_TELEPORT] > 1)
        you.duration[DUR_TELEPORT]--;
    else if (you.duration[DUR_TELEPORT] == 1)
    {
        you_teleport2(true);
        you.duration[DUR_TELEPORT] = 0;
    }

    if (you.duration[DUR_CONTROL_TELEPORT] > 1)
    {
        you.duration[DUR_CONTROL_TELEPORT]--;

        if (you.duration[DUR_CONTROL_TELEPORT] == 6)
        {
            mpr("You start to feel a little uncertain.", MSGCH_DURATION);
            if (coinflip())
                you.duration[DUR_CONTROL_TELEPORT]--;
        }
    }
    else if (you.duration[DUR_CONTROL_TELEPORT] == 1)
    {
        mpr("You feel uncertain.", MSGCH_DURATION);
        you.duration[DUR_CONTROL_TELEPORT] = 0;
        you.attribute[ATTR_CONTROL_TELEPORT]--;
    }

    if (you.duration[DUR_RESIST_POISON] > 1)
    {
        you.duration[DUR_RESIST_POISON]--;
        if (you.duration[DUR_RESIST_POISON] == 6)
        {
            mpr("Your poison resistance is about to expire.", MSGCH_DURATION);
            if (coinflip())
                you.duration[DUR_RESIST_POISON]--;
        }
    }
    else if (you.duration[DUR_RESIST_POISON] == 1)
    {
        mpr("Your poison resistance expires.", MSGCH_DURATION);
        you.duration[DUR_RESIST_POISON] = 0;
    }

    if (you.duration[DUR_DEATH_CHANNEL] > 1)
    {
        you.duration[DUR_DEATH_CHANNEL]--;
        if (you.duration[DUR_DEATH_CHANNEL] == 6)
        {
            mpr("Your unholy channel is weakening.", MSGCH_DURATION);
            if (coinflip())
                you.duration[DUR_DEATH_CHANNEL]--;
        }
    }
    else if (you.duration[DUR_DEATH_CHANNEL] == 1)
    {
        mpr("Your unholy channel expires.", MSGCH_DURATION);    // Death channel wore off
        you.duration[DUR_DEATH_CHANNEL] = 0;
    }

    if (you.duration[DUR_SHUGGOTH_SEED_RELOAD] > 0)     //jmf: added
        you.duration[DUR_SHUGGOTH_SEED_RELOAD]--;

    if (you.duration[DUR_INFECTED_SHUGGOTH_SEED] > 1)
        you.duration[DUR_INFECTED_SHUGGOTH_SEED]--;
    else if (you.duration[DUR_INFECTED_SHUGGOTH_SEED] == 1)
    {
        //jmf: use you.max_hp instead? or would that be too evil?
        you.duration[DUR_INFECTED_SHUGGOTH_SEED] = 0;
        mpr("A horrible thing bursts from your chest!", MSGCH_WARN);
        ouch(1 + you.hp / 2, 0, KILLED_BY_SHUGGOTH);
        make_shuggoth(you.x_pos, you.y_pos, 1 + you.hp / 2);
    }

    if (you.duration[DUR_TELEPORT] > 1)
        you.duration[DUR_TELEPORT]--;
    else if (you.duration[DUR_TELEPORT] == 1)
    {
        you_teleport2(true);
        you.duration[DUR_TELEPORT] = 0;
    }

    if (you.invis > 1)
    {
        you.invis--;

        if (you.hunger >= 40)
            make_hungry(5, true);

        if (you.invis == 6)
        {
            mpr("You flicker for a moment.", MSGCH_DURATION);
            if (coinflip())
                you.invis--;
        }
    }
    else if (you.invis == 1)
    {
        mpr("You flicker back into view.", MSGCH_DURATION);
        you.invis = 0;
    }

    if (you.conf > 1)
        you.conf--;
    else if (you.conf == 1)
    {
        mpr("You feel steadier.", MSGCH_DURATION);
        you.conf = 0;
    }

    if (you.paralysis > 1)
        you.paralysis--;
    else if (you.paralysis == 1)
    {
        mpr("You can move again.", MSGCH_DURATION);
        you.paralysis = 0;
    }

    if (you.exhausted > 1)
        you.exhausted--;
    else if (you.exhausted == 1)
    {
        mpr("You feel less fatigued.", MSGCH_DURATION);
        you.exhausted = 0;
    }

    if (you.slow > 1)
    {
        // BCR - Amulet of resist slow affects slow counter
        if (wearing_amulet(AMU_RESIST_SLOW))
        {
            you.slow -= 5;
            if (you.slow < 1)
                you.slow = 1;
        }
        else
            you.slow--;
    }
    else if (you.slow == 1)
    {
        mpr("You feel yourself speed up.", MSGCH_DURATION);
        you.slow = 0;
    }

    if (you.haste > 1)
    {
        // BCR - Amulet of resist slow affects haste counter
        if (!wearing_amulet(AMU_RESIST_SLOW))
            you.haste--;
        else if (coinflip())
            you.haste--;

        if (you.haste == 6)
        {
            mpr("Your extra speed is starting to run out.", MSGCH_DURATION);
            if (coinflip())
                you.haste--;
        }
    }
    else if (you.haste == 1)
    {
        mpr("You feel yourself slow down.", MSGCH_DURATION);
        you.haste = 0;
    }

    if (you.might > 1)
        you.might--;
    else if (you.might == 1)
    {
        mpr("You feel a little less mighty now.", MSGCH_DURATION);
        you.might = 0;
        modify_stat(STAT_STRENGTH, -5, true);
    }

    if (you.berserker > 1)
        you.berserker--;
    else if (you.berserker == 1)
    {
        mpr("You are no longer berserk.", MSGCH_DURATION);
        you.berserker = 0;

        //jmf: guilty for berserking /after/ berserk
        naughty(NAUGHTY_STIMULANTS, 6 + random2(6));

        //
        // Sometimes berserk leaves us physically drained
        //
        if (you.berserk_penalty == NO_BERSERK_PENALTY
            || (you.religion == GOD_TROG && you.piety > random2(150))
            || !one_chance_in(10 + you.mutation[MUT_BERSERK] * 10))
        {
            mpr("You are exhausted.");
        }
        else
        {
            mpr("You pass out from exhaustion.", MSGCH_WARN);
            you.paralysis += 3 + random2(5);
        }

        // this resets either from an actual penalty, or from
        // a NO_BERSERK_PENALTY
        you.berserk_penalty = 0;
        you.exhausted += 12 + random2avg(23, 2);
        you.slow += you.exhausted;

        make_hungry(700, true);

        if (you.hunger < 50)
            you.hunger = 50;

        calc_hp();
    }

    if (you.confusing_touch > 1)
        you.confusing_touch--;
    else if (you.confusing_touch == 1)
    {
        mpr("Your hands stop glowing.", MSGCH_DURATION);
        you.confusing_touch = 0;
    }

    if (you.sure_blade > 1)
        you.sure_blade--;
    else if (you.sure_blade == 1)
    {
        mpr("The bond with your blade fades away.", MSGCH_DURATION);
        you.sure_blade = 0;
    }

    if (you.levitation > 1)
    {
        if (you.species != SP_KENKU || you.experience_level < 15)
            you.levitation--;

        if (you.equip[EQ_BOOTS] != -1
            && you.inv_dam[you.equip[EQ_BOOTS]] % 30 == SPARM_LEVITATION)
            you.levitation = 2;

        if (you.levitation == 10)
        {
            mpr("You are starting to lose your buoyancy!", MSGCH_DURATION);
            you.levitation -= random2(6);

            if (you.duration[DUR_CONTROLLED_FLIGHT] > 0)
                you.duration[DUR_CONTROLLED_FLIGHT] = you.levitation;
        }
    }
    else if (you.levitation == 1)
    {
        mpr("You float gracefully downwards.", MSGCH_DURATION);
        you.levitation = 0;
        burden_change();
        you.duration[DUR_CONTROLLED_FLIGHT] = 0;
        if (grd[you.x_pos][you.y_pos] == DNGN_LAVA
            || grd[you.x_pos][you.y_pos] == DNGN_DEEP_WATER)
        {
            if (you.species == SP_MERFOLK && grd[you.x_pos][you.y_pos] == DNGN_DEEP_WATER)
            {
                mpr("You dive into the water and return to your normal form.");
                merfolk_start_swimming();
            }

            fall_into_a_pool(true, grd[you.x_pos][you.y_pos]);
        }
    }

    if (you.rotting > 0)
    {
        if (you.species == SP_MUMMY)
            you.rotting = 0;
        else if (random2(20) <= (you.rotting - 1))
        {
            mpr("You feel your flesh rotting away.", MSGCH_WARN);
            ouch(1, 0, KILLED_BY_ROTTING);
            you.hp_max--;
            you.base_hp--;
            you.rotting--;
        }
    }

    // ghoul rotting is special, but will deduct from you.rotting
    // if it happens to be positive - because this is placed after
    // the "normal" rotting check, rotting attacks can be somewhat
    // more painful on ghouls - reversing order would make rotting
    // attacks somewhat less painful, but that seems wrong-headed {dlb}:
    if (you.species == SP_GHOUL)
    {
        if (one_chance_in(400))
        {
            mpr("You feel your flesh rotting away.", MSGCH_WARN);
            ouch(1, 0, KILLED_BY_ROTTING);
            you.hp_max--;
            you.base_hp--;

            if (you.rotting > 0)
                you.rotting--;
        }
    }


    if (you.disease > 0)
    {
        you.disease--;

        if (you.species == SP_KOBOLD && you.disease > 5)
            you.disease -= 2;

        if (!you.disease)
            mpr("You feel your health improve.", MSGCH_RECOVERY);
    }

    if (you.poison > 0)
    {
        if (random2(5) <= (you.poison - 1))
        {
            if (you.poison > 10 && random2(you.poison) >= 8)
            {
                ouch(random2(10) + 5, 0, KILLED_BY_POISON);
                mpr("You feel extremely sick.", MSGCH_DANGER);
            }
            else if (you.poison > 5 && coinflip())
            {
                ouch((coinflip()? 3 : 2), 0, KILLED_BY_POISON);
                mpr("You feel very sick.", MSGCH_WARN);
            }
            else
            {
                // the poison running through your veins.");
                ouch(1, 0, KILLED_BY_POISON);
                mpr("You feel sick.");
            }

            if ((you.hp == 1 && one_chance_in(3)) || one_chance_in(8))
            {
                mpr("You feel a little better.", MSGCH_RECOVERY);
                you.poison--;
            }
        }
    }

    if (you.deaths_door)
    {
        if (you.hp > you.skills[SK_NECROMANCY] +
                        (you.religion == GOD_KIKUBAAQUDGHA ? 13 : 0))
        {
            mpr("Your life is in your own hands once again.", MSGCH_DURATION);
            you.paralysis += 5 + random2(5);
            you.conf += 10 + random2(10);
            you.hp_max--;
            deflate_hp(you.hp_max, false);
            you.deaths_door = 0;
        }
        else
            you.deaths_door--;

        if (you.deaths_door == 10)
        {
            mpr("Your time is quickly running out!", MSGCH_DURATION);
            you.deaths_door -= random2(6);
        }
        if (you.deaths_door == 1)
        {
            mpr("Your life is in your own hands again!", MSGCH_DURATION);
            more();
        }
    }

    if (you.is_undead != US_UNDEAD)
    {
        int food_use = player_hunger_rate() + you.burden_state;

        if (food_use > 0 && you.hunger >= 40)
            make_hungry(food_use, true);
    }
    else
    {
        you.hunger = 6000;      // this is a kludge {dlb}
    }

    if (you.hp < you.hp_max && !you.disease && !you.deaths_door)
        you.hit_points_regeneration += player_regen();

    if (you.magic_points < you.max_magic_points)
        you.magic_points_regeneration += 7 + you.max_magic_points / 2;

    while (you.hit_points_regeneration >= 100)
    {
        if (you.hp >= you.hp_max - 1 && you.running
                                        && you.run_x == 0 && you.run_y == 0)
        {
            you.running = 0;
        }

        inc_hp(1, false);
        you.hit_points_regeneration -= 100;
    }

    while (you.magic_points_regeneration >= 100)
    {
        if (you.magic_points >= you.max_magic_points - 1 && you.running
                                        && you.run_x == 0 && you.run_y == 0)
        {
            you.running = 0;
        }

        inc_mp(1, false);
        you.magic_points_regeneration -= 100;
    }

    viewwindow(1, true);

    monster();

    ASSERT(you.time_taken >= 0);
    // make sure we don't overflow
    ASSERT(DBL_MAX - you.elapsed_time > you.time_taken);

    you.elapsed_time += you.time_taken;

    if (you.synch_time <= you.time_taken)
    {
        handle_time(200 + (you.time_taken - you.synch_time));
        you.synch_time = 200;

        if (one_chance_in(50))
            cull_items();
    }
    else
    {
        you.synch_time -= you.time_taken;
    }

    manage_clouds();

    if (you.shock_shield > 0)
        manage_shock_shield();

    /*
       If visible [0] is 1, at least one statue is visible.
       This was supposed to be more complex (with values of 2 etc), but I
       couldn't get it to work.
     */
    if (visible[0])
    {
        char wc[30];

        if (visible[1])
        {
            switch (visible[1])
            {
            case 0:
                break;
            //case 1: mpr("You feel a pleasing absence."); break;
            case 2:
                if (one_chance_in(4))
                {
                    strcpy(info, "The silver statue's eyes glow a ");
                    weird_colours(random2(256), wc);
                    strcat(info, wc);
                    strcat(info, " colour.");
                    mpr(info, MSGCH_WARN);
                    create_monster(
                        summon_any_demon((coinflip() ? DEMON_COMMON
                                                     : DEMON_LESSER)),
                                             ENCH_ABJ_V, BEH_HOSTILE,
                                             you.x_pos, you.y_pos,
                                             MHITYOU, 250);
                }
                break;
            //case 3: mpr("You feel a terrible presence observing you."); break;
            }

            visible[1]--;
        }

        if (visible[2])
        {
            if (one_chance_in(3))
            {
                mpr("A hostile presence attacks your mind!", MSGCH_WARN);
                miscast_effect(SPTYP_DIVINATION, random2(15), random2(150), 100);
            }
            visible[2]--;
        }
        visible[0]--;
    }

    if (you.hunger <= 500)
    {
        if (!you.paralysis && one_chance_in(40))
        {
            mpr("You lose consciousness!", MSGCH_FOOD);
            you.paralysis += 5 + random2(8);

            if (you.paralysis > 13)
                you.paralysis = 13;
        }

        if (you.hunger <= 100)
        {
            mpr("You have starved to death.", MSGCH_FOOD);
            ouch(-9999, 0, KILLED_BY_STARVATION);
        }
    }

    //jmf: added silence messages
    its_quiet = silenced(you.x_pos, you.y_pos);

    if (you.attribute[ATTR_WAS_SILENCED] != its_quiet)
    {
        if (its_quiet)
        {
            if (random2(30))
                mpr("You are enveloped in profound silence.", MSGCH_WARN);
            else
                mpr("The dungeon seems quiet ... too quiet!", MSGCH_WARN);
        }
        else
        {
            mpr("Your hearing returns.", MSGCH_RECOVERY);
        }

        you.attribute[ATTR_WAS_SILENCED] = its_quiet;
    }

    viewwindow(1, false);

    if (you.paralysis > 0 && any_messages())
        more();

    // place normal dungeon monsters,  but not in player LOS
    if (you.level_type == LEVEL_DUNGEON
        && you.where_are_you != BRANCH_ECUMENICAL_TEMPLE
        && one_chance_in(240))
    {
        int prox = 2;       // not near player

        // generate 10% of wandering monsters near stairs
        if (one_chance_in(10))
            prox = 3;

        mons_place(WANDERING_MONSTER, BEH_HOSTILE, MHITNOT, false,
            50,50, LEVEL_DUNGEON, prox);
    }

    // place Abyss monsters.
    if (you.level_type == LEVEL_ABYSS && one_chance_in(5))
    {
        mons_place(WANDERING_MONSTER, BEH_HOSTILE, MHITNOT, false,
            50,50, LEVEL_ABYSS);
    }

    // place Pandemonium monsters
    if (you.level_type == LEVEL_PANDEMONIUM && one_chance_in(50))
        pandemonium_mons();

    // No monsters in the Labyrinth,  or Ecumenical Temple
    return;
}

/*
   Opens doors and handles some aspects of untrapping. If either move_x or
   move_y are non-zero,  the pair carries a specific direction for the door
   to be opened (eg if you type ctrl - dir).
 */
static void open_door(char move_x, char move_y)
{
    struct dist door_move;
    int dx, dy;             // door x, door y

    door_move.dx = move_x;
    door_move.dy = move_y;

    if (move_x || move_y)
    {
        // convenience
        dx = you.x_pos + move_x;
        dy = you.y_pos + move_y;

        if (mgrd[dx][dy] != NON_MONSTER
            && (monster_habitat( menv[ mgrd[dx][dy]].type ) == DNGN_FLOOR
            ||  menv[ mgrd[dx][dy]].number == 0))
        {
            you_attack(mgrd[dx][dy], true);
            you.turn_is_over = 1;

            if (you.berserk_penalty != NO_BERSERK_PENALTY)
            {
                // we don't want to create a penalty if there isn't
                // supposed to be one
                you.berserk_penalty = 0;
            }

            return;
        }

        if (grd[dx][dy] >= DNGN_TRAP_MECHANICAL
            && grd[dx][dy] <= DNGN_TRAP_III)
        {
            if (env.cgrid[dx][dy] != EMPTY_CLOUD)
            {
                mpr("You can't get to that trap right now.");
                return;
            }

            disarm_trap(door_move);
            return;
        }
    }

    if (!(move_x || move_y))
    {
        mpr("Which direction?", MSGCH_PROMPT);
        direction(door_move, DIR_DIR);
        if (!door_move.isValid)
            return;

        // convenience
        dx = you.x_pos + door_move.dx;
        dy = you.y_pos + door_move.dy;

    }

    if (grd[dx][dy] == DNGN_CLOSED_DOOR)
    {
        int skill = you.dex + (SK_TRAPS_DOORS + SK_STEALTH) / 2;

        if (one_chance_in(skill) && !silenced(you.x_pos, you.y_pos))
        {
            mpr( "As you open the door, it creaks loudly!" );
            noisy( 15, you.x_pos, you.y_pos );
        }
        else
        {
            mpr( (you.levitation) ? "You reach down and open the door."
                                 : "You open the door." );
        }

        grd[dx][dy] = DNGN_OPEN_DOOR;
        you.turn_is_over = 1;
    }
    else
    {
        mpr("You swing at nothing.");
        you.turn_is_over = 1;

        if (you.is_undead != US_UNDEAD)
            make_hungry(3, true);
    }
}                               // end open_door()

/*
   Similar to open_door. Can you spot the difference?
 */
static void close_door(char door_x, char door_y)
{
    struct dist door_move;
    int dx, dy;             // door x, door y

    door_move.dx = door_x;
    door_move.dy = door_y;

    if (!(door_x || door_y))
    {
        mpr("Which direction?", MSGCH_PROMPT);
        direction(door_move, DIR_DIR);
        if (!door_move.isValid)
            return;
    }

    if (door_move.dx == 0 && door_move.dy == 0)
    {
        mpr("You can't close doors on yourself!");
        return;
    }

    // convenience
    dx = you.x_pos + door_move.dx;
    dy = you.y_pos + door_move.dy;

    if (mgrd[dx][dy] != NON_MONSTER)
    {
        // Need to make sure that turn_is_over = 1 if creature is invisible
        mpr("There's a creature in the doorway!");
        door_move.dx = 0;
        door_move.dy = 0;
        return;
    }

    if (grd[dx][dy] == DNGN_OPEN_DOOR)
    {

        if (igrd[dx][dy] != NON_ITEM)
        {
            mpr("There's something blocking the doorway.");
            door_move.dx = 0;
            door_move.dy = 0;
            return;
        }

        int skill = you.dex + (SK_TRAPS_DOORS + SK_STEALTH) / 2;

        if (one_chance_in(skill) && !silenced(you.x_pos, you.y_pos))
        {
            mpr("As you close the door, it creaks loudly!");
            noisy(15, you.x_pos, you.y_pos);
        }
        else
        {
            mpr((you.levitation) ? "You reach down and close the door."
                                 : "You close the door.");
        }

        grd[dx][dy] = DNGN_CLOSED_DOOR;
        you.turn_is_over = 1;
    }
    else
    {
        mpr("There isn't anything that you can close there!");
    }
}                               // end open_door()


// initialise whole lot of stuff...
// returns true if a new character
static bool initialise(void)
{
    bool ret;

    int i = 0, j = 0;           // counter variables {dlb}

    your_sign = '@';
    your_colour = LIGHTGREY;

    // system initialisation stuff:
    textbackground(0);

#ifdef DOS
    directvideo = 1;
#endif

#ifdef USE_EMX
    init_emx();
#endif

    srandom(time(NULL));
    srand(time(NULL));
    cf_setseed();               // required for stuff::coinflip()

    mon_init(mcolour);          // this needs to be way up top {dlb}
    init_playerspells();        // this needs to be way up top {dlb}

    clrscr();

// init item array:
    for (i = 1; i < MAX_ITEMS; i++)
    {
        mitm.base_type[i] = 0;
        mitm.sub_type[i] = 0;
        mitm.x[i] = 0;
        mitm.y[i] = 0;
        mitm.quantity[i] = 0;
        mitm.special[i] = 0;
        mitm.pluses[i] = 0;
        mitm.pluses2[i] = 0;
        mitm.link[i] = NON_ITEM;
    }

// empty messaging string
    strcpy(info, "");

    for (i = 0; i < MAX_MONSTERS; i++)
    {
        menv[i].type = -1;
        menv[i].speed_increment = 10;
        menv[i].target_x = 155;
        menv[i].flags = 0;
        menv[i].behavior = BEH_SLEEP;

        menv[i].foe = NON_MONSTER;
        menv[i].attitude = ATT_HOSTILE;

        for (j = 0; j < NUM_MON_ENCHANTS; j++)
            menv[i].enchantment[j] = ENCH_NONE;

        for (j = 0; j < NUM_MONSTER_SLOTS; j++)
            menv[i].inv[j] = NON_ITEM;

        menv[i].number = 0;
    }

    for (i = 0; i < GXM; i++)
    {
        for (j = 0; j < GYM; j++)
        {
            igrd[i][j] = NON_ITEM;
            mgrd[i][j] = NON_MONSTER;
            env.map[i][j] = 0;
        }
    }

    for (i = 0; i < 10; i++)
    {
        visible[i] = 0;
    }

    // initialize tag system before we try loading anything!
    tag_init();

    // sets up a new game:
    bool newc = new_game();
    ret = newc;  // newc will be mangled later so we'll take a copy --bwr

    if (!newc)
        restore_game();

    game_has_started = true;

    calc_hp();
    calc_mp();

    //if ( newc )
    //  stair_taken = 82;

    // DON'T uSE num_inv_items!!
    // you.num_inv_items = 0;

    bool just_made_new_lev = !newc;
    bool moving_level = newc;

    //load(82, moving_level, level_saved, was_a_labyrinth, old_level, just_made_new_lev);
    load( 82, moving_level, false, 0, false, just_made_new_lev,
                                                    you.where_are_you );

    moving_level = false;
    just_made_new_lev = false;
    newc = false;

    init_properties();
    burden_change();
    make_hungry(0,true);

    if (newc)
    {
        for (i = 0; i < GXM; i++)
        {
            for (j = 0; j < GYM; j++)
                if (grd[i][j] == 68)
                {
                    you.x_pos = i;
                    you.y_pos = j;
                }

            if (grd[i][j] == DNGN_FLOOR)
                break;
        }

        new_level();
    }                           // end if (newc)

    you.redraw_strength = 1;
    you.redraw_intelligence = 1;
    you.redraw_dexterity = 1;
    you.redraw_armor_class = 1;
    you.redraw_evasion = 1;
    you.redraw_experience = 1;
    you.redraw_gold = 1;
    wield_change = true;

    draw_border(you.your_name, player_title(), you.species);
    new_level();

    // set vision radius to player's current vision
    setLOSRadius(you.current_vision);
    viewwindow(1, false);   // This just puts the view up for the first turn.
    item();

    return (ret);
}

// An attempt to tone down berserk a little bit. -- bwross
//
// This function does the accounting for not attacking while berserk
// This gives a triangluar number function for the additional penalty
// Turn:    1  2  3   4   5   6   7   8
// Penalty: 1  3  6  10  15  21  28  36
//
// Total penalty (including the standard one during upkeep is:
//          2  5  9  14  20  27  35  44
//
static void do_berserk_no_combat_penalty(void)
{
    // Butchering/eating a corpse will maintain a blood rage.
    if (you.delay_t && (you.delay_doing == 4 || you.delay_doing == 0))
        return;

    if (you.berserk_penalty == NO_BERSERK_PENALTY)
        return;

    if (you.berserker)
    {
        you.berserk_penalty++;

        switch (you.berserk_penalty)
        {
        case 2:
            mpr("You feel a strong urge to attack something.", MSGCH_DURATION);
            break;
        case 4:
            mpr("You feel your anger subside.", MSGCH_DURATION);
            break;
        case 6:
            mpr("Your blood rage is quickly leaving you.", MSGCH_DURATION);
            break;
        }

        // I do these three separately, because the might and
        // haste counters can be different.
        you.berserker -= you.berserk_penalty;
        if (you.berserker < 1)
            you.berserker = 1;

        you.might -= you.berserk_penalty;
        if (you.might < 1)
            you.might = 1;

        you.haste -= you.berserk_penalty;
        if (you.haste < 1)
            you.haste = 1;
    }
    return;
}                               // end do_berserk_no_combat_penalty()


// Called when the player moves by walking/running. Also calls
// attack function and trap function etc when necessary.
static void move_player(char move_x, char move_y)
{
    bool attacking = false;
    bool moving = true;         // used to prevent eventual movement (swap)

    const int targ_x = you.x_pos + move_x;
    const int targ_y = you.y_pos + move_y;
    const unsigned char old_grid   =  grd[ you.x_pos ][ you.y_pos ];
    const unsigned char targ_grid  =  grd[ targ_x ][ targ_y ];
    const unsigned char targ_monst = mgrd[ targ_x ][ targ_y ];

    //char move_x, move_y;
    // probably should use the extern'd version instead {dlb}
    //char info[200];

    int i;
    bool trap_known;

    if (you.conf)
    {
        if (!one_chance_in(3))
        {
            move_x = random2(3) - 1;
            move_y = random2(3) - 1;
        }

        const int new_targ_x = you.x_pos + move_x;
        const int new_targ_y = you.y_pos + move_y;
        const unsigned char new_targ_grid = grd[ new_targ_x ][ new_targ_y ];

        if (new_targ_grid < MINMOVE)
        {
            you.turn_is_over = 1;
            mpr("Ouch!");
            return;
        }

        if ((new_targ_grid == DNGN_LAVA || new_targ_grid == DNGN_DEEP_WATER)
             && !you.levitation)
        {
            if (you.species == SP_MERFOLK && new_targ_grid == DNGN_DEEP_WATER)
            {
                mpr("You stumble into the water and return to your normal form.");
                merfolk_start_swimming();
            }

            fall_into_a_pool( false, new_targ_grid );
            you.turn_is_over = 1;
            do_berserk_no_combat_penalty();
            return;
        }
    }                           // end of if you.conf

    if (you.running > 0 && you.running != 2
            && targ_grid != DNGN_FLOOR && targ_grid != DNGN_UNDISCOVERED_TRAP)
    {
        // BCR - Easy doors running
        if (targ_grid == DNGN_CLOSED_DOOR && Options.easy_open)
            open_door(move_x, move_y);
        else
        {
            you.running = 0;
            move_x = 0;
            move_y = 0;
            you.turn_is_over = 0;
        }
        return;
    }

    if (targ_monst != NON_MONSTER)
    {
        struct monsters *mon = &menv[targ_monst];

        if (monster_habitat( menv[ targ_monst ].type ) != DNGN_FLOOR
                          && menv[ targ_monst ].number == 1)
        {
            goto break_out;
        }

        // you can swap places with a friendly monster if you
        // can see it and you're not confused
        if (mons_friendly(mon) && (!mons_has_ench(mon, ENCH_INVIS)
            || player_see_invis()) && !you.conf)
        {
            if (!swap_places( &menv[ targ_monst ] ))
                moving = false;

            goto break_out;
        }

        you_attack( targ_monst, true );
        you.turn_is_over = 1;

        // we don't want to create a penalty if there isn't
        // supposed to be one
        if (you.berserk_penalty != NO_BERSERK_PENALTY)
            you.berserk_penalty = 0;

        attacking = true;
    }

  break_out:
    // Handle dangerous tiles
    if ((targ_grid == DNGN_LAVA || targ_grid == DNGN_DEEP_WATER)
        && !attacking && !you.levitation && moving)
    {
        // Merfold automatically enter deep water... every other case
        // we ask for confirmation.
        if (you.species == SP_MERFOLK && targ_grid == DNGN_DEEP_WATER)
        {
            // Only mention diving if we just entering the water.
            if (!player_in_water())
            {
                mpr("You dive into the water and return to your normal form.");
                merfolk_start_swimming();
                //fall_into_a_pool( false, targ_grid );
                //you.turn_is_over = 1;
                //do_berserk_no_combat_penalty();
            }
        }
        else
        {
            bool enter = yesno("Do you really want to step there?", false);

            if (enter)
            {
                fall_into_a_pool( false, targ_grid );
                you.turn_is_over = 1;
                do_berserk_no_combat_penalty();
                return;
            }
            else
            {
                canned_msg(MSG_OK);
                return;
            }
        }
    }

    if (!attacking && targ_grid >= MINMOVE && moving)
    {
        if (targ_grid == DNGN_UNDISCOVERED_TRAP
                && random2(you.skills[SK_TRAPS_DOORS] + 1) > 3)
        {
            strcpy(info, "Wait a moment, ");
            strcat(info, you.your_name);
            strcat(info, "! Do you really want to step there?");
            mpr(info, MSGCH_WARN);
            more();
            you.turn_is_over = 0;

            i = trap_at_xy( targ_x, targ_y );
            grd[ targ_x ][ targ_y ] = trap_category(env.trap_type[i]);
            return;
        }

        you.x_pos += move_x;
        you.y_pos += move_y;

        if (targ_grid == DNGN_SHALLOW_WATER && !you.levitation)
        {
            if (you.species != SP_MERFOLK)
            {
                if (one_chance_in(3) && !silenced(you.x_pos, you.y_pos))
                {
                    mpr("Splash!");
                    noisy(10, you.x_pos, you.y_pos);
                }

                you.time_taken *= 13 + random2(8);
                you.time_taken /= 10;

                if (old_grid != DNGN_SHALLOW_WATER)
                {
                    mpr( "You enter the shallow water. "
                         "Moving in this stuff is going to be slow." );

                    if (you.invis)
                        mpr( "And don't expect to remain undetected." );
                }
            }
            else if (old_grid != DNGN_SHALLOW_WATER
                    && old_grid != DNGN_DEEP_WATER)
            {
                mpr("You return to your normal form as you enter the water.");
                merfolk_start_swimming();
            }
        }

        move_x = 0;
        move_y = 0;

        you.time_taken *= player_movement_speed();
        you.time_taken /= 10;

#if 0
        if (player_fast_run())
        {
            you.time_taken *= 6;
            you.time_taken /= 10;
        }

        if (you.attribute[ATTR_WALK_SLOWLY])
        {
            you.time_taken *= 14;
            you.time_taken /= 10;
        }
#endif

        you.turn_is_over = 1;
        item_check(0);

        if (targ_grid >= DNGN_TRAP_MECHANICAL
                                && targ_grid <= DNGN_UNDISCOVERED_TRAP)
        {
            if (targ_grid == DNGN_UNDISCOVERED_TRAP)
            {
                i = trap_at_xy(you.x_pos, you.y_pos);
                grd[ you.x_pos ][ you.y_pos ] = trap_category(env.trap_type[i]);
                trap_known = false;
            }
            else
                trap_known = true;

            i = trap_at_xy( you.x_pos, you.y_pos );

            if (you.levitation
                && trap_category(env.trap_type[i]) == DNGN_TRAP_MECHANICAL)
            {
                goto out_of_traps;      // can fly over mechanical traps
            }

            handle_traps(env.trap_type[i], i, trap_known);
        }                       // end of if another grd == trap
    }

  out_of_traps:
    // BCR - Easy doors single move
    if (targ_grid == DNGN_CLOSED_DOOR && Options.easy_open)
        open_door(move_x, move_y);
    else if (targ_grid <= MINMOVE)
    {
        move_x = 0;
        move_y = 0;
        you.running = 0;
        you.turn_is_over = 0;
    }

    if (you.running == 2)
        you.running = 1;

    if (you.level_type == LEVEL_ABYSS
            && (you.x_pos <= 21 || you.x_pos >= (GXM - 21)
                    || you.y_pos <= 15 || you.y_pos >= (GYM - 16)))
    {
        env.cloud_no = area_shift();
        you.pet_target = MHITNOT;
#ifdef WIZARD
        mpr("Shifting.");
        int igly = 0;
        int ig2 = 0;

        for (igly = 0; igly < MAX_ITEMS; igly++)
        {
            if (mitm.quantity[igly] != 0)
                ig2++;
        }

        strcpy(info, "No of items present: ");
        itoa(ig2, st_prn, 10);
        strcat(info, st_prn);
        mpr(info);
        ig2 = 0;

        for (igly = 0; igly < MAX_MONSTERS; igly++)
        {
            if (menv[igly].type != -1)
                ig2++;
        }

        strcpy(info, "No of monsters present: ");
        itoa(ig2, st_prn, 10);
        strcat(info, st_prn);
        mpr(info);

        strcpy(info, "No of clouds present: ");
        itoa(env.cloud_no, st_prn, 10);
        strcat(info, st_prn);
        mpr(info);

#endif
    }

    if (!attacking)
    {
        do_berserk_no_combat_penalty();
    }
}                               // end move_player()

/*
 *  File:       direct.cc
 *  Summary:    Functions used when picking squares.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *       <4>     11/23/99       LRH                             Looking at monsters now
 *                                                                                              displays more info
 *       <3>     5/12/99        BWR             changes to allow for space selection of target.
 *                                              CR, ESC, and 't' in targeting.
 *
 *       <2>     5/09/99        JDJ             look_around no longer prints a prompt.
 *       <1>     -/--/--        LRH             Created
 */

#include "AppHdr.h"
#include "direct.h"

#ifdef DOS
#include <conio.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "externs.h"

#include "debug.h"
#include "view.h"
#include "itemname.h"
#include "mstruct.h"
#include "describe.h"
#include "player.h"
#include "monstuff.h"
#include "stuff.h"

#ifdef MACROS
#include "macro.h"
#endif

char mons_find(unsigned char xps, unsigned char yps, char mfp[2], char direction);


//---------------------------------------------------------------
//
// direction
//
// Handles entering of directions. Passes to look_around for cursor
// aiming.
//
//---------------------------------------------------------------
void direction(char rnge, struct dist moves[1])
{
    char ink = 0;
    char looked = 0;

    moves[0].nothing = dir_cursor(rnge);
    char info[200];

    if (moves[0].nothing == -9999 || moves[0].nothing == -10000 || moves[0].nothing == -10001)
    {
        mpr("Aim (move cursor or select with '-' or '+'/'=', then 'p', '.', or '>')");
        moves[0].prev_targ = 0;
        if (moves[0].nothing == -10000)
            moves[0].prev_targ = 1;
        if (moves[0].nothing == -10001)
            moves[0].prev_targ = 2;
        moves[0].nothing = look_around(moves);
        looked = 1;
    }

    if (moves[0].nothing > 50000)
    {
        moves[0].nothing -= 50000;
    }

    if (moves[0].nothing > 10000)
    {
        moves[0].nothing -= 10000;
        ink = 1;
    }

    if (moves[0].nothing == -1)
    {
        mpr("What an unusual direction.");
        return;
    }

    if (moves[0].nothing == 253)
    {
        if (you.prev_targ == MHITNOT || you.prev_targ >= MNST)
        {
            strcpy(info, "You haven't got a target.");
            mpr(info);
            moves[0].nothing = -1;
            return;
        }

        if (!mons_near(you.prev_targ)
            || (menv[you.prev_targ].enchantment[2] == ENCH_INVIS
                && player_see_invis() == 0))
        {
            strcpy(info, "You can't see that creature any more.");
            mpr(info);
            moves[0].nothing = -1;
            return;
        }

        moves[0].move_x = menv[you.prev_targ].x - you.x_pos;
        moves[0].move_y = menv[you.prev_targ].y - you.y_pos;
        moves[0].target_x = moves[0].move_x + you.x_pos;
        moves[0].target_y = moves[0].move_y + you.y_pos;
        return;
    }


    if (moves[0].nothing == 254)
    {
        moves[0].nothing = -1;
        return;
    }

    if (looked == 0)
    {
        moves[0].move_x = ((int) (moves[0].nothing / 100)) - 7;
        moves[0].move_y = moves[0].nothing - (((int) moves[0].nothing / 100) * 100) - 7;
    }

    /*target_x = move_x + you.x_pos;
       target_y = move_y + you.y_pos; */

    if (ink == 1)
    {
        moves[0].target_x = 1;
        moves[0].target_y = 1;
    }


    if (moves[0].move_x == 0)
    {
        if (moves[0].move_y > 0)
            moves[0].move_y = 1;
        if (moves[0].move_y < 0)
            moves[0].move_y = -1;
    }

    if (moves[0].move_y == 0)
    {
        if (moves[0].move_x > 0)
            moves[0].move_x = 1;
        if (moves[0].move_x < 0)
            moves[0].move_x = -1;
    }

    if (moves[0].move_x == moves[0].move_y || moves[0].move_x * -1 == moves[0].move_y)
    {
        if (moves[0].move_y > 0)
            moves[0].move_y = 1;
        if (moves[0].move_y < 0)
            moves[0].move_y = -1;
        if (moves[0].move_x > 0)
            moves[0].move_x = 1;
        if (moves[0].move_x < 0)
            moves[0].move_x = -1;
    }

    if (env.mgrid[moves[0].target_x][moves[0].target_y] != MNG)
    {

        if (rnge != 100)
        {
            you.prev_targ = env.mgrid[moves[0].target_x][moves[0].target_y];
        }
    }
}                               // end of direction()


//---------------------------------------------------------------
//
// dir_cursor
//
// Another cursor input thing.
//
//---------------------------------------------------------------
int dir_cursor(char rng)
{
    char mve_x = 0, mve_y = 0;
    char bk = 0;
    int keyy;

    if (rng == 100)
        return -9999;

  getkey:
    keyy = getch();

#ifdef LINUX
    keyy = translate_keypad(keyy);
#endif

    if (keyy != 0 && keyy != '*' && keyy != '.')
    {
        switch (keyy)
        {
        case 'b':
        case '1':
            mve_x = -1;
            mve_y = 1;
            break;

        case 'j':
        case '2':
            mve_y = 1;
            mve_x = 0;
            break;

        case 'u':
        case '9':
            mve_x = 1;
            mve_y = -1;
            break;

        case 'k':
        case '8':
            mve_y = -1;
            mve_x = 0;
            break;

        case 'y':
        case '7':
            mve_y = -1;
            mve_x = -1;
            break;

        case 'h':
        case '4':
            mve_x = -1;
            mve_y = 0;
            break;

        case 'n':
        case '3':
            mve_y = 1;
            mve_x = 1;
            break;

        case 'l':
        case '6':
            mve_x = 1;
            mve_y = 0;
            break;

        case 't':
        case 'p':
            return 253;

        case '=':
        case '+':
            return -10000;

        case '-':
            return -10001;

        case 27:
            return 254;

        default:
            goto getkey;
        }
        return mve_x * 100 + mve_y + 707 + 10000;

    }


    if (keyy != '*' && keyy != '.')
        keyy = getch();

    switch (keyy)
    {
    case 'O':
        mve_x = -1;
        mve_y = 1;
        bk = 1;
        break;
    case 'P':
        mve_y = 1;
        mve_x = 0;
        bk = 1;
        break;
    case 'I':
        mve_x = 1;
        mve_y = -1;
        bk = 1;
        break;
    case 'H':
        mve_y = -1;
        mve_x = 0;
        bk = 1;
        break;
    case 'G':
        mve_y = -1;
        mve_x = -1;
        bk = 1;
        break;
    case 'K':
        mve_x = -1;
        mve_y = 0;
        bk = 1;
        break;
    case 'Q':
        mve_y = 1;
        mve_x = 1;
        bk = 1;
        break;
    case 'M':
        mve_x = 1;
        mve_y = 0;
        bk = 1;
        break;
    case '.':
    case 'S':
        bk = 1;
        mve_x = 0;
        mve_y = 0;
        break;
    case '*':
        bk = 0;
        break;                  // was 'L'

    default:
        return -1;
    }

    if (bk == 1)
    {
        return mve_x * 100 + mve_y + 707 + 10000;
    }

    if (rng == 0)
        return 254;

    return -9999;

}


//---------------------------------------------------------------
//
// look_around
//
// Accessible by the x key and when using cursor aiming. Lets you
// find out what symbols mean, and is the way to access monster
// descriptions.
//
//---------------------------------------------------------------
int look_around(struct dist moves[1])
{
    int xps = 17;
    int yps = 9;
    int gotch;
    char mve_x = 0;
    char mve_y = 0;
    int trf = 0;
    char monsfind_pos[2];
    int p = 0;

    char printed_already = 1;

    monsfind_pos[0] = you.x_pos;
    monsfind_pos[1] = you.y_pos;

    if (you.prev_targ != MHITNOT && you.prev_targ < MNST)
    {
        if (mons_near(you.prev_targ)
            && (menv[you.prev_targ].enchantment[2] != ENCH_INVIS
                || player_see_invis() != 0))
        {
            strcpy(info, "You are currently targetting ");
            strcat(info, monam(menv[you.prev_targ].number, menv[you.prev_targ].type,
                               menv[you.prev_targ].enchantment[2], 1));
            strcat(info, " (p to target).");
            mpr(info);
        }
        else
            mpr("You have no current target.");
    }

    gotoxy(xps, yps);
    gotoxy(xps + 1, yps);

    losight(env.show, grd, you.x_pos, you.y_pos);

    do
    {
        if (moves[0].prev_targ == 0)
        {
            gotch = getch();

#ifdef LINUX
            gotch = translate_keypad(gotch);
#endif
        }
        else
        {
            if (moves[0].prev_targ == 1)
                gotch = '+';
            else
                gotch = '-';
            moves[0].prev_targ = 0;
            printed_already = 0;
        }


        if (gotch != 0 && gotch != 13)
        {
            switch (gotch)
            {
            case '.':
            case ' ':
            case '\n':
            case '5':
                mve_x = 0;
                mve_y = 0;
                gotch = 'S';
                goto thingy;

            case 'b':
            case '1':
                mve_x = -1;
                mve_y = 1;
                break;

            case 'j':
            case '2':
                mve_y = 1;
                mve_x = 0;
                break;

            case 'u':
            case '9':
                mve_x = 1;
                mve_y = -1;
                break;

            case 'k':
            case '8':
                mve_y = -1;
                mve_x = 0;
                break;

            case 'y':
            case '7':
                mve_y = -1;
                mve_x = -1;
                break;

            case 'h':
            case '4':
                mve_x = -1;
                mve_y = 0;
                break;

            case 'n':
            case '3':
                mve_y = 1;
                mve_x = 1;
                break;

            case 'l':
            case '6':
                mve_x = 1;
                mve_y = 0;
                break;

            case '?':
                mve_x = 0;
                mve_y = 0;
                if (mgrd[you.x_pos + xps - 17][you.y_pos + yps - 9] == MNG)
                    continue;

                if (menv[mgrd[you.x_pos + xps - 17][you.y_pos + yps - 9]].enchantment[2] == ENCH_INVIS && player_see_invis() == 0)
                    continue;

                if (menv[mgrd[you.x_pos + xps - 17][you.y_pos + yps - 9]].type >= MONS_LAVA_WORM && menv[mgrd[you.x_pos + xps - 17][you.y_pos + yps - 9]].number == 1)
                    continue;

                describe_monsters(menv[mgrd[you.x_pos + xps - 17][you.y_pos + yps - 9]].type, mgrd[you.x_pos + xps - 17][you.y_pos + yps - 9]);

#ifdef PLAIN_TERM
                redraw_screen();
#endif
                break;

            case 'p':
                goto finished_looking;

            case '>':
                goto finished_looking;

            case '-':
                mve_x = 0;
                mve_y = 0;
                if (mons_find(xps, yps, monsfind_pos, -1) == 1)
                {
                    xps = monsfind_pos[0];
                    yps = monsfind_pos[1];
                }
                break;

            case '+':
            case '=':
                mve_x = 0;
                mve_y = 0;
                if (mons_find(xps, yps, monsfind_pos, 1) == 1)
                {
                    xps = monsfind_pos[0];
                    yps = monsfind_pos[1];
                }
                break;

            default:
                return -1;
            }

            goto gotchy;
        }

        if (gotch != 13)
            gotch = getch();

        mve_x = 0;
        mve_y = 0;

      thingy:
        switch (gotch)
        {
        case 13:
            gotch = 'S';
            break;

        case 'O':
            mve_x = -1;
            mve_y = 1;
            break;

        case 'P':
            mve_y = 1;
            mve_x = 0;
            break;

        case 'I':
            mve_x = 1;
            mve_y = -1;
            break;

        case 'H':
            mve_y = -1;
            mve_x = 0;
            break;

        case 'G':
            mve_y = -1;
            mve_x = -1;
            break;

        case 'K':
            mve_x = -1;
            mve_y = 0;
            break;

        case 'Q':
            mve_y = 1;
            mve_x = 1;
            break;

        case 'M':
            mve_x = 1;
            mve_y = 0;
            break;

        case 'S':
            break;
            // need <, > etc

        default:
            return -1;
        }

      gotchy:
        gotoxy(xps, yps);

        if (xps + mve_x >= 9 && xps + mve_x < 26)
            xps += mve_x;
        if (yps + mve_y >= 1 && yps + mve_y < 18)
            yps += mve_y;

        if (printed_already == 1)
            mesclr();
        printed_already = 1;

        if (env.show[xps - 8][yps] == 0 && (xps != 17 || yps != 9))
        {
            mpr("You can't see that place.");
            goto glogokh;
        }

        if (mgrd[you.x_pos + xps - 17][you.y_pos + yps - 9] != MNG)
        {
            int i = mgrd[you.x_pos + xps - 17][you.y_pos + yps - 9];

            if (grd[you.x_pos + xps - 17][you.y_pos + yps - 9] == DNGN_SHALLOW_WATER)
            {
                if (menv[i].enchantment[2] == ENCH_INVIS && mons_flies(menv[i].type) == 0 && player_see_invis() == 0)
                {
                    mpr("There is a strange disturbance in the water here.");
                }
            }

            if (menv[i].enchantment[2] == ENCH_INVIS && player_see_invis() == 0)
                goto look_clouds;

            int mmov_x = menv[i].inv[0];

            if (menv[i].type == MONS_DANCING_WEAPON)
            {
                item_name(mitm.pluses2[mmov_x], mitm.base_type[mmov_x], mitm.sub_type[mmov_x],
                          mitm.special[mmov_x], mitm.pluses[mmov_x], mitm.quantity[mmov_x],
                          mitm.id[mmov_x], 2, str_pass);
                strcpy(info, str_pass);
                strcat(info, ".");
                mpr(info);
            }
            else
            {
                strcpy(info, monam(menv[i].number, menv[i].type, menv[i].enchantment[2], 2));
                strcat(info, ".");
                mpr(info);
                if (mmov_x != ING)
                {
                    strcpy(info, "It is wielding ");
                    item_name(mitm.pluses2[mmov_x], mitm.base_type[mmov_x], mitm.sub_type[mmov_x],
                              mitm.special[mmov_x], mitm.pluses[mmov_x], mitm.quantity[mmov_x],
                              mitm.id[mmov_x], 3, str_pass);
                    strcat(info, str_pass);
                    if (menv[i].type == MONS_TWO_HEADED_OGRE && menv[i].inv[1] != ING)
                    {
                        strcat(info, ",");
                        mpr(info);
                        strcpy(info, " and ");
                        item_name(mitm.pluses2[menv[i].inv[1]], mitm.base_type[menv[i].inv[1]], mitm.sub_type[menv[i].inv[1]],
                                  mitm.special[menv[i].inv[1]], mitm.pluses[menv[i].inv[1]], mitm.quantity[menv[i].inv[1]],
                                  mitm.id[menv[i].inv[1]], 3, str_pass);
                        strcat(info, str_pass);
                        // 2-headed ogres can wield 2 weapons
                    }
                    strcat(info, ".");
                    mpr(info);
                }
            }

            if (menv[i].type == 106)
            {
                strcpy(info, "It has ");
                itoa(menv[i].number, st_prn, 10);
                strcat(info, st_prn);
                strcat(info, " heads.");
                mpr(info);
            }

            print_wounds(i);

            if (menv[i].behavior == 7)
                mpr("It is friendly.");

            if (menv[i].behavior == 0)
                mpr("It doesn't appear to have noticed you.");

            if (menv[i].enchantment1)
            {
                for (p = 0; p < 3; p++)
                {
                    switch (menv[i].enchantment[p])
                    {
                    case ENCH_SLOW:
                        mpr("It is moving slowly.");
                        break;
                    case ENCH_HASTE:
                        mpr("It is moving very quickly.");
                        break;
                    case ENCH_CONFUSION:
                        mpr("It appears to be bewildered and confused.");
                        break;
                    case ENCH_INVIS:
                        mpr("It is slightly transparent.");
                        break;
                    case ENCH_CHARM:
                        mpr("It is in your thrall.");
                        break;
                    case ENCH_YOUR_STICKY_FLAME_I:
                    case ENCH_YOUR_STICKY_FLAME_II:
                    case ENCH_YOUR_STICKY_FLAME_III:
                    case ENCH_YOUR_STICKY_FLAME_IV:
                    case ENCH_STICKY_FLAME_I:
                    case ENCH_STICKY_FLAME_II:
                    case ENCH_STICKY_FLAME_III:
                    case ENCH_STICKY_FLAME_IV:
                        mpr("It is covered in liquid flames.");
                        break;
                    }
                }
            }

#ifdef WIZARD
            stethoscope(i);
#endif
        }

      look_clouds:
        if (env.cgrid[you.x_pos + xps - 17][you.y_pos + yps - 9] != CNG)
        {
            switch (env.cloud_type[env.cgrid[you.x_pos + xps - 17][you.y_pos + yps - 9]] % 100)         // (!!!) {dlb}

            {
            case CLOUD_FIRE:
                strcpy(info, "There is a cloud of flame here.");
                break;

            case CLOUD_STINK:
                strcpy(info, "There is a cloud of noxious fumes here.");
                break;

            case CLOUD_COLD:
                strcpy(info, "There is a cloud of freezing vapour here.");
                break;

            case CLOUD_POISON:
                strcpy(info, "There is a cloud of poison gas here.");
                break;

            case CLOUD_GREY_SMOKE:
                strcpy(info, "There is a cloud of grey smoke here.");
                break;

            case CLOUD_BLUE_SMOKE:
                strcpy(info, "There is a cloud of blue smoke here.");
                break;

            case CLOUD_PURP_SMOKE:
                strcpy(info, "There is a cloud of purple smoke here.");
                break;

            case CLOUD_STEAM:
                strcpy(info, "There is a cloud of steam here.");
                break;

            case CLOUD_MIASMA:
                strcpy(info, "There is an evil black miasma here.");
                break;

            case CLOUD_BLACK_SMOKE:
                strcpy(info, "There is a cloud of black smoke here.");
                break;
            }
            mpr(info);
        }
        // end of look_clouds:

        if (igrd[you.x_pos + xps - 17][you.y_pos + yps - 9] != ING)
        {
            if (mitm.base_type[igrd[you.x_pos + xps - 17][you.y_pos + yps - 9]] == OBJ_GOLD)
            {
                mpr("You see some money here.");
            }
            else
            {
                strcpy(info, "You see ");
                it_name(igrd[you.x_pos + xps - 17][you.y_pos + yps - 9], 3, str_pass);
                strcat(info, str_pass);
                strcat(info, " here.");
                mpr(info);
            }

            if (mitm.link[igrd[you.x_pos + xps - 17][you.y_pos + yps - 9]] != ING)
                mpr("There is something else lying underneath.");
        }

        switch (grd[you.x_pos + xps - 17][you.y_pos + yps - 9])
        {
        case DNGN_STONE_WALL:
            mpr("A stone wall.");
            break;
        case DNGN_ROCK_WALL:
        case DNGN_SECRET_DOOR:
            if (you.level_type == LEVEL_PANDEMONIUM)
            {
                mpr("A wall of the weird stuff which makes up Pandemonium.");
            }
            else
                mpr("A rock wall.");
            break;
        case DNGN_CLOSED_DOOR:
            mpr("A closed door.");
            break;
        case DNGN_METAL_WALL:
            mpr("A metal wall.");
            break;
        case DNGN_GREEN_CRYSTAL_WALL:
            mpr("A wall of green crystal.");
            break;
        case 7:
            mpr("An orcish idol.");
            break;
        case 8:
            mpr("A wall of solid wax.");
            break;
        case 21:
            mpr("A silver statue.");
            break;
        case 22:
            mpr("A granite statue.");
            break;
        case 23:
            mpr("An orange crystal statue.");
            break;
        case 61:
            mpr("Some lava.");
            break;
        case 62:
            mpr("Some deep water.");
            break;
        case 65:
            mpr("Some shallow water.");
            break;
        case 78:                // undiscovered trap

        case 67:
            mpr("Floor.");
            break;
        case 70:
            mpr("An open door.");
            break;
        case 85:
            mpr("A rock staircase leading down.");
            break;
        case 82:
        case 83:
        case 84:
            mpr("A stone staircase leading down.");
            break;
        case 89:
            mpr("A rock staircase leading upwards.");
            break;
        case 86:
        case 87:
        case 88:
            mpr("A stone staircase leading up.");
            break;
        case 69:
            mpr("A gateway to hell.");
            break;
        case 71:
            mpr("A staircase to a branch level.");
            break;
        case 75:
        case 76:
        case 77:
            for (trf = 0; trf < NTRAPS; trf++)
            {
                if (env.trap_x[trf] == you.x_pos + xps - 17 && env.trap_y[trf] == you.y_pos + yps - 9)
                    break;
                if (trf == NTRAPS - 1)
                {
                    mpr("Error - couldn't find that trap.");
                    error_message_to_player();
                    break;
                }
            }
            switch (env.trap_type[trf])
            {
            case 0:
                mpr("A dart trap.");
                break;
            case 1:
                mpr("An arrow trap.");
                break;
            case 2:
                mpr("A spear trap.");
                break;
            case 3:
                mpr("An axe trap.");
                break;
            case 4:
                mpr("A teleportation trap.");
                break;
            case 5:
                mpr("An amnesia trap.");
                break;
            case 6:
                mpr("A blade trap.");
                break;
            case 7:
                mpr("A bolt trap.");
                break;
            case 8:
                mpr("A Zot trap.");
                break;
            default:
                mpr("An undefined trap. Huh?");
                error_message_to_player();
                break;
            }
            break;
        case 80:
            mpr("A shop.");
            break;
        case 81:
            mpr("A labyrinth entrance.");
            break;
        case 92:
            mpr("A gateway to the Iron City of Dis.");
            break;
        case 93:
            mpr("A gateway to Gehenna.");
            break;
        case 94:
            mpr("A gateway to the freezing wastes of Cocytus.");
            break;
        case 95:
            mpr("A gateway to the decaying netherworld of Tartarus.");
            break;
        case 96:
            mpr("A gateway to the infinite Abyss.");
            break;
        case 97:
            mpr("A gateway leading out of the Abyss.");
            break;
        case 98:
            mpr("An empty arch of ancient stone.");
            break;
        case 99:
            mpr("A gate leading to the halls of Pandemonium.");
            break;
        case 100:
            mpr("A gate leading out of Pandemonium.");
            break;
        case 101:
            mpr("A gate leading to another region of Pandemonium.");
            break;
        case 110:
            mpr("A staircase to the Orcish Mines.");
            break;
        case 111:
            mpr("A staircase to the Hive.");
            break;
        case 112:
            mpr("A staircase to the Lair.");
            break;
        case 113:
            mpr("A staircase to the Slime Pits.");
            break;
        case 114:
            mpr("A staircase to the Vaults.");
            break;
        case 115:
            mpr("A staircase to the Crypt.");
            break;
        case 116:
            mpr("A staircase to the Hall of Blades.");
            break;
        case 117:
            mpr("A gate to the Realm of Zot.");
            break;
        case 118:
            mpr("A staircase to the Ecumenical Temple.");
            break;
        case 119:
            mpr("A staircase to the Snake Pit.");
            break;
        case 120:
            mpr("A staircase to the Elven Halls.");
            break;
        case 121:
            mpr("A staircase to the Tomb.");
            break;
        case 122:
            mpr("A staircase to the Swamp.");
            break;
        case 130:
        case 131:
        case 132:
        case 134:
        case 138:
            mpr("A staircase back to the Dungeon.");
            break;
        case 133:
            mpr("A staircase back to the Lair.");
            break;
        case 135:
            mpr("A staircase back to the Vaults.");
            break;
        case 136:
            mpr("A staircase back to the Crpyt.");
            break;
        case 139:
            mpr("A staircase back to the Lair.");
            break;
        case 140:
            mpr("A staircase back to the Mines.");
            break;
        case 141:
            mpr("A staircase back to the Crypt.");
            break;
        case 137:
            mpr("A gate leading back out of this place.");
            break;
        case 142:
            mpr("A staircase back to the Lair.");
            break;
        case 180:
            mpr("A glowing white marble altar of Zin.");
            break;
        case 181:
            mpr("A glowing golden altar of the Shining One.");
            break;
        case 182:
            mpr("An ancient bone altar of Kikubaaqudgha.");
            break;
        case 183:
            mpr("A basalt altar of Yredelemnul.");
            break;
        case 184:
            mpr("A shimmering altar of Xom.");
            break;
        case 185:
            mpr("A shining altar of Vehumet.");
            break;
        case 186:
            mpr("An iron altar of Okawaru.");
            break;
        case 187:
            mpr("A burning altar of Makhleb.");
            break;
        case 188:
            mpr("A deep blue altar of Sif Muna.");
            break;
        case 189:
            mpr("A bloodstained altar of Trog.");
            break;
        case 190:
            mpr("A sparkling altar of Nemelex Xobeh.");
            break;
        case 191:
            mpr("A silver altar of Elyvilon.");
            break;
        case DNGN_BLUE_FOUNTAIN:
            mpr("A fountain of clear blue water.");
            break;
        case DNGN_SPARKLING_FOUNTAIN:
            mpr("A fountain of sparkling water.");
            break;
        case DNGN_DRY_FOUNTAIN_I:
        case DNGN_DRY_FOUNTAIN_II:
        case DNGN_DRY_FOUNTAIN_IV:
        case DNGN_DRY_FOUNTAIN_VI:
        case DNGN_DRY_FOUNTAIN_VIII:
        case DNGN_PERMADRY_FOUNTAIN:
            mpr("A dry fountain.");
            break;
        }

      glogokh:                  // test relay_message();
        itoa((int) grd[you.x_pos + xps - 17][you.y_pos + yps - 9], st_prn, 10);
        strcpy(info, st_prn);

        gotoxy(xps + 1, yps);
    }
    while (gotch != 'S');

  finished_looking:
    moves[0].move_x = xps - 17;
    moves[0].move_y = yps - 9;
    moves[0].target_x = you.x_pos + xps - 17;
    moves[0].target_y = you.y_pos + yps - 9;

    if (gotch == 'p')
        return 253;
    if (gotch == '>')
        return 50001;

    return 0;                   //mve_x * 100 + mve_y + 707 + 10000;

}                               // end of look_around





//---------------------------------------------------------------
//
// mons_find
//
// Finds the next monster (moving in a spiral outwards from the
// player, so closer monsters are chosen first; starts to player's
// left) and puts its coordinates in mfp. Returns 1 if it found
// a monster, zero otherwise. If direction is -1, goes backwards.
//
//---------------------------------------------------------------
char mons_find(unsigned char xps, unsigned char yps, char mfp[2], char direction)
{

    unsigned char temp_xps = xps;
    unsigned char temp_yps = yps;
    char x_change = 0;
    char y_change = 0;

    int i, j;

    if (direction == 1 && temp_xps == 9 && temp_yps == 17)
        return 0;               // end of spiral

    while (temp_xps >= 8 && temp_xps <= 25 && temp_yps <= 17)   // yps always >= 0

    {

        if (direction == -1 && temp_xps == 17 && temp_yps == 9)
            return 0;           // can't go backwards from you

        if (direction == 1)
        {
            if (temp_xps == 8)
            {
                x_change = 0;
                y_change = -1;
            }
            else if (temp_xps - 17 == 0 && temp_yps - 9 == 0)
            {
                x_change = -1;
                y_change = 0;
            }
            else if (abs(temp_xps - 17) <= abs(temp_yps - 9))
            {
                if (temp_xps - 17 >= 0 && temp_yps - 9 <= 0)
                {
                    if (abs(temp_xps - 17) > abs(temp_yps - 9 + 1))
                    {
                        x_change = 0;
                        y_change = -1;
                        if (temp_xps - 17 > 0)
                            y_change = 1;
                        goto finished_spiralling;
                    }
                }
                x_change = -1;
                if (temp_yps - 9 < 0)
                    x_change = 1;
                y_change = 0;
            }
            else
            {
                x_change = 0;
                y_change = -1;
                if (temp_xps - 17 > 0)
                    y_change = 1;
            }
        }                       // end if (direction == 1)

        else
        {
/*
   This part checks all eight surrounding squares to find the one that
   leads on to the present square.
 */
            for (i = -1; i < 2; i++)
            {
                for (j = -1; j < 2; j++)
                {
                    if (i == 0 && j == 0)
                        continue;

                    if (temp_xps + i == 8)
                    {
                        x_change = 0;
                        y_change = -1;
                    }
                    else if (temp_xps + i - 17 == 0 && temp_yps + j - 9 == 0)
                    {
                        x_change = -1;
                        y_change = 0;
                    }
                    else if (abs(temp_xps + i - 17) <= abs(temp_yps + j - 9))
                    {
                        if (temp_xps + i - 17 >= 0 && temp_yps + j - 9 <= 0)
                        {
                            if (abs(temp_xps + i - 17) > abs(temp_yps + j - 9 + 1))
                            {
                                x_change = 0;
                                y_change = -1;
                                if (temp_xps + i - 17 > 0)
                                    y_change = 1;
                                goto finished_spiralling;
                            }
                        }
                        x_change = -1;
                        if (temp_yps + j - 9 < 0)
                            x_change = 1;
                        y_change = 0;
                    }
                    else
                    {
                        x_change = 0;
                        y_change = -1;
                        if (temp_xps + i - 17 > 0)
                            y_change = 1;
                    }

                    if (temp_xps + i + x_change == temp_xps && temp_yps + j + y_change == temp_yps)
                        goto finished_spiralling;
                }
            }
        }                       // end else


      finished_spiralling:
        x_change *= direction;
        y_change *= direction;

        temp_xps += x_change;
        if (temp_yps + y_change <= 17)  // it can wrap, unfortunately

            temp_yps += y_change;

        // We don't want to be looking outside the bounds of the arrays:
        if (temp_xps <= 25 && temp_xps >= 8 && temp_yps <= 17   // && temp_yps >= 1
         && you.x_pos + temp_xps - 17 >= 0 && you.x_pos + temp_xps - 17 < GXM
         && you.y_pos + temp_yps - 9 >= 0 && you.y_pos + temp_yps - 9 < GYM)
        {
            if (mgrd[you.x_pos + temp_xps - 17][you.y_pos + temp_yps - 9] != MNG
                && env.show[temp_xps - 8][temp_yps] != 0
                && (menv[mgrd[you.x_pos + temp_xps - 17][you.y_pos + temp_yps - 9]].enchantment[2] != ENCH_INVIS || player_see_invis() != 0)
                && (menv[mgrd[you.x_pos + temp_xps - 17][you.y_pos + temp_yps - 9]].type < MONS_LAVA_WORM || menv[mgrd[you.x_pos + temp_xps - 17][you.y_pos + temp_yps - 9]].number != 1))
                // & not invis etc
            {
//       mpr("Found something!");
                //       more();
                mfp[0] = temp_xps;
                mfp[1] = temp_yps;
                return 1;
            }
        }
    }


    return 0;

}

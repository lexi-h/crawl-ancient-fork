/*
 *  File:       religion.cc
 *  Summary:    Misc religion related functions.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *
 *   <7>   11jan2001     gdl    added M. Valvoda's changes to
 *                              god_colour() and god_name()
 *   <6>   06-Mar-2000   bwr    added penance, gift_timeout,
 *                              divine_retribution(), god_speaks()
 *   <5>   11/15/99      cdl    Fixed Daniel's yellow Xom patch  :)
 *                              Xom will sometimes answer prayers
 *   <4>   10/11/99      BCR    Added Daniel's yellow Xom patch
 *   <3>    6/13/99      BWR    Vehumet book giving code.
 *   <2>    5/20/99      BWR    Added screen redraws
 *   <1>    -/--/--      LRH    Created
 */

#include "AppHdr.h"
#include "religion.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "externs.h"

#include "beam.h"
#include "debug.h"
#include "decks.h"
#include "describe.h"
#include "dungeon.h"
#include "effects.h"
#include "food.h"
#include "it_use2.h"
#include "itemname.h"
#include "misc.h"
#include "monplace.h"
#include "mutation.h"
#include "newgame.h"
#include "ouch.h"
#include "player.h"
#include "shopping.h"
#include "spells.h"
#include "spells1.h"
#include "spells2.h"
#include "spells3.h"
#include "stuff.h"

char *sacrifice[] = {
    {" glows silver and disappears."},
    {" glows a brilliant golden colour and disappears."},
    {" rots away in an instant."},
    {" crumbles to dust."},
    {" is eaten by a bug."},    /* Xom - no sacrifices */
    {" explodes into nothingness."},
    {" is consumed in a burst of flame."},
    {" is consumed in a roaring column of flame."},
    {" glows faintly for a moment, then is gone."},
    {" is consumed in a roaring column of flame."},
    {" glows with a rainbow of weird colours and disappears."},
    {" evaporates."}
};

void altar_prayer(void);
void dec_penance(int god, int val);
void divine_retribution(int god);
void god_pitch(unsigned char which_god);
void inc_penance(int god, int val);
void inc_penance(int val);

void dec_penance(int god, int val)
{
    if (you.penance[god] > 0)
    {
        if (you.penance[god] <= val)
        {
            simple_god_message(" seems mollified.", god);
            you.penance[god] = 0;
        }
        else
            you.penance[god] -= val;
    }
}                               // end dec_penance()

void dec_penance(int val)
{
    dec_penance(you.religion, val);
}                               // end dec_penance()

void inc_penance(int god, int val)
{
    if ((int) you.penance[god] + val > 200)
        you.penance[god] = 200;
    else
        you.penance[god] += val;
}                               // end inc_penance()

void inc_penance(int val)
{
    inc_penance(you.religion, val);
}                               // end inc_penance()

static void inc_gift_timeout(int val)
{
    if ((int) you.gift_timeout + val > 200)
        you.gift_timeout = 200;
    else
        you.gift_timeout += val;
}                               // end inc_gift_timeout()

void pray(void)
{
    int temp_rand = 0;
    unsigned char was_praying = you.duration[DUR_PRAYER];

    if (silenced(you.x_pos, you.y_pos))
    {
        mpr("You are unable to make a sound!");
        return;
    }

    // all prayers take time
    you.turn_is_over = 1;

    if (you.religion != GOD_NO_GOD
            && grd[you.x_pos][you.y_pos] == 179 + you.religion)
    {
        altar_prayer();
    }
    else if (grd[you.x_pos][you.y_pos] >= 180
                && grd[you.x_pos][you.y_pos] <= 199)
    {
        if (you.species == SP_DEMIGOD)
        {
            mpr("Sorry, a being of your status cannot worship here.");
            return;
        }
        god_pitch(grd[you.x_pos][you.y_pos] - 179);
        return;
    }

    if (you.religion == GOD_NO_GOD)
    {
        strcpy(info, "You spend a moment contemplating the meaning of ");
        if (you.is_undead)
            strcat(info, "un");
        strcat(info, "life.");
        mpr(info);
        return;
    }
    else if (you.religion == GOD_XOM)
    {
        if (one_chance_in(100))
        {
            // Every now and then, Xom listens
            // This is for flavour, not effect, so praying should not be
            // encouraged.

            // Xom is nicer to experienced players  (0 is bad, 1 is nice)
            bool nice = 27 <= random2(27 + you.experience_level);

            // and he's not very nice even then
            int sever = (nice) ? random2(random2(you.experience_level))
                               : you.experience_level;

            // bad results are enforced, good are not
            bool force = !nice;

            Xom_acts(nice, 1 + sever, force);
        }
        else
            mpr("Xom ignores you.");

        return;
    }

    strcpy(info, "You offer a prayer to ");
    strcat(info, god_name(you.religion));
    strcat(info, ".");
    mpr(info);

    you.duration[DUR_PRAYER] = 9 + (random2(you.piety) / 20)
                                            + (random2(you.piety) / 20);

    if (player_under_penance())
        simple_god_message(" demands penance!");
    else
    {
        strcpy(info, god_name(you.religion));
        strcat(info, " is ");

        strcat(info, (you.piety > 130) ? "exalted by your worship" :
                     (you.piety > 100) ? "extremely pleased with you" :
                     (you.piety >  70) ? "greatly pleased with you" :
                     (you.piety >  40) ? "most pleased with you" :
                     (you.piety >  20) ? "pleased with you" :
                     (you.piety >   5) ? "noncommittal"
                                       : "displeased");

        strcat(info, ".");
        god_speaks(you.religion, info);

        if (you.piety > 130)
            you.duration[DUR_PRAYER] *= 3;
        else if (you.piety > 70)
            you.duration[DUR_PRAYER] *= 2;
    }

/*
    itoa(you.piety, st_prn, 10);
    strcpy(info, "Debug info - Piety: ");
    strcat(info, st_prn);
    mpr(info);
*/

    // Consider a gift if we don't have a timeout and weren't
    // already praying when we prayed.
    if (!you.penance[you.religion] && !you.gift_timeout && !was_praying)
    {
        //   Remember to check for water/lava
        //jmf: "good" god will sometimes feed you (a la Nethack)
        if (you.religion == GOD_ZIN && you.hunger_state == HS_STARVING
            && random2(250) <= you.piety)
        {
            god_speaks(you.religion, "Your stomach feels content.");
            set_hunger(6000, true);
            lose_piety(5 + random2avg(10, 2));
            inc_gift_timeout(30 + random2avg(10, 2));
            return;
        }

        if (you.religion == GOD_NEMELEX_XOBEH
            && random2(200) <= you.piety
            && (!you.attribute[ATTR_CARD_TABLE] || one_chance_in(3))
            && !you.attribute[ATTR_CARD_COUNTDOWN]
            && grd[you.x_pos][you.y_pos] != DNGN_LAVA
            && grd[you.x_pos][you.y_pos] != DNGN_DEEP_WATER)
        {
            int Nemelex = 0;
            unsigned char gift_type = MISC_DECK_OF_TRICKS;

            if (!you.attribute[ATTR_CARD_TABLE])
            {
                Nemelex = items(1, OBJ_MISCELLANY,
                                MISC_PORTABLE_ALTAR_OF_NEMELEX, 1, 1, 250);

                you.attribute[ATTR_CARD_TABLE] = 1;
            }
            else
            {
                if (random2(200) <= you.piety && one_chance_in(4))
                    gift_type = MISC_DECK_OF_SUMMONINGS;
                if (random2(200) <= you.piety && coinflip())
                    gift_type = MISC_DECK_OF_WONDERS;
                if (random2(200) <= you.piety && one_chance_in(4))
                    gift_type = MISC_DECK_OF_POWER;
                Nemelex = items(1, OBJ_MISCELLANY, gift_type, 1, 1, 250);
            }

            canned_msg(MSG_SOMETHING_APPEARS);
            more();

            int what_was_theren = igrd[you.x_pos][you.y_pos];

            mitm.link[Nemelex] = what_was_theren;
            igrd[you.x_pos][you.y_pos] = Nemelex;

            you.attribute[ATTR_CARD_COUNTDOWN] = 10;
            inc_gift_timeout(5 + random2avg(9, 2));
        }

        if ((you.religion == GOD_OKAWARU || you.religion == GOD_TROG)
            && you.piety > 130
            && random2(you.piety) > 120
            && grd[you.x_pos][you.y_pos] != DNGN_LAVA
            && grd[you.x_pos][you.y_pos] != DNGN_DEEP_WATER
            && one_chance_in(4))
        {
            simple_god_message(" grants you a gift!");

            more();

            if (you.religion == GOD_TROG
                || (you.religion == GOD_OKAWARU && coinflip()))
            {
                acquirement(OBJ_WEAPONS);
            }
            else
                acquirement(OBJ_ARMOUR);

            inc_gift_timeout(30 + random2avg(19, 2));
        }

        if (you.religion == GOD_YREDELEMNUL
            && random2(you.piety) > 80 && one_chance_in(10))
        {
            int thing_called = MONS_PROGRAM_BUG;  // error trapping {dlb}

            temp_rand = random2(100);
            thing_called = ((temp_rand > 66) ? MONS_WRAITH :            // 33%
                            (temp_rand > 52) ? MONS_WIGHT :             // 12%
                            (temp_rand > 40) ? MONS_SPECTRAL_WARRIOR :  // 16%
                            (temp_rand > 31) ? MONS_ROTTING_HULK :      //  9%
                            (temp_rand > 23) ? MONS_SKELETAL_WARRIOR :  //  8%
                            (temp_rand > 16) ? MONS_VAMPIRE :           //  7%
                            (temp_rand > 10) ? MONS_GHOUL :             //  6%
                            (temp_rand >  4) ? MONS_MUMMY               //  6%
                                             : MONS_FLAYED_GHOST);      //  5%

            if (create_monster(thing_called, 0, BEH_ENSLAVED, you.x_pos,
                               you.y_pos, you.pet_target, 250) != -1)
            {
                simple_god_message(" grants you an undead servant!");
                more();
                inc_gift_timeout(4 + random2avg(7, 2));
            }
        }

        if ((you.religion == GOD_KIKUBAAQUDGHA
                || you.religion == GOD_SIF_MUNA
                || you.religion == GOD_VEHUMET)
            && you.piety > 160 && random2(you.piety) > 100)
        {
            unsigned char gift = NUM_BOOKS;

            switch (you.religion)
            {
            case GOD_KIKUBAAQUDGHA:     // gives death books
                if (!you.had_item[BOOK_NECROMANCY])
                    gift = BOOK_NECROMANCY;
                else if (!you.had_item[BOOK_DEATH])
                    gift = BOOK_DEATH;
                else if (!you.had_item[BOOK_UNLIFE])
                    gift = BOOK_UNLIFE;
                else if (!you.had_item[BOOK_NECRONOMICON])
                    gift = BOOK_NECRONOMICON;
                break;

            case GOD_SIF_MUNA:
                gift = OBJ_RANDOM;      // Sif Muna - gives any
                break;

            //jmf: error check; Vehumet just gave me a Book of Minor Magic
            // Vehumet - gives conj/summ. Gives bks first for whichever
            // skill is higher
            case GOD_VEHUMET:
                if (!you.had_item[BOOK_CONJURATIONS_I])
                    gift = give_first_conjuration_book();
                else if (!you.had_item[BOOK_POWER])
                    gift = BOOK_POWER;
                else if (!you.had_item[BOOK_ANNIHILATIONS])
                    gift = BOOK_ANNIHILATIONS;  // conj books

                if (you.skills[SK_CONJURATIONS] < you.skills[SK_SUMMONINGS]
                    || gift == BOOK_MINOR_MAGIC_I)
                {
                    if (!you.had_item[BOOK_SUMMONINGS])
                        gift = BOOK_SUMMONINGS;
                    else if (!you.had_item[BOOK_INVOCATIONS])
                        gift = BOOK_INVOCATIONS;
                    else if (!you.had_item[BOOK_DEMONOLOGY])
                        gift = BOOK_DEMONOLOGY; // summoning bks
                }
                break;
            }

            if (gift != NUM_BOOKS
                && (grd[you.x_pos][you.y_pos] != DNGN_LAVA
                    && grd[you.x_pos][you.y_pos] != DNGN_DEEP_WATER))
            {
                simple_god_message(" grants you a gift!");

                more();

                if (gift == OBJ_RANDOM)
                    acquirement(OBJ_BOOKS);
                else
                {
                    int thing_created = items(1, OBJ_BOOKS, gift, 1, 1, 250);

                    canned_msg(MSG_SOMETHING_APPEARS);
                    more();

                    int what_was_there = igrd[you.x_pos][you.y_pos];

                    mitm.link[thing_created] = what_was_there;
                    igrd[you.x_pos][you.y_pos] = thing_created;
                }

                inc_gift_timeout(40 + random2avg(19, 2));

                // Vehumet gives books less readily
                if (you.religion == GOD_VEHUMET)
                    inc_gift_timeout(10 + random2(10));
            }                   // end of giving book
        }                       // end of book gods
    }                           // end of gift giving
}                               // end pray()

char *god_name(int which_god,bool long_name) // mv - rewritten
{
    static char buffer[80];

    switch (which_god)
    {
    case GOD_NO_GOD:
        sprintf(buffer, "No God");
        break;
    case GOD_ZIN:
        sprintf(buffer, "Zin%s", long_name?" the Law-Giver":"");
        break;
    case GOD_SHINING_ONE:
        sprintf(buffer, "The Shining One");
        break;
    case GOD_KIKUBAAQUDGHA:
        sprintf(buffer, "Kikubaaqudgha%s", long_name?" the Vile":"");
        break;
    case GOD_YREDELEMNUL:
        sprintf(buffer, "Yredelemnul%s", long_name?" the Dark":"");
        break;
    case GOD_XOM:
        sprintf(buffer, "Xom%s", long_name?" of Chaos":"");
        break;
    case GOD_VEHUMET:
        sprintf(buffer, "Vehumet%s", long_name?" the Annihilator":"");
        break;
    case GOD_OKAWARU:
        sprintf(buffer, "Okawaru%s", long_name?" the Warmaster":"");
        break;
    case GOD_MAKHLEB:
        sprintf(buffer, "Makhleb%s", long_name?" the Destroyer":"");
        break;
    case GOD_SIF_MUNA:
        sprintf(buffer, "Sif Muna%s", long_name?" the Eldritch":"");
        break;
    case GOD_TROG:
        sprintf(buffer, "Trog%s", long_name?" the Fierce":"");
        break;
    case GOD_NEMELEX_XOBEH:
        sprintf(buffer, "Nemelex Xobeh%s", long_name?" the Trickster":"");
        break;
    case GOD_ELYVILON:
        sprintf(buffer, "Elyvilon%s", long_name?" the Healer":"");
        break;
    default:
        sprintf(buffer, "Lord of Bugs (%d)", which_god);

    }
        return buffer;
}                               // end god_name()

#ifdef USE_GOD_COLOURS

void god_speaks( int god, const char *mesg )
{
    mpr( mesg, MSGCH_GOD, god );
}                               // end god_speaks()

#else

inline void god_speaks(int god, const char *mesg )
{
    mpr( mesg );
}                               // end god_speaks()

#endif

void Xom_acts(bool niceness, int sever, bool force_sever)
{
    // niceness = false - bad, true - nice
    int temp_rand;              // probability determination {dlb}
    bool done_bad = false;      // flag to clarify logic {dlb}
    bool done_good = false;     // flag to clarify logic {dlb}

    struct bolt beam;

    if (sever < 1)
        sever = 1;

    if (!force_sever)
        sever = random2(sever);

    if (sever == 0)
        return;

  okay_try_again:

    if (!niceness || one_chance_in(3))
    {
        // begin "Bad Things"
        done_bad = false;

        // this should always be first - it will often be called
        // deliberately, with a low sever value
        if (random2(sever) <= 2)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "Xom notices you." :
                (temp_rand == 1) ? "Xom's attention turns to you for a moment.":
                (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                 : "You hear Xom's maniacal laughter.");

            miscast_effect(SPTYP_RANDOM, 5 + random2(10), random2(100), 0);

            done_bad = true;
        }
        else if (random2(sever) <= 2)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Suffer!\"" :
                (temp_rand == 1) ? "Xom's malign attention turns to you for a moment." :
                (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                 : "You hear Xom's maniacal laughter.");

            lose_stat(STAT_RANDOM, 1 + random2(3), true);

            done_bad = true;
        }
        else if (random2(sever) <= 2)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "Xom notices you." :
                (temp_rand == 1) ? "Xom's attention turns to you for a moment.":
                (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                 : "You hear Xom's maniacal laughter.");

            miscast_effect(SPTYP_RANDOM, 5 + random2(15), random2(250), 0);

            done_bad = true;
        }
        else if (!you.is_undead)
        {
            if (random2(sever) <= 3)
            {
                temp_rand = random2(4);

                god_speaks(GOD_XOM,
                    (temp_rand == 0) ? "\"You need some minor adjustments, mortal!\"" :
                    (temp_rand == 1) ? "\"Let me alter your pitiful body.\"" :
                    (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                     : "You hear Xom's maniacal laughter.");

                mpr("Your body is suffused with distortional energy.");

                set_hp(1 + random2(you.hp), false);
                deflate_hp(you.hp_max / 2, true);

                bool success = false;
                for (int i = 0; i < 4; i++) {
                    if (mutate(100)) {
                        success = true;
                        break;
                    }
                }

                if (!success)
                    mpr("You feel queasy for a moment.");

                done_bad = true;
            }

            if (random2(sever) <= 3)
            {
                temp_rand = random2(4);

                god_speaks(GOD_XOM,
                    (temp_rand == 0) ? "\"You have displeased me, mortal.\"" :
                    (temp_rand == 1) ? "\"You have grown too confident for your meagre worth.\"" :
                    (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                     : "You hear Xom's maniacal laughter.");

                if (one_chance_in(4))
                {
                    drain_exp();
                    if (random2(sever) > 3)
                        drain_exp();
                    if (random2(sever) > 3)
                        drain_exp();
                }
                else
                {
                    mpr("A wave of agony tears through your body!");
                    set_hp(1 + (you.hp / 2), false);
                }

                done_bad = true;
            }
        }                       // end "if (!you.is_undead)"
        else if (random2(sever) <= 3)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Time to have some fun!\"" :
                (temp_rand == 1) ? "\"Fight to survive, mortal.\"" :
                (temp_rand == 2) ? "\"Let's see if it's strong enough to survive yet.\""
                                 : "You hear Xom's maniacal laughter.");

            if (one_chance_in(4))
                dancing_weapon(100, true);      // nasty, but fun
            else
            {
                create_monster(MONS_NEQOXEC + random2(5), 22, BEH_CHASING_I,
                               you.x_pos, you.y_pos, MHITNOT, 250);

                if (one_chance_in(3))
                    create_monster(MONS_NEQOXEC + random2(5), 22,
                                   BEH_CHASING_I, you.x_pos, you.y_pos,
                                   MHITNOT, 250);

                if (one_chance_in(4))
                    create_monster(MONS_NEQOXEC + random2(5), 22,
                                   BEH_CHASING_I, you.x_pos, you.y_pos,
                                   MHITNOT, 250);

                if (one_chance_in(3))
                    create_monster(MONS_HELLION + random2(10), 22,
                                   BEH_CHASING_I, you.x_pos, you.y_pos,
                                   MHITNOT, 250);

                if (one_chance_in(4))
                    create_monster(MONS_HELLION + random2(10), 22,
                                   BEH_CHASING_I, you.x_pos, you.y_pos,
                                   MHITNOT, 250);
            }

            done_bad = true;
        }
        else if (you.your_level == 0)
        {
            // this should remain the last possible outcome {dlb}
            temp_rand = random2(3);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"You have grown too comfortable in your little world, mortal!\"" :
                (temp_rand == 1) ? "Xom casts you into the Abyss!"
                                 : "The world seems to spin as Xom's maniacal laughter rings in your ears.");

            banished(DNGN_ENTER_ABYSS);

            done_bad = true;
        }
    }                           // end "Bad Things"
    else
    {
        // begin "Good Things"
        done_good = false;

// Okay, now for the nicer stuff (note: these things are not necessarily nice):
        if (random2(sever) <= 2)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Go forth and destroy!\"" :
                (temp_rand == 1) ? "\"Go forth and destroy, mortal!\"" :
                (temp_rand == 2) ? "Xom grants you a minor favour."
                                 : "Xom smiles on you.");

            switch (random2(7))
            {
            case 0:
                potion_effect(POT_HEALING, 150);
                break;
            case 1:
                potion_effect(POT_HEAL_WOUNDS, 150);
                break;
            case 2:
                potion_effect(POT_SPEED, 150);
                break;
            case 3:
                potion_effect(POT_MIGHT, 150);
                break;
            case 4:
                potion_effect(POT_INVISIBILITY, 150);
                break;
            case 5:
                if (one_chance_in(6))
                    potion_effect(POT_EXPERIENCE, 150);
                else
                {
                    you.berserk_penalty = NO_BERSERK_PENALTY;
                    potion_effect(POT_BERSERK_RAGE, 150);
                }
                break;
            case 6:
                you.berserk_penalty = NO_BERSERK_PENALTY;
                potion_effect(POT_BERSERK_RAGE, 150);
                break;
            }

            done_good = true;
        }
        else if (random2(sever) <= 4)
        {
            temp_rand = random2(3);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Serve the mortal, my children!\"" :
                (temp_rand == 1) ? "Xom grants you some temporary aid."
                                 : "Xom opens a gate.");

            create_monster(MONS_NEQOXEC + random2(5), 22, BEH_ENSLAVED,
                           you.x_pos, you.y_pos, you.pet_target, 250);
            create_monster(MONS_NEQOXEC + random2(5), 22, BEH_ENSLAVED,
                           you.x_pos, you.y_pos, you.pet_target, 250);

            if (random2(you.experience_level) >= 8)
                create_monster(MONS_NEQOXEC + random2(5), 22, BEH_ENSLAVED,
                               you.x_pos, you.y_pos, you.pet_target, 250);

            if (random2(you.experience_level) >= 8)
                create_monster(MONS_HELLION + random2(10), 22, BEH_ENSLAVED,
                               you.x_pos, you.y_pos, you.pet_target, 250);

            if (random2(you.experience_level) >= 8)
                create_monster(MONS_HELLION + random2(10), 22, BEH_ENSLAVED,
                               you.x_pos, you.y_pos, you.pet_target, 250);

            done_good = true;
        }
        else if (random2(sever) <= 3)
        {
            temp_rand = random2(3);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Take this token of my 'esteem'.\"" :
                (temp_rand == 1) ? "Xom grants you a gift!"
                                 : "Xom's generous nature manifests itself.");

            if (grd[you.x_pos][you.y_pos] == DNGN_LAVA
                || grd[you.x_pos][you.y_pos] == DNGN_DEEP_WATER)
            {
                // How unfortunate. I'll bet Xom feels sorry for you.
                mpr("You hear a splash.");
            }
            else
            {
                int thing_created = items(1, OBJ_RANDOM, OBJ_RANDOM, 1,
                                              you.experience_level * 3, 250);

                canned_msg(MSG_SOMETHING_APPEARS);

                int what_was_there = igrd[you.x_pos][you.y_pos];

                mitm.link[thing_created] = what_was_there;
                igrd[you.x_pos][you.y_pos] = thing_created;
            }

            done_good = true;
        }
        else if (random2(sever) <= 4)
        {
            const int demon = (random2(you.experience_level) < 6)
                                                ? MONS_WHITE_IMP + random2(5)
                                                : MONS_NEQOXEC + random2(5);

            if (create_monster( demon, 0, BEH_ENSLAVED, you.x_pos, you.y_pos,
                                                 you.pet_target, 250) != -1)
            {
                temp_rand = random2(3);

                god_speaks(GOD_XOM,
                    (temp_rand == 0) ? "\"Serve the mortal, my child!\"" :
                    (temp_rand == 1) ? "Xom grants you a demonic servitor."
                                     : "Xom opens a gate.");
            }

            done_good = true;   // well, for Xom, trying == doing {dlb}
        }
        else if (random2(sever) <= 4)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"Take this instrument of destruction!\"" :
                (temp_rand == 1) ? "\"You have earned yourself a gift.\"" :
                (temp_rand == 2) ? "Xom grants you an implement of death."
                                 : "Xom smiles on you.");

            acquirement(OBJ_WEAPONS);

            done_good = true;
        }
        else if (!you.is_undead && random2(sever) <= 5)
        {
            temp_rand = random2(4);

            god_speaks(GOD_XOM,
                (temp_rand == 0) ? "\"You need some minor adjustments, mortal!\"" :
                (temp_rand == 1) ? "\"Let me alter your pitiful body.\"" :
                (temp_rand == 2) ? "Xom's power touches on you for a moment."
                                 : "You hear Xom's maniacal chuckling.");

            mpr("Your body is suffused with distortional energy.");

            set_hp(1 + random2(you.hp), false);
            deflate_hp(you.hp_max / 2, true);

            give_good_mutation();

            done_good = true;
        }
        else if (random2(sever) <= 2)
        {
            // this should remain the last possible outcome {dlb}
            if (!one_chance_in(10))
                you.attribute[ATTR_DIVINE_LIGHTNING_PROTECTION] = 1;

            god_speaks(GOD_XOM, "Xom hurls a blast of lightning!");

            beam.beam_source = NON_MONSTER;
            beam.type = SYM_BURST;
            beam.damage = 130;
            beam.flavour = BEAM_ELECTRICITY;
            beam.target_x = you.x_pos;
            beam.target_y = you.y_pos;
            strcpy(beam.beam_name, "blast of lightning");
            beam.colour = LIGHTCYAN;
            beam.thrower = KILL_YOU;    // your explosion
            beam.ex_size = 2;

            explosion(beam);

            you.attribute[ATTR_DIVINE_LIGHTNING_PROTECTION] = 0;

            done_good = true;
        }
    }                           // end "Good Things"

    if (done_bad || done_good || one_chance_in(4))
        return;
    else
        goto okay_try_again;
}                               // end Xom_acts()

void done_good(char thing_done, int pgain)
{
    if (you.religion == GOD_NO_GOD)
        return;

    switch (thing_done)
    {
    case GOOD_KILLED_LIVING:
        switch (you.religion)
        {
        case GOD_ELYVILON:
            simple_god_message(" did not appreciate that!");
            naughty(NAUGHTY_KILLING, 10);
            break;
        case GOD_KIKUBAAQUDGHA:
        case GOD_YREDELEMNUL:
        case GOD_VEHUMET:
        case GOD_OKAWARU:
        case GOD_MAKHLEB:
        case GOD_TROG:
            simple_god_message(" accepts your kill.");
            if (random2(18 + pgain) > 5)
                gain_piety(1);
            break;
        }
        break;

    case GOOD_KILLED_UNDEAD:
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_VEHUMET:
        case GOD_MAKHLEB:
            simple_god_message(" accepts your kill.");
            if (random2(18 + pgain) > 4)
                gain_piety(1);
            break;
        }
        break;

    case GOOD_KILLED_DEMON:
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_VEHUMET:
        case GOD_MAKHLEB:
            simple_god_message(" accepts your kill.");
            if (random2(18 + pgain) > 3)
                gain_piety(1);
            break;
        }
        break;

    case GOOD_KILLED_ANGEL_I:
    case GOOD_KILLED_ANGEL_II:
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_ELYVILON:
            simple_god_message(" did not appreciate that!");
            naughty(NAUGHTY_ATTACK_HOLY, (you.conf ? 3 : pgain * 3));
            break;
        }
        break;

    case GOOD_HACKED_CORPSE:    // NB - pgain is you.experience_level (maybe)
        switch (you.religion)
        {
        case GOD_KIKUBAAQUDGHA:
        case GOD_OKAWARU:
        case GOD_MAKHLEB:
        case GOD_TROG:
            simple_god_message(" accepts your offering.");
            if (random2(10 + pgain) > 5)
                gain_piety(1);
            break;

        case GOD_ZIN:
        case GOD_SHINING_ONE:
        case GOD_ELYVILON:
            simple_god_message(" did not appreciate that!");

            naughty(NAUGHTY_BUTCHER, 8);
            break;
        }
        break;

    case GOOD_OFFER_STUFF:
        simple_god_message(" is pleased with your offering.");

        gain_piety(1);
        break;

    case GOOD_SLAVES_KILL_LIVING:
        switch (you.religion)
        {
        case GOD_KIKUBAAQUDGHA:
        case GOD_YREDELEMNUL:
        case GOD_VEHUMET:
            simple_god_message(" accepts your slave's kill.");

            if (random2(pgain + 18) > 5)
                gain_piety(1);
            break;
        }
        break;

    case GOOD_SERVANTS_KILL:
        switch (you.religion)
        {
        case GOD_VEHUMET:
        case GOD_MAKHLEB:
            simple_god_message(" accepts your collateral kill.");

            if (random2(pgain + 18) > 5)
                gain_piety(1);
            break;
        }
        break;

    case GOOD_CARDS:
        switch (you.religion)
        {
        case GOD_NEMELEX_XOBEH:
            gain_piety(pgain);
            break;
        }
        break;
    // Offering at altars is covered in another function.
    }
}                               // end done_good()

void gain_piety(char pgn)
{
    // check to see if we owe anything first
    if (you.penance[you.religion] > 0)
    {
        dec_penance(pgn);
        return;
    }
    else if (you.gift_timeout > 0)
    {
        if (you.gift_timeout > pgn)
            you.gift_timeout -= pgn;
        else
            you.gift_timeout = 0;

        // Slow down piety gain to account for the fact that gifts
        // no longer have a piety cost for getting them
        if (!one_chance_in(8))
            return;
    }

    // slow down gain at upper levels of piety
    if (you.piety > 199
        || (you.piety > 150 && one_chance_in(3))
        || (you.piety > 100 && one_chance_in(3)))
        return;

    int old_piety = you.piety;

    you.piety += pgn;

    if (you.piety >= 30 && old_piety < 30)
    {
        switch (you.religion)
        {
        case GOD_NO_GOD:
        case GOD_XOM:
        case GOD_NEMELEX_XOBEH:
        case GOD_SIF_MUNA:
            break;
        default:
            strcpy(info, "You can now ");
            strcat(info,
                    (you.religion == GOD_ZIN || you.religion == GOD_SHINING_ONE)
                            ? "repel the undead" :

                    (you.religion == GOD_KIKUBAAQUDGHA)
                            ? "recall your undead slaves" :
                    (you.religion == GOD_YREDELEMNUL)
                            ? "animate corpses" :
                    (you.religion == GOD_VEHUMET)
                            ? "gain power from killing in Vehumet's name" :
                    (you.religion == GOD_MAKHLEB)
                            ? "gain power from killing in Makhleb's name" :
                    (you.religion == GOD_OKAWARU)
                            ? "give your great, but temporary, body strength" :
                    (you.religion == GOD_TROG)
                            ? "go berserk at will" :
                    (you.religion == GOD_ELYVILON)
                            ? "call upon Elyvilon for minor healing"
                // Unknown god
                            : "endure this program bug");

            strcat(info, ".");
            god_speaks(you.religion, info);
            break;
        }
    }

    if (you.piety >= 50 && old_piety < 50)
    {
        switch (you.religion)
        {
        case GOD_NO_GOD:
        case GOD_XOM:
        case GOD_NEMELEX_XOBEH:
            break;
        case GOD_KIKUBAAQUDGHA:
            simple_god_message(" is protecting you from some side-effects of death magic.");
            break;

        case GOD_VEHUMET:
            god_speaks(you.religion, "You can call upon Vehumet to aid your destructive magics with prayer.");
            break;

        default:
            strcpy(info, "You can now ");

            strcat(info,
                   (you.religion == GOD_ZIN)
                           ? "call upon Zin for minor healing" :
                   (you.religion == GOD_SHINING_ONE)
                           ? "smite your foes" :
                   (you.religion == GOD_YREDELEMNUL)
                           ? "recall your undead slaves" :
                   (you.religion == GOD_OKAWARU)
                           ? "call upon Okawaru for minor healing" :
                   (you.religion == GOD_MAKHLEB)
                           ? "harness Makhleb's destructive might" :
                   (you.religion == GOD_SIF_MUNA)
                           ? "freely open your mind to new spells" :
                   (you.religion == GOD_TROG)
                           ? "give your body great, but temporary, strength" :
                   (you.religion == GOD_ELYVILON)
                           ? "call upon Elyvilon for purification"
                   // Unknown god
                           : "endure this program bug");

            strcat(info, ".");
            god_speaks(you.religion, info);
            break;
        }
    }

    if (you.piety >= 75 && old_piety < 75)
    {
        switch (you.religion)
        {
        case GOD_NO_GOD:
        case GOD_XOM:
        case GOD_OKAWARU:
        case GOD_NEMELEX_XOBEH:
        case GOD_SIF_MUNA:
        case GOD_TROG:
            break;
        case GOD_VEHUMET:
            god_speaks(you.religion,"During prayer you have some protection from summoned creatures.");
            break;

        default:
            strcpy(info, "You can now ");
            strcat(info,
                     (you.religion == GOD_ZIN)
                                 ? "call down a plague" :
                     (you.religion == GOD_SHINING_ONE)
                                 ? "dispel the undead" :
                     (you.religion == GOD_KIKUBAAQUDGHA)
                                 ? "permanently enslave the undead" :
                     (you.religion == GOD_YREDELEMNUL)
                                 ? "animate legions of the dead" :
                     (you.religion == GOD_MAKHLEB)
                                 ? "summon a lesser servant of Makhleb" :
                     (you.religion == GOD_ELYVILON)
                                 ? "call upon Elyvilon for moderate healing"
                     // Unknown god
                                 : "endure this program bug");
            strcat(info, ".");
            god_speaks(you.religion, info);
            break;
        }
    }

    if (you.piety >= 100 && old_piety < 100)
    {
        switch (you.religion)
        {
        case GOD_NO_GOD:
        case GOD_XOM:
        case GOD_OKAWARU:
        case GOD_NEMELEX_XOBEH:
        case GOD_KIKUBAAQUDGHA:
            break;
        case GOD_SIF_MUNA:
            simple_god_message
                (" is protecting you from some side-effects of spellcasting.");
            break;

        default:
            strcpy(info, "You can now ");

            strcat(info,
                        (you.religion == GOD_ZIN)
                                ? "utter a Holy Word" :
                        (you.religion == GOD_SHINING_ONE)
                                ? "hurl bolts of divine anger" :
                        (you.religion == GOD_YREDELEMNUL)
                                ? "drain ambient lifeforce" :
                        (you.religion == GOD_VEHUMET)
                                ? "tap ambient magical fields" :
                        (you.religion == GOD_MAKHLEB)
                                ? "hurl Makleb's greater destruction" :
                        (you.religion == GOD_TROG)
                                ? "haste yourself" :
                        (you.religion == GOD_ELYVILON)
                                ? "call upon Elyvilon to restore your abilities"
                        // Unknown god
                                : "endure this program bug");

            strcat(info, ".");
            god_speaks(you.religion, info);
            break;
        }
    }

    if (you.piety >= 120 && old_piety < 120)
    {
        switch (you.religion)
        {
        case GOD_NO_GOD:
        case GOD_XOM:
        case GOD_NEMELEX_XOBEH:
        case GOD_VEHUMET:
        case GOD_SIF_MUNA:
        case GOD_TROG:
            break;
        default:
            strcpy(info, "You can now ");

            strcat(info,
                     (you.religion == GOD_ZIN)
                                ? "summon a guardian angel" :
                     (you.religion == GOD_SHINING_ONE)
                                ? "summon a divine warrior" :
                     (you.religion == GOD_KIKUBAAQUDGHA)
                                ? "summon an emmisary of Death" :
                     (you.religion == GOD_YREDELEMNUL)
                                ? "control the undead" :
                     (you.religion == GOD_OKAWARU)
                                ? "haste yourself" :
                     (you.religion == GOD_MAKHLEB)
                                ? "summon a greater servant of Makhleb" :
                     (you.religion == GOD_ELYVILON)
                                ? "call upon Elyvilon for incredible healing"
                     // Unknown god
                                : "endure this program bug");

            strcat(info, ".");
            god_speaks(you.religion, info);
            break;
        }
    }
}                               // end gain_piety()

void naughty(char type_naughty, int naughtiness)
{
    int penance = 0;
    int piety_loss = 0;

    // if you currently worship no deity in particular, exit function {dlb}
    if (you.religion == GOD_NO_GOD)
        return;

    switch (you.religion)
    {
    case GOD_ZIN:
        switch (type_naughty)
        {
        case NAUGHTY_NECROMANCY:
        case NAUGHTY_UNHOLY:
        case NAUGHTY_ATTACK_HOLY:
            piety_loss = naughtiness;
            penance = piety_loss * 2;
            break;
        case NAUGHTY_ATTACK_FRIEND:
            piety_loss = naughtiness;
            penance = piety_loss * 3;
            break;
        case NAUGHTY_FRIEND_DIES:
            piety_loss = naughtiness;
            break;
        case NAUGHTY_BUTCHER:
            piety_loss = naughtiness;
            if (one_chance_in(3))
                penance = piety_loss;
            break;
        }
        break;

    case GOD_SHINING_ONE:
        switch (type_naughty)
        {
        case NAUGHTY_NECROMANCY:
        case NAUGHTY_UNHOLY:
        case NAUGHTY_ATTACK_HOLY:
            piety_loss = naughtiness;
            penance = piety_loss;
            break;
        case NAUGHTY_ATTACK_FRIEND:
            piety_loss = naughtiness;
            penance = piety_loss * 3;
            break;
        case NAUGHTY_FRIEND_DIES:
            piety_loss = naughtiness;
            break;
        case NAUGHTY_BUTCHER:
            piety_loss = naughtiness;
            if (one_chance_in(3))
                penance = piety_loss;
            break;
        case NAUGHTY_STABBING:
            piety_loss = naughtiness;
            if (one_chance_in(5))       // can be accidental so we're nice here
                penance = piety_loss;
            break;
        case NAUGHTY_POISON:
            piety_loss = naughtiness;
            penance = piety_loss * 2;
            break;
        }
        break;

    case GOD_ELYVILON:
        switch (type_naughty)
        {
        case NAUGHTY_NECROMANCY:
        case NAUGHTY_UNHOLY:
        case NAUGHTY_ATTACK_HOLY:
            piety_loss = naughtiness;
            penance = piety_loss;
            break;
        case NAUGHTY_KILLING:
            piety_loss = naughtiness;
            penance = piety_loss * 2;
            break;
        case NAUGHTY_ATTACK_FRIEND:
            piety_loss = naughtiness;
            penance = piety_loss * 3;
            break;
        // Healer god gets a bit more upset since you should have
        // used your healing powers to save them.
        case NAUGHTY_FRIEND_DIES:
            piety_loss = naughtiness;
            penance = piety_loss;
            break;
        case NAUGHTY_BUTCHER:
            piety_loss = naughtiness;
            if (one_chance_in(3))
                penance = piety_loss;
            break;
        }
        break;

    case GOD_OKAWARU:
        switch (type_naughty)
        {
        case NAUGHTY_ATTACK_FRIEND:
            piety_loss = naughtiness;
            penance = piety_loss * 3;
            break;
        case NAUGHTY_FRIEND_DIES:
            piety_loss = naughtiness;
            break;
        }
        break;

    case GOD_TROG:
        switch (type_naughty)
        {
        case NAUGHTY_SPELLCASTING:
            piety_loss = naughtiness;
            // This penance isn't so bad since its much easier to
            // gain piety with Trog than the other gods in this function.
            penance = piety_loss * 10;
            break;
        }
        break;
    }

    // exit function early iff piety loss is zero:
    if (!piety_loss)
        return;

    // output guilt message:
    strcpy(info, "You feel");

    strcat(info, (piety_loss == 1) ? " a little " :
                 (piety_loss <  5) ? " " :
                 (piety_loss < 10) ? " very "
                                   : " extremely ");

    strcat(info, "guilty.");
    mpr(info);

    lose_piety(piety_loss);

    if (you.piety < 1)
        excommunication();
    else if (penance)       // Don't bother unless we're not kicking them out
    {
        //jmf: FIXME: add randomness to following message:
        god_speaks(you.religion, "\"You will pay for your transgression, mortal!\"");
        inc_penance(penance);
    }
}                               // end naughty()

void lose_piety(char pgn)
{
    int old_piety = you.piety;

    if (you.piety - pgn < 0)
        you.piety = 0;
    else
        you.piety -= pgn;

    // Don't bother printing out these messages if you're under
    // penance, you wouldn't notice since all these abilities
    // are withheld.
    if (!player_under_penance() && you.piety != old_piety)
    {
        if (you.piety < 120 && old_piety >= 120)
        {
            switch (you.religion)
            {
            case GOD_NO_GOD:
            case GOD_XOM:
            case GOD_NEMELEX_XOBEH:
            case GOD_VEHUMET:
            case GOD_SIF_MUNA:
            case GOD_TROG:
                break;
            default:
                strcpy(info, "You can no longer ");

                strcat(info,
                           (you.religion == GOD_ZIN)
                                ? "summon guardian angels" :
                           (you.religion == GOD_SHINING_ONE)
                                ? "summon divine warriors" :
                           (you.religion == GOD_KIKUBAAQUDGHA)
                                ? "summon Death's emissaries" :
                           (you.religion == GOD_YREDELEMNUL)
                                ? "control undead beings" :
                           (you.religion == GOD_OKAWARU)
                                ? "haste yourself" :
                           (you.religion == GOD_MAKHLEB)
                                ? "summon a greater servent of Makhleb" :
                           (you.religion == GOD_ELYVILON)
                                ? "call upon Elyvilon for incredible healing"
                           // Unknown god
                                : "endure this program bug");

                strcat(info, ".");
                god_speaks(you.religion, info);
                break;
            }
        }

        if (you.piety < 100 && old_piety >= 100)
        {
            switch (you.religion)
            {
            case GOD_NO_GOD:
            case GOD_XOM:
            case GOD_OKAWARU:
            case GOD_NEMELEX_XOBEH:
            case GOD_KIKUBAAQUDGHA:
                break;
            case GOD_SIF_MUNA:
                god_speaks(you.religion,"Sif Muna is no longer protecting you from miscast magic.");
                break;
            default:
                strcpy(info, "You can no longer ");
                strcat(info,
                        (you.religion == GOD_ZIN)
                            ? "utter a Holy Word" :
                        (you.religion == GOD_ELYVILON)
                            ? "call upon Elyvilon to restore your abilities" :
                        (you.religion == GOD_SHINING_ONE)
                            ? "hurl bolts of divine anger" :
                        (you.religion == GOD_YREDELEMNUL)
                            ? "drain ambient life force" :
                        (you.religion == GOD_VEHUMET)
                            ? "tap ambient magical fields" :
                        (you.religion == GOD_MAKHLEB)
                            ? "direct Makleb's greater destructive powers" :
                        (you.religion == GOD_TROG)
                            ? "haste yourself"
                        // Unknown god
                            : "endure this program bug");

                strcat(info, ".");
                god_speaks(you.religion, info);
                break;
            }
        }

        if (you.piety < 75 && old_piety >= 75)
        {
            switch (you.religion)
            {
            case GOD_NO_GOD:
            case GOD_XOM:
            case GOD_OKAWARU:
            case GOD_NEMELEX_XOBEH:
            case GOD_SIF_MUNA:
            case GOD_TROG:
                break;
            case GOD_VEHUMET:
                simple_god_message(" will longer shield you from summoned creatures.");
                break;
            default:
                strcpy(info, "You can no longer ");

                strcat(info,
                       (you.religion == GOD_ZIN)
                                ? "call down a plague" :
                       (you.religion == GOD_SHINING_ONE)
                                ? "dispel undead" :
                       (you.religion == GOD_KIKUBAAQUDGHA)
                                ? "enslave undead" :
                       (you.religion == GOD_YREDELEMNUL)
                                ? "animate legions of the dead" :
                       (you.religion == GOD_MAKHLEB)
                                ? "summon a servant of Makhleb" :
                       (you.religion == GOD_ELYVILON)
                                ? "call upon Elyvilon for moderate healing"
                       // Unknown god
                                : "endure this program bug");

                strcat(info, ".");
                god_speaks(you.religion, info);
                break;
            }
        }

        if (you.piety < 50 && old_piety >= 50)
        {
            switch (you.religion)
            {
            case GOD_NO_GOD:
            case GOD_XOM:
            case GOD_NEMELEX_XOBEH:
                break;
            case GOD_KIKUBAAQUDGHA:
                simple_god_message(" is no longer shielding you from miscast death magic.");
                break;
            case GOD_VEHUMET:
                simple_god_message(" will no longer aid your destructive magics.");
                break;

            default:
                strcpy(info, "You can no longer ");

                strcat(info,
                       (you.religion == GOD_ZIN)
                            ? "call upon Zin for minor healing" :
                       (you.religion == GOD_SHINING_ONE)
                            ? "smite your foes" :
                       (you.religion == GOD_YREDELEMNUL)
                            ? "recall your undead slaves" :
                       (you.religion == GOD_OKAWARU)
                            ? "call upon Okawaru for minor healing" :
                       (you.religion == GOD_MAKHLEB)
                            ? "hurl Makhleb's destruction" :
                       (you.religion == GOD_SIF_MUNA)
                            ? "forget spells at will" :
                       (you.religion == GOD_TROG)
                            ? "give your body great, but temporary, strength" :
                       (you.religion == GOD_ELYVILON)
                            ? "call upon Elyvilon for Purification"
                       // Unknown god
                            : "endure this program bug");

                strcat(info, ".");
                god_speaks(you.religion, info);
                break;
            }
        }

        if (you.piety < 30 && old_piety >= 30)
        {
            switch (you.religion)
            {
            case GOD_NO_GOD:
            case GOD_XOM:
            case GOD_NEMELEX_XOBEH:
            case GOD_SIF_MUNA:
                break;
            default:
                strcpy(info, "You can no longer ");

                strcat(info,
                    (you.religion == GOD_ZIN || you.religion == GOD_SHINING_ONE)
                            ? "repel the undead" :
                    (you.religion == GOD_KIKUBAAQUDGHA)
                            ? "recall your undead slaves" :
                    (you.religion == GOD_YREDELEMNUL)
                            ? "animate corpses" :
                    (you.religion == GOD_VEHUMET)
                            ? "gain power from killing in Vehumet's name" :
                    (you.religion == GOD_MAKHLEB)
                            ? "gain power from killing in Makhleb's name" :
                    (you.religion == GOD_OKAWARU)
                            ? "give your body great, but temporary, strength" :
                    (you.religion == GOD_TROG)
                            ? "go berserk at will" :
                    (you.religion == GOD_ELYVILON)
                            ? "call upon Elyvilon for minor healing."
                    // Unknown god
                            : "endure this program bug");

                strcat(info, ".");
                god_speaks(you.religion, info);
                break;
            }
        }
    }
}                               // end lose_piety()

void divine_retribution(int god)
{
    ASSERT(god != GOD_NO_GOD);

    int loopy = 0;              // general purpose loop variable {dlb}
    int temp_rand = 0;          // probability determination {dlb}
    int punisher = MONS_PROGRAM_BUG;
    bool success = false;
    int how_many = 0;
    int divine_hurt = 0;

    // Good gods don't use divine retribution on their followers, they
    // will consider it for those who have gone astray however.
    if (god == you.religion)
    {
        if (god == GOD_SHINING_ONE || god == GOD_ZIN || god == GOD_ELYVILON)
            return;
    }

    // Just the thought of retribution (getting this far) mollifies the
    // god a few points... the punishment might reduce penance further.
    dec_penance(god, 1 + random2(3));

    switch (god)
    {
    case GOD_XOM:
        {
            // One in ten chance that Xom might do something good...
            // but that isn't forced, bad things are though
            bool nice = one_chance_in(10);
            bool force = !nice;

            Xom_acts(nice, you.experience_level, force);
        }
        break;

    case GOD_SHINING_ONE:
        // Doesn't care unless you've gone over to evil
        //jmf: expanded definition of evil above to include Vehumet and Makhleb
        // XXX: Not sure destructive gods belong here
        if (you.religion == GOD_KIKUBAAQUDGHA || you.religion == GOD_MAKHLEB
            || you.religion == GOD_YREDELEMNUL || you.religion == GOD_VEHUMET)
        {
            if (coinflip())
            {
                success = false;
                how_many = 1 + random2(you.experience_level / 5) + random2(3);

                for (loopy = 0; loopy < how_many; loopy++)
                {
                    if (create_monster( MONS_DAEVA, 0, BEH_CHASING_I,
                                    you.x_pos, you.y_pos, MHITNOT, 250) != -1)
                    {
                        success = true;
                    }
                }

                if (success)
                {
                    simple_god_message(" sends the divine host to punish your evil ways!", god);
                }
            }
            else
            {
                divine_hurt = 10 + random2(10);

                for (loopy = 0; loopy < 5; loopy++)
                    divine_hurt += random2(you.experience_level);

                if (!player_under_penance() && you.piety > random2(400))
                {
                    strcpy(info, "Mortal, I have averted the wrath of "
                        "the Shining One... this time.");
                    god_speaks(you.religion, info);
                }
                else
                {
                    simple_god_message(" smites you!", god);
                    ouch(divine_hurt, 0, KILLED_BY_TSO_SMITING);
                    dec_penance(GOD_SHINING_ONE, 1);
                }
            }
        }
        break;

    case GOD_ZIN:
        // Doesn't care unless you've gone over to evil
        if (you.religion == GOD_KIKUBAAQUDGHA || you.religion == GOD_MAKHLEB
            || you.religion == GOD_YREDELEMNUL || you.religion == GOD_VEHUMET)
        {
            if (random2(you.experience_level) > 7)
            {
                success = false;
                how_many = 1 + (you.experience_level / 10) + random2(3);

                for (loopy = 0; loopy < how_many; loopy++)
                {
                    if (create_monster(MONS_ANGEL, 0, BEH_CHASING_I,
                                    you.x_pos, you.y_pos, MHITNOT, 250) != -1)
                    {
                        success = true;
                    }
                }

                if (success)
                {
                    simple_god_message (" sends the divine host to punish you for your evil ways!", god);
                }
            }
            else
            {
                how_many = 2 + (you.experience_level / 5);

                for (loopy = 0; loopy < how_many; loopy++)
                {
                    summon_swarm(0);    // power = 0 gives unfriendly
                }
                simple_god_message(" sends down a plague upon you!", god);
            }
        }
        break;

    case GOD_MAKHLEB:
        if (random2(you.experience_level) > 7)
        {
            if (create_monster(MONS_EXECUTIONER + random2(5), 0,
                               BEH_CHASING_I, you.x_pos, you.y_pos,
                               MHITNOT, 250) != -1)
            {
                simple_god_message(" sends a greater servant against you!",
                                   god);
            }
        }
        else
        {
            success = false;
            how_many = 1 + (you.experience_level / 7);

            for (loopy = 0; loopy < how_many; loopy++)
            {
                if (create_monster(MONS_NEQOXEC + random2(5), 0, BEH_CHASING_I,
                                    you.x_pos, you.y_pos, MHITNOT, 250) != -1)
                {
                    success = true;
                }
            }

            if (success)
                simple_god_message(" sets servents against you.", god);
        }
        break;

    case GOD_KIKUBAAQUDGHA:
        if (random2(you.experience_level) > 7)
        {
            success = false;
            how_many = 1 + (you.experience_level / 7) + random2(3);

            for (loopy = 0; loopy < how_many; loopy++)
            {
                if (create_monster(MONS_REAPER, 0, BEH_CHASING_I, you.x_pos,
                                   you.y_pos, MHITNOT, 250) != -1)
                {
                    success = true;
                }
            }

            if (success)
                simple_god_message(" unleashes Death upon you!", god);
        }
        else
        {
            god_speaks(god, (coinflip()) ? "You hear Kikubaaqudgha cackling."
                                         : "Kikubaaqudgha's malice focuses upon you.");

            miscast_effect(SPTYP_NECROMANCY, 5 + you.experience_level,
                           random2avg(88, 3), 100);
        }
        break;

    case GOD_YREDELEMNUL:
        if (random2(you.experience_level) > 4)
        {
            success = false;
            how_many = 1 + random2(1 + (you.experience_level / 5));

            for (loopy = 0; loopy < how_many; loopy++)
            {
                temp_rand = random2(100);

                punisher = ((temp_rand > 66) ? MONS_WRAITH :            // 33%
                            (temp_rand > 52) ? MONS_WIGHT :             // 12%
                            (temp_rand > 40) ? MONS_SPECTRAL_WARRIOR :  // 16%
                            (temp_rand > 31) ? MONS_ROTTING_HULK :      //  9%
                            (temp_rand > 23) ? MONS_SKELETAL_WARRIOR :  //  8%
                            (temp_rand > 16) ? MONS_VAMPIRE :           //  7%
                            (temp_rand > 10) ? MONS_GHOUL :             //  6%
                            (temp_rand >  4) ? MONS_MUMMY               //  6%
                                             : MONS_FLAYED_GHOST);      //  5%

                if (create_monster(punisher, 0, BEH_CHASING_I, you.x_pos,
                                   you.y_pos, MHITNOT, 250) != -1)
                {
                    success = true;
                }
            }

            if (success)
                simple_god_message(" sends a servent to punish you.", god);
        }
        else
        {
            simple_god_message("'s anger turns toward you for a moment.",
                               god);

            miscast_effect(SPTYP_NECROMANCY, 5 + you.experience_level,
                           random2avg(88, 3), 100);
        }
        break;

    case GOD_TROG:
        switch (random2(6))
        {
        case 0:
        case 1:
        case 2:
            {
                // Would be better if berzerking monsters were available,
                // we just send some big bruisers for now.
                success = false;

                int points = 3 + random2(you.experience_level * 2);

                while (points > 0)
                {
                    if (points > 20)
                    {
                        // quick reduction for large values
                        punisher = MONS_DEEP_TROLL;
                        points -= 15;
                        break;
                    }
                    else
                    {
                        switch (random2(10))
                        {
                        case 0:
                            punisher = MONS_IRON_TROLL;
                            points -= 10;
                            break;

                        case 1:
                            punisher = MONS_ROCK_TROLL;
                            points -= 10;
                            break;

                        case 2:
                            punisher = MONS_TROLL;
                            points -= 6;
                            break;

                        case 3:
                        case 4:
                            punisher = MONS_MINOTAUR;
                            points -= 3;
                            break;

                        case 5:
                        case 6:
                            punisher = MONS_TWO_HEADED_OGRE;
                            points -= 4;
                            break;

                        case 7:
                        case 8:
                        case 9:
                        default:
                            punisher = MONS_OGRE;
                            points -= 3;
                        }
                    }

                    if (create_monster(punisher, 0, BEH_CHASING_I, you.x_pos,
                                       you.y_pos, MHITNOT, 250) != -1)
                    {
                        success = true;
                    }
                }

                if (success)
                    simple_god_message(" sends monsters to punish you.", god);
            }
            break;

        case 3:
        case 4:
            simple_god_message( " says: Feel my wrath!", god );

            // A collection of physical effects that might be better
            // suited to Trog than wild fire magic... messagaes sould
            // be better here... something more along the lines of apathy
            // or lose of rage to go with the anti-berzerk effect-- bwr
            switch (random2(6))
            {
            case 0:
                potion_effect( POT_DECAY, 100 );
                break;

            case 1:
            case 2:
                lose_stat(STAT_STRENGTH, 1 + random2(you.strength / 5), true);
                break;

            case 3:
                if (!you.paralysis)
                {
                    dec_penance(GOD_TROG, 3);
                    mpr( "You suddenly pass out!", MSGCH_WARN );
                    you.paralysis = 2 + random2(6);
                }
                break;

            case 4:
            case 5:
                if (you.slow < 90)
                {
                    dec_penance(GOD_TROG, 1);
                    mpr( "You suddenly feel exhausted!", MSGCH_WARN );
                    you.slow = 90;
                    you.exhausted = 90;
                }
                break;
            };
            break;

        //jmf: returned Trog's old Fire damage
        // -- actually, this function partially exists to remove that,
        //    we'll leave this effect in, but we'll remove the wild
        //    fire magic. -- bwr
        case 5:
            dec_penance(GOD_TROG, 2);
            simple_god_message(" hurls flames of wrath!", god);
            you.duration[DUR_LIQUID_FLAMES] += random2avg(7, 3) + 1;
            break;
        }
        break;

    case GOD_OKAWARU:
        {
            success = false;
            how_many = 1 + (you.experience_level / 7);

            for (loopy = 0; loopy < how_many; loopy++)
            {
                temp_rand = random2(100);

                punisher = ((temp_rand > 84) ? MONS_ORC_WARRIOR :
                            (temp_rand > 69) ? MONS_ORC_KNIGHT :
                            (temp_rand > 59) ? MONS_NAGA_WARRIOR :
                            (temp_rand > 49) ? MONS_CENTAUR_WARRIOR :
                            (temp_rand > 39) ? MONS_STONE_GIANT :
                            (temp_rand > 29) ? MONS_FIRE_GIANT :
                            (temp_rand > 19) ? MONS_FROST_GIANT :
                            (temp_rand >  9) ? MONS_CYCLOPS
                                             : MONS_HILL_GIANT);

                if (create_monster(punisher, 0, BEH_CHASING_I,
                                   you.x_pos, you.y_pos, MHITNOT, 250) != -1)
                {
                    success = true;
                }
            }

            // XXX: This message is weak -- it doesn't suggest punishment
            if (success)
                simple_god_message(" decides to test your mettle.", god);
        }
        break;

    case GOD_VEHUMET:
        simple_god_message("'s vengence finds you.", god);
        miscast_effect(coinflip() ? SPTYP_CONJURATION : SPTYP_SUMMONING,
                       8 + you.experience_level, random2avg(98, 3), 100);
        break;

    case GOD_NEMELEX_XOBEH:
        simple_god_message(" forces you to draw from the Deck of Punishment.",
                           god);
        deck_of_cards(DECK_OF_PUNISHMENT);
        break;

    case GOD_SIF_MUNA:
        // XXX: This message is weak
        simple_god_message("'s wrath finds you.", god);
        dec_penance(GOD_SIF_MUNA, 1);

        switch (random2(10))
        {
        case 0:
        case 1:
            lose_stat(STAT_INTELLIGENCE, 1 + random2( you.intel / 5 ), true);
            break;

        case 2:
        case 3:
        case 4:
            // We access directly here to avoid the player saving
            if (you.conf < 20)
            {
                you.conf = 20;
                mpr( "You can't seem to think straight!" );
            }
            break;

        case 5:
        case 6:
            miscast_effect( SK_DIVINATIONS, 9, 90, 100 );
            break;

        case 7:
        case 8:
            if (you.magic_points)
            {
                dec_mp( 100 );  // this should zero it.
                mpr( "You suddenly feel drained of magical energy!",
                     MSGCH_WARN );
            }
            break;

        case 9:
            // This is the one Sif Muna abusers should dread... it will
            // set all the extendable duration spells to a duration
            // of one round, thus potentially exposing the player to
            // real danger.
            dec_penance(GOD_SIF_MUNA, 3);
            antimagic();
            mpr( "You sense a magical dispell.", MSGCH_WARN );
            break;
        }
        break;

    case GOD_ELYVILON:  // Elyvilon doesn't seek revenge
    default:
        break;
    }

    // Sometimes divine experiences are overwelming...
    // We access directly here to avoid the player saving against these
    if (one_chance_in(5))
    {
        if (coinflip()) {
            if (you.conf < 20)
            {
                mpr( "The divine experience left you feeling confused!",
                     MSGCH_WARN );

                you.conf += random2(10);
                if (you.conf > 20)
                    you.conf = 20;
            }
        }
        else
        {
            if (you.slow < 90)
            {
                mpr( "The divine experience left you feeling exhausted!",
                     MSGCH_WARN );

                you.slow += random2(10);
                if (you.slow > 90)
                    you.slow = 90;
            }
        }
    }

    return;
}                               // end divine_retribution()

void excommunication(void)
{
    mpr("You have lost your religion!");
    more();

    switch (you.religion)
    {
    case GOD_XOM:
        Xom_acts(false, (you.experience_level * 2), true);
        inc_penance(50);
        break;

    case GOD_KIKUBAAQUDGHA:
    case GOD_YREDELEMNUL:
        simple_god_message(" does not appreciate desertion!");
        miscast_effect(SPTYP_NECROMANCY, 5 + you.experience_level,
                       random2avg(88, 3), 100);
        inc_penance(30);
        break;

    case GOD_VEHUMET:
    case GOD_MAKHLEB:
        simple_god_message(" does not appreciate desertion!");
        miscast_effect((coinflip() ? SPTYP_CONJURATION : SPTYP_SUMMONING),
                       8 + you.experience_level, random2avg(98, 3), 100);
        inc_penance(25);
        break;

    case GOD_TROG:
        // Penence has to come before retribution to prevent "mollify"
        inc_penance(50);
        // This is a better thing for Trog to do than wild fire magic
        divine_retribution(GOD_TROG);
        break;

    // these like to haunt players for a bit more than the standard
    case GOD_NEMELEX_XOBEH:
    case GOD_SIF_MUNA:
        inc_penance(50);
        break;

    default:
        inc_penance(15);
        break;
    }

    you.religion = GOD_NO_GOD;
    you.piety = 0;
    //you.gift_timeout = 0; //jmf: ought to do this?
}                               // end excommunication()


void altar_prayer(void)
{
    int i, j;
    char subst_id[4][50];

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 50; j++)
        {
            subst_id[i][j] = 1;
        }
    }

    mpr("You kneel at the altar and pray.");

    if (igrd[you.x_pos][you.y_pos] == NON_ITEM
        || you.religion == GOD_SHINING_ONE || you.religion == GOD_XOM)
    {
        return;
    }

    i = igrd[you.x_pos][you.y_pos];

    do
    {
        if (one_chance_in(1000))
            break;

        j = mitm.link[i];

        const int value = item_value( mitm.base_type[i], mitm.sub_type[i],
                                        mitm.special[i], mitm.pluses[i],
                                        mitm.pluses2[i], mitm.quantity[i], 3,
                                        subst_id);
        switch (you.religion)
        {
        case GOD_ZIN:
        case GOD_OKAWARU:
        case GOD_MAKHLEB:
        case GOD_NEMELEX_XOBEH:
            it_name(i, 0, str_pass);
            strcpy(info, str_pass);
            strcat(info, sacrifice[you.religion - 1]);
            mpr(info);

            if (mitm.base_type[i] == OBJ_CORPSES || random2( value ) >= 50)
                gain_piety(1);

            destroy_item(i);
            break;

        case GOD_SIF_MUNA:
            it_name(i, 0, str_pass);
            strcpy(info, str_pass);
            strcat(info, sacrifice[you.religion - 1]);
            mpr(info);

            if (value >= 150)
                gain_piety(1 + random2(3));

            destroy_item(i);
            break;

        case GOD_KIKUBAAQUDGHA:
        case GOD_TROG:
            if (mitm.base_type[i] != OBJ_CORPSES)
                break;

            it_name(i, 0, str_pass);
            strcpy(info, str_pass);
            strcat(info, sacrifice[you.religion - 1]);
            mpr(info);

            gain_piety(1);
            destroy_item(i);
            break;

        case GOD_ELYVILON:
            if (mitm.base_type[i] != OBJ_WEAPONS
                    && mitm.base_type[i] != OBJ_MISSILES)
            {
                break;
            }

            it_name(i, 0, str_pass);
            strcpy(info, str_pass);
            strcat(info, sacrifice[you.religion - 1]);
            mpr(info);

            if (random2( value ) >= random2(50))
                gain_piety(1);

            destroy_item(i);
            break;
        }
        i = j;
    }
    while (i != NON_ITEM);
}                               // end altar_prayer()

void god_pitch(unsigned char which_god)
{
    strcpy(info, "You kneel at the altar of ");
    strcat(info, god_name(which_god));
    strcat(info, ".");
    mpr(info);

    more();

    if ((you.is_undead || you.species == SP_DEMONSPAWN)
        && (which_god == GOD_ZIN || which_god == GOD_SHINING_ONE
            || which_god == GOD_ELYVILON))
    {
        simple_god_message(" does not accept worship from those such as you!",
                           which_god);
        return;
    }

    describe_god(which_god);

    if (!yesno("Do you wish to join this religion?"))
    {
        redraw_screen();
        return;
    }

    if (!yesno("Are you sure?"))
    {
        redraw_screen();
        return;
    }

    if (you.religion != GOD_NO_GOD)
        excommunication();

    you.religion = which_god;   //jmf: moved up so god_speaks gives right colour
    you.piety = 15;             // to prevent near instant excommunication

    simple_god_message(" welcomes you!");

    more();

    redraw_screen();

    // XXX: Currently penance is just zeroed, this could be much
    // more interesting
    you.penance[you.religion] = 0;

    if (you.religion == GOD_KIKUBAAQUDGHA || you.religion == GOD_YREDELEMNUL
        || you.religion == GOD_VEHUMET || you.religion == GOD_MAKHLEB)
    {
        if (you.penance[GOD_SHINING_ONE] > 0)
        {
            inc_penance(GOD_SHINING_ONE, 30);
            god_speaks(GOD_SHINING_ONE, "\"You will pay for your evil ways, mortal!\"");
        }
    }
}                               // end god_pitch()

void offer_corpse(int corpse)
{
    it_name(corpse, 0, str_pass);
    strcpy(info, str_pass);
    strcat(info, sacrifice[you.religion - 1]);
    mpr(info);

    done_good(GOOD_HACKED_CORPSE, 10);
}                               // end offer_corpse()

//jmf: moved stuff from items::handle_time()
void handle_god_time(void)
{
    if (one_chance_in(100))
    {
        // Choose a god randomly from those to whom we owe penance.
        //jmf: changed so every god has equal opportunity here
        //bwr: returned it back to the original algoritm (which actually
        // works... as opposed to jmf's which gives a scew to earlier gods)
        //
        // Proof: (By induction)
        //
        // 1) n = 1, probability of choosing god is one_chance_in(1)
        // 2) Asuume true for n = k (ie. prob = 1 / n for all n)
        // 3) For n = k + 1,
        //
        //      P:new-found-god = 1 / n (see algorithm)
        //      P:other-gods = (1 - P:new-found-god) * P:god-at-n=k
        //                             1        1
        //                   = (1 - -------) * ---
        //                           k + 1      k
        //
        //                          k         1
        //                   = ----------- * ---
        //                        k + 1       k
        //
        //                       1       1
        //                   = -----  = ---
        //                     k + 1     n
        //
        // Therefore, by induction the probability is uniform.  As for
        // why we do it this way... it requires only one pass and doesn't
        // require an array.

        int which_god = GOD_NO_GOD;
        unsigned int count = 0;

        for (int i = GOD_NO_GOD; i < NUM_GODS; i++)
        {
            if (you.penance[i])
            {
                count++;
                if (one_chance_in(count))
                    which_god = i;
            }
        }

        if (which_god != GOD_NO_GOD)
            divine_retribution(which_god);
    }

    // Update the god's opinion of the player
    if (you.religion != GOD_NO_GOD)
    {
        switch (you.religion)
        {
        case GOD_XOM:
            if (one_chance_in(75))
                Xom_acts(true, you.experience_level + random2(15), true);
            break;

        case GOD_ZIN:           // These gods like long-standing worshippers
        case GOD_ELYVILON:
            if (you.piety < 150 && one_chance_in(20))
                gain_piety(1);
            break;

        case GOD_SHINING_ONE:
            if (you.piety < 150 && one_chance_in(15))
                gain_piety(1);
            break;

        case GOD_YREDELEMNUL:
        case GOD_KIKUBAAQUDGHA:
        case GOD_VEHUMET:
            if (one_chance_in(17))
                lose_piety(1);
            if (you.piety < 1)
                excommunication();
            break;

        case GOD_OKAWARU: // These gods accept corpses, so they time-out faster
        case GOD_TROG:
            if (one_chance_in(14))
                lose_piety(1);
            if (you.piety < 1)
                excommunication();
            break;

        case GOD_MAKHLEB:
            if (one_chance_in(16))
                lose_piety(1);
            if (you.piety < 1)
                excommunication();
            break;

        case GOD_SIF_MUNA:
            if (one_chance_in(20))
                lose_piety(1);
            if (you.piety < 1)
                excommunication();
            break;

        case GOD_NEMELEX_XOBEH: // relatively patient
            if (one_chance_in(35))
                lose_piety(1);
            if (you.attribute[ATTR_CARD_COUNTDOWN] > 0 && coinflip())
                you.attribute[ATTR_CARD_COUNTDOWN]--;
            if (you.piety < 1)
                excommunication();
            break;

        default:
            DEBUGSTR("Bad god, no bishop!");
        }
    }
}                               // end handle_god_time()

// yet another wrapper for mpr() {dlb}:
void simple_god_message(const char *event, int which_deity)
{
    if (which_deity == GOD_NO_GOD)
        which_deity = you.religion;

    strcpy(info, god_name(which_deity));
    strcat(info, event);

    god_speaks(which_deity, info);
}

char god_colour( char god ) //mv - added
{
    switch (god)
    {
    case GOD_SHINING_ONE:
    case GOD_ZIN:
    case GOD_ELYVILON:
    case GOD_OKAWARU:
        return(CYAN);
        break;

    case GOD_YREDELEMNUL:
    case GOD_KIKUBAAQUDGHA:
    case GOD_MAKHLEB:
    case GOD_VEHUMET:
    case GOD_TROG:
        return(LIGHTRED);
        break;

    case GOD_XOM:
        return(YELLOW);
        break;

    case GOD_NEMELEX_XOBEH:
        return(LIGHTMAGENTA);
        break;

    case GOD_SIF_MUNA:
        return(LIGHTBLUE);
        break;

    case GOD_NO_GOD:
    default:
        return(YELLOW);
    }
}

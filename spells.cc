/*
 *  File:       spells.cc
 *  Summary:    Some spell related functions.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *     <2>     10/1/99     BCR     Added messages for failed animate dead
 *     <1>     -/--/--     LRH     Created
 */

/* This determines how likely it is that more powerful wild magic effects
 * will occur. Set to 100 for the old probabilities (although the individual
 * effects have been made much nastier since then).
 */
#define WILD_MAGIC_NASTINESS 100

#include "AppHdr.h"
#include "spells.h"

#ifdef DOS
#include <conio.h>
#endif

#include <string.h>
#include <stdlib.h>

#include "externs.h"

#include "bang.h"
#include "beam.h"
#include "dungeon.h"
#include "effects.h"
#include "invent.h"
#include "items.h"
#include "it_use2.h"
#include "it_use3.h"
#include "monplace.h"
#include "monstuff.h"
#include "misc.h"
#include "mutation.h"
#include "player.h"
#include "ouch.h"
#include "religion.h"
#include "spell.h"
#include "spells0.h"
#include "spells1.h"
#include "spells3.h"
#include "stuff.h"
#include "view.h"

#ifdef MACROS
#include "macro.h"
#endif

char mutate(int which_mutation);


int learned = 0;
int spell_container = 0;
int sc_read_1, sc_read_2;
int i;
int book_thing;
int keyin;


char miscast_effect(char sp_type, char mag_pow, char mag_fail, char force_effect)
{

/*  sp_type is the type of the spell
 *  mag_pow is overall power of the spell or effect (ie its level)
 *  mag_fail is the degree to which you failed
 *  force_effect forces a certain effect to occur. Currently unused.
 */

    struct bolt beam[1];

    char loopj = 0;
    int spec_effect = 0;
    int hurted = 0;

    spec_effect = (mag_pow * mag_fail * (10 + mag_pow) / 7 * WILD_MAGIC_NASTINESS) / 100;
    if (force_effect == 100 && random2(40) > spec_effect && random2(40) > spec_effect)
    {
        goto nothing_happening;
    }
    spec_effect = spec_effect / 100;

#ifdef WIZARD
    strcpy(info, "Sptype: ");
    itoa(sp_type, st_prn, 10);
    strcat(info, st_prn);
    strcat(info, ", failure1: ");
    itoa(spec_effect, st_prn, 10);
    strcat(info, st_prn);
#endif


    spec_effect = random2(spec_effect);
    if (spec_effect > 3)
        spec_effect = 3;
    if (spec_effect < 0)
        spec_effect = 0;

#ifdef WIZARD
    strcat(info, ", failure2: ");
    itoa(spec_effect, st_prn, 10);
    strcat(info, st_prn);
    mpr(info);
#endif



    if (force_effect != 100)
        spec_effect = force_effect;


    switch (sp_type)
    {

    case SPTYP_CONJURATION:

        switch (spec_effect)
        {
        case 0:         // just a harmless message

            switch (random2(10))
            {
            case 0:
                strcpy(info, "Sparks fly from your hands!");
                break;
            case 1:
                strcpy(info, "The air around you crackles with energy!");
                break;
            case 2:
                strcpy(info, "Wisps of smoke drift from your fingertips.");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "You are momentarily dazzled by a flash of light!");
                break;
            case 5:
                strcpy(info, "Strange energies run through your body.");
                break;
            case 6:
                strcpy(info, "Your skin tingles.");
                break;
            case 7:
                strcpy(info, "Your skin glows momentarily.");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You smell something strange.");
                break;
            }
            mpr(info);
            break;

        case 1:         // a bit less harmless stuff

            switch (random2(2))
            {
            case 0:
                strcpy(info, "Smoke pours from your fingertips!");
                mpr(info);
                big_cloud(CLOUD_GREY_SMOKE, you.x_pos, you.y_pos, 20);
                break;
            case 1:
                mpr("A wave of violent energy washes through your body!");
                mpr("Ouch.");
                ouch(6 + random2(4) + random2(4), 0, KILLED_BY_WILD_MAGIC);
                break;
            }
            break;

        case 2:         // rather less harmless stuff

            switch (random2(2))
            {
            case 0:
                mpr("Energy rips through your body!");
                mpr("Ouch!");
                ouch(9 + random2(9) + random2(9), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                strcpy(info, "You conjure up a violent explosion!");
                mpr(info);
                strcpy(info, "Oops.");
                mpr(info);
                beam[0].type = 43;
                beam[0].damage = 112;
                beam[0].flavour = BEAM_MISSILE;         // unsure about this
                // BEAM_EXPLOSION instead? {dlb}

                beam[0].bx = you.x_pos;
                beam[0].by = you.y_pos;
                strcpy(beam[0].beam_name, "explosion");
                beam[0].colour = random2(15) + 1;
                beam[0].thing_thrown = 1;       // your explosion (is this right?)

                explosion(0, beam);
                noisy(10, you.x_pos, you.y_pos);
                break;
            }
            break;

        case 3:         // considerably less harmless stuff

            switch (random2(2))
            {
            case 0:
                strcpy(info, "You are blasted with magical energy!");
                mpr(info);
                ouch(12 + random2(15) + random2(15), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                strcpy(info, "There is a sudden explosion of magical energy!");
                mpr(info);
                beam[0].type = 43;
                beam[0].damage = 120;
                beam[0].flavour = BEAM_MISSILE;         // unsure about this
                // BEAM_EXPLOSION instead? {dlb}

                beam[0].bx = you.x_pos;
                beam[0].by = you.y_pos;
                strcpy(beam[0].beam_name, "explosion");
                beam[0].colour = random2(15) + 1;
                beam[0].thing_thrown = 1;       // your explosion (is this right?)

                explosion(random2(2), beam);
                noisy(20, you.x_pos, you.y_pos);
                break;
            }
            break;

        }
        break;                  // end conjuration

    case SPTYP_ENCHANTMENT:
        switch (spec_effect)
        {
        case 0:         // harmless messages only

            switch (random2(10))
            {
            case 0:
                strcpy(info, "Your hands glow momentarily.");
                break;
            case 1:
                strcpy(info, "The air around you crackles with energy!");
                break;
            case 2:
                strcpy(info, "Multicolored lights dance before your eyes!");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "Waves of light ripple over your body.");
                break;
            case 5:
                strcpy(info, "Strange energies run through your body.");
                break;
            case 6:
                strcpy(info, "Your skin tingles.");
                break;
            case 7:
                strcpy(info, "Your skin glows momentarily.");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You hear something strange.");
                break;
            }
            mpr(info);
            break;

        case 1:         // slightly annoying

            switch (random2(2))
            {
            case 0:
                potion_effect(POT_LEVITATION, 20);

                break;
            case 1:
                random_uselessness(2 + random2(7), 0);
                break;
            }
            break;

        case 2:         // much more annoying

            switch (random2(3))
            {
            case 0:             // curse

                curse_an_item(0, 0);
                strcpy(info, "You sense a malignant aura.");
                mpr(info);
                break;
            case 1:
                potion_effect(POT_SLOWING, 10);
                break;
            case 2:
                potion_effect(POT_BERSERK_RAGE, 10);
                break;
            }
            break;

        case 3:         // potentially lethal

            switch (random2(4))
            {
            case 0:             // curse

                do
                {
                    curse_an_item(0, 0);
                    loopj = random2(3);
                }
                while (loopj != 0);
                strcpy(info, "You sense an overwhelmingly malignant aura!");
                mpr(info);
                break;
            case 1:
                potion_effect(POT_PARALYSIS, 10);
                break;
            case 2:
                potion_effect(POT_CONFUSION, 10);
                break;
            case 3:
                if (mutate(100) == 0)
                    mpr("Nothing appears to happen.");
                break;
            }
            break;

        }
        break;                  // end enchantments

    case SPTYP_TRANSLOCATION:
        switch (spec_effect)
        {
        case 0:         // harmless messages only

            switch (random2(10))
            {
            case 0:
                strcpy(info, "Space warps around you.");
                break;
            case 1:
                strcpy(info, "The air around you crackles with energy!");
                break;
            case 2:
                strcpy(info, "You feel a wrenching sensation.");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "You spin around.");
                break;
            case 5:
                strcpy(info, "Strange energies run through your body.");
                break;
            case 6:
                strcpy(info, "Your skin tingles.");
                break;
            case 7:
                strcpy(info, "The world appears momentarily distorted!");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You feel uncomfortable.");
                break;
            }
            mpr(info);
            break;

        case 1:         // mostly harmless

            switch (random2(3))
            {
            case 0:
                strcpy(info, "You create a localised field of spatial distortion.");
                mpr(info);
                strcpy(info, "Ouch!");
                mpr(info);
                ouch(4 + random2(5) + random2(5), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                strcpy(info, "Space bends around you!");
                mpr(info);
                random_blink();
                ouch(4 + random2(4) + random2(4), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 2:
                strcpy(info, "Space twists in upon itself!");
                mpr(info);
                create_monster(MONS_SPATIAL_VORTEX, 24, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            }
            break;

        case 2:         // less harmless

            switch (random2(2))
            {
            case 0:
                strcpy(info, "You create a strong localised field of spatial distortion.");
                mpr(info);
                strcpy(info, "Ouch!!");
                mpr(info);
                ouch(9 + random2(12) + random2(12), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                strcpy(info, "Space warps around you!");
                mpr(info);
                if (one_chance_in(3))
                    you_teleport2(true);
                else
                    random_blink();
                ouch(5 + random2(5) + random2(5), 0, KILLED_BY_WILD_MAGIC);
                potion_effect(POT_CONFUSION, 10);       // conf

                break;
            case 2:
                strcpy(info, "Space twists in upon itself!");
                mpr(info);
                for (loopj = 0; loopj < 2 + random2(3); loopj++)
                {
                    create_monster(MONS_SPATIAL_VORTEX, 22, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                }
                break;
            }
            break;

        case 3:         // much less harmless

            switch (random2(4))
            {
            case 0:
                strcpy(info, "You create an extremely strong localised field of spatial distortion!");
                mpr(info);
                strcpy(info, "Ouch!!!");
                mpr(info);
                ouch(15 + random2(15) + random2(15), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                strcpy(info, "Space warps crazily around you!");
                mpr(info);
                you_teleport2(true);
                ouch(9 + random2(9) + random2(9), 0, KILLED_BY_WILD_MAGIC);
                potion_effect(POT_CONFUSION, 30);
                break;
            case 2:
                strcpy(info, "You are cast into the Abyss!");
                mpr(info);
                more();
                banished(96);   // sends you to the abyss

                break;
            case 3:
                if (mutate(100) == 0)
                    mpr("Nothing appears to happen.");
                break;
            }
            break;
        }
        break;                  // end translocations

    case SPTYP_SUMMONING:
        switch (spec_effect)
        {
        case 0:         // harmless messages only

            switch (random2(10))
            {
            case 0:
                strcpy(info, "Shadowy shapes form in the air around you, then vanish.");
                break;
            case 1:
                strcpy(info, "You hear strange voices.");
                break;
            case 2:
                strcpy(info, "Your head hurts.");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "Your brain hurts!");
                break;
            case 5:
                strcpy(info, "Strange energies run through your body.");
                break;
            case 6:
                strcpy(info, "The world appears momentarily distorted.");
                break;
            case 7:
                strcpy(info, "Space warps around you.");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "Distant voices call out to you!");
                break;
            }
            mpr(info);
            break;

        case 1:         // a little bad

            switch (random2(3))
            {
            case 0:             // identical to translocation

                strcpy(info, "You create a localised field of spatial distortion.");
                mpr(info);
                strcpy(info, "Ouch!");
                mpr(info);
                ouch(5 + random2(5) + random2(5), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                strcpy(info, "Space twists in upon itself!");
                mpr(info);
                create_monster(MONS_SPATIAL_VORTEX, 24, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            case 2:
                strcpy(info, "Something appears in a flash of light!");
                mpr(info);
                create_monster(summon_any_demon(DEMON_LESSER), 24, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            }

        case 2:         // more bad

            switch (random2(3))
            {
            case 0:
                strcpy(info, "Space twists in upon itself!");
                mpr(info);
                for (loopj = 0; loopj < 2 + random2(3); loopj++)
                {
                    create_monster(MONS_SPATIAL_VORTEX, 22, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                }
                break;
            case 1:
                strcpy(info, "Something forms out of thin air!");
                mpr(info);
                create_monster(summon_any_demon(DEMON_COMMON), 24, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            case 2:
                strcpy(info, "A chorus of chattering voices calls out to you!");
                mpr(info);
                create_monster(summon_any_demon(DEMON_LESSER), 24, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                create_monster(summon_any_demon(DEMON_LESSER), 24, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                if (coinflip())
                    create_monster(summon_any_demon(DEMON_LESSER), 24, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                if (coinflip())
                    create_monster(summon_any_demon(DEMON_LESSER), 24, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            }
            break;

        case 3:         // more bad

            switch (random2(4))
            {
            case 0:
                strcpy(info, "Something forms out of thin air.");
                mpr(info);
                create_monster(MONS_ABOMINATION_SMALL, 0, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            case 1:
                strcpy(info, "You sense a hostile presence.");
                mpr(info);
                create_monster(summon_any_demon(DEMON_GREATER), 0, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            case 2:
                strcpy(info, "Something turns its malign attention towards you...");
                mpr(info);
                create_monster(summon_any_demon(DEMON_COMMON), 22, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                create_monster(summon_any_demon(DEMON_COMMON), 22, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                if (coinflip())
                    create_monster(summon_any_demon(DEMON_COMMON), 22, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            case 3:
                strcpy(info, "You are cast into the Abyss!");
                mpr(info);
                banished(96);   // sends you to the abyss

                break;
            }
            break;
// A powerful entity turns its attention onto you
        }                       // end Summonings

        break;

    case SPTYP_DIVINATION:
        switch (spec_effect)
        {
        case 0:         // just a harmless message

            switch (random2(10))
            {
            case 0:
                strcpy(info, "Weird images run through your mind.");
                break;
            case 1:
                strcpy(info, "You hear strange voices.");
                break;
            case 2:
                strcpy(info, "Your head hurts.");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "Your brain hurts!");
                break;
            case 5:
                strcpy(info, "Strange energies run through your body.");
                break;
            case 6:
                strcpy(info, "Everything looks hazy for a moment.");
                break;
            case 7:
                strcpy(info, "You seem to have forgotten something, but you can't remember what it was!");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You feel uncomfortable.");
                break;
            }
            mpr(info);
            break;

        case 1:         // more annoying things

            switch (random2(2))
            {
            case 0:
                strcpy(info, "You feel slightly disoriented.");
                mpr(info);
                forget_map(10 + random2(10));
                break;
            case 1:
                potion_effect(POT_CONFUSION, 1);
                break;
            }
            break;

        case 2:         // even more annoying things

            switch (random2(2))
            {
            case 0:
                if (you.is_undead)
                {
                    strcpy(info, "You suddenly recall your previous life!");
                    mpr(info);
                    break;
                }
                if (player_sust_abil() != 0 || you.intel <= 3)
                {
                    strcpy(info, "You have a terrible headache.");
                    mpr(info);
                    break;
                }
                strcpy(info, "You have damaged your brain!");
                mpr(info);
                you.intel -= random2(3) + 1;
                you.redraw_intelligence = 1;
                potion_effect(POT_CONFUSION, 1);
                break;
            case 1:
                strcpy(info, "You feel lost.");
                mpr(info);
                forget_map(40 + random2(40));
                potion_effect(POT_CONFUSION, 1);
                break;
            }
            break;

        case 3:         // nasty

            switch (random2(3))
            {
            case 0:
                if (forget_spell() == 1)
                {
                    strcpy(info, "You have forgotten a spell!");
                }
                else
                    strcpy(info, "You get a splitting headache.");
                mpr(info);
                break;
            case 1:
                strcpy(info, "You feel completely lost.");
                mpr(info);
                forget_map(100);
                potion_effect(POT_CONFUSION, 100);
                break;
            case 2:
                if (you.is_undead)
                {
                    strcpy(info, "You suddenly recall your previous life.");
                    mpr(info);
                    break;
                }
                if (player_sust_abil() != 0 || you.intel <= 3)
                {
                    strcpy(info, "You have a terrible headache.");
                    mpr(info);
                    break;
                }
                strcpy(info, "You have damaged your brain!");
                mpr(info);
                you.intel -= 3 + random2(3);
/*                if (you.intel <= 3)
   you.intel = 3;
   No, let's not be this kind.
 */
                you.redraw_intelligence = 1;
                potion_effect(POT_CONFUSION, 100);
                break;
            }
            break;
        }
        break;                  // end divinations

    case SPTYP_NECROMANCY:
        if (you.religion == GOD_KIKUBAAQUDGHA
            && (!player_under_penance()
                && you.piety >= 50 && random2(150) <= you.piety))
        {
            mpr("Nothing appears to happen.");
            break;
        }

        switch (spec_effect)
        {
        case 0:
            switch (random2(10))
            {
            case 0:
                strcpy(info, "You smell decay.");
                break;
            case 1:
                strcpy(info, "You hear strange and distant voices.");
                break;
            case 2:
                strcpy(info, "Pain shoots through your body.");
                break;
            case 3:
                strcpy(info, "Your bones ache.");
                break;
            case 4:
                strcpy(info, "The world around you seems to dim momentarily.");
                break;
            case 5:
                strcpy(info, "Strange energies run through your body.");
                break;
            case 6:
                strcpy(info, "You shiver with cold.");
                break;
            case 7:
                strcpy(info, "You sense a malignant aura.");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You feel very uncomfortable.");
                break;
            }
            mpr(info);
            break;

        case 1:         // a bit nasty

            switch (random2(3))
            {
            case 0:
                if (you.is_undead)
                {
                    strcpy(info, "You feel weird for a moment.");
                    mpr(info);
                    break;
                }
                strcpy(info, "Pain shoots through your body!");
                mpr(info);
                ouch(5 + random2(8) + random2(8), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                strcpy(info, "You feel horribly lethargic.");
                mpr(info);
                potion_effect(POT_SLOWING, 15);
                break;
            case 2:
                strcpy(info, "You smell decay.");       // identical to a harmless message

                mpr(info);
                you.rotting++;
                break;
            }
            break;

        case 2:         // much nastier

            switch (random2(3))
            {
            case 0:
                strcpy(info, "You are surrounded by flickering shadows.");
                mpr(info);
                create_monster(MONS_SHADOW, 21, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                if (coinflip())
                    create_monster(MONS_SHADOW, 21, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                if (coinflip())
                    create_monster(MONS_SHADOW, 21, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            case 1:
                if (player_prot_life() == 0 && one_chance_in(3))
                {
                    drain_exp();
                    break;
                }               // otherwise it just flows through...

            case 2:
                if (you.is_undead)
                {
                    strcpy(info, "You feel weird for a moment.");
                    mpr(info);
                    break;
                }
                strcpy(info, "You convulse helplessly as pain tears through your body!");
                mpr(info);
                ouch(10 + random2(12) + random2(12) + 5, 0, KILLED_BY_WILD_MAGIC);
                break;
            }
            break;

        case 3:         // even nastier

            switch (random2(6))
            {
            case 0:
                if (you.is_undead)
                {
                    strcpy(info, "Something just walked over your grave. No, really!");
                    mpr(info);
                    break;
                }
                strcpy(info, "Your body is wracked with pain!");
                mpr(info);
                loopj = (you.hp / 2) - 1;
                if (loopj <= 0)
                    loopj = 0;
                ouch(loopj, 0, KILLED_BY_MONSTER);      // can never die from this, right?

                you.redraw_hit_points = 1;
                break;
            case 1:
                strcpy(info, "You are engulfed in negative energy!");
                mpr(info);
                if (player_prot_life() == 0)
                {
                    drain_exp();
                    break;
                }               // otherwise it just flows through...

            case 2:
                lose_stat(100, 1 + random2(4) + random2(4));
                break;
            case 3:
                if (you.is_undead)
                {
                    strcpy(info, "You feel terrible.");
                    mpr(info);
                    break;
                }
                strcpy(info, "You feel your flesh start to rot away!");
                mpr(info);
                you.rotting += random2(4) + 1 + random2(4);
                break;
            case 4:
                strcpy(info, "Something reaches out for you...");
                mpr(info);
                create_monster(MONS_SOUL_EATER, 23, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            case 5:
                strcpy(info, "Death has come for you...");
                mpr(info);
                create_monster(MONS_REAPER, 23, BEH_CHASING_I, you.x_pos, you.y_pos, MHITNOT, 250);
                break;
            }
            break;
        }
        break;                  // end necromancy

    case SPTYP_TRANSMIGRATION:
        switch (spec_effect)
        {
        case 0:         // just a harmless message

            switch (random2(10))
            {
            case 0:
                strcpy(info, "Your hands glow momentarily.");
                break;
            case 1:
                strcpy(info, "The air around you crackles with energy!");
                break;
            case 2:
                strcpy(info, "Multicolored lights dance before your eyes!");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "Waves of light ripple over your body.");
                break;
            case 5:
                strcpy(info, "Strange energies run through your body.");
                break;
            case 6:
                strcpy(info, "Your skin tingles.");
                break;
            case 7:
                strcpy(info, "Your skin glows momentarily.");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You smell something strange.");
                break;
            }
            mpr(info);
            break;

        case 1:         // slightly annoying

            switch (random2(2))
            {
            case 0:
                mpr("Your body is twisted painfully.");
                ouch(1 + random2(6) + random2(6), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                random_uselessness(2 + random2(7), 0);
                break;
            }
            break;

        case 2:         // much more annoying

            switch (random2(4))
            {
            case 0:
                mpr("Your body is twisted very painfully!");
                ouch(3 + random2(12) + random2(12), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                mpr("Strange energies tear through your body!");
                mutate(100);
                break;
            case 2:
                potion_effect(POT_PARALYSIS, 10);
                break;
            case 3:
                potion_effect(POT_CONFUSION, 10);
                break;
            }
            break;

        case 3:         // even nastier

            switch (random2(3))
            {
            case 0:
                mpr("Your body is distorted in a weird and horrible way!");
                for (int i = 0; i < 4; i++)     //jmf: changed to loop

                    mutate(100);
                //mutate(100);
                //mutate(100);
                //mutate(100);
                ouch(7 + random2(12) + random2(12), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                mpr("You feel very strange.");
                delete_mutation(100);
                ouch(5 + random2(12) + random2(12), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 2:
                mpr("Your body is distorted in a weirdly horrible way!");
                for (int i = 0; i < 4; i++)     //jmf: changed to loop

                    if (give_bad_mutation() != 0)
                        break;
                //if (give_bad_mutation() == 0)
                //    if (give_bad_mutation() == 0)
                //        if (give_bad_mutation() == 0)
                //            give_bad_mutation();
                ouch(5 + random2(12) + random2(12), 0, KILLED_BY_WILD_MAGIC);
                break;
            }
            break;
        }
        break;                  // end transmigrations

    case SPTYP_FIRE:
        switch (spec_effect)
        {
        case 0:         // just a harmless message

            switch (random2(10))
            {
            case 0:
                strcpy(info, "Sparks fly from your hands!");
                break;
            case 1:
                strcpy(info, "The air around you burns with energy!");
                break;
            case 2:
                strcpy(info, "Wisps of smoke drift from your fingertips.");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "You smell smoke.");
                break;
            case 5:
                strcpy(info, "Heat runs through your body.");
                break;
            case 6:
                strcpy(info, "You feel uncomfortably hot.");
                break;
            case 7:
                strcpy(info, "Lukewarm flames ripple over your body.");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You hear a sizzling sound.");
                break;
            }
            mpr(info);
            break;

        case 1:         // a bit less harmless stuff

            switch (random2(2))
            {
            case 0:
                strcpy(info, "Smoke pours from your fingertips!");
                mpr(info);
                big_cloud(CLOUD_GREY_SMOKE + random2(3), you.x_pos, you.y_pos, 20);
                break;
            case 1:
                strcpy(info, "Flames sear your flesh.");
                mpr(info);
                scrolls_burn(3, OBJ_SCROLLS);
                if (player_res_fire() < 100)
                    ouch(2 + random2(7) + random2(7), 0, KILLED_BY_WILD_MAGIC);
                break;
            }
            break;

        case 2:         // rather less harmless stuff

            switch (random2(2))
            {
            case 0:
                strcpy(info, "You are blasted with fire.");
                mpr(info);
                ouch(check_your_resists(5 + random2(15) + random2(15), 2), 0, KILLED_BY_WILD_MAGIC);
                scrolls_burn(5, OBJ_SCROLLS);
                break;
            case 1:
                strcpy(info, "You conjure up a fiery explosion!");
                mpr(info);
                strcpy(info, "Oops.");
                mpr(info);
                beam[0].type = 43;
                beam[0].damage = 114;
                beam[0].flavour = BEAM_FIRE;
                beam[0].bx = you.x_pos;
                beam[0].by = you.y_pos;
                strcpy(beam[0].beam_name, "explosion");
                beam[0].colour = RED;
                beam[0].thing_thrown = 1;       // your explosion (is this right?)

                explosion(0, beam);
                noisy(10, you.x_pos, you.y_pos);
                break;
            }
            break;

        case 3:         // considerably less harmless stuff

            switch (random2(3))
            {
            case 0:
                mpr("You are blasted with searing flames!");
                ouch(check_your_resists(9 + random2(17) + random2(17), 2), 0, KILLED_BY_WILD_MAGIC);
                scrolls_burn(10, OBJ_SCROLLS);
                break;
            case 1:
                strcpy(info, "There is a sudden and violent explosion of flames!");
                mpr(info);
                beam[0].type = 43;
                beam[0].damage = 120;
                beam[0].flavour = BEAM_FIRE;
                beam[0].bx = you.x_pos;
                beam[0].by = you.y_pos;
                strcpy(beam[0].beam_name, "fireball");
                beam[0].colour = RED;
                beam[0].thing_thrown = 1;       // your explosion (is this right?)

                explosion(random2(2), beam);
                noisy(20, you.x_pos, you.y_pos);
                break;
            case 2:
                strcpy(info, "You are covered in liquid fire!");
                mpr(info);
                you.duration[DUR_LIQUID_FLAMES] += 1 + random2(3) + random2(3) + random2(3);
                break;
            }
            break;
        }
        break;                  // end fire

    case SPTYP_ICE:
        switch (spec_effect)
        {
        case 0:         // just a harmless message

            switch (random2(10))
            {
            case 0:
                strcpy(info, "You shiver with cold.");
                break;
            case 1:
                strcpy(info, "A chill runs through your body.");
                break;
            case 2:
                strcpy(info, "Wisps of condensation drift from your fingertips.");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "Your hands feel numb with cold.");
                break;
            case 5:
                strcpy(info, "A chill runs through your body.");
                break;
            case 6:
                strcpy(info, "You feel uncomfortably cold.");
                break;
            case 7:
                strcpy(info, "Frost covers your body.");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You hear a crackling sound.");
                break;
            }
            mpr(info);
            break;

        case 1:         // a bit less harmless stuff

            switch (random2(2))
            {
            case 0:
                strcpy(info, "You feel extremely cold.");
                mpr(info);
                break;
            case 1:
                strcpy(info, "You are covered in a thin layer of ice");
                mpr(info);
                scrolls_burn(2, OBJ_POTIONS);
                if (player_res_cold() < 100)
                    ouch(4 + random2(3) + random2(3), 0, KILLED_BY_WILD_MAGIC);
                break;
            }
            break;

        case 2:         // rather less harmless stuff

            switch (random2(2))
            {
            case 0:
                mpr("Heat is drained from your body.");
                ouch(check_your_resists(5 + random2(6) + random2(7), 3), 0, KILLED_BY_WILD_MAGIC);
                scrolls_burn(4, OBJ_POTIONS);
                break;
            case 1:
                strcpy(info, "You conjure up an explosion of ice and frost!");
                mpr(info);
                strcpy(info, "Oops.");
                mpr(info);
                beam[0].type = 43;
                beam[0].damage = 111;
                beam[0].flavour = BEAM_COLD;
                beam[0].bx = you.x_pos;
                beam[0].by = you.y_pos;
                strcpy(beam[0].beam_name, "explosion");
                beam[0].colour = WHITE;
                beam[0].thing_thrown = 1;       // your explosion (is this right?)

                explosion(0, beam);
                noisy(10, you.x_pos, you.y_pos);
                break;
            }
            break;

        case 3:         // less harmless stuff

            switch (random2(2))
            {
            case 0:
                mpr("You are blasted with ice!");
                ouch(check_your_resists(9 + random2(12) + random2(12), 3), 0, KILLED_BY_WILD_MAGIC);
                scrolls_burn(9, OBJ_POTIONS);
                break;
            case 1:
                strcpy(info, "Freezing gasses pour from your hands!");
                mpr(info);
                big_cloud(CLOUD_COLD, you.x_pos, you.y_pos, 20);
                break;
            }
            break;
        }
        break;                  // end ice

    case SPTYP_EARTH:
        switch (spec_effect)
        {
        case 0:         // just a harmless message

        case 1:
            switch (random2(10))
            {
            case 0:
                strcpy(info, "You feel earthy.");
                break;
            case 1:
                strcpy(info, "You are showered with tiny particles of grit.");
                break;
            case 2:
                strcpy(info, "Sand pours from your fingers.");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "You sympathise with the stones.");
                break;
            case 5:
                strcpy(info, "You feel gritty.");
                break;
            case 6:
                strcpy(info, "You feel like a piece of rock.");
                break;
            case 7:
                strcpy(info, "You feel like a paving stone.");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You feel chalky.");
                break;
            }
            mpr(info);
            break;

        case 2:         // slightly less harmless stuff

            switch (random2(1))
            {
            case 0:
                switch (random2(3))
                {
                case 0:
                    strcpy(info, "You are hit by flying rocks!");
                    break;
                case 1:
                    strcpy(info, "You are blasted with sand!");
                    break;
                case 2:
                    strcpy(info, "Rocks fall onto you out of nowhere!");
                    break;
                }
                mpr(info);
                hurted = 10 + random2(7) + random2(7);
                if (player_AC() > 0)
                    hurted -= random2(player_AC());
                ouch(hurted, 0, KILLED_BY_WILD_MAGIC);
                break;
            }
            break;

        case 3:         // less harmless stuff

            switch (random2(1))
            {
            case 0:
                strcpy(info, "You conjure up an explosion of flying shrapnel!");
                mpr(info);
                strcpy(info, "Oops.");
                mpr(info);
                beam[0].type = 43;
                beam[0].damage = 115;
                beam[0].flavour = BEAM_FRAG;
                beam[0].bx = you.x_pos;
                beam[0].by = you.y_pos;
                strcpy(beam[0].beam_name, "explosion");
                beam[0].colour = CYAN;
                if (one_chance_in(5))
                    beam[0].colour = BROWN;
                if (one_chance_in(5))
                    beam[0].colour = LIGHTCYAN;
                beam[0].thing_thrown = 1;       // your explosion (is this right?)

                explosion(0, beam);
                noisy(10, you.x_pos, you.y_pos);
                break;
            }
            break;
        }
        break;                  // end Earth

    case SPTYP_AIR:
        switch (spec_effect)
        {
        case 0:         // just a harmless message

            switch (random2(10))
            {
            case 0:
                strcpy(info, "Ouch! You gave yourself an electric shock.");
                break;
            case 1:
                strcpy(info, "You feel momentarily weightless.");
                break;
            case 2:
                strcpy(info, "Wisps of vapour drift from your fingertips.");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "You feel electric!");
                break;
            case 5:
                strcpy(info, "Sparks of electricity dance on your fingertips.");
                break;
            case 6:
                strcpy(info, "You are blasted with air!");
                break;
            case 7:
                strcpy(info, "You hear a whooshing sound.");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You hear a crackling sound.");
                break;
            }
            mpr(info);
            break;

        case 1:         // a bit less harmless stuff

            switch (random2(2))
            {
            case 0:
                strcpy(info, "There is a shower of sparks.");
                mpr(info);
                break;
            case 1:
                strcpy(info, "The wind howls around you!");
                mpr(info);
                break;
            }
            break;

        case 2:         // rather less harmless stuff

            switch (random2(2))
            {
            case 0:
                mpr("Electricity courses through your body.");
                ouch(check_your_resists(4 + random2(5) + random2(5), 5), 0, KILLED_BY_WILD_MAGIC);
                break;
            case 1:
                strcpy(info, "Noxious gasses pour from your hands!");
                mpr(info);
                big_cloud(CLOUD_STINK, you.x_pos, you.y_pos, 20);
                break;
            }
            break;

        case 3:         // less harmless stuff

            switch (random2(2))
            {
            case 0:
                strcpy(info, "You conjure up an explosion of electrical discharges!");
                mpr(info);
                strcpy(info, "Oops.");
                mpr(info);
                beam[0].type = 43;
                beam[0].damage = 108;
                beam[0].flavour = BEAM_ELECTRICITY;
                beam[0].bx = you.x_pos;
                beam[0].by = you.y_pos;
                strcpy(beam[0].beam_name, "explosion");
                beam[0].colour = LIGHTBLUE;
                beam[0].thing_thrown = 1;       // your explosion (is this right?)

                explosion(0 + (!one_chance_in(4)), beam);
                noisy(10, you.x_pos, you.y_pos);
                break;
            case 1:
                strcpy(info, "Venomous gasses pour from your hands!");
                mpr(info);
                big_cloud(CLOUD_POISON, you.x_pos, you.y_pos, 20);
                break;
            }
            break;
        }
        break;                  // end air

    case SPTYP_POISON:
        switch (spec_effect)
        {
        case 0:         // just a harmless message

            switch (random2(10))
            {
            case 0:
                strcpy(info, "You feel mildly nauseous.");
                break;
            case 1:
                strcpy(info, "You feel slightly ill.");
                break;
            case 2:
                strcpy(info, "Wisps of poison gas drift from your fingertips.");
                break;
            case 3:
                strcpy(info, "You feel a strange surge of energy!");
                break;
            case 4:
                strcpy(info, "You feel faint for a moment.");
                break;
            case 5:
                strcpy(info, "You feel sick.");
                break;
            case 6:
                strcpy(info, "You feel odd.");
                break;
            case 7:
                strcpy(info, "You feel weak for a moment.");
                break;
            case 8:
                strcpy(info, "Nothing appears to happen.");
                break;
            case 9:
                strcpy(info, "You hear a slurping sound.");
                break;
            }
            mpr(info);
            break;

        case 1:         // a bit less harmless stuff

            switch (random2(2))
            {
            case 0:
                if (player_res_poison() != 0)
                    goto nothing_happening;
                strcpy(info, "You feel sick.");
                you.poison += 2 + random2(3);
                break;
            case 1:
                strcpy(info, "Noxious gasses pour from your hands!");
                mpr(info);
                place_cloud(CLOUD_STINK, you.x_pos, you.y_pos, 2 + random2(4));
                break;
            }
            break;

        case 2:         // rather less harmless stuff

            switch (random2(3))
            {
            case 0:
                if (player_res_poison() != 0)
                    goto nothing_happening;
                strcpy(info, "You feel very sick.");
                you.poison += 3 + random2(5) + random2(5);
                break;
            case 1:
                strcpy(info, "Noxious gasses pour from your hands!");
                mpr(info);
                big_cloud(CLOUD_STINK, you.x_pos, you.y_pos, 20);
                break;
            case 2:
                if (player_res_poison() != 0)
                    goto nothing_happening;
                lose_stat(100, 1);
                break;
            }
            break;

        case 3:         // less harmless stuff

            switch (random2(3))
            {
            case 0:
                if (player_res_poison() != 0)
                    goto nothing_happening;
                strcpy(info, "You feel incredibly sick.");
                you.poison += 10 + random2(10) + random2(10);
                break;
            case 1:
                strcpy(info, "Venomous gasses pour from your hands!");
                mpr(info);
                big_cloud(CLOUD_POISON, you.x_pos, you.y_pos, 20);
                break;
            case 2:
                if (player_res_poison() != 0)
                    goto nothing_happening;
                lose_stat(100, 1 + random2(3) + random2(3));
                break;
            }
            break;
        }
        break;                  // end poison

/*
   11 = conjuration
   12 = enchantment
   13 = fire
   14 = ice
   15 = transmigration
   16 = necromancy
   17 = holy
   18 = summoning
   19 = divination
   20 = translocation
   21 = poison
   22 = earth
   23 = air
 */
//}
        //}
    }
//break;

//}

    return 1;

  nothing_happening:
    mpr("Nothing appears to happen.");

    return 0;
}

bool learn_a_spell(unsigned char splbook, char bitty)
{
    int spells [SPELLBOOK_SIZE];

    spellbook_template( you.inv_type[splbook], spells );

    if (spells [bitty] != SPELL_NO_SPELL)
    {
        learned = bitty + 1;
        return true;
    }
    else
    {
        return false;
    }
}

bool which_spellbook()
{
    unsigned char nthing = 0;

    if (player_spell_levels() < 1)
    {
        mpr("You can't memorise any more spells yet.");
        return false;
    }

    if (you.num_inv_items == 0)
    {
        mpr("You aren't carrying anything.");
        return false;
    }

  query:
    strcpy(info, "You can memorise ");
    itoa(player_spell_levels(), st_prn, 10);
    strcat(info, st_prn);
    strcat(info, " more level");
    if (!(st_prn[0] == '1' && st_prn[1] == 0))
        strcat(info, "s");
    strcat(info, " of spells");
    strcat(info, ".");

    mpr(info);
    mpr("Memorise from which spellbook?");

    keyin = get_ch();
    if (keyin == '?' || keyin == '*')
    {
        if (keyin == '?')
            nthing = get_invent(OBJ_BOOKS);

        if (keyin == '*')
            nthing = get_invent(-1);

        if ((nthing >= 65 && nthing <= 90) || (nthing >= 97 && nthing <= 122))
        {
            keyin = nthing;

        }
        else
        {
            mesclr();
            goto query;
        }
    }

    sc_read_1 = (int) keyin;

    if (sc_read_1 < 65 || (sc_read_1 > 90 && sc_read_1 < 97) || sc_read_1 > 122)
    {
        mpr("You don't have any such object.");
        return false;
    }

    sc_read_2 = conv_lett(sc_read_1);

    if (you.inv_quantity[sc_read_2] == 0)
    {
        mpr("You haven't any such object.");
        return false;
    }

    if (you.inv_class[sc_read_2] != OBJ_BOOKS
        || you.inv_type[sc_read_2] == BOOK_MANUAL)
    {
        mpr("That isn't a spellbook!");
        return false;
    }

    if (you.inv_type[sc_read_2] == BOOK_DESTRUCTION)
    {
        tome_of_power(sc_read_2);
        return false;
    }

    spell_container = sc_read_2;

    read_book(spell_container);

    clrscr();
    return true;
}




void read_book(char book_read)
{
    char key2 = 0;

    /* remember that this function is called from staff spells as well. */
    if (you.inv_class[book_read] == OBJ_STAVES)
        key2 = spellbook_contents(you.inv_plus[book_read], you.inv_type[book_read] + 40);
    else
        key2 = spellbook_contents(you.inv_plus[book_read], you.inv_type[book_read]);

    if (you.inv_class[book_read] == OBJ_BOOKS)
    {
        you.had_item[you.inv_type[book_read]] = 1;

        if (you.inv_type[book_read] == BOOK_MINOR_MAGIC_I
            || you.inv_type[book_read] == BOOK_MINOR_MAGIC_II
            || you.inv_type[book_read] == BOOK_MINOR_MAGIC_III)
        {
            you.had_item[BOOK_MINOR_MAGIC_I] = 1;
            you.had_item[BOOK_MINOR_MAGIC_II] = 1;
            you.had_item[BOOK_MINOR_MAGIC_III] = 1;
        }

        else if (you.inv_type[book_read] == BOOK_CONJURATIONS_I
                 || you.inv_type[book_read] == BOOK_CONJURATIONS_II)
        {
            you.had_item[BOOK_CONJURATIONS_I] = 1;
            you.had_item[BOOK_CONJURATIONS_II] = 1;
        }
    }

#ifdef PLAIN_TERM
    redraw_screen();
#endif
    /* Put special book effects in another function which can be called from
       memorise as well */
    you.turn_is_over = 1;
    you.inv_ident[book_read] = 1;
    book_thing = key2;
    keyin = key2;               // FIX this should probably go...  // I agree {dlb}

}




void which_spell()
{
    int chance = 0;
    int powm;
    char spell_string[60];
    int levels_needed = 0;

    int i;
    int j = 0;

    for (i = SK_SPELLCASTING; i <= SK_POISON_MAGIC; i++)
    {
        if (you.skills[i] != 0)
            j++;
    }

    if (j == 0)
    {
        mpr("You can't use spell magic! I'm afraid it's scrolls only for now.");
        return;
    }

    if (!which_spellbook())
        return;

    sc_read_1 = (int) book_thing;

    if (sc_read_1 < 65 || (sc_read_1 > 90 && sc_read_1 < 97) || sc_read_1 > 122)
    {
      whatt:
#ifdef PLAIN_TERM
        redraw_screen();
#endif
        mpr("What?");
        return;
    }

    sc_read_2 = conv_lett(sc_read_1);

    if (sc_read_2 > SPELLBOOK_SIZE)
        goto whatt;

    if (!learn_a_spell(spell_container, sc_read_2))
        goto whatt;

    unsigned char specspell = which_spell_in_book(you.inv_type[spell_container], learned);

    if (specspell == SPELL_NO_SPELL)
        goto whatt;

    if (you.spell_no == 21 && specspell != SPELL_SELECTIVE_AMNESIA)
    {                           // if changed, must also change for priest in level_change. You can always memorise selective amnesia

        mpr("Your head is already too full of spells!");
        return;
    }

    if (you.species == SP_MUMMY && spell_type(specspell, SPTYP_HOLY))
    {
        mpr("You can't use this type of magic!");
        return;
    }

    if ((you.is_undead == US_HUNGRY_DEAD && undead_can_memorise(specspell) == 2) || (you.is_undead == US_UNDEAD && undead_can_memorise(specspell) != 0))
    {
        mpr("You can't use this spell.");
        return;
    }

    for (i = 0; i < 25; i++)
    {
        if (you.spells[i] == specspell)
        {
#ifdef PLAIN_TERM
            redraw_screen();
#endif
            mpr("You already know that spell!");
            you.turn_is_over = 1;
            return;
        }
    }

    levels_needed = spell_value(specspell);

    if (player_spell_levels() < levels_needed)
    {
#ifdef PLAIN_TERM
        redraw_screen();
#endif
        mpr("You can't memorise that many levels of magic yet!");
        //sprintf( info, "levels: %d  needed: %d  spec_spells: %d", player_spell_levels(), levels_needed, specspell);
        //mpr(info);
        you.turn_is_over = 1;
        return;
    }

    if (you.experience_level < spell_value(specspell))
    {
#ifdef PLAIN_TERM
        redraw_screen();
#endif
        mpr("You're too inexperienced to learn that spell!");
        you.turn_is_over = 1;
        return;
    }

    chance = 0;

    powm = spell_spec(specspell, 0);

    chance = spell_fail(specspell);

#ifdef PLAIN_TERM
    redraw_screen();
#endif

    if (chance >= 80)
        mpr("This spell is very difficult to memorise.");
    else if (chance >= 60)
        mpr("This spell is quite difficult to commit to memory.");
    else if (chance >= 45)
        mpr("This spell is rather tricky to learn.");
    else if (chance >= 30)
        mpr("This spell is a little tricky to absorb.");

    strcpy(info, "Memorise ");
    spell_name(specspell, spell_string);
    strcat(info, spell_string);
    strcat(info, "?");
    mpr(info);

    for (;;)
    {
        keyin = getch();

        if (keyin == 'n' || keyin == 'N')
        {
#ifdef PLAIN_TERM
            redraw_screen();
#endif
            return;
        }

        if (keyin == 'y' || keyin == 'Y')
            break;
    }

    mesclr();

    if (you.mutation[MUT_BLURRY_VISION] > 0 && random2(4) < you.mutation[MUT_BLURRY_VISION])
    {
        mpr("The writing blurs into unreadable gibberish.");
        you.turn_is_over = 1;
        return;
    }

    if (random2(40) + random2(40) + random2(40) < chance)
    {
#ifdef PLAIN_TERM
        redraw_screen();
#endif
        mpr("You fail to memorise the spell.");
        you.turn_is_over = 1;

        if (you.inv_type[spell_container] == BOOK_NECRONOMICON)
        {
            mpr("The pages of the Necronomicon glow with a malevolent light...");
            miscast_effect(SPTYP_NECROMANCY, 8, random2(30) + random2(30) + random2(30), 100);
        }

        if (you.inv_type[spell_container] == BOOK_DEMONOLOGY)
        {
            mpr("This book does not appreciate being disturbed by one of your ineptitude!");
            miscast_effect(SPTYP_SUMMONING, 7, random2(30) + random2(30) + random2(30), 100);
        }

        if (you.inv_type[spell_container] == BOOK_ANNIHILATIONS)
        {
            mpr("This book does not appreciate being disturbed by one of your ineptitude!");
            miscast_effect(SPTYP_CONJURATION, 8, random2(30) + random2(30) + random2(30), 100);
        }

#ifndef WIZARD
        return;
#endif

#ifdef WIZARD
        mpr("But I'll let you memorise it anyway, okay?");
#endif
    }

    for (i = 0; i < 25; i++)
    {
        if (you.spells[i] == SPELL_NO_SPELL)
            break;
    }

    you.spells[i] = specspell;

    // you.spell_levels -= levels_needed;
    you.spell_no++;

    you.delay_t = spell_value(you.spells[i]);
    you.delay_doing = 3;

    you.turn_is_over = 1;
#ifdef PLAIN_TERM
    redraw_screen();
#endif

    naughty(NAUGHTY_SPELLCASTING, 2 + random2(5));
}

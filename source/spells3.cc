/*
 *  File:       spells3.cc
 *  Summary:    Implementations of some additional spells.
 *  Written by: Linley Henzell
 *
 *  Change History (most recent first):
 *
 *      <2>     9/11/99        LRH    Teleportation takes longer in the Abyss
 *      <2>     8/05/99        BWR    Added allow_control_teleport
 *      <1>     -/--/--        LRH    Created
 */

#include "AppHdr.h"
#include "spells3.h"

#include <string.h>

#include "externs.h"

#include "abyss.h"
#include "direct.h"
#include "fight.h"
#include "it_use2.h"
#include "itemname.h"
#include "misc.h"
#include "monplace.h"
#include "mon-pick.h"
#include "monstuff.h"
#include "mon-util.h"
#include "player.h"
#include "spells0.h"
#include "spells1.h"
#include "spl-util.h"
#include "stuff.h"
#include "view.h"
#include "wpn-misc.h"

extern bool wield_change;       // defined in output.cc
static bool monster_on_level(int monster);

void cast_selective_amnesia(bool force)
{
    char ep_gain = 0;
    char index_value = 0;
    unsigned char keyin = 0;

    if (you.spell_no == 0)
        mpr("You don't know any spells.");      // re: sif muna {dlb}
    else
    {
        // query - conditional ordering is important {dlb}:
        for (;;)
        {
            mpr( "Forget which spell? ([a-y] spell [?|*] list [spc|tab|ret] "
                 "exit) ", MSGCH_PROMPT );

            keyin = (unsigned char) get_ch();

            if (keyin == '\t' || keyin == '\r' || keyin == ' ')
                return;         // early return {dlb}

            if (keyin == '?' || keyin == '*')
            {
                // this reassignment is "key" {dlb}
                keyin = (unsigned char) spell_list();

                redraw_screen();
            }

            if (keyin < 'a' || keyin > 'y')
                mesclr();
            else
                break;
        }

        // actual handling begins here {dlb}:
        index_value = letter_to_index(keyin);

        if (index_value >= 25 || you.spells[index_value] == SPELL_NO_SPELL)
            mpr("You don't know that spell.");
        else
        {
            if (!force
                 && (you.religion != GOD_SIF_MUNA
                     && random2(you.skills[SK_SPELLCASTING])
                         < random2(spell_difficulty(you.spells[index_value]))))
            {
                mpr("Oops! This spell sure is a blunt instrument.");
                forget_map(20 + random2(50));
            }
            else
            {
                ep_gain = spell_mana(you.spells[index_value]);

                you.spell_no--;
                you.spells[index_value] = SPELL_NO_SPELL;

                if (ep_gain > 0)
                {
                    inc_mp(ep_gain, false);
                    mpr( "The spell releases its latent energy back to you as "
                         "it unravels." );
                }
            }
        }
    }

    return;
}                               // end cast_selective_amnesia()

bool remove_curse(bool suppress_msg)
{
    int loopy = 0;              // general purpose loop variable {dlb}
    bool success = false;       // whether or not curse(s) removed {dlb}

    // special "wield slot" case - see if you can figure out why {dlb}:
    if (you.equip[EQ_WEAPON] != -1
                && you.inv_class[you.equip[EQ_WEAPON]] == OBJ_WEAPONS)
    {
        if (you.inv_plus[you.equip[EQ_WEAPON]] > 130)
        {
            you.inv_plus[you.equip[EQ_WEAPON]] -= 100;
            success = true;
            wield_change = true;
        }
    }

    // everything else uses the same paradigm - are we certain?
    // what of artefact rings and amulets? {dlb}:
    for (loopy = EQ_CLOAK; loopy < NUM_EQUIP; loopy++)
    {
        if (you.equip[loopy] != -1
                    && you.inv_plus[you.equip[loopy]] > 130)
        {
            you.inv_plus[you.equip[loopy]] -= 100;
            success = true;
        }
    }

    // messaging output {dlb}:
    if (!suppress_msg)
    {
        if (success)
            mpr("You feel as if something is helping you.");
        else
            canned_msg(MSG_NOTHING_HAPPENS);
    }

    return (success);
}                               // end remove_curse()

bool detect_curse(bool suppress_msg)
{
    int loopy = 0;              // general purpose loop variable {dlb}
    bool success = false;       // whether or not any curses found {dlb}

    for (loopy = 0; loopy < ENDOFPACK; loopy++)
    {
        if (you.inv_quantity[loopy]
            && (you.inv_class[loopy] == OBJ_WEAPONS
                || you.inv_class[loopy] == OBJ_ARMOUR
                || you.inv_class[loopy] == OBJ_JEWELLERY)
            && you.inv_ident[loopy] == 0)
        {
            // well, this is not quite right, as it triggers a "detect"
            you.inv_ident[loopy] = 1;
            success = true;   // response for uncursed rings, too {dlb}
        }
    }

    // messaging output {dlb}:
    if (!suppress_msg)
    {
        if (success)
            mpr("You sense the presence of curses on your possessions.");
        else
            canned_msg(MSG_NOTHING_HAPPENS);
    }

    return (success);
}                               // end detect_curse()

bool cast_smiting(int power)
{
    bool success = false;
    struct dist beam;
    struct monsters *monster = 0;       // NULL {dlb}

    if (power > 150)
        power = 150;

    mpr("Smite whom?", MSGCH_PROMPT);

    direction(beam, DIR_TARGET);

    if (!beam.isValid
        || mgrd[beam.tx][beam.ty] == NON_MONSTER
        || beam.isMe)
    {
        canned_msg(MSG_SPELL_FIZZLES);
    }
    else
    {
        monster = &menv[mgrd[beam.tx][beam.ty]];

        strcpy(info, "You smite ");
        strcat(info, ptr_monam( monster, 1 ));
        strcat(info, "!");
        mpr(info);

        hurt_monster(monster, random2(8) + (random2(power) / 3));

        if (monster->hit_points < 1)
            monster_die(monster, KILL_YOU, 0);
        else
            print_wounds(monster);

        success = true;
    }

    return (success);
}                               // end cast_smiting()

bool airstrike(int power)
{
    bool success = false;
    struct dist beam;
    struct monsters *monster = 0;       // NULL {dlb}
    int hurted = 0;

    if (power > 150)
        power = 150;

    mpr("Strike whom?", MSGCH_PROMPT);

    direction(beam, DIR_TARGET);

    if (!beam.isValid
        || mgrd[beam.tx][beam.ty] == NON_MONSTER
        || beam.isMe)
    {
        canned_msg(MSG_SPELL_FIZZLES);
    }
    else
    {
        monster = &menv[mgrd[beam.tx][beam.ty]];

        strcpy(info, "The air twists around and strikes ");
        strcat(info, ptr_monam( monster, 1 ));
        strcat(info, "!");
        mpr(info);

        hurted = random2( random2(12) + (random2(power) / 6)
                                      + (random2(power) / 7) );
        hurted -= random2(1 + monster->armor_class);

        if (hurted < 0)
            hurted = 0;
        else
        {
            hurt_monster(monster, hurted);

            if (monster->hit_points < 1)
                monster_die(monster, KILL_YOU, 0);
            else
                print_wounds(monster);
        }

        success = true;
    }

    return (success);
}                               // end airstrike()

bool cast_bone_shards(int power)
{
    bool success = false;
    struct bolt beam;
    struct dist spelld;

    // cap the incoming power to 150 (for a third level spell)
    if (power > 150)
        power = 150;

    if (you.equip[EQ_WEAPON] == -1
                    || you.inv_class[you.equip[EQ_WEAPON]] != OBJ_CORPSES)
    {
        canned_msg(MSG_SPELL_FIZZLES);
    }
    else if (you.inv_type[you.equip[EQ_WEAPON]] != CORPSE_SKELETON)
        mpr("The corpse collapses into a mass of pulpy flesh.");
    else if (spell_direction(spelld, beam) != -1)
    {
        // max of 150 * 15 + 3000 = 5250
        power *= 15;
        power += mons_weight(you.inv_plus[you.equip[EQ_WEAPON]]);

        mpr("The skeleton explodes into sharp fragments of bone!");

        unwield_item(you.equip[EQ_WEAPON]);
        you.inv_quantity[you.equip[EQ_WEAPON]]--;

        /* can this be false? */
        if (you.inv_quantity[you.equip[EQ_WEAPON]] == 0)
        {
            you.equip[EQ_WEAPON] = -1;
            mpr("You are now empty-handed.");
        }

        zapping(ZAP_BONE_SHARDS, power, beam);
        burden_change();

        success = true;
    }

    return (success);
}                               // end cast_bone_shards()

void sublimation(int power)
{
    unsigned char was_wielded = 0;
    unsigned char loopy = 0;    // general purpose loop variable {dlb}

    if (you.equip[EQ_WEAPON] == -1
        || you.inv_class[you.equip[EQ_WEAPON]] != OBJ_FOOD
        || you.inv_type[you.equip[EQ_WEAPON]] != FOOD_CHUNK)
    {
        if (you.deaths_door)
        {
            mpr( "A conflicting enchantment prevents the spell from "
                 "coming into effect." );
        }
        else if (!enough_hp(2, true))
             mpr("Your attempt to draw power from your own body fails.");
        else
        {
            mpr("You draw magical energy from your own body!");

            while (you.magic_points < you.max_magic_points && you.hp > 1)
            {
                inc_mp(1, false);
                dec_hp(1, false);

                for (loopy = 0; loopy < (you.hp > 1 ? 3 : 0); loopy++)
                {
                    if (random2(power) < 6)
                        dec_hp(1, false);
                }

                if (random2(power) < 6)
                    break;
            }
        }
    }
    else
    {
        mpr("The chunk of flesh you are holding crumbles to dust.");
        mpr("A flood of magical energy pours into your mind!");

        inc_mp(7 + random2(7), false);

        was_wielded = you.equip[EQ_WEAPON];
        unwield_item(you.equip[EQ_WEAPON]);
        you.equip[EQ_WEAPON] = -1;

        you.inv_quantity[was_wielded]--;

        if (you.inv_quantity[was_wielded] < 1)
            you.inv_quantity[was_wielded] = 0;

        burden_change();

    }

    return;
}                               // end sublimation()

// Simulacrum
//
// This spell extends creating undead to Ice mages, as such it's high
// level, doesn't produce permanent undead, requires wielding of the
// material component, and the undead aren't overly powerful (they're
// also vulnerable to fire).
//
// As for what it offers necromancers considering all the downsides
// above... it allows the turning of a single corpse into an army of
// monsters (one per food chunk)... which is also a good reason for
// why it's high level and non-permanent (slow down creation, keep
// army size in check).
//
// Hides and other "animal part" items are intentionally left out, it's
// unrequired complexity, and fresh flesh makes more sense for a spell
// reforming the original monster out of ice anyways.
void simulacrum(int power)
{
    int max_num = 1 + random2(power) / 20;
    if (max_num > 8)
        max_num = 8;

    const int chunk = you.equip[EQ_WEAPON];

    if (chunk != -1
        && you.inv_quantity[ chunk ] > 0
        && (you.inv_class[ chunk ] == OBJ_CORPSES
            || (you.inv_class[ chunk ] == OBJ_FOOD
                && you.inv_type[ chunk ] == FOOD_CHUNK)))
    {
        const int mons_type = you.inv_plus[ chunk ];

        unwield_item( chunk );

        // Can't create more than the available chunks
        if (you.inv_quantity[ chunk ] < max_num)
            max_num = you.inv_quantity[ chunk ];

        you.inv_quantity[ chunk ] -= max_num;

        if (you.inv_quantity[ chunk ] <= 0)
        {
            you.equip[EQ_WEAPON] = -1;
            mpr("You are now empty-handed.");
        }

        burden_change();

        int summoned = 0;

        for (int i = 0; i < max_num; i++)
        {
            // yes, the snowmen eventually melt away... -- bwr
            if (create_monster( MONS_SIMULACRUM_SMALL, ENCH_ABJ_VI,
                                BEH_ENSLAVED, you.x_pos, you.y_pos,
                                MHITNOT, mons_type ) != -1)
            {
                summoned++;
            }
        }

        if (summoned)
        {
            strcpy( info, (summoned == 1) ? "An icy figure forms "
                                          : "Some icy figures form " );
            strcat( info, "before you!" );
            mpr( info );
        }
        else
            mpr( "You feel cold for a second." );
    }
    else
    {
        mpr( "You need to wield a piece of raw flesh for this spell "
             "to be effective!" );
    }

    return;
}                               // end sublimation()

void dancing_weapon(int pow, bool force_hostile)
{
    int numsc = 21 + (random2(pow) / 5);

    if (numsc > 25)
        numsc = 25;

    int summs = 0;
    int i = 0;
    char behavi = BEH_ENSLAVED;
    FixedVector < char, 2 > empty;

    if (!empty_surrounds(you.x_pos, you.y_pos, DNGN_FLOOR, false, empty))
    {
      failed_spell:
        if (silenced(you.x_pos, you.y_pos))
            mpr("Your weapon vibrates.");
        else
            mpr("You hear a popping sound.");

        return;
    }

    if (you.equip[EQ_WEAPON] == -1
        || you.inv_class[you.equip[EQ_WEAPON]] != OBJ_WEAPONS
        || launches_things(you.inv_type[you.equip[EQ_WEAPON]])
        || you.inv_dam[you.equip[EQ_WEAPON]] >= NWPN_SINGING_SWORD)
    {
        goto failed_spell;
    }

    if (you.inv_plus[you.equip[EQ_WEAPON]] >= 100 || force_hostile)
        behavi = BEH_CHASING_I; // cursed weapons become hostile

    if ((summs = create_monster( MONS_DANCING_WEAPON, numsc, behavi, empty[0],
                                        empty[1], you.pet_target, 1) ) != -1)
    {
        goto failed_spell;
    }

    for (i = 0; i < MAX_ITEMS; i++)
    {
        if (i >= 480)
        {
            mpr("The demon of the infinite void grins at you.");
            return;
        }

        if (!mitm.quantity[i])
        {
            mitm.id[i] = you.inv_ident[you.equip[EQ_WEAPON]];
            mitm.base_type[i] = you.inv_class[you.equip[EQ_WEAPON]];
            mitm.sub_type[i] = you.inv_type[you.equip[EQ_WEAPON]];
            mitm.pluses[i] = you.inv_plus[you.equip[EQ_WEAPON]];
            mitm.pluses2[i] = you.inv_plus2[you.equip[EQ_WEAPON]];
            mitm.special[i] = you.inv_dam[you.equip[EQ_WEAPON]];
            mitm.colour[i] = you.inv_colour[you.equip[EQ_WEAPON]];
            mitm.quantity[i] = 1;
            mitm.link[i] = NON_ITEM;
            break;
        }
    }                           // end for i loop

    in_name(you.equip[EQ_WEAPON], 4, str_pass);
    strcpy(info, str_pass);
    strcat(info, " dances into the air!");
    mpr(info);

    unwield_item(you.equip[EQ_WEAPON]);

    you.inv_quantity[you.equip[EQ_WEAPON]] = 0;
    you.equip[EQ_WEAPON] = -1;

    menv[summs].inv[MSLOT_WEAPON] = i;
    menv[summs].number = mitm.colour[i];
}                               // end dancing_weapon()

static bool monster_on_level(int monster)
{
    for (int i = 0; i < MAX_MONSTERS; i++)
    {
        if (menv[i].type == monster)
            return true;
    }

    return false;
}                               // end monster_on_level()

//
// This function returns true if the player can use controlled
// teleport here.
//
bool allow_control_teleport(void)
{
    bool ret = true;

    if (you.level_type == LEVEL_ABYSS || you.level_type == LEVEL_LABYRINTH)
        ret = false;
    else
    {
        switch (you.where_are_you)
        {
        case BRANCH_TOMB:
            // The tomb is a laid out maze, it'd be a shame if the player
            // just teleports through any of it.
            ret = false;
            break;

        case BRANCH_SLIME_PITS:
            // Cannot teleport into the slime pit vaults until
            // royal jelly is gone.
            if (monster_on_level(MONS_ROYAL_JELLY))
                ret = false;
            break;

        case BRANCH_ELVEN_HALLS:
            // Cannot raid the elven halls vaults with teleport
            if (you.branch_stairs[STAIRS_ELVEN_HALLS] +
                branch_depth(STAIRS_ELVEN_HALLS) == you.your_level)
                ret = false;
            break;

        case BRANCH_HALL_OF_ZOT:
            // Cannot teleport to the Orb
            if (you.branch_stairs[STAIRS_HALL_OF_ZOT] +
                branch_depth(STAIRS_HALL_OF_ZOT) == you.your_level)
                ret = false;
            break;
        }
    }

    // Tell the player why if they have teleport control.
    if (!ret && you.attribute[ATTR_CONTROL_TELEPORT])
        mpr("A powerful magic prevents control of your teleportation.");

    return ret;
}                               // end allow_control_teleport()

void you_teleport(void)
{
    if (scan_randarts(RAP_PREVENT_TELEPORTATION))
        mpr("You feel a weird sense of stasis.");
    else if (you.duration[DUR_TELEPORT])
    {
        mpr("You feel strangely stable.");
        you.duration[DUR_TELEPORT] = 0;
    }
    else
    {
        mpr("You feel strangely unstable.");

        you.duration[DUR_TELEPORT] = 4 + random2(3);

        if (you.level_type == LEVEL_ABYSS && !one_chance_in(3))
        {
            mpr
                ("You have a feeling this translocation may take a while to kick in...");
            you.duration[DUR_TELEPORT] += 3 + random2(3);
        }
    }

    return;
}                               // end you_teleport()

void you_teleport2(bool allow_control)
{
    bool is_controlled = (allow_control && !you.conf
                              && you.attribute[ATTR_CONTROL_TELEPORT]
                              && allow_control_teleport());

    if (scan_randarts(RAP_PREVENT_TELEPORTATION))
    {
        mpr("You feel a strange sense of stasis.");
        return;
    }

    FixedVector < int, 2 > plox;

    plox[0] = 1;
    plox[1] = 0;

    if (is_controlled)
    {
        mpr("You may choose your destination (press '.' or delete to select).");
        mpr("Expect minor deviation; teleporting into an open area is "
            "recommended.");
        more();

        show_map(plox);

        redraw_screen();

        plox[0] += random2(3) - 1;
        plox[1] += random2(3) - 1;

        if (one_chance_in(4))
        {
            plox[0] += random2(3) - 1;
            plox[1] += random2(3) - 1;
        }

#ifdef WIZARD
        strcpy(info, "Target square: ");
        itoa(plox[0], st_prn, 10);
        strcat(info, st_prn);
        strcat(info, ", ");
        itoa(plox[1], st_prn, 10);
        strcat(info, st_prn);
        mpr(info);
#endif

        if (plox[0] < 6 || plox[1] < 6 || plox[0] > (GXM - 5)
                || plox[1] > (GYM - 5))
        {
            mpr("Nearby solid objects disrupt your rematerialisation!");
            is_controlled = false;
        }

        if (is_controlled)
        {
            you.x_pos = plox[0];
            you.y_pos = plox[1];

            if ((grd[you.x_pos][you.y_pos] != DNGN_FLOOR
                    && grd[you.x_pos][you.y_pos] != DNGN_SHALLOW_WATER)
                || mgrd[you.x_pos][you.y_pos] != NON_MONSTER
                || env.cgrid[you.x_pos][you.y_pos] != EMPTY_CLOUD)
            {
                mpr("Oops!");
                is_controlled = false;
            }
        }
    }                           // end "if is_controlled"

    if (!is_controlled)
    {
        mpr("Your surroundings suddenly seem different.");

        do
        {
            you.x_pos = 10 + random2(GXM - 10);
            you.y_pos = 10 + random2(GYM - 10);
        }
        while ((grd[you.x_pos][you.y_pos] != DNGN_FLOOR
                   && grd[you.x_pos][you.y_pos] != DNGN_SHALLOW_WATER)
               || mgrd[you.x_pos][you.y_pos] != NON_MONSTER
               || env.cgrid[you.x_pos][you.y_pos] != EMPTY_CLOUD);
    }

    if (you.level_type == LEVEL_ABYSS)
    {
        abyss_teleport();
        env.cloud_no = 0;
        you.pet_target = MHITNOT;
    }
}                               // end you_teleport()

bool entomb(void)
{
    int loopy = 0;              // general purpose loop variable {dlb}
    bool proceed = false;       // loop management varaiable {dlb}
    int which_trap = 0;         // used in clearing out certain traps {dlb}
    char srx = 0, sry = 0;
    char number_built = 0;

    FixedVector < unsigned char, 7 > safe_to_overwrite;

    // hack - passing chars through '...' promotes them to ints, which
    // barfs under gcc in fixvec.h.  So don't.
    safe_to_overwrite[0] = DNGN_FLOOR;
    safe_to_overwrite[1] = DNGN_SHALLOW_WATER;
    safe_to_overwrite[2] = DNGN_OPEN_DOOR;
    safe_to_overwrite[3] = DNGN_TRAP_MECHANICAL;
    safe_to_overwrite[4] = DNGN_TRAP_MAGICAL;
    safe_to_overwrite[5] = DNGN_TRAP_III;
    safe_to_overwrite[6] = DNGN_UNDISCOVERED_TRAP;


    for (srx = you.x_pos - 1; srx < you.x_pos + 2; srx++)
    {
        for (sry = you.y_pos - 1; sry < you.y_pos + 2; sry++)
        {
            proceed = false;

            // tile already occupied by monster or yourself {dlb}:
            if (mgrd[srx][sry] != NON_MONSTER
                    || (srx == you.x_pos && sry == you.y_pos))
            {
                continue;
            }

            // the break here affects innermost containing loop {dlb}:
            for (loopy = 0; loopy < 7; loopy++)
            {
                if (grd[srx][sry] == safe_to_overwrite[loopy])
                {
                    proceed = true;
                    break;
                }
            }

            // checkpoint one - do we have a legitimate tile? {dlb}
            if (!proceed)
                continue;

            int objl = igrd[srx][sry];
            int hrg = 0;

            while (objl != NON_ITEM)
            {
                // hate to see the orb get  detroyed by accident {dlb}:
                if (mitm.base_type[objl] == OBJ_ORBS)
                {
                    proceed = false;
                    break;
                }

                hrg = mitm.link[objl];
                objl = hrg;
            }

            // checkpoint two - is the orb resting in the tile? {dlb}:
            if (!proceed)
                continue;

            objl = igrd[srx][sry];
            hrg = 0;

            while (objl != NON_ITEM)
            {
                hrg = mitm.link[objl];
                destroy_item(objl);
                objl = hrg;
            }

            // deal with clouds {dlb}:
            if (env.cgrid[srx][sry] != EMPTY_CLOUD)
            {
                env.cloud_type[env.cgrid[srx][sry]] = CLOUD_NONE;
                env.cgrid[ env.cloud_x[ env.cgrid[srx][sry] ] ]
                         [ env.cloud_y[ env.cgrid[srx][sry] ] ] = EMPTY_CLOUD;
                env.cloud_decay[env.cgrid[srx][sry]] = 0;
                env.cloud_no--;
            }

            // mechanical traps are destroyed {dlb}:
            if ((which_trap = trap_at_xy(srx, sry)) != -1)
            {
                if (trap_category(env.trap_type[which_trap])
                                                    == DNGN_TRAP_MECHANICAL)
                {
                    env.trap_type[which_trap] = TRAP_UNASSIGNED;
                    env.trap_x[which_trap] = 1;
                    env.trap_y[which_trap] = 1;
                }
            }

            // finally, place the wall {dlb}:
            grd[srx][sry] = DNGN_ROCK_WALL;
            number_built++;
        }                       // end "for srx,sry"
    }

    if (number_built > 0)
        mpr("Walls emerge from the floor!");
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return (number_built > 0);
}                               // end entomb()

void cast_poison_ammo(void)
{
    if (you.equip[EQ_WEAPON] == -1
        || you.inv_class[you.equip[EQ_WEAPON]] != OBJ_MISSILES
        || you.inv_dam[you.equip[EQ_WEAPON]] != 0
        || you.inv_type[you.equip[EQ_WEAPON]] == MI_STONE
        || you.inv_type[you.equip[EQ_WEAPON]] == MI_LARGE_ROCK)
    {
        canned_msg(MSG_NOTHING_HAPPENS);
        return;
    }

    in_name(you.equip[EQ_WEAPON], 4, str_pass);
    strcpy(info, str_pass);
    strcat( info,
            (you.inv_quantity[you.equip[EQ_WEAPON]] == 1) ? " is" : " are" );
    strcat(info, " covered in a thin film of poison.");
    mpr(info);

    you.inv_dam[you.equip[EQ_WEAPON]] = SPMSL_POISONED;

    wield_change = true;
}                               // end cast_poison_ammo()

bool create_noise(void)
{
    bool success = false;
    FixedVector < int, 2 > plox;

    plox[0] = 1;
    plox[1] = 0;

    mpr( "Choose the noise's source (press '.' or delete to select)." );
    more();
    show_map(plox);

    redraw_screen();

#ifdef WIZARD
    strcpy(info, "Target square: ");
    itoa(plox[0], st_prn, 10);
    strcat(info, st_prn);
    strcat(info, ", ");
    itoa(plox[1], st_prn, 10);
    strcat(info, st_prn);
    mpr(info);
#endif

    if (!silenced(plox[0], plox[1]))
    {
        if (plox[0] < 1 || plox[1] < 1 || plox[0] > (GXM - 2)
                || plox[1] > (GYM - 2))
        {
            if (!silenced(you.x_pos, you.y_pos))
                mpr("You hear a muffled thud.");
        }
        else
        {
            noisy(30, plox[0], plox[1]);

            if (!silenced(you.x_pos, you.y_pos))
                mpr("You hear a distant voice call your name.");

            success = true;
        }
    }

    return (success);
}                               // end create_noise()

/*
   Type recalled:
   0 = anything
   1 = undead only (Kiku religion ability)
 */
bool recall(char type_recalled)
{
    int loopy = 0;              // general purpose looping variable {dlb}
    bool success = false;       // more accurately: "apparent success" {dlb}
    int start_count = 0;
    int step_value = 1;
    int end_count = (MAX_MONSTERS - 1);
    FixedVector < char, 2 > empty;
    struct monsters *monster = 0;       // NULL {dlb}

    empty[0] = empty[1] = 0;

// someone really had to make life difficult {dlb}:
// sometimes goes through monster list backwards
    if (coinflip())
    {
        start_count = (MAX_MONSTERS - 1);
        end_count = 0;
        step_value = -1;
    }

    for (loopy = start_count; loopy != end_count; loopy += step_value)
    {
        monster = &menv[loopy];

        if (monster->type == -1)
            continue;

        if (monster->behavior != BEH_ENSLAVED)
            continue;

        if (monster_habitat(monster->type) != DNGN_FLOOR)
            continue;

        if (type_recalled == 1)
        {
            /* abomin created by twisted res, although it gets others too */
            if ( !((monster->type == MONS_ABOMINATION_SMALL
                            || monster->type == MONS_ABOMINATION_LARGE)
                        && (monster->number == BROWN
                            || monster->number == RED
                            || monster->number == LIGHTRED)) )
            {
                if (monster->type != MONS_REAPER
                        && mons_holiness(monster->type) != MH_UNDEAD)
                {
                    continue;
                }
            }
        }

        if (empty_surrounds(you.x_pos, you.y_pos, DNGN_FLOOR, false, empty))
        {
            // clear old cell pointer -- why isn't there a function for moving a monster?
            mgrd[monster->x][monster->y] = NON_MONSTER;
            // set monster x,y to new value
            monster->x = empty[0];
            monster->y = empty[1];
            // set new monster grid pointer to this monster.
            mgrd[monster->x][monster->y] = monster_index(monster);

            // only informed if monsters recalled are visible {dlb}:
            if (simple_monster_message(monster, " is recalled."))
                success = true;
        }
        else
        {
            break;              // no more room to place monsters {dlb}
        }
    }

    if (!success)
        mpr("Nothing appears to have answered your call.");

    return (success);
}                               // end recall()

void portal(void)
{
    char dir_sign = 0;
    unsigned char keyi;
    int target_level = 0;
    int old_level = you.your_level;

    if (you.where_are_you != BRANCH_MAIN_DUNGEON
                                    || you.level_type != LEVEL_DUNGEON)
    {
        mpr("This spell doesn't work here.");
    }
    else if (grd[you.x_pos][you.y_pos] != DNGN_FLOOR)
    {
        mpr("You must find a clear area in which to cast this spell.");
    }
    else
    {
        // the first query {dlb}:
        mpr("Which direction ('<' for up, '>' for down, 'x' to quit)?", MSGCH_PROMPT);

        for (;;)
        {
            keyi = (unsigned char) get_ch();

            if (keyi == '<')
            {
                if (you.your_level == 0)
                    mpr("You can't go any further upwards with this spell.");
                else
                {
                    dir_sign = -1;
                    break;
                }
            }

            if (keyi == '>')
            {
                if (you.your_level == 35)
                    mpr("You can't go any further downwards with this spell.");
                else
                {
                    dir_sign = 1;
                    break;
                }
            }

            if (keyi == 'x')
            {
                canned_msg(MSG_OK);
                return;         // an early return {dlb}
            }
        }

        // the second query {dlb}:
        mpr("How many levels (1 - 9, 'x' to quit)?", MSGCH_PROMPT);

        for (;;)
        {
            keyi = (unsigned char) get_ch();

            if (keyi == 'x')
            {
                canned_msg(MSG_OK);
                return;         // another early return {dlb}
            }

            if (!(keyi < '1' || keyi > '9'))
            {
                target_level = you.your_level + ((keyi - '0') * dir_sign);
                break;
            }
        }

        // actual handling begins here {dlb}:
        if (you.where_are_you == BRANCH_MAIN_DUNGEON)
        {
            if (target_level < 0)
                target_level = 0;
            else if (target_level > 26)
                target_level = 26;
        }

        mpr( "You fall through a mystic portal, and materialise at the "
             "foot of a staircase." );
        more();

        you.your_level = target_level - 1;
        grd[you.x_pos][you.y_pos] = DNGN_STONE_STAIRS_DOWN_I;

        down_stairs(true, old_level);
    }

    return;
}                               // end portal()

bool cast_death_channel(int power)
{
    bool success = false;

    if (you.duration[DUR_DEATH_CHANNEL] < 30)
    {
        mpr("Malign forces permeate your being, awaiting release.");

        you.duration[DUR_DEATH_CHANNEL] += 15 + random2(1 + (power / 3));

        if (you.duration[DUR_DEATH_CHANNEL] > 100)
            you.duration[DUR_DEATH_CHANNEL] = 100;

        success = true;
    }
    else
    {
        canned_msg(MSG_NOTHING_HAPPENS);
    }

    return (success);
}                               // end cast_death_channel()

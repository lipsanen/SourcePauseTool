# SourcePauseTool - FindClosestPassableSpace Visualization

A fork of [spt](https://github.com/YaLTeR/SourcePauseTool) that allows overriding, recording, and visualization of the algorithm in the [FindClosestPassableSpace](https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/game/shared/portal/portal_util_shared.cpp#L1494) (FCPS) function. Check out the original repo for installation/build instructions. <ins>**Currently, this implementation is only supported for unmodded portal 1 - version 5135.**</ins> If you want to load modded maps, you should probably copy the map to your vanilla portal folder.

## What for?

FCPS is responsible for a not-so-small percentage of the wackier glitches and tricks we see in portal 1. While stepping through a debugger to get an understanding of the algorithm *does* work, it also makes me want to cry. Implementing a visualization of the algorithm in spt for version 5135 is great because of accessibility - this is the most commonly used version of portal 1 used by speedrunners and most already know how to set up spt, and unlike *other* methods of trying to understand the algorithm this implementation allows a near real time and in-depth understanding.

---

## **Usage**

All commands related to FCPS start with the prefix "fcps_", and should have good enough help text to be mostly self-explanatory.

---

### **Setup**

`fcps_override 1` specifies that the default implementation of FCPS should be replaced with a custom version implemented in spt that records every time FCPS is called and what happens during the algorithm. Up to 200 separate FCPS events can be stored, after that new events will replace old ones. (The events are not saved across different game sessions or the plugin unloading, but you can use the save/load functionality for that).

Every time an event is recorded, a message is printed to the console. To see all in-memory events, you can use `fcps_show_recorded_events` or `fcps_show_loaded_events`.

---

### **Animating events**

After you have specified that FCPS events should be recorded and did something that triggers it, you can find the IDs of the events you want to animate and use <nobr>`fcps_animate_recorded_events 3`</nobr> to animated the event with ID 3 for example. You can also specify a range of events to animate at a time like "3^5", this will animate events with IDs 3, 4, then 5 back-to-back. For convenience, you can also use `fcps_animate_last_recorded_event` (which is useful to bind to a key), or use negative values to get events relative to the end of the queue. When you have loaded events from a .fcps file, you can use the same syntax with the `fcps_animate_loaded_events` command.

---

### **Adjusting the animation speed**

By default, each animation (which may have multiple events) will be animated over a total of (close to) 20 seconds. You can adjust `fcps_animation_time` to change this (even while an animation is playing). To stop an animation, use `fcps_stop_animation`. If you want to go through the visualization one step at a time, you can set `fcps_animation_time` to 0 and use `fcps_step_animation` (if you're cool you might make a bind for that). This will step the animation on every key press, but this can be a little annoying if you want to step many many times. So alternatively, you can instead use `fcps_skip_current_step_type` which will for example skip all rays that are fired between the corners and go straight to nudging the entity to a new location.

But what if you wanted something like `fcps_step_animation` that would fast forward even when the key was held? Well that's a thing too! Change your bind to `+fcps_step_animation` and there'll be no limit to how cool you can be!

---

### **Saving and loading events**

In case of nerds, nerd friends, or tricks that are hard to reproduce, one may choose to save FCPS events to be animated during a later game session. To save specific events to be animated later you must specify a directory to save to (no extension) and an event ID or a range of IDs to save. E.g. <nobr>`fcps_save_events fcps/AAA 3^6`</nobr> will save events with IDs 3 to 6 inclusive to two files: `portal/fcps/AAA.fcps` and `portal/fcps/AAA.txt`. The text file will contain a dump of the event(s) and is not necessary for them to be loaded later. You can then use <nobr>`fcps_load_events fcps/AAA`</nobr> to load all events in the .fcps file. Loading events will not override any events which have been recorded in the current session, but it will replace any other loaded events.

---

### **HUD**

`fcps_hud_progress 1` is a thing.

---

## **What triggers FCPS?**

Here is a list of the functions and my best understanding of the conditions that trigger FCPS.

---

`CC_Debug_FixMyPosition`

Triggers FCPS when running the command `debug_fixmyposition`. vIndecisivePush has a magnitude of 0.

---

`CPortalSimulator::RemoveEntityFromPortalHole`

Triggers FCPS when an object is partially intersecting a portal and that portal changes location or closes (or if the opposite portal closes). vIndecisivePush is set to the portal normal.

Examples:
- Most basic reentry methods (e.g. PQR/PBR/trigger reentry)
- Self bumping (e.g. getting put into cube droppers) when replacing the portal you're standing in
- Crazy elevator in portal prelude challenge map 15 (shown at the end of the video)

---

`CProp_Portal::TeleportTouchingEntity`

Triggers FCPS on the object the player is holding when the player teleports through a portal and the `m_bHeldObjectOnOppositeSideOfPortal` flag switches from 1 to 0. vIndecisivePush is set to the portal normal. This is one of the two mechanisms behind object teleport (the other being that the object is teleported to where the player is looking).

---

`CPortal_Player::VPhysicsShadowUpdate`

Triggers FCPS when the following conditions are met:
- The `m_hPortalEnvironment` field is set for the player (the player is in a portal bubble or has saveglitch).
- The *next* position of the player's vphysics shadow (yellow bounding box) is calculated. The distance between the player's *current* position and the *next* vphysics shadow position exceeds some threshold (~4 units) OR the difference in velocity exceeds some threshold (~10 units).
- The player is not in the ground AND is not touching another physics object. (Note: AFAIK the player is never considered to be "on the ground" when in a portal environment).
- The player is stuck on something (checked with `m_hPortalEnvironment`) in both the current player position and in the *next* calculated vphysics shadow position.

vIndecisivePush is set to the difference of the *next* calculated vphysics shadow position and the *current* position of the player. On an FCPS failure, this function deals one damage to the player AND teleports the player 10% of the distance from their *current* position (specifically their origin) to the center of the portal that the player is in (or to their saveglitch portal if relevant).

Examples:
- Vertical wall warps
- Edge glitch / crouch trigger teleport
- Getting pushed out of the portal when the player "changes shape" due to portal orientations
- Getting pushed out of the ground when the player gets curled up into a little ball (happens when going through specific portal orientations)
- Getting pushed out of wall geometry when the player's saveglitch portal closes or changes location.

---

`CPortalGameMovement::CheckStuck`

There seem to be almost no restrictions for this to trigger FCPS, the player just has to be stuck an in a portal environment. However, this function only gets triggered every 13 ticks (value gotten experimentally). vIndecisivePush is set to the portal normal. On an FCPS failure, the function deals 1e10 (10 billion) damage to the player. If FCPS places the player behind the plane of portal they are in, they will instead be placed back to their original location plus 5 units in the direction of the portal normal.

I'm pretty sure that `CheckStuck` can get triggered in all the same cases that `VPhysicsShadowUpdate` can, and on top of that can get triggered even when the player is touching a physics object.

---

`CPortalSimulator::MoveTo`

Unknown to me when this triggers FCPS. vIndecisivePush is set to the portal normal.

---

## **Video**

I made a video in which I explained how the algorithm in FCPS works and I showed a bunch of examples, check it out here: https://youtu.be/XrCw1ZpyXP8.

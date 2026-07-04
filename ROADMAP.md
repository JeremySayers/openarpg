# Roadmap

The agreed plan for getting from "a window opens" to a playable ARPG loop, in
vertical-slice phases. Settled in conversation, July 2026. This refines the
"Architecture (first pass)" sketch in [CLAUDE.md](CLAUDE.md) — where the two
disagree, this document wins.

Each phase ends with something runnable and is sized to land as one PR (or a
short series of PRs). Expect later phases to get re-scoped as earlier ones
teach us things.

## Settled decisions

1. **Controls: WASD movement + mouse aim.** No pathfinding needed for the
   first playable phase. Click-to-move can be revisited once there's terrain
   worth pathing around.

2. **Fixed 60 Hz simulation timestep**, driven by an accumulator: each render
   frame adds real elapsed time to a bucket and runs as many fixed sim ticks
   as the bucket contains, so a slow render frame runs multiple catch-up
   ticks and the world always moves at correct speed. The accumulator is
   clamped (~0.25 s) so a long hitch causes brief slow-motion instead of a
   catch-up spiral. Render interpolation between sim states is a later polish
   item, not a Phase 0 requirement.

3. **Entity handles are index + generation counter**, not a bare index. The
   generation is bumped when a slot is recycled, so stale handles (a
   projectile referencing an entity that died and got recycled) are detected
   instead of silently hitting the wrong entity.

4. **Component membership is a per-entity `uint32_t` bitmask** over parallel
   component arrays (`masks[i] & (COMP_POSITION | COMP_VELOCITY)`), rather
   than per-array `active` flags. Chosen mainly for the despawn story:
   destroying an entity is `masks[i] = 0`, with no per-component-array
   bookkeeping to forget. Mask `0` means the slot is empty. Cap of 32
   component types is acceptable; widen to `uint64_t` if ever needed.

5. **Event queue semantics:**
   - **Broadcast, not consume.** Every interested system sees every event —
     loot, exp, and (later) quest tracking all react to `EVENT_ENTITY_DIED`
     independently, without knowing about each other.
   - **Dedicated dispatch phase.** The tick runs
     `input → gameplay systems (push events) → event dispatch → cleanup`.
     Systems never drain the queue mid-tick; every event pushed during a tick
     is fully resolved in that same tick (an enemy dies and its loot appears
     in the same frame).
   - **Cascades allowed, bounded.** Handlers may push new events
     (damage → died → loot dropped); dispatch drains FIFO until the queue is
     empty, guarded by a cap on total events processed per tick (hit it =
     log loudly and stop — it means an event loop) and the queue's fixed
     capacity (overflow = drop + log, with a high-water mark surfaced on the
     debug overlay to tune `MAX_EVENTS_PER_FRAME` from data).
   - **Deferred entity destruction.** Handlers never destroy entities
     directly. Death marks the entity and pushes the event; a cleanup sweep
     at the very end of the tick — after dispatch — actually frees slots. So
     handlers can still read a dying entity's position/loot table during
     dispatch, and generation counters catch anyone holding the handle
     afterwards. Death animations don't keep the dead entity alive: a death
     handler spawns a separate short-lived corpse/VFX entity (position +
     sprite + timer, no health or collision).
   - **Fixed handler order.** Handlers of the same event run in a fixed,
     compile-time order (deterministic and debuggable). On-death effects that
     depend on each other's output need explicit sequencing via chained
     events, not handler priorities.
   - **Payload is a tagged union**, fixed size, no allocation.

6. **Simulation code is raylib-free.** ECS, events, and gameplay systems
   include no raylib headers; only input-gathering and rendering files touch
   raylib. This is what makes headless unit tests of damage/loot/exp logic
   possible, and keeps our snake_case game code visibly separate from
   raylib's TitleCase API.

7. **Content lives in static `const` C tables** for now
   (`static const enemy_def_t enemy_defs[] = {...}`), structured so the shape
   could later be loaded from files. Designing a data-file schema before we
   know what an item is would be guessing; revisit around Phase 4.

8. **Headless test runner from the start**, added in the same phase as the
   ECS core, run in CI. The ECS and event queue are exactly the code that's
   miserable to debug visually and trivial to test headlessly.

## Phase 0 — Foundations

No visible gameplay change. This is where the decisions above get cashed in.

- `ecs.c/h`: entity alloc/free with generation counters, component arrays +
  bitmasks, `MAX_ENTITIES`.
- `events.c/h`: fixed-capacity queue, `event_t` tagged union, push/dispatch
  with the semantics from decision 5.
- Restructure the main loop into fixed-tick `simulate()` + `render()` with
  the clamped accumulator from decision 2.
- Headless test runner + first tests (entity recycling/generations, event
  dispatch, cascade guard), wired into CI.

## Phase 1 — A moving player

- Input system: raylib input read once per frame into an `input_state_t` the
  sim consumes, so keybinds are centralized from day one.
- Player entity with position/velocity, movement system, camera follow.
- Everything drawn as colored shapes — no art yet, on purpose.

## Phase 2 — Combat vertical slice

The milestone that validates the whole architecture, because it exercises the
exact chain the event system exists for: player fires a projectile →
collision → `EVENT_DAMAGE_DEALT` → health system applies it →
`EVENT_ENTITY_DIED` → enemy despawns.

- Dumb enemies (stand there or walk at the player).
- One ability (projectile).
- Collision (circle overlap is plenty).
- Health component, damage, death.

## Phase 3 — The ARPG loop

After this phase the game is technically a game: kill, loot, level.

- Loot system reacting to `EVENT_ENTITY_DIED`. Seeded RNG utility comes in
  here — deterministic loot rolls are testable loot rolls.
- Item pickup.
- Exp/level system reacting to the same death events independently — the
  payoff moment for broadcast dispatch.
- Basic chase AI so enemies are a threat.

## Phase 4 — Presentation & content

- Sprites + animation component.
- A simple tilemap/arena instead of the void.
- Minimal HUD (health/exp bars).
- Sound hooks.
- A few enemy types and items, to prove the content tables scale past n=1.

## Ongoing / infra (slot in whenever)

- Debug overlay (F3): FPS, entity count, event-queue high-water mark — the
  high-water mark is how the `MAX_*` capacities get tuned honestly.
- Release packaging, once there's a build worth shipping.

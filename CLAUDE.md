# OpenARPG

An open source action RPG, built in C with [raylib](https://www.raylib.com), free for anyone to play, modify, and contribute to.

The project is brand new. `src/openarpg.c` is currently just a minimal window that opens and clears the screen — no gameplay yet.

## Build & Run

Build/run commands live in [README.md](README.md#building).

`.github/workflows/build_{linux,macos,windows,webassembly}.yml` build all four targets on every push that touches `src/**` or `CMakeLists.txt`, and upload the result as a workflow artifact. There's no release/publish automation yet — add it once there's a build worth shipping.

## Workflow

`main` is protected: every change lands via a pull request, and the four CI build workflows (`build_linux`, `build_macos`, `build_windows`, `build_web`) must pass before merging. There's no required-review count yet (no regular outside contributors), so a PR can be self-merged once CI is green.

This applies to AI-driven changes too — always work on a branch and open a PR, never push directly to `main`, even though the repo owner has admin bypass available for their own emergency use.

Concrete steps, using the `gh` CLI:

```sh
git checkout -b <branch-name>
# ...make changes, then...
git add <files>
git commit -m "..."
git push -u origin <branch-name>          # first push on a new branch
gh pr create --title "..." --body "..."   # opens the PR, prints its URL/number
gh pr merge <number>                      # once CI is green; ask which merge strategy if unspecified
```

If a branch already has an upstream (i.e. this isn't the first push), plain `git push` is enough — `-u` is only needed once.

## Pinning Dependencies

External dependencies — GitHub Actions in `.github/workflows/*.yml`, and third-party libraries fetched via CMake's `FetchContent` (raylib, etc.) — are pinned to an immutable full commit SHA, not a moving tag or branch (`@v4`, `@main`, a version string without a hash). A tag can be silently repointed to different content upstream; a commit SHA can't. Pin actions like `uses: owner/repo@<40-char-sha> # vX.Y.Z` — the trailing comment records the human-readable version for anyone reading the workflow.

This isn't about staying on the latest version — it's fine for a pin to sit unbumped for a long time. Bump it deliberately when there's a reason to (a needed fix/feature, or a security advisory), not automatically.

## Coding Conventions

Convention | Style | Example
--- | --- | ---
Functions | `snake_case` | `spawn_entity()`, `apply_damage()`
Variables | `snake_case` | `int screen_width`
Types (structs/enums) | `snake_case` with `_t` suffix | `entity_t`, `event_type_t`
Constants / macros | `ALL_CAPS` | `MAX_ENTITIES`
Files | `snake_case` | `ecs_world.c`

This is a deliberate departure from raylib's own `TitleCase` house style, chosen so our game code reads distinctly from raylib API calls in the same file.

Other rules:
- C99, `-Wall` clean.
- Four spaces, no tabs. Braces on their own line.

## Memory Strategy

The rule is "no allocation in the hot per-frame path," not "no malloc anywhere."

- **ECS hot path** (entities, components, the event queue) — allocated once at startup into fixed-capacity arrays/pools. This is the code touched every frame for potentially hundreds of entities, so it's where allocator latency and fragmentation would actually hurt. Capacity limits (`MAX_ENTITIES`, `MAX_EVENTS_PER_FRAME`, etc.) are chosen upfront and hit a hard cap rather than growing.
- **Everything else** (level/asset loading, save/load, tooling, one-off systems) — normal `malloc`/`free` is fine when it's a natural fit and doesn't happen every frame. Don't force genuinely variable-size, infrequent work into an oversized fixed buffer just to avoid `malloc`; that's a worse trade than a clearly-owned heap allocation.

## Architecture (first pass — expect this to change)

The plan is a small, self-rolled ECS plus an event system, specifically to keep gameplay chains (ability hits enemy → damage applied → on-hit effects trigger → enemy dies → loot rolls → player gets exp) decoupled instead of one system reaching directly into the next.

Rough shape, to be refined in conversation as it's actually built:

- **Entities** are plain integer IDs indexing into fixed-size component arrays. No sparse sets or archetypes yet — start simple, add that complexity only if profiling says so.
- **Components** are plain data structs (`position_t`, `health_t`, `sprite_t`, `loot_table_t`, ...), one flat pre-allocated array per component type.
- **Systems** are plain functions that loop over the component arrays they care about each frame. No registration/reflection machinery — a system is just a function called in a fixed order from the main loop.
- **Events** decouple the systems: instead of the combat system calling into the loot system directly, it pushes an `EVENT_ENTITY_DIED` (or `EVENT_DAMAGE_DEALT`, etc.) onto a fixed-capacity queue. The loot and exp systems drain the queue independently and react. This is what lets "ability hits, target takes damage, on-hit procs fire, target dies, loot drops, exp is granted" stay as several small independent systems instead of one tangled call chain.

None of the above exists in code yet — the next real step is standing up the entity/component storage and the event queue as a first pass, then iterating from there.

## Contribution Philosophy

This project exists to be freely played, modified, and contributed to. Favor a low barrier to entry over gatekeeping: welcome small contributions (a bug fix, one new item, one enemy type) as much as large ones, assume good faith from first-time contributors, and explain *why* in review comments rather than just what to change. Formal `CONTRIBUTING.md` / issue templates / code of conduct haven't been added yet — that's intentional for now, revisit once there's enough real content to attract outside contributors.

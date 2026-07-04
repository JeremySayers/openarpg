# Itemization

Design notes for items, affixes, and crafting, settled in conversation, July
2026. This is a design doc, not an implementation plan — nothing below exists
in code yet. Per [ROADMAP.md](ROADMAP.md), items don't show up until Phase 3
(loot/pickup) and Phase 4 (content), so this gets to sit and settle before any
of it needs to compile. Where this document and in-conversation decisions
later disagree, revise this document — it should stay the source of truth.

Inspiration: Diablo 2's item/affix shape (2 affixes on Magic, up to 6 on
Rare) and the tradeoff of trading total affix count for something exclusive,
crossed with Path of Exile's much deeper crafting — but deliberately stopping
well short of PoE's determinism, which was called out as making ground loot
feel pointless.

## Settled decisions

1. **Character structure: a starting archetype + a shared passive tree.**
   Not fixed D2-style classes (too rigid for the itemization flexibility we
   want) and not fully classless (loses per-character identity). Picking an
   archetype at creation nudges playstyle and unlocks a small archetype-only
   sub-tree, similar to PoE ascendancies, but the bulk of the passive web is
   shared across all archetypes.

2. **Rarity ladder: Normal / Magic / Rare / Unique.** No Set items — a
   unique-only "build-defining item" tier covers that role without the extra
   design/balance surface of a whole second collection meta-game.

3. **Affix budget per rarity:**
   - Normal: 0 affixes, just a base implicit (armor value, weapon damage
     range, base socket count for that item type).
   - Magic: 1–2 affixes (1 prefix + 1 suffix cap).
   - Rare: 4–6 affixes (up to 3 prefix + 3 suffix).
   - Unique: a fixed, hand-authored affix list with small roll ranges
     (D2-style tight ranges, not PoE-wide ones) — doesn't touch the affix
     pool at all.

4. **Magic vs. Rare draw from the same affix pool, but the top tier of every
   affix is Magic-exclusive.** Confirmed real D2 precedent: `+1`/`+2` to a
   skill can roll on either Magic or Rare, `+3` only on Magic. We're taking
   that as the universal rule for every affix, not a per-affix inconsistency
   — a Rare can always reach one tier below an affix's max, full stop. This
   keeps the system legible (no wondering whether *this specific* Rare stat
   secretly had a locked ceiling — the rule is always the same) while
   preserving the real tradeoff: pick Magic for a shot at a stat's true
   ceiling, at the cost of total affix count.

5. **Tiers are defined per-affix, not as one global item tier.** Each affix
   gets however many tiers its practical value range calls for — e.g. Life
   probably wants ~10 tiers for the stat to feel meaningfully different
   across the whole leveling curve, while Attributes might only need ~4.
   There is no single "item tier" badge; an item's tooltip shows each rolled
   affix at its own tier independently. (This is also what resolves an
   earlier idea of separate numbered M-tier/R-tier tracks — once tiers are
   per-affix instead of per-item, there's nothing to reconcile across
   rarities.)

6. **Sockets are a base-item property**, rolled/fixed by item type + item
   level — not an affix. **Gems are contextual modifiers**: a gem's effect
   depends on the item type it's socketed into (a gem in a helm does
   something different than the same gem in a weapon). Gems are never skill
   gems — active skills live entirely in the archetype/shared tree and are
   never granted or modified by gear.

7. **Crafting currency is bounded and non-deterministic** — consumable
   ground-loot drops that each do one fixed thing, never "pick the affix you
   want":
   - Upgrade: Normal→Magic or Magic→Rare, rolling a random affix into the
     new slot(s).
   - Reroll: rerolls all current affixes on a Magic/Rare item, same rarity.
   - A scarce "add one affix" for a Rare not yet at its 6-affix cap.
   - An even scarcer "remove one random affix, add one random affix" as the
     chase-a-specific-combo ceiling piece.

   Deliberately excluded: pick-your-mod crafting benches, deterministic
   mod-stacking, or anything letting a player build an item mod-by-mod.
   Ground Rares stay the fastest path to a good item; currency only improves
   what's already found.

8. **The Crafted-item bench** (placeholder name: "Crafted" items — a better
   name is still deferred). This is the runeword-equivalent system, built
   around affixes instead of runes:
   - A recipe defines a fixed set of required affix *tags* (e.g. Strength +
     Life + Fire Resist) and a target base item type, producing a fixed,
     predetermined bonus effect.
   - The bench takes a Normal (affixless) base item plus **1–3 donor**
     Magic/Rare items — not required to fill all 3 slots; a single item that
     already covers every required tag on its own is a valid, efficient
     craft.
   - Every donor's *entire* set of qualifying affix tags counts, not just
     one tag per donor. Recipe difficulty is expressed through how many
     total tags it requires relative to the fixed 3-donor cap, not through
     more slots — a 3-tag recipe is trivial (one tag per donor), a 9–10 tag
     recipe forces each donor to average 3+ matching tags at once, which
     means hunting for a Rare that happens to roll heavily into that
     specific recipe's tag set. Same simple bench UI throughout the game;
     donor scarcity is what makes late recipes hard.
   - Donors are fully consumed regardless of how many of their affixes were
     "useful." The base item's own implicit and base stats carry through
     unchanged, same as a D2 runeword base.
   - All qualifying donor affixes across the placed items form one combined
     tag bag. **Any recipe whose full requirement is a subset of that bag is
     satisfied.** If more than one recipe is satisfied at once, the bench
     randomly picks between them. This is the risk/skill-expression knob: a
     donor with many affixes that happen to overlap several recipes is a
     liability (you don't know which result you'll get), while an item with
     *only* the tags one specific recipe wants is the safe, targeted choice.
   - Crafting can only be committed if **at least one** recipe is currently
     satisfied — never a wasted craft with zero output, the risk is which
     result you get, not whether you get one. The UI shows all
     currently-satisfiable recipes given the items placed, before commit.
   - The recipe's guaranteed fixed effect scales off the **minimum** tier
     among the donor affixes that satisfied its required tags (weakest-link,
     not average) — rewards uniformly high-tier donors over padding count
     with low-tier ones.
   - **Bonus pass-through:** if a donor happens to carry one of the
     Magic-exclusive top-tier affixes (see decision 4 and decision 9), that
     exact affix instance — value and tier as-is, no rescaling — carries
     through onto the finished Crafted item as a bonus on top of the
     recipe's guaranteed effect. This applies uniformly to any recipe, no
     recipe needs to be specially authored to support it. This is the
     intended endgame chase loop: get the (boss-gated, see decision 9)
     Magic→Rare upgrade currency, gamble it into a Rare that both matches a
     good recipe's tags *and* rolled a strong Magic-exclusive affix, then
     feed it in for a strictly-better version of that Crafted item.

9. **The Magic→Rare upgrade currency can produce a Rare carrying a
   Magic-exclusive top-tier affix** — something that could never happen from
   a natural Rare drop. This is an intentional "shouldn't exist" chase item,
   as long as that upgrade currency stays very rare (boss-gated is the
   working assumption). Implementation note: the item data model must not
   hard-assume "Rare implies no Magic-exclusive-tier affixes" as a structural
   invariant — that assumption only holds at natural roll time. Each affix
   instance just needs to record its own tier; nothing downstream should
   special-case rarity vs. tier eligibility.

## Open questions / deferred

Roughly in the order they'll probably come up:

- The concrete affix list and stat pool (needs the damage/defense model
  settled first — elemental types, resistances, mitigation math).
- Exact per-affix tier counts and the item-level breakpoints that gate them.
- A real name for the Crafted-item system (currently just "Crafted").
- Whether to layer in a small, purely-exclusive utility-affix pool on top of
  the top-tier-lock rule (D2's "of the Whale" style effects that aren't a
  tier of any shared stat at all) — not required for the tradeoff to work,
  but could still be worth it for flavor.
- Full item slot list and base-item catalog.
- Concrete recipe list for the Crafted bench.
- Skills: how they're granted/upgraded, resource system (mana or otherwise).
- Passive tree: shape/size of the shared web, how archetype sub-trees plug
  into it.
- Monsters & bosses: rare-monster affixes, boss-specific mechanics and loot.
- Progression: character level vs. item level, where the level cap sits.
- Endgame loop: what there is to do at max level.

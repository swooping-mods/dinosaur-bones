# Dinosaur Bone Discoveries: behavior and implementation

Describes the exported implementation **0.2.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

Creates four fossil discovery sites, including the textured skull. The active world supplies placement and local terrain orientation. Aim at a nearby fossil and press E to mark it discovered and open the collection journal; Ctrl+Shift+F3 toggles the journal. The four sites are anchored near the cardinal stations; discovered bits control the collection presentation. Scene roots are created lazily and retried rather than copied into the original game assets.

## State and lifecycle

A small versioned per-world sidecar stores the four discovery bits. Host authority is latched; authority changes suspend writes. Dirty state retries a failed save. World teardown attempts a valid host save, destroys site objects and drops managed roots. Guests do not become writers after losing their host connection.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/dinosaur-bones/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

## Implementation sources

- [mod.c](../source/mods/dinosaur-bones/mod.c)
- [feature.h](../source/mods/dinosaur-bones/feature.h)
- [skull_model.h](../source/mods/dinosaur-bones/skull_model.h)
- [store.h](../source/mods/dinosaur-bones/store.h)

## Verification boundary

This is a bounded set of authored discoveries, not procedural archaeology or a new native item inventory. Placement, journal readability, saved discoveries and remote presentation need native-game qualification.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).

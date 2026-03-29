#!/bin/bash
# pack.sh — project-specific cpkg hooks for sigma.test
#
# cpkg sources this file immediately after config.sh and before it resolves
# REQUIRES into requires_objects. Clearing REQUIRES here prevents dependency
# packages (sigma.core, sigma.memory, etc.) from being baked into sigma.test.o.
# They must be linked separately at final link time — baking them in causes
# duplicate .init_array entries when the consumer also links the same packages.
REQUIRES=()

pre_pack() { :; }

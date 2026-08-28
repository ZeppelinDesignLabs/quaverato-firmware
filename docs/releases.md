# Archived releases

These checksums are for the **original field-verified** binaries preserved under [`archive/`](../archive/) and on `master`. They are **not** reproducible from the current toolchain (see [build-environment.md](build-environment.md)).

## SHA-256

| File | SHA-256 |
|------|---------|
| `archive/v1.1.3/Quaverato_1.1.3.ino.hex` | `75a3e067717fb8b106cb7fd2d1af82e44cbfb002ae6f4bd083f49c88a65862a8` |
| `archive/v1.1.3/Quaverato_1.1.3.ino` | `00078b3bbea1da4ac4fe60f4c990d24475e7c22b2a37807e89d9cc23a32a4258` |
| `archive/v2.3.6/Quaverato_2.3.6.ino.hex` | `cae3f43bc6ebb4997d9aca923a40b644d0900f9236f589530dfa1733ffd3890d` |
| `archive/v2.3.6/Quaverato_2.3.6.ino` | `c2e3d848d2442291070cd7e563ef1090950b40037c4628c2d2a1335752aff56e` |
| `archive/v2.4.2/Quaverato_2.4.2.ino.hex` | `5f623da31c03e56b408cc757d330fbc46750acd566f05735e3acf484d15467d6` |
| `archive/v2.4.2/Quaverato_2.4.2.ino` | `26b095bb602db5e09bc79083a8c1bef7f84de5390e971311fffd482547111a7a` |
| `eeprom/quaverato-default-presets.hex` | `5cbf6e46185b1edaef53914fdd53a15a941ffdf06088d6f7388068caa4978c89` |

## Git tags (historical commits on `master`)

| Tag | Commit | Notes |
|-----|--------|-------|
| `v1.1.3` | `a0ee5c3` | Initial public firmware (no MIDI) |
| `v2.3.6` | `4db287e` | Full MIDI + rate-knob rework |
| `v2.4.2` | `e7c52cc` | Four-stage relay + isolator (pin 11) |

Upload each tag's `.ino` + `.hex` (and the EEPROM image on MIDI-era releases) as GitHub Release assets. Mark release notes as **archived / not reproducible from current CI**.

`master` remains the permanent binary archive and is not rewritten.

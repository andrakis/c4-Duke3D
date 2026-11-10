# Duke Nukem 3D's data

`3dduke13.zip` is 3D Realms' shareware package of Duke Nukem 3D v1.3D,
5,924,374 bytes, unmodified, as 3D Realms' FTP site had it. It holds the
first episode, "L.A. Meltdown".

Its `LICENSE.TXT` lets individuals give copies away without charge, and
free web and FTP sites make it available for downloading, as long as no
copyright information or trademark is added or removed and the game's
files are included without modification. c4's page is free, so c4 may
fetch the package and put the episode on its disks, as it does Rise of
the Triad's and Wolfenstein 3D's. The registered game, and any modified
version, may not be distributed: the full game is the person's own
`DUKE3D.GRP`, which they add with the page's Uploads (README.c4.md).

`build-images.sh` doesn't change the package. `tools/dukedata.py` reads
the game's files out of it and puts them here, and the disks get them:

- `DUKE3DSW.GRP`, the package's `DUKE3D.GRP` (11,035,779 bytes, CRC32
  `983AD923`), renamed so that the person's own `DUKE3D.GRP` never hides
  it on a disk the Uploads are laid over;
- `DUKE.RTS`, the remote ridicule sounds;
- `LICENSE.TXT`, the licence itself.

The package holds them in `DN3DSW13.SHR`, a DOS self-extracting archive
that Python's zipfile reads as it is. Its demos (`DEMO1.DMO` to
`DEMO3.DMO`) were recorded by version 1.3D, which the Atomic Edition's
code this port is built on does not play, so they are left in it.

## Getting it

None of these files is in this repository. In c4, `make duke3d-data`
puts them here with c4's `games/fetch.sh`:
- from `$DUKE3D_ZIP`, when that names your copy;
- or else from the first URL in `MIRRORS` that gives exactly this file.

Either way it is refused unless its SHA-256 is
`c67efd179022bc6d9bde54f404c707cbcbdc15423c20be72e277bc2bdddf3d0e`.
The URL in `MIRRORS` was checked to give exactly that file (2026-11-10).

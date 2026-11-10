#!/usr/bin/env python3
# dukedata.py ZIP DIR -- what c4's disks take from 3D Realms' shareware
# package of Duke Nukem 3D v1.3D (3dduke13.zip), into DIR:
#
#   DUKE3DSW.GRP  the episode (the package's DUKE3D.GRP), named so that the
#                 person's own DUKE3D.GRP never hides it
#   DUKE.RTS      the remote ridicule sounds
#   LICENSE.TXT   3D Realms' terms, which allow the package to be passed on
#
# The package holds them in DN3DSW13.SHR, a self-extracting archive that
# Python's zipfile reads as it is. Nothing is changed; each file is
# checked against the size the licence lists for it.
import sys, os, io, zipfile, zlib

z, out = sys.argv[1], sys.argv[2]
outer = zipfile.ZipFile(z)
inner = zipfile.ZipFile(io.BytesIO(outer.read('DN3DSW13.SHR')))
want = {'DUKE3D.GRP': ('DUKE3DSW.GRP', 11035779), 'DUKE.RTS': ('DUKE.RTS', 188954),
        'LICENSE.TXT': ('LICENSE.TXT', 9108)}
for name, (dest, size) in want.items():
    data = inner.read(name)
    if len(data) != size:
        sys.exit('dukedata.py: %s is %d bytes, not %d' % (name, len(data), size))
    if name == 'DUKE3D.GRP' and zlib.crc32(data) != 0x983AD923:
        sys.exit('dukedata.py: DUKE3D.GRP is not the shareware 1.3D episode')
    tmp = os.path.join(out, dest + '.part')
    with open(tmp, 'wb') as f:
        f.write(data)
    os.replace(tmp, os.path.join(out, dest))

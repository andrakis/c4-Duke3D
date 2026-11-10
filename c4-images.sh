# c4-images.sh c4ix|bare -- Duke Nukem 3D's part of c4's build-images.sh,
# which sources it from c4's root (games/README.md in c4)
GAMEDIR=games/duke3d
case "$1" in
c4ix)
    # linked against C4IX's libc, with the shareware episode beside it
    # (c4ix.vfs.txt: /usr/games/duke3d). C4IX's disk is flat and shared, so
    # the package's licence goes there under a name of its own.
    if [ -f .duke3d/duke-sum.c4o ] && [ -f $PB/lib/libc.c4s ]; then
        say "linking Duke Nukem 3D for C4IX"
        ./c4rlink32 .duke3d/*.c4o .duke3d/plat/duke-c4ix.c4o -l $PB/lib/libc.c4s -o $DISK/c4ix-duke3d.c4r > /dev/null
        if [ -f $GAMEDIR/data/DUKE3DSW.GRP ]; then
            cp $GAMEDIR/data/DUKE3DSW.GRP $GAMEDIR/data/DUKE.RTS $DISK/
            cp $GAMEDIR/data/LICENSE.TXT $DISK/duke3d-license.txt
        fi
        # the Build engine's licence travels with the engine (BUILDLIC.TXT [4])
        cp $GAMEDIR/BUILDLIC.TXT $DISK/duke3d-buildlic.txt
    fi ;;
bare)
    if [ -f .duke3d/duke-sum.c4o ]; then
        item duke3d
        ./c4rlink32 .duke3d/*.c4o .duke3d/plat/duke-bare.c4o .libc-bare/libc-*.c4o -o $GAMESDISK/duke3d.c4r > /dev/null
        cp $GAMEDIR/BUILDLIC.TXT $GAMESDISK/
        if [ -f $GAMEDIR/data/DUKE3DSW.GRP ]; then
            cp $GAMEDIR/data/DUKE3DSW.GRP $GAMEDIR/data/DUKE.RTS $GAMEDIR/data/LICENSE.TXT $GAMESDISK/
        fi
        GAMES_ON_DISK="$GAMES_ON_DISK DUKE3D"
    fi ;;
esac

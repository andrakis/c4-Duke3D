# c4.mk -- this game's part of c4's Makefile. c4 includes it from
# games/duke3d/c4.mk, its submodule, and runs it from c4's root, with c4's
# tools and variables (games/README.md in c4). Without it, c4 builds
# everything else, and test-duke3d says it was skipped.

# Duke Nukem 3D (games/duke3d/README.c4.md): Chocolate Duke3D's units with
# c4lc, the same objects for C4IX, the bare machine and C4DOS, but for the
# two that say whether the display has to be asked for (duke-c4ix.c,
# duke-bare.c, in .duke3d/plat). c4-images.sh links them: against C4IX's
# libc, and whole with the bare libc of libc-bare-objs.
DUKE3D       := games/duke3d
DUKE3D_ENG   := cache draw dummy_multi engine filesystem fixedPoint_math network tiles
DUKE3D_GAME  := actors animlib config console control cvar_defs cvars game gamedef global keyboard \
                menues player premap rts scriplib sector sounds
# audiolib's MIDI player and AdLib driver, for the music on the card's OPL
DUKE3D_AUD   := midi al_midi gmtimbre ll_man
# c4's alone: the display driver, the clock and the test, the display, the card
DUKE3D_C4    := duke-display duke-sum duke-gui duke-snd
DUKE3D_PLAT  := duke-c4ix duke-bare
DUKE3D_HDRS  := $(wildcard $(DUKE3D)/Engine/src/*.h $(DUKE3D)/Game/src/*.h $(DUKE3D)/Game/src/audiolib/*.h \
                $(DUKE3D)/c4/*.h $(DUKE3D)/c4/inc/*.h $(DUKE3D)/c4/inc/sys/*.h) \
                $(wildcard src/c4ix/libc/include/*.h src/c4ix/libc/include/sys/*.h) \
                src/c4ix/include/display.h src/c4ix/include/window.h src/c4ix/include/window.c \
                libjs/guest/gui.h libjs/guest/gui.c libjs/guest/snd.h libjs/guest/snd.c
DUKE3D_INC   := -D C4DUKE -D PLATFORM_UNIX -I $(DUKE3D)/c4/inc -I src/c4ix/libc/include -I src/c4ix/include \
                -I libjs/guest -I $(DUKE3D)/Engine/src -I $(DUKE3D)/Game/src -I $(DUKE3D)/c4
DUKE3D_CC    := ./c4sp32 -c 400000 src/c4sp/lisp/c4lc.lisp -O $(IMG_G) -mfuse -fno-builtin -c -conforming $(DUKE3D_INC)
DUKE3D_OBJS  := $(DUKE3D_ENG:%=.duke3d/%.c4o) $(DUKE3D_GAME:%=.duke3d/%.c4o) $(DUKE3D_AUD:%=.duke3d/%.c4o) \
                $(DUKE3D_C4:%=.duke3d/%.c4o)
duke3d-objs: $(DUKE3D_OBJS) $(DUKE3D_PLAT:%=.duke3d/plat/%.c4o)
.duke3d/%.c4o: $(DUKE3D)/Engine/src/%.c $(DUKE3D_HDRS) $(C4LC_LISP) .img_g | c4sp32
	@mkdir -p .duke3d/plat
	$(DUKE3D_CC) $< $@ > /dev/null
.duke3d/%.c4o: $(DUKE3D)/Game/src/%.c $(DUKE3D_HDRS) $(C4LC_LISP) .img_g | c4sp32
	@mkdir -p .duke3d/plat
	$(DUKE3D_CC) $< $@ > /dev/null
.duke3d/%.c4o: $(DUKE3D)/Game/src/audiolib/%.c $(DUKE3D_HDRS) $(C4LC_LISP) .img_g | c4sp32
	@mkdir -p .duke3d/plat
	$(DUKE3D_CC) $< $@ > /dev/null
.duke3d/%.c4o: $(DUKE3D)/c4/%.c $(DUKE3D_HDRS) $(C4LC_LISP) .img_g | c4sp32
	@mkdir -p .duke3d/plat
	$(DUKE3D_CC) $< $@ > /dev/null
.duke3d/plat/%.c4o: $(DUKE3D)/c4/%.c $(DUKE3D_HDRS) $(C4LC_LISP) .img_g | c4sp32
	@mkdir -p .duke3d/plat
	$(DUKE3D_CC) $< $@ > /dev/null
PHONY += duke3d-objs

# ... and by gcc for the host, 32-bit as the c4lc build is, with no display
# and no sound (duke-host.c, duke-nosnd.c): the reference test-duke3d holds
# the c4lc builds to. gnu99, and the errors gcc 14 made of old C's
# conversions are warnings again; -fcommon for the globals the headers
# define.
DUKE3D_HOST_SRCS := $(DUKE3D_ENG:%=$(DUKE3D)/Engine/src/%.c) $(DUKE3D_GAME:%=$(DUKE3D)/Game/src/%.c) \
                    $(DUKE3D)/c4/duke-display.c $(DUKE3D)/c4/duke-sum.c $(DUKE3D)/c4/duke-host.c $(DUKE3D)/c4/duke-nosnd.c
duke3d-host: $(DUKE3D_HOST_SRCS) $(DUKE3D_HDRS)
	gcc -m32 -std=gnu99 -O2 -fcommon -msse2 -mfpmath=sse -ffp-contract=off -fno-strict-aliasing -w \
	    -Wno-error=int-conversion -Wno-error=incompatible-pointer-types -Wno-error=implicit-function-declaration \
	    -DC4DUKE -DPLATFORM_UNIX -I$(DUKE3D)/c4/inc -I$(DUKE3D)/Engine/src -I$(DUKE3D)/Game/src -I$(DUKE3D)/c4 \
	    -o $@ $(DUKE3D_HOST_SRCS) -lm

# The data, which is not in the repository: 3D Realms' shareware package
# of v1.3D as it was distributed, checked by its SHA-256, and what the
# game reads out of it (tools/dukedata.py): the episode's DUKE3D.GRP as
# DUKE3DSW.GRP, so that the person's own never hides it, DUKE.RTS, and the
# package's LICENSE.TXT, which allows it to be passed on (data/README.md).
# games/fetch.sh takes the package from $DUKE3D_ZIP when that names a
# copy, or else from the mirrors in data/MIRRORS.
DUKE3D_DATA := $(DUKE3D)/data
duke3d-data: $(DUKE3D_DATA)/DUKE3DSW.GRP
$(DUKE3D_DATA)/3dduke13.zip:
	@sh games/fetch.sh $@ c67efd179022bc6d9bde54f404c707cbcbdc15423c20be72e277bc2bdddf3d0e DUKE3D_ZIP $$(cat $(DUKE3D_DATA)/MIRRORS 2>/dev/null)
$(DUKE3D_DATA)/DUKE3DSW.GRP: $(DUKE3D_DATA)/3dduke13.zip $(DUKE3D)/tools/dukedata.py
	@python3 -I $(DUKE3D)/tools/dukedata.py $< $(DUKE3D_DATA)
PHONY += duke3d-data

# test-duke3d: E1L1 played from a fixed script of keys and mouse moves
# (duke-sum.c, -script), on a clock that moves only as the game runs, by
# gcc's build and by c4lc's -- on the bare machine, under C4DOS and inside
# C4IX -- must show the same frames, to the checksum of every pixel and
# the palette. With sound and music off (/ns /nm): what plays does not
# change what is drawn. Then, inside C4IX, the screen size changed as a
# player changes it, in the game's menu (Options, Video Setup: -menu, from
# 320x200 to 400x300), the detail set low there, and played on so; then
# quit from the game (F10, Y), which must end it.
DUKE3D_RUN  := /v1 /l1 /s2 /ns /nm -sum 50 -frames 600 -script
DUKE3D_RES  := /v1 /l1 /s2 /ns /nm -sum 20 -frames 400 -menu
test-duke3d: duke3d-data duke3d-host c4bb-images test-libc-bare
	@rm -rf .duke3d_t && mkdir -p .duke3d_t
	@cp $(DUKE3D_DATA)/DUKE3DSW.GRP $(DUKE3D_DATA)/DUKE.RTS .duke3d_t/
	@cd .duke3d_t && ../duke3d-host $(DUKE3D_RUN) 2>/dev/null | grep '^sum\|^timedemo' > host.txt
	@cd .duke3d_t && ../duke3d-host $(DUKE3D_RES) 2>/dev/null | grep '^sum\|^timedemo' > res.txt
	@tail -1 .duke3d_t/host.txt
	@cd src/c4bb/images/games && { timeout 1500 node ../../../../libjs/cli.js -m 64 -d . duke3d.c4r $(DUKE3D_RUN) \
	     > ../../../../.duke3d_t/bare.out 2>/dev/null & \
	   printf 'duke3d $(DUKE3D_RUN)\nver\n' | timeout 1500 node ../../../../libjs/cli.js -m 64 -d . ../c4dos32.c4r \
	     > ../../../../.duke3d_t/dos.out 2>/dev/null; wait; }
	@printf 'duke3d $(DUKE3D_RUN)\nexit\n' | timeout 1500 node libjs/cli.js -m 64 -d src/c4bb/images/disk src/c4bb/images/c4ix32.c4r \
	   2>/dev/null > .duke3d_t/c4ix.out
	@printf 'duke3d $(DUKE3D_RES)\nexit\n' | timeout 1500 node libjs/cli.js -m 64 -d src/c4bb/images/disk src/c4bb/images/c4ix32.c4r \
	   2>/dev/null > .duke3d_t/res.out
	@for w in bare dos c4ix; do grep '^sum\|^timedemo' .duke3d_t/$$w.out | cmp -s .duke3d_t/host.txt - \
	   || { echo "test-duke3d: $$w's frames differ:"; tail -5 .duke3d_t/$$w.out; exit 1; }; done
	@grep '^sum\|^timedemo' .duke3d_t/res.out | cmp -s .duke3d_t/res.txt - \
	   || { echo "test-duke3d: with the screen size changed in the menu, the frames differ:"; tail -5 .duke3d_t/res.out; exit 1; }
	@grep -q '^duke3d: quit from the game' .duke3d_t/res.out \
	   || { echo "test-duke3d: the game's quit did not end it:"; tail -5 .duke3d_t/res.out; exit 1; }
	@grep -A1 '^timedemo' .duke3d_t/dos.out | grep -Eq '^A(:\\)?>C4DOS version' \
	   || { echo "test-duke3d: C4DOS did not come back to its prompt:"; tail -5 .duke3d_t/dos.out; exit 1; }
	@rm -rf .duke3d_t
	@echo "test-duke3d: OK"
PHONY += test-duke3d

GAME_OBJS  += duke3d-objs
GAME_TESTS += test-duke3d

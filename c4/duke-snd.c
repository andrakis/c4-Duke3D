// duke-snd.c -- the game's sound, Apogee's FX_ (effects) and MUSIC_ (MIDI)
// interfaces, on libjs's sound card (libjs/guest/snd.h), in place of
// Chocolate Duke3D's fx_man.c and multivoc.c (on SDL) and sdl_midi.c
// (SDL_mixer). Taken from c4's Rise of the Triad (games/rott, rott-snd.c),
// which plays the same audiolib on the same card.
//
// The effects are the GRP's VOC and WAV sounds. They play as they are on
// the card's eight sample channels, at their own rate moved by the pitch
// the game asks for, panned by angle and distance as audiolib's mixer
// (multivoc.c) panned them.
//
// The music is the DOS game's own: audiolib's MIDI player (midi.c), and
// its AdLib driver (al_midi.c) with the game's own timbre bank
// (D3DTIMBR.TMB, which the game registers over gmtimbre.c's). The card's
// OPL stands where the AdLib was: al_midi.c's port writes come to
// duke_outp here. audiolib's task manager ran the player from the timer
// interrupt. Here it is duke_snd_poll(), which duke-display.c calls
// whenever the game takes its input or shows a frame. The poll runs the
// player a tick at a time, with snd_wait between ticks, so each write is
// stamped with the stream's time, until the card's stream is a quarter of
// a second ahead.
//
// When the card has a General MIDI synth on its MIDI port (the person
// chose General MIDI music), the player's events go there instead, as
// they went to an MPU-401 and the Sound Canvas behind it, and midi.c
// plays the score's General MIDI tracks: the game's music as it was
// written, rather than as the AdLib could play it.
//
// With no card, or no OPL, the game plays silent rather than stopping, as
// it would on a DOS machine that had none.

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "audiolib/fx_man.h"
#include "audiolib/music.h"
#include "audiolib/midi.h"
#include "audiolib/al_midi.h"
#include "audiolib/task_man.h"
#include "filesystem.h"
#include "snd.h"

// a quarter of a second ahead, as Doom keeps its music: a frame that
// takes longer (a level's first) then costs no gap
enum { AHEAD_US = 250000, MAXSTEPS = 4000 };

static int fx_on, music_on;
int MUSIC_SoundDevice;                    // midi.c's, which asks it nothing here
static int fx_error, music_error;

// ---- the task manager, run by the poll ----

enum { NTASKS = 4 };
static task tasks[NTASKS];
static int  task_used[NTASKS];
static long task_next[NTASKS];    // when each runs next, on the stream's clock
static long stream_us;            // the stream's clock: what snd_wait has moved
volatile int TS_InInterrupt = 0;

task *TS_ScheduleTask(void (*function)(task *), int rate, int priority, void *data) {
	int i;
	for (i = 0; i < NTASKS; i++)
		if (!task_used[i]) {
			task_used[i] = 1;
			tasks[i].TaskService = function;
			tasks[i].rate = rate;
			tasks[i].priority = priority;
			tasks[i].data = data;
			tasks[i].count = 0;
			tasks[i].active = 0;
			task_next[i] = stream_us;
			return &tasks[i];
		}
	return NULL;
}
int TS_Terminate(task *t) {
	int i;
	for (i = 0; i < NTASKS; i++) if (&tasks[i] == t) task_used[i] = 0;
	return TASK_Ok;
}
void TS_Dispatch(void) {
	int i;
	for (i = 0; i < NTASKS; i++) if (task_used[i]) tasks[i].active = 1;
}
void TS_SetTaskRate(task *t, int rate) { t->rate = rate; }
void TS_Shutdown(void) {
	int i;
	for (i = 0; i < NTASKS; i++) task_used[i] = 0;
}
int  TS_LockMemory(void) { return TASK_Ok; }
void TS_UnlockMemory(void) {}

int USER_CheckParameter(const char *parameter) { (void) parameter; return 0; }

// What audiolib asked of DOS: memory locked against paging, and interrupts
// off around what its timer task touched. The task runs from the poll
// here, never in the middle of anything, so none of it is needed. (Not
// through dpmi.h, whose register struct has fields named CS, SP and SS,
// which duke3d.h, reached through music.h, has made macros.)
int DPMI_LockMemory(void *address, unsigned length) { return 0; }
int DPMI_LockMemoryRegion(void *start, void *end) { return 0; }
int DPMI_UnlockMemory(void *address, unsigned length) { return 0; }
int DPMI_UnlockMemoryRegion(void *start, void *end) { return 0; }
unsigned long DisableInterrupts(void) { return 0; }
void RestoreInterrupts(unsigned long flags) { (void) flags; }

// General MIDI: when the card has a synth on its MIDI port (the person
// chose General MIDI), midi.c's events go there as they went to an
// MPU-401 (mpu401.c), each a message on the card's stream, and midi.c
// plays the score's General MIDI tracks (EMIDI) instead of its AdLib's.
// midi.c sets the channels' volumes itself, as it did for an MPU-401.
int c4_music_gm;                         // midi.c's too: it picks the General MIDI tracks
static void gm_noteoff(int ch, int key, int vel) { snd_midi(0x80 | ch, key, vel); }
static void gm_noteon(int ch, int key, int vel) { snd_midi(0x90 | ch, key, vel); }
static void gm_polyaftertouch(int ch, int key, int pressure) { snd_midi(0xa0 | ch, key, pressure); }
static void gm_controlchange(int ch, int number, int value) { snd_midi(0xb0 | ch, number, value); }
static void gm_programchange(int ch, int program) { snd_midi(0xc0 | ch, program, 0); }
static void gm_channelaftertouch(int ch, int pressure) { snd_midi(0xd0 | ch, pressure, 0); }
static void gm_pitchbend(int ch, int lsb, int msb) { snd_midi(0xe0 | ch, lsb, msb); }
static char gm_on[6] = { (char) 0xf0, 0x7e, 0x7f, 0x09, 0x01, (char) 0xf7 };   // General MIDI System On

// al_midi.c's AdLib, at 0x388 (register) and 0x389 (data): the card's OPL
static int opl_reg;
void duke_outp(int port, int value) {
	if ((port & 1) == 0) opl_reg = value & 255;
	else snd_opl(opl_reg, value & 255);
}

// ---- the effects ----

enum { NVOICES = SND_CHANNELS };
typedef struct {
	int handle;              // 0: free
	int priority;
	uint32_t callbackval;
	int loop;
	char *data;              // the samples, unsigned 8-bit mono
	int len, rate, vol;
	char *own;               // converted samples this voice allocated, or 0
} fxvoice;
static fxvoice voice[NVOICES];
static int next_handle = 1;
static int fx_volume = 255, fx_reverse;
static void (*fx_callback)(int32_t);

static void fx_end(int i) {
	fxvoice *v = &voice[i];
	uint32_t cb = v->callbackval;
	int was = v->handle;
	if (v->own) free(v->own);
	v->own = 0;
	v->handle = 0;
	if (was && fx_callback) fx_callback(cb);
}

// voices whose channel has gone quiet: looped ones play again, the rest
// are finished, and the game is told (SD_MakeCacheable)
static void fx_reap(void) {
	int i, busy = snd_busy();
	for (i = 0; i < NVOICES; i++)
		if (voice[i].handle && !(busy & (1 << i))) {
			if (voice[i].loop) snd_play(i, voice[i].data, voice[i].len, voice[i].rate, voice[i].vol);
			else fx_end(i);
		}
}

// a channel for a sound of this priority: a free one, else the lowest
// playing at that priority or below; -1 if there is none
static int fx_channel(int priority) {
	int i, low = -1;
	fx_reap();
	for (i = 0; i < NVOICES; i++) if (!voice[i].handle) return i;
	for (i = 0; i < NVOICES; i++)
		if (voice[i].priority <= priority && (low < 0 || voice[i].priority < voice[low].priority)) low = i;
	return low;
}

static int fx_find(int handle) {
	int i;
	if (handle <= 0) return -1;
	for (i = 0; i < NVOICES; i++) if (voice[i].handle == handle) return i;
	return -1;
}

// left and right (0-255 each) as the card wants them, the volume and the
// reversal applied
static int fx_lr(int left, int right) {
	int t;
	if (left < 0) left = 0;
	if (left > 255) left = 255;
	if (right < 0) right = 0;
	if (right > 255) right = 255;
	if (fx_reverse) { t = left; left = right; right = t; }
	left = left * fx_volume / 255;
	right = right * fx_volume / 255;
	return left | (right << 8);
}

// the rate moved by PITCHOFFSET cents, as audiolib's PITCH_GetScale did
static int fx_pitched(int rate, int cents) {
	static int semitone[13] = {
		65536, 69433, 73562, 77936, 82570, 87480, 92682,
		98193, 104032, 110218, 116773, 123716, 131072 };
	int oct, n, s, scale;
	if (cents == 0) return rate;
	oct = cents / 1200;
	n = cents - oct * 1200;
	if (n < 0) { n += 1200; oct--; }
	s = n / 100;
	scale = semitone[s] + (semitone[s + 1] - semitone[s]) * (n % 100) / 100;
	rate = (int) (((long long) rate * scale) >> 16);
	if (oct > 0) rate <<= oct;
	if (oct < 0) rate >>= -oct;
	return rate;
}

// audiolib's MV_Pan3D and MV_CalcPanTable: 32 angles round the listener,
// 0 ahead and 8 to the right; the far ear's level falls to nothing over a
// quarter turn, and distance (0-255) takes the level down for both
static void fx_pan3d(int angle, int distance, int *left, int *right) {
	int level, i, d;
	if (distance < 0) { distance = -distance; angle += 16; }
	if (distance > 255) distance = 255;
	level = 255 - distance;
	i = angle & 31;
	*left = *right = level;
	if (i <= 16) {
		d = i <= 8 ? i : 16 - i;
		*left = level - level * d / 8;
	} else {
		d = i <= 22 ? i - 16 : i <= 24 ? 8 : 31 - i;
		*right = level - level * d / 8;
	}
}

static int le16(unsigned char *p) { return p[0] | (p[1] << 8); }
static long le32(unsigned char *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((long) p[3] << 24); }

// A Creative Voice File: its sound data, (type 1) blocks with their
// continuations (type 2), or a type 9 block of 8-bit mono. *own is set
// when the blocks had to be joined into memory of its own.
static int voc_parse(unsigned char *p, char **data, int *len, int *rate, char **own) {
	unsigned char *q, *body;
	char *join;
	int type, blen, n, total, codec;
	*own = 0;
	if (memcmp(p, "Creative Voice File", 19) != 0) return 0;
	q = p + le16(p + 20);
	total = 0; n = 0; *rate = 0; *data = 0;
	// first pass: how much there is, and where the first block's samples are
	for (body = q; (type = body[0]) != 0; body = body + 4 + blen) {
		blen = body[1] | (body[2] << 8) | (body[3] << 16);
		if (type == 1) {
			codec = body[5];
			if (codec != 0) return 0;              // only unsigned 8-bit PCM
			if (!*rate) *rate = 1000000 / (256 - body[4]);
			if (!n) *data = (char *) body + 6;
			total += blen - 2; n++;
		} else if (type == 2) {
			total += blen; n++;
		} else if (type == 9) {
			if (body[8] != 8 || body[9] != 1) return 0;   // 8 bits, mono
			*rate = (int) le32(body + 4);
			if (!n) *data = (char *) body + 16;
			total += blen - 12; n++;
		} else if (type == 7) break;                    // end of a repeat: the first pass is enough
	}
	if (!n || !*rate) return 0;
	*len = total;
	if (n == 1) return 1;
	if (!(join = malloc(total))) return 0;
	total = 0;
	for (body = q; (type = body[0]) != 0 && type != 7; body = body + 4 + blen) {
		blen = body[1] | (body[2] << 8) | (body[3] << 16);
		if (type == 1) { memcpy(join + total, body + 6, blen - 2); total += blen - 2; }
		else if (type == 2) { memcpy(join + total, body + 4, blen); total += blen; }
		else if (type == 9) { memcpy(join + total, body + 16, blen - 12); total += blen - 12; }
	}
	*data = join; *own = join;
	return 1;
}

// A RIFF WAVE: 8-bit as it is; 16-bit, or stereo, made 8-bit mono in
// memory of its own
static int wav_parse(unsigned char *p, char **data, int *len, int *rate, char **own) {
	unsigned char *q, *end, *pcm = 0;
	long n = 0, size;
	int channels = 0, bits = 0, i, frames, v;
	char *out;
	*own = 0;
	if (memcmp(p, "RIFF", 4) != 0 || memcmp(p + 8, "WAVE", 4) != 0) return 0;
	end = p + 8 + le32(p + 4);
	for (q = p + 12; q + 8 <= end; q = q + 8 + ((size + 1) & ~1)) {
		size = le32(q + 4);
		if (memcmp(q, "fmt ", 4) == 0) {
			if (le16(q + 8) != 1) return 0;      // PCM
			channels = le16(q + 10);
			*rate = (int) le32(q + 12);
			bits = le16(q + 22);
		} else if (memcmp(q, "data", 4) == 0) { pcm = q + 8; n = size; }
	}
	if (!pcm || !channels || (bits != 8 && bits != 16)) return 0;
	if (bits == 8 && channels == 1) { *data = (char *) pcm; *len = (int) n; return 1; }
	frames = (int) (n / (channels * (bits / 8)));
	if (!(out = malloc(frames))) return 0;
	for (i = 0; i < frames; i++) {
		if (bits == 8) v = (pcm[i * channels] + pcm[i * channels + channels - 1]) / 2;
		else {
			v = (short) le16(pcm + i * channels * 2) + (short) le16(pcm + (i * channels + channels - 1) * 2);
			v = (v / 2 >> 8) + 128;
		}
		out[i] = (char) v;
	}
	*data = out; *len = frames; *own = out;
	return 1;
}

static int fx_start(char *data, int len, int rate, char *own, int pitchoffset, int left, int right,
                    int priority, uint32_t callbackval, int loop) {
	int i;
	fxvoice *v;
	if (!fx_on || len <= 0) { if (own) free(own); return FX_Warning; }
	if ((i = fx_channel(priority)) < 0) { if (own) free(own); return FX_Warning; }
	if (voice[i].handle) { snd_stop(i); fx_end(i); }
	v = &voice[i];
	v->handle = next_handle++;
	if (next_handle > 0x7fff) next_handle = 1;
	v->priority = priority;
	v->callbackval = callbackval;
	v->loop = loop;
	v->data = data; v->len = len; v->own = own;
	v->rate = fx_pitched(rate, pitchoffset);
	v->vol = fx_lr(left, right);
	snd_play(i, v->data, v->len, v->rate, v->vol);
	return v->handle;
}

static int fx_play(char *ptr, int pitchoffset, int left, int right, int priority,
                   uint32_t callbackval, int loop) {
	char *data, *own;
	int len, rate;
	if (!fx_on) return FX_Warning;
	if (!voc_parse((unsigned char *) ptr, &data, &len, &rate, &own) &&
	    !wav_parse((unsigned char *) ptr, &data, &len, &rate, &own)) return FX_Error;
	return fx_start(data, len, rate, own, pitchoffset, left, right, priority, callbackval, loop);
}

char *FX_ErrorString(int ErrorNumber) {
	if (ErrorNumber == FX_Warning || ErrorNumber == FX_Error) ErrorNumber = fx_error;
	if (ErrorNumber == FX_Ok) return "Fx ok.";
	if (ErrorNumber == FX_SoundCardError) return "The machine has no sound card with sample channels.";
	return "Unknown Fx error code.";
}
// Duke asks a Sound Blaster first (sounds.c, for SoundBlaster and AWE32
// in its settings): the card here is libjs's, whatever was chosen
int FX_GetBlasterSettings(fx_blaster_config *blaster) { (void) blaster; return FX_Error; }
int FX_SetupSoundBlaster(fx_blaster_config blaster, int *MaxVoices, int *MaxSampleBits, int *MaxChannels) {
	(void) blaster;
	*MaxVoices = NVOICES;
	*MaxSampleBits = 8;
	*MaxChannels = 2;
	return FX_Ok;
}
int FX_SetupCard(int SoundCard, fx_device *device) {
	(void) SoundCard;
	if (!snd_present() || !(snd_caps() & SND_HAS_PCM)) { fx_error = FX_SoundCardError; return FX_Error; }
	device->MaxVoices = NVOICES;
	device->MaxSampleBits = 8;
	device->MaxChannels = 2;
	return FX_Ok;
}
int FX_Init(int SoundCard, int numvoices, int numchannels, int samplebits, unsigned mixrate) {
	(void) SoundCard; (void) numvoices; (void) numchannels; (void) samplebits; (void) mixrate;
	// no card: the game plays silent rather than stopping (sounds.c calls
	// Error when this fails)
	if (!snd_present() || !(snd_caps() & SND_HAS_PCM)) { fx_error = FX_SoundCardError; return FX_Ok; }
	if (fx_on) FX_Shutdown();
	memset(voice, 0, sizeof voice);
	fx_on = 1;
	return FX_Ok;
}
int FX_Shutdown(void) {
	if (fx_on) FX_StopAllSounds();
	fx_on = 0;
	return FX_Ok;
}
int  FX_SetCallBack(void (*function)(int32_t)) { fx_callback = function; return FX_Ok; }
void FX_SetVolume(int volume) { fx_volume = volume < 0 ? 0 : volume > 255 ? 255 : volume; }
int  FX_GetVolume(void) { return fx_volume; }
void FX_SetReverseStereo(int setting) { fx_reverse = setting; }
int  FX_GetReverseStereo(void) { return fx_reverse; }
void FX_SetReverb(int reverb) { (void) reverb; }
void FX_SetFastReverb(int reverb) { (void) reverb; }
int  FX_GetMaxReverbDelay(void) { return 0; }
int  FX_GetReverbDelay(void) { return 0; }
void FX_SetReverbDelay(int delay) { (void) delay; }
int  FX_VoiceAvailable(int priority) {
	int i;
	if (!fx_on) return 0;
	fx_reap();
	for (i = 0; i < NVOICES; i++) if (!voice[i].handle || voice[i].priority <= priority) return 1;
	return 0;
}
int  FX_EndLooping(int handle) {
	int i = fx_find(handle);
	if (i >= 0) voice[i].loop = 0;
	return FX_Ok;
}
int  FX_SetPan(int handle, int vol, int left, int right) {
	int i = fx_find(handle);
	(void) vol;
	if (i < 0) return FX_Warning;
	voice[i].vol = fx_lr(left, right);
	snd_volume(i, voice[i].vol);
	return FX_Ok;
}
// a sound's pitch is its rate, and the card takes that when it starts
int  FX_SetPitch(int handle, int pitchoffset) { (void) handle; (void) pitchoffset; return FX_Ok; }
int  FX_SetFrequency(int handle, int frequency) { (void) handle; (void) frequency; return FX_Ok; }
int  FX_PlayVOC(uint8_t *ptr, int pitchoffset, int vol, int left, int right, int priority, uint32_t callbackval) {
	(void) vol;
	return fx_play((char *) ptr, pitchoffset, left, right, priority, callbackval, 0);
}
int  FX_PlayLoopedVOC(uint8_t *ptr, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
                      int32_t left, int32_t right, int32_t priority, uint32_t callbackval) {
	(void) loopstart; (void) loopend; (void) vol;
	return fx_play((char *) ptr, pitchoffset, left, right, priority, callbackval, 1);
}
int  FX_PlayWAV(uint8_t *ptr, int pitchoffset, int vol, int left, int right, int priority, uint32_t callbackval) {
	return FX_PlayVOC(ptr, pitchoffset, vol, left, right, priority, callbackval);
}
int  FX_PlayLoopedWAV(uint8_t *ptr, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
                      int32_t left, int32_t right, int32_t priority, uint32_t callbackval) {
	return FX_PlayLoopedVOC(ptr, loopstart, loopend, pitchoffset, vol, left, right, priority, callbackval);
}
int  FX_PlayVOC3D(uint8_t *ptr, int32_t pitchoffset, int32_t angle, int32_t distance, int32_t priority, uint32_t callbackval) {
	int left, right;
	fx_pan3d(angle, distance, &left, &right);
	return fx_play((char *) ptr, pitchoffset, left, right, priority, callbackval, 0);
}
int  FX_PlayWAV3D(uint8_t *ptr, int pitchoffset, int angle, int distance, int priority, uint32_t callbackval) {
	return FX_PlayVOC3D(ptr, pitchoffset, angle, distance, priority, callbackval);
}
int  FX_PlayRaw(uint8_t *ptr, uint32_t length, uint32_t rate, int32_t pitchoffset, int32_t vol, int32_t left,
                int32_t right, int32_t priority, uint32_t callbackval) {
	(void) vol;
	return fx_start((char *) ptr, (int) length, (int) rate, 0, pitchoffset, left, right, priority, callbackval, 0);
}
int  FX_PlayLoopedRaw(uint8_t *ptr, uint32_t length, char *loopstart, char *loopend, uint32_t rate,
                      int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval) {
	(void) loopstart; (void) loopend; (void) vol;
	return fx_start((char *) ptr, (int) length, (int) rate, 0, pitchoffset, left, right, priority, callbackval, 1);
}
int32_t FX_Pan3D(int handle, int angle, int distance) {
	int i = fx_find(handle), left, right;
	if (i < 0) return FX_Warning;
	fx_pan3d(angle, distance, &left, &right);
	voice[i].vol = fx_lr(left, right);
	snd_volume(i, voice[i].vol);
	return FX_Ok;
}
int32_t FX_SoundActive(int32_t handle) {
	int i = fx_find(handle);
	return i >= 0 && ((snd_busy() & (1 << i)) || voice[i].loop);
}
int32_t FX_SoundsPlaying(void) {
	int i, n = 0;
	if (!fx_on) return 0;
	fx_reap();
	for (i = 0; i < NVOICES; i++) if (voice[i].handle) n++;
	return n;
}
int32_t FX_StopSound(int handle) {
	int i = fx_find(handle);
	if (i < 0) return FX_Warning;
	snd_stop(i);
	fx_end(i);
	return FX_Ok;
}
int32_t FX_StopAllSounds(void) {
	int i;
	if (!fx_on) return FX_Ok;
	snd_stop(-1);
	for (i = 0; i < NVOICES; i++) if (voice[i].handle) fx_end(i);
	return FX_Ok;
}
int32_t FX_StartDemandFeedPlayback(void (*function)(char **ptr, uint32_t *length), int32_t rate, int32_t pitchoffset,
                                int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval) {
	(void) function; (void) rate; (void) pitchoffset; (void) vol; (void) left; (void) right;
	(void) priority; (void) callbackval;
	return FX_Error;
}
int  FX_StartRecording(int MixRate, void (*function)(char *ptr, int length)) { (void) MixRate; (void) function; return FX_Error; }
void FX_StopRecord(void) {}

// ---- the music: music.c's AdLib path ----

static midifuncs al_funcs;
static task *fade_task;
static int fade_rate, fade_length, fade_current, fade_last, fade_end;

char *MUSIC_ErrorString(int ErrorNumber) {
	if (ErrorNumber == MUSIC_Warning || ErrorNumber == MUSIC_Error) ErrorNumber = music_error;
	if (ErrorNumber == MUSIC_Ok) return "Music ok.";
	if (ErrorNumber == MUSIC_SoundCardError) return "The machine has no sound card with an OPL or a MIDI synth.";
	if (ErrorNumber == MUSIC_MidiError) return "Error playing MIDI file.";
	return "Unknown Music error code.";
}
int MUSIC_Init(int SoundCard, int Address) {
	int i;
	(void) SoundCard; (void) Address;
	c4_music_gm = snd_present() && (snd_caps() & SND_HAS_MIDI);
	if (!c4_music_gm && (!snd_present() || !(snd_caps() & SND_HAS_OPL))) { music_error = MUSIC_SoundCardError; return MUSIC_Ok; }
	snd_clock(1000000);                     // snd_wait in microseconds
	for (i = 0; i < 128; i++) MIDI_PatchMap[i] = i;
	memset(&al_funcs, 0, sizeof al_funcs);
	if (c4_music_gm) {
		snd_midi_bytes(gm_on, sizeof gm_on);
		al_funcs.NoteOff = gm_noteoff;
		al_funcs.NoteOn = gm_noteon;
		al_funcs.PolyAftertouch = gm_polyaftertouch;
		al_funcs.ControlChange = gm_controlchange;
		al_funcs.ProgramChange = gm_programchange;
		al_funcs.ChannelAftertouch = gm_channelaftertouch;
		al_funcs.PitchBend = gm_pitchbend;
		MIDI_SetMidiFuncs(&al_funcs);
		music_on = 1;
		return MUSIC_Ok;
	}
	AL_Init(0);                             // an AdLib whatever the card (al_midi.c)
	al_funcs.NoteOff = AL_NoteOff;
	al_funcs.NoteOn = AL_NoteOn;
	al_funcs.ControlChange = AL_ControlChange;
	al_funcs.ProgramChange = AL_ProgramChange;
	al_funcs.PitchBend = AL_SetPitchBend;
	MIDI_SetMidiFuncs(&al_funcs);
	music_on = 1;
	return MUSIC_Ok;
}
int MUSIC_Shutdown(void) {
	if (!music_on) return MUSIC_Ok;
	MIDI_StopSong();
	MUSIC_StopFade();
	if (!c4_music_gm) AL_Shutdown();
	music_on = 0;
	return MUSIC_Ok;
}
void MUSIC_SetMaxFMMidiChannel(int channel) { if (music_on && !c4_music_gm) AL_SetMaxMidiChannel(channel); }
void MUSIC_SetVolume(int volume) {
	volume = volume < 0 ? 0 : volume > 255 ? 255 : volume;
	if (music_on) MIDI_SetVolume(volume);
}
void MUSIC_SetMidiChannelVolume(int channel, int volume) { if (music_on) MIDI_SetUserChannelVolume(channel, volume); }
void MUSIC_ResetMidiChannelVolumes(void) { if (music_on) MIDI_ResetUserChannelVolume(); }
int  MUSIC_GetVolume(void) { return music_on ? MIDI_GetVolume() : 0; }
void MUSIC_SetLoopFlag(int loopflag) { if (music_on) MIDI_SetLoopFlag(loopflag); }
int  MUSIC_SongPlaying(void) { return music_on ? MIDI_SongPlaying() : 0; }
void MUSIC_Continue(void) { if (music_on) MIDI_ContinueSong(); }
void MUSIC_Pause(void) { if (music_on) MIDI_PauseSong(); }
int  MUSIC_StopSong(void) {
	if (!music_on) return MUSIC_Ok;
	MUSIC_StopFade();
	MIDI_StopSong();
	return MUSIC_Ok;
}
int  MUSIC_PlaySong(char *song, int loopflag) {
	if (!music_on) return MUSIC_Warning;
	MIDI_StopSong();
	if (MIDI_PlaySong((unsigned char *) song, loopflag) != MIDI_Ok) { music_error = MUSIC_MidiError; return MUSIC_Warning; }
	return MUSIC_Ok;
}

// sounds.c's playmusic: a song of the GRP's, read whole and played
// looped, as sdl_midi.c did
void PlayMusic(char *filename) {
	static char *song;
	static int songsize;
	int fd, len;
	if (!music_on) return;
	if ((fd = kopen4load(filename, 0)) < 0) return;
	len = kfilelength(fd);
	MIDI_StopSong();
	if (len > songsize) {
		free(song);
		songsize = 0;
		if (!(song = malloc(len))) { kclose(fd); return; }
		songsize = len;
	}
	kread(fd, song, len);
	kclose(fd);
	MUSIC_PlaySong(song, MUSIC_LoopSong);
}
void MUSIC_SetContext(int context) { if (music_on) MIDI_SetContext(context); }
int  MUSIC_GetContext(void) { return music_on ? MIDI_GetContext() : 0; }
void MUSIC_SetSongTick(uint32_t PositionInTicks) { if (music_on) MIDI_SetSongTick(PositionInTicks); }
void MUSIC_SetSongTime(uint32_t milliseconds) { if (music_on) MIDI_SetSongTime(milliseconds); }
void MUSIC_SetSongPosition(int measure, int beat, int tick) { if (music_on) MIDI_SetSongPosition(measure, beat, tick); }
void MUSIC_GetSongPosition(songposition *pos) {
	if (music_on) MIDI_GetSongPosition(pos); else memset(pos, 0, sizeof *pos);
}
void MUSIC_GetSongLength(songposition *pos) {
	if (music_on) MIDI_GetSongLength(pos); else memset(pos, 0, sizeof *pos);
}

// music.c's fade: the volume stepped 40 times a second, on the task manager
static void MUSIC_FadeRoutine(task *Task) {
	int volume;
	fade_current += fade_rate;
	if (fade_length == 0) {
		MIDI_SetVolume(fade_end);
		TS_Terminate(Task);
		fade_task = NULL;
	} else {
		fade_length--;
		volume = fade_current >> 7;
		if (fade_last != volume) {
			fade_last = volume;
			MIDI_SetVolume(volume);
		}
	}
}
int  MUSIC_FadeVolume(int tovolume, int milliseconds) {
	int fromvolume;
	if (!music_on) return MUSIC_Ok;
	if (fade_task) MUSIC_StopFade();
	tovolume = tovolume < 0 ? 0 : tovolume > 255 ? 255 : tovolume;
	fromvolume = MUSIC_GetVolume();
	fade_length = milliseconds / 25;
	if (fade_length <= 0) { MIDI_SetVolume(tovolume); return MUSIC_Ok; }
	fade_rate = ((tovolume - fromvolume) << 7) / fade_length;
	fade_last = fromvolume;
	fade_current = fromvolume << 7;
	fade_end = tovolume;
	if (!(fade_task = TS_ScheduleTask(MUSIC_FadeRoutine, 40, 1, NULL))) { MIDI_SetVolume(tovolume); return MUSIC_Ok; }
	TS_Dispatch();
	return MUSIC_Ok;
}
int  MUSIC_FadeActive(void) { return fade_task != NULL; }
void MUSIC_StopFade(void) {
	if (fade_task) { TS_Terminate(fade_task); fade_task = NULL; }
}
void MUSIC_RerouteMidiChannel(int channel, int cdecl (*function)(int event, int c1, int c2)) {
	if (music_on) MIDI_RerouteMidiChannel(channel, function);
}
void MUSIC_RegisterTimbreBank(unsigned char *timbres) { AL_RegisterTimbreBank(timbres); }

// ---- the poll ----

// Called whenever the game reads the clock or its input, and with every
// frame (duke-display.c): sounds that have finished are let go, and the music's tasks
// run in stream time until the card's stream is AHEAD_US ahead of now. A
// stream that fell behind (a level loading) the card brings up to now,
// so the music pauses rather than catching up.
void duke_snd_poll(void) {
	static int polling;
	int i, k, n;
	if (polling || (!fx_on && !music_on)) return;
	polling = 1;
	if (fx_on) fx_reap();
	// the stream's clock is a 32-bit count of microseconds: moved back,
	// with the tasks' times, long before it could wrap (36 minutes)
	if (stream_us > 1000000000) {
		for (i = 0; i < NTASKS; i++) task_next[i] -= 1000000000;
		stream_us -= 1000000000;
	}
	for (n = 0; music_on && n < MAXSTEPS && snd_ahead() < AHEAD_US; n++) {
		k = -1;
		for (i = 0; i < NTASKS; i++)
			if (task_used[i] && tasks[i].active && tasks[i].rate > 0 && (k < 0 || task_next[i] < task_next[k])) k = i;
		if (k < 0) break;
		if (task_next[k] > stream_us) {
			snd_wait((int) (task_next[k] - stream_us));
			stream_us = task_next[k];
		}
		task_next[k] += 1000000 / tasks[k].rate;
		tasks[k].TaskService(&tasks[k]);
	}
	polling = 0;
}

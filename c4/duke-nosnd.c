// duke-nosnd.c -- audiolib's FX_ (effects) and MUSIC_ (MIDI) interfaces,
// silent: for gcc's build on a host, the reference of the test, which plays
// with sound and music off (/ns /nm). c4's builds have duke-snd.c instead.
#include <stdint.h>
#include "audiolib/fx_man.h"
#include "audiolib/music.h"

int MUSIC_ErrorCode = MUSIC_Ok;

char *FX_ErrorString (int ErrorNumber) { return "no sound"; }
int FX_SetupCard (int SoundCard, fx_device *device) { return FX_Ok; }
int FX_GetBlasterSettings (fx_blaster_config *blaster) { return FX_Error; }
int FX_SetupSoundBlaster (fx_blaster_config blaster, int *MaxVoices, int *MaxSampleBits, int *MaxChannels) { return FX_Ok; }
int FX_Init (int SoundCard, int numvoices, int numchannels, int samplebits, unsigned mixrate) { return FX_Ok; }
int FX_Shutdown (void) { return FX_Ok; }
int FX_SetCallBack (void (*function) (int32_t)) { return FX_Ok; }
void FX_SetVolume (int volume) {}
int FX_GetVolume (void) { return 0; }
void FX_SetReverseStereo (int setting) {}
int FX_GetReverseStereo (void) { return 0; }
void FX_SetReverb (int reverb) {}
void FX_SetFastReverb (int reverb) {}
int FX_GetMaxReverbDelay (void) { return 0; }
int FX_GetReverbDelay (void) { return 0; }
void FX_SetReverbDelay (int delay) {}
int FX_VoiceAvailable (int priority) { return 0; }
int FX_EndLooping (int handle) { return FX_Ok; }
int FX_SetPan (int handle, int vol, int left, int right) { return FX_Ok; }
int FX_SetPitch (int handle, int pitchoffset) { return FX_Ok; }
int FX_SetFrequency (int handle, int frequency) { return FX_Ok; }
int FX_PlayVOC (uint8_t *ptr, int pitchoffset, int vol, int left, int right, int priority, uint32_t callbackval) { return FX_Warning; }
int FX_PlayLoopedVOC (uint8_t *ptr, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
	int32_t left, int32_t right, int32_t priority, uint32_t callbackval) { return FX_Warning; }
int FX_PlayWAV (uint8_t *ptr, int pitchoffset, int vol, int left, int right, int priority, uint32_t callbackval) { return FX_Warning; }
int FX_PlayLoopedWAV (uint8_t *ptr, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
	int32_t left, int32_t right, int32_t priority, uint32_t callbackval) { return FX_Warning; }
int FX_PlayVOC3D (uint8_t *ptr, int32_t pitchoffset, int32_t angle, int32_t distance, int32_t priority, uint32_t callbackval) { return FX_Warning; }
int FX_PlayWAV3D (uint8_t *ptr, int pitchoffset, int angle, int distance, int priority, uint32_t callbackval) { return FX_Warning; }
int FX_PlayRaw (uint8_t *ptr, uint32_t length, uint32_t rate, int32_t pitchoffset, int32_t vol, int32_t left,
	int32_t right, int32_t priority, uint32_t callbackval) { return FX_Warning; }
int FX_PlayLoopedRaw (uint8_t *ptr, uint32_t length, char *loopstart, char *loopend, uint32_t rate,
	int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval) { return FX_Warning; }
int32_t FX_Pan3D (int handle, int angle, int distance) { return FX_Ok; }
int32_t FX_SoundActive (int32_t handle) { return 0; }
int32_t FX_SoundsPlaying (void) { return 0; }
int32_t FX_StopSound (int handle) { return FX_Ok; }
int32_t FX_StopAllSounds (void) { return FX_Ok; }
int32_t FX_StartDemandFeedPlayback (void (*function) (char **ptr, uint32_t *length), int32_t rate,
	int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval) { return FX_Warning; }
int FX_StartRecording (int MixRate, void (*function) (char *ptr, int length)) { return FX_Error; }
void FX_StopRecord (void) {}

char *MUSIC_ErrorString (int ErrorNumber) { return "no music"; }
int MUSIC_Init (int SoundCard, int Address) { return MUSIC_Ok; }
int MUSIC_Shutdown (void) { return MUSIC_Ok; }
void MUSIC_SetMaxFMMidiChannel (int channel) {}
void MUSIC_SetVolume (int volume) {}
void MUSIC_SetMidiChannelVolume (int channel, int volume) {}
void MUSIC_ResetMidiChannelVolumes (void) {}
int MUSIC_GetVolume (void) { return 0; }
void MUSIC_SetLoopFlag (int loopflag) {}
int MUSIC_SongPlaying (void) { return 0; }
void MUSIC_Continue (void) {}
void MUSIC_Pause (void) {}
int MUSIC_StopSong (void) { return MUSIC_Ok; }
int MUSIC_PlaySong (char *songData, int loopflag) { return MUSIC_Ok; }
void MUSIC_SetContext (int context) {}
int MUSIC_GetContext (void) { return 0; }
void MUSIC_SetSongTick (uint32_t PositionInTicks) {}
void MUSIC_SetSongTime (uint32_t milliseconds) {}
void MUSIC_SetSongPosition (int measure, int beat, int tick) {}
void MUSIC_GetSongPosition (songposition *pos) {}
void MUSIC_GetSongLength (songposition *pos) {}
int MUSIC_FadeVolume (int tovolume, int milliseconds) { return MUSIC_Ok; }
int MUSIC_FadeActive (void) { return 0; }
void MUSIC_StopFade (void) {}
void MUSIC_RerouteMidiChannel (int channel, int cdecl (*function) (int event, int c1, int c2)) {}
void MUSIC_RegisterTimbreBank (unsigned char *timbres) {}
void PlayMusic (char *filename) {}
void duke_snd_poll (void) {}

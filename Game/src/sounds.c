//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
aint32_t with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#if PLATFORM_DOS
#include <conio.h>
#endif

#include <stdio.h>
#include <string.h>
#include "../../Game/src/types.h"
#include "util_lib.h"
#include "duke3d.h"
#include "global.h"
#include "filesystem.h"


#define LOUDESTVOLUME 150

int32_t backflag,numenvsnds;

/*
===================
=
= SoundStartup
=
===================
*/

void SoundStartup( void )
   {
   int32 status;

   // if they chose None lets return
   if (FXDevice == NumSoundCards) return;

   // Do special Sound Blaster, AWE32 stuff
   if (
         ( FXDevice == SoundBlaster ) ||
         ( FXDevice == Awe32 )
      )
      {
      int MaxVoices;
      int MaxBits;
      int MaxChannels;

      status = FX_SetupSoundBlaster
                  (
                  BlasterConfig, (int *)&MaxVoices, (int *)&MaxBits, (int *)&MaxChannels
                  );
      }
   else
      {
      status = FX_Ok;
      }

   if ( status == FX_Ok )
      {
      if ( eightytwofifty && numplayers > 1)
         {
         status = FX_Init( FXDevice, min( NumVoices,4 ), 1, 8, 8000 );
         }
      else
         {
         status = FX_Init( FXDevice, NumVoices, NumChannels, NumBits, MixRate );
         }
      if ( status == FX_Ok )
         {

         FX_SetVolume( FXVolume );
         if (ReverseStereo == 1)
            {
            FX_SetReverseStereo(!FX_GetReverseStereo());
            }
         }
      }
   if ( status != FX_Ok )
      {
      Error(EXIT_FAILURE, FX_ErrorString( FX_Error ));
      }

   status = FX_SetCallBack( TestCallBack );

   if ( status != FX_Ok )
      {
      Error(EXIT_FAILURE, FX_ErrorString( FX_Error ));
      }
   }

/*
===================
=
= SoundShutdown
=
===================
*/

void SoundShutdown( void )
   {
   int32 status;

   // if they chose None lets return
   if (FXDevice == NumSoundCards)
      return;

   status = FX_Shutdown();
   if ( status != FX_Ok )
      {
      Error(EXIT_FAILURE, FX_ErrorString( FX_Error ));
      }
   }

/*
===================
=
= MusicStartup
=
===================
*/

void MusicStartup( void )
   {
   int32 status;

   // if they chose None lets return
   if ((MusicDevice == NumSoundCards) || (eightytwofifty && numplayers > 1) )
      return;

   // satisfy AWE32 and WAVEBLASTER stuff
   BlasterConfig.Midi = MidiPort;

   // Do special Sound Blaster, AWE32 stuff
   if (
         ( FXDevice == SoundBlaster ) ||
         ( FXDevice == Awe32 )
      )
      {
      int MaxVoices;
      int MaxBits;
      int MaxChannels;

      FX_SetupSoundBlaster
                  (
                  BlasterConfig, (int *)&MaxVoices, (int *)&MaxBits, (int *)&MaxChannels
                  );
      }
   status = MUSIC_Init( MusicDevice, MidiPort );

   if ( status == MUSIC_Ok )
      {
      MUSIC_SetVolume( MusicVolume );
      }
   else
   {
      SoundShutdown();
      uninittimer();
      uninitengine();
      CONTROL_Shutdown();
      CONFIG_WriteSetup();
      KB_Shutdown();
      uninitgroupfile();
      unlink("duke3d.tmp");
      Error(EXIT_FAILURE, "Couldn't find selected sound card, or, error w/ sound card itself\n");
   }
}

/*
===================
=
= MusicShutdown
=
===================
*/

void MusicShutdown( void )
   {
   int32 status;

   // if they chose None lets return
   if ((MusicDevice == NumSoundCards) || (eightytwofifty && numplayers > 1) )
      return;

   status = MUSIC_Shutdown();
   if ( status != MUSIC_Ok )
      {
      //Error( MUSIC_ErrorString( MUSIC_ErrorCode ));
      }
   }


int USRHOOKS_GetMem(void  **ptr, uint32_t size )
{
   *ptr = malloc(size);

   if (*ptr == NULL)
      return(USRHOOKS_Error);

   return( USRHOOKS_Ok);

}

int USRHOOKS_FreeMem(void  *ptr)
{
   free(ptr);
   return( USRHOOKS_Ok);
}

uint8_t  menunum=0;

void intomenusounds(void)
{
    static const short menusnds[] =
    {
        LASERTRIP_EXPLODE,
        DUKE_GRUNT,
        DUKE_LAND_HURT,
        CHAINGUN_FIRE,
        SQUISHED,
        KICK_HIT,
        PISTOL_RICOCHET,
        PISTOL_BODYHIT,
        PISTOL_FIRE,
        SHOTGUN_FIRE,
        BOS1_WALK,
        RPG_EXPLODE,
        PIPEBOMB_BOUNCE,
        PIPEBOMB_EXPLODE,
        NITEVISION_ONOFF,
        RPG_SHOOT,
        SELECT_WEAPON
    };
    sound(menusnds[(int)menunum++]);
    menunum %= 17;
}

void playmusic(char  *fn)
{
    if(MusicToggle == 0) return;
    if(MusicDevice == NumSoundCards) return;

    // the SDL_mixer version does more or less this same thing.  --ryan.
    PlayMusic(fn);
}

uint8_t  loadsound(uint16_t num)
{
    int32_t   fp, l;

    if(num >= NUM_SOUNDS || SoundToggle == 0) return 0;
    if (FXDevice == NumSoundCards) return 0;

    fp = TCkopen4load(sounds[num],0);
    if(fp == -1)
    {
        sprintf(&fta_quotes[113][0],"Sound %s(#%d) not found.",sounds[num],num);
        FTA(113,&ps[myconnectindex],1);
        return 0;
    }

    l = kfilelength( fp );
    soundsiz[num] = l;

    Sound[num].lock = 200;

    allocache(&Sound[num].ptr,l,(uint8_t  *)&Sound[num].lock);
    kread( fp, Sound[num].ptr , l);
    kclose( fp );
    return 1;
}

int xyzsound(short num,short i,int32_t x,int32_t y,int32_t z)
{
    int32_t sndist, cx, cy, cz, j,k;
    short pitche,pitchs,cs;
    int voice, sndang, ca, pitch;
	
    if( num >= NUM_SOUNDS ||
        FXDevice == NumSoundCards ||
        ( (soundm[num]&8) && ud.lockout ) ||
        SoundToggle == 0 ||
#ifdef C4DUKE
        // c4: a sprite's sound with four of it playing replaces the oldest (below)
        ( Sound[num].num > 3 && PN == MUSICANDSFX ) ||
#else
        Sound[num].num > 3 ||
#endif
        FX_VoiceAvailable(soundpr[num]) == 0 ||
        (ps[myconnectindex].timebeforeexit > 0 && ps[myconnectindex].timebeforeexit <= 26*3) ||
        ps[myconnectindex].gm&MODE_MENU) return -1;

    if( soundm[num]&128 )
    {
        sound(num);
        return 0;
    }

    if( soundm[num]&4 )
    {
		// FIX_00041: Toggle to hear the opponent sound in DM (like it used to be in v1.3d)
		if(VoiceToggle==0 || (ud.multimode > 1 && PN == APLAYER && sprite[i].yvel != screenpeek && /*ud.coop!=1 &&*/ !OpponentSoundToggle) ) return -1; //xduke : 1.3d Style: makes opponent sound in DM as in COOP

        for(j=0;j<NUM_SOUNDS;j++)
          for(k=0;k<Sound[j].num;k++)
            if( (Sound[j].num > 0) && (soundm[j]&4) )
              return -1;
    }

    cx = ps[screenpeek].oposx;
    cy = ps[screenpeek].oposy;
    cz = ps[screenpeek].oposz;
    cs = ps[screenpeek].cursectnum;
    ca = ps[screenpeek].ang+ps[screenpeek].look_ang;

    sndist = FindDistance3D((cx-x),(cy-y),(cz-z)>>4);

    if( i >= 0 && (soundm[num]&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9 )
        sndist = divscale14(sndist,(SHT+1));

    pitchs = soundps[num];
    pitche = soundpe[num];
    cx = klabs(pitche-pitchs);

    if(cx)
    {
        if( pitchs < pitche )
             pitch = pitchs + ( rand()%cx );
        else pitch = pitche + ( rand()%cx );
    }
    else pitch = pitchs;

    sndist += soundvo[num];
    if(sndist < 0) sndist = 0;
    if( sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,SX,SY,SZ-(24<<8),SECT) )
        sndist += sndist>>5;

    switch(num)
    {
        case PIPEBOMB_EXPLODE:
        case LASERTRIP_EXPLODE:
        case RPG_EXPLODE:
            if(sndist > (6144) )
                sndist = 6144;
            if(sector[ps[screenpeek].cursectnum].lotag == 2)
                pitch -= 1024;
            break;
        default:
            if(sector[ps[screenpeek].cursectnum].lotag == 2 && (soundm[num]&4) == 0)
                pitch = -768;
            if( sndist > 31444 && PN != MUSICANDSFX)
                return -1;
            break;
    }


    if( Sound[num].num > 0 && PN != MUSICANDSFX )
    {
#ifdef C4DUKE
        // c4: the sprite's own sound of this kind stops, wherever its entry
        // is, as the game meant. Others' play on, up to the four it keeps
        // track of; a fifth stops the oldest. The original stopped the
        // newest as soon as two were playing (stopsound): with the counts
        // kept true, an explosion in a fight then cut one a tenth of a
        // second old, and with the DOS game's counts (TestCallBack) one
        // that had only just begun, every other time.
        for(j=0;j<Sound[num].num && SoundOwner[num][j].i != i;j++);
        if( j < Sound[num].num ) stopsoundat(num,j);
        else if( Sound[num].num > 3 ) stopsoundat(num,0);
        else if( badguy(&sprite[i]) && sprite[i].extra <= 0 ) stopsoundat(num,0);
#else
        if( SoundOwner[num][0].i == i ) stopsound(num);
        else if( Sound[num].num > 1 ) stopsound(num);
        else if( badguy(&sprite[i]) && sprite[i].extra <= 0 ) stopsound(num);
#endif
    }

    if( PN == APLAYER && sprite[i].yvel == screenpeek )
    {
        sndang = 0;
        sndist = 0;
    }
    else
    {
        sndang = 2048 + ca - getangle(cx-x,cy-y);
        sndang &= 2047;
    }

    if(Sound[num].ptr == 0) { if( loadsound(num) == 0 ) return 0; }
    else
    {
       if (Sound[num].lock < 200)
          Sound[num].lock = 200;
       else Sound[num].lock++;
    }

    if( soundm[num]&16 ) sndist = 0;

    if(sndist < ((255-LOUDESTVOLUME)<<6) )
        sndist = ((255-LOUDESTVOLUME)<<6);

    if( soundm[num]&1 )
    {
        uint16_t start;

        if(Sound[num].num > 0) return -1;

        start = *(uint16_t *)(Sound[num].ptr + 0x14);

        if(*Sound[num].ptr == 'C')
            voice = FX_PlayLoopedVOC( Sound[num].ptr, start, start + soundsiz[num],
                    pitch,sndist>>6,sndist>>6,0,soundpr[num],num);
        else
            voice = FX_PlayLoopedWAV( Sound[num].ptr, start, start + soundsiz[num],
                    pitch,sndist>>6,sndist>>6,0,soundpr[num],num);
    }
    else
    {
        if( *Sound[num].ptr == 'C')
            voice = FX_PlayVOC3D( Sound[ num ].ptr,pitch,sndang>>6,sndist>>6, soundpr[num], num );
        else voice = FX_PlayWAV3D( Sound[ num ].ptr,pitch,sndang>>6,sndist>>6, soundpr[num], num );
    }

    if ( voice > FX_Ok )
    {
        SoundOwner[num][Sound[num].num].i = i;
        SoundOwner[num][Sound[num].num].voice = voice;
        Sound[num].num++;
    }
    else Sound[num].lock--;
    return (voice);
}

void sound(short num)
{
    short pitch,pitche,pitchs,cx;
    int voice;
    int32_t start;

    if (FXDevice == NumSoundCards) return;
    if(SoundToggle==0) return;
    if(VoiceToggle==0 && (soundm[num]&4) ) return;
    if( (soundm[num]&8) && ud.lockout ) return;
    if(FX_VoiceAvailable(soundpr[num]) == 0) return;

    pitchs = soundps[num];
    pitche = soundpe[num];
    cx = klabs(pitche-pitchs);

    if(cx)
    {
        if( pitchs < pitche )
             pitch = pitchs + ( rand()%cx );
        else pitch = pitche + ( rand()%cx );
    }
    else pitch = pitchs;

    if(Sound[num].ptr == 0) { if( loadsound(num) == 0 ) return; }
    else
    {
       if (Sound[num].lock < 200)
          Sound[num].lock = 200;
       else Sound[num].lock++;
    }

    if( soundm[num]&1 )
    {
        if(*Sound[num].ptr == 'C')
        {
            start = (int32_t)*(uint16_t *)(Sound[num].ptr + 0x14);
            voice = FX_PlayLoopedVOC( Sound[num].ptr, start, start + soundsiz[num],
                    pitch,LOUDESTVOLUME,LOUDESTVOLUME,LOUDESTVOLUME,soundpr[num],num);
        }
        else
        {
            start = (int32_t)*(uint16_t *)(Sound[num].ptr + 0x14);
            voice = FX_PlayLoopedWAV( Sound[num].ptr, start, start + soundsiz[num],
                    pitch,LOUDESTVOLUME,LOUDESTVOLUME,LOUDESTVOLUME,soundpr[num],num);
        }
    }
    else
    {
        if(*Sound[num].ptr == 'C')
            voice = FX_PlayVOC3D( Sound[ num ].ptr, pitch,0,255-LOUDESTVOLUME,soundpr[num], num );
        else
            voice = FX_PlayWAV3D( Sound[ num ].ptr, pitch,0,255-LOUDESTVOLUME,soundpr[num], num );
    }

    if(voice > FX_Ok) return;
    Sound[num].lock--;
}

int spritesound(uint16_t num, short i)
{
    if(num >= NUM_SOUNDS) return -1;
    return xyzsound(num,i,SX,SY,SZ);
}

void stopsound(short num)
{
    if(Sound[num].num > 0)
    {
        FX_StopSound(SoundOwner[num][Sound[num].num-1].voice);
        testcallback(num);
    }
}

#ifdef C4DUKE
// c4: the sound of this kind that entry j plays (0 the oldest)
void stopsoundat(short num, short j)
{
    if(j < Sound[num].num)
    {
        FX_StopSound(SoundOwner[num][j].voice);
        testcallback(num);
    }
}
#endif

void stopenvsound(short num,short i)
{
    short j, k;

    if(Sound[num].num > 0)
    {
        k = Sound[num].num;
        for(j=0;j<k;j++)
           if(SoundOwner[num][j].i == i)
        {
            FX_StopSound(SoundOwner[num][j].voice);
            break;
        }
    }
}

void pan3dsound(void)
{
    int32_t sndist, sx, sy, sz, cx, cy, cz;
    short sndang,ca,j,k,i,cs;

    numenvsnds = 0;

    if(ud.camerasprite == -1)
    {
        cx = ps[screenpeek].oposx;
        cy = ps[screenpeek].oposy;
        cz = ps[screenpeek].oposz;
        cs = ps[screenpeek].cursectnum;
        ca = ps[screenpeek].ang+ps[screenpeek].look_ang;
    }
    else
    {
        cx = sprite[ud.camerasprite].x;
        cy = sprite[ud.camerasprite].y;
        cz = sprite[ud.camerasprite].z;
        cs = sprite[ud.camerasprite].sectnum;
        ca = sprite[ud.camerasprite].ang;
    }

    for(j=0;j<NUM_SOUNDS;j++) for(k=0;k<Sound[j].num;k++)
    {
        i = SoundOwner[j][k].i;

        sx = sprite[i].x;
        sy = sprite[i].y;
        sz = sprite[i].z;

        if( PN == APLAYER && sprite[i].yvel == screenpeek)
        {
            sndang = 0;
            sndist = 0;
        }
        else
        {
            sndang = 2048 + ca - getangle(cx-sx,cy-sy);
            sndang &= 2047;
            sndist = FindDistance3D((cx-sx),(cy-sy),(cz-sz)>>4);
            if( i >= 0 && (soundm[j]&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (sector[SECT].lotag&0xff) < 9 )
                sndist = divscale14(sndist,(SHT+1));
        }

        sndist += soundvo[j];
        if(sndist < 0) sndist = 0;

        if( sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,sx,sy,sz-(24<<8),SECT) )
            sndist += sndist>>5;

        if(PN == MUSICANDSFX && SLT < 999)
            numenvsnds++;

        switch(j)
        {
            case PIPEBOMB_EXPLODE:
            case LASERTRIP_EXPLODE:
            case RPG_EXPLODE:
                if(sndist > (6144)) sndist = (6144);
                break;
            default:
                if( sndist > 31444 && PN != MUSICANDSFX)
                {
                    stopsound(j);
                    continue;
                }
        }

        if(Sound[j].ptr == 0 && loadsound(j) == 0 ) continue;
        if( soundm[j]&16 ) sndist = 0;

        if(sndist < ((255-LOUDESTVOLUME)<<6) )
            sndist = ((255-LOUDESTVOLUME)<<6);

        FX_Pan3D(SoundOwner[j][k].voice,sndang>>6,sndist>>6);
    }
}

#ifdef C4DUKE
// c4: a voice of sound NUM has ended (the driver's callback, as audiolib's
// was): its entry in SoundOwner -- the one whose voice the driver no
// longer holds -- comes out, and those after it move down, so that the
// first is always the oldest. The original took out the last entry,
// whichever voice had ended. And the game calls testcallback (below)
// after its own FX_StopSound, whose callback has already been here: in
// the DOS game, whose linker did not tell testcallback from TestCallBack,
// that took out a second entry. Either way the counts fell behind what
// played, sounds went on that nothing tracked or moved with their
// sprites, and the next of a kind cut one that had only just begun.
int c4_fx_held(int32_t handle);    // duke-snd.c: the driver holds this voice still

static short c4_ended(int32_t num)
{
    short j;
    for(j=0;j<Sound[num].num;j++)
        if(!c4_fx_held(SoundOwner[num][j].voice)) return j;
    return -1;
}

void TestCallBack(int32_t num)
{
    short i,j,k;

    if(num < 0)
    {
        if(lumplockbyte[-num] >= 200)
            lumplockbyte[-num]--;
        return;
    }

    // a voice of sound() ended, which has no entry: only its lock
    if((j = c4_ended(num)) >= 0)
    {
        k = Sound[num].num;
        i = SoundOwner[num][j].i;
        if( (soundm[num]&16) == 0 && i >= 0 && sprite[i].picnum == MUSICANDSFX &&
            sector[sprite[i].sectnum].lotag < 3 && sprite[i].lotag < 999 )
            hittype[i].temp_data[0] = 0;
        for(;j<k-1;j++)
            SoundOwner[num][j] = SoundOwner[num][j+1];
        Sound[num].num--;
        SoundOwner[num][k-1].i = -1;
    }

    Sound[num].lock--;
}

// after the game's own FX_StopSound: the callback has let that voice go,
// unless the stop found no voice to stop; then its entry goes here
void testcallback(uint32_t num)
{
    if((int32_t)num >= 0 && c4_ended(num) >= 0)
        TestCallBack(num);
}

#else
void TestCallBack(int32_t num)
{
    short tempi,tempj,tempk;

        if(num < 0)
        {
            if(lumplockbyte[-num] >= 200)
                lumplockbyte[-num]--;
            return;
        }

        tempk = Sound[num].num;

        if(tempk > 0)
        {
            if( (soundm[num]&16) == 0)
                for(tempj=0;tempj<tempk;tempj++)
            {
                tempi = SoundOwner[num][tempj].i;
                if(sprite[tempi].picnum == MUSICANDSFX && sector[sprite[tempi].sectnum].lotag < 3 && sprite[tempi].lotag < 999)
                {
                    hittype[tempi].temp_data[0] = 0;
                    if( (tempj + 1) < tempk )
                    {
                        SoundOwner[num][tempj].voice = SoundOwner[num][tempk-1].voice;
                        SoundOwner[num][tempj].i     = SoundOwner[num][tempk-1].i;
                    }
                    break;
                }
            }

            Sound[num].num--;
            SoundOwner[num][tempk-1].i = -1;
        }

        Sound[num].lock--;
}


// no idea if this is right. I added this function.  --ryan.
void testcallback(uint32_t num)
{
//    STUBBED("wtf?");
    TestCallBack(num);
}
#endif


void clearsoundlocks(void)
{
    int32_t i;
    
   

    for(i=0;i<NUM_SOUNDS;i++)
 
            Sound[i].lock = 199;

    for(i=0;i<11;i++)
 
            lumplockbyte[i] = 199;
}


#include <Carbon/Carbon.h>
#include "oss.h"
#include "wavelog.h"
#include "m1snd.h"
#include "m1ui.h"

const int kMaxBuffers = 4;

ExtSoundHeaderPtr    header;
static void                *buffer[kMaxBuffers], *playBuffer;
SndChannelPtr        channel = nil;
SndCallBackUPP        callback = nil;
SndCommand            playCmd, callbackCmd;

int nDSoundSegLen = 0;

void (*m1sdr_Callback)(unsigned long dwUser, short *samples);
static int msg_callback(void *, int message, char *txt, int iparm);

volatile int buffers_used = 0, buffers_read = 0, buffers_write = 0;
volatile int fade_count;
volatile int pause = 0, playing = 0;

static EventLoopTimerRef timer;

void m1sdr_Pause(int p)
{
     pause = p;
}

static pascal void play_generation_callback(EventLoopTimerRef ttimer, void *snd)
{
     unsigned long t;

     if (pause)
         return;

     t = TickCount();

     while (playing && (buffers_used < (kMaxBuffers - 1)))
     {
         m1sdr_Callback(nDSoundSegLen, (short *)buffer[buffers_write]);
         waveLogFrame((unsigned char *)buffer[buffers_write],  
nDSoundSegLen << 2);

         if (++buffers_write >= kMaxBuffers) buffers_write = 0;
         buffers_used++;

#if 1
         if ((TickCount() - t < 60) && (buffers_used < (kMaxBuffers  
 >> 1)))
         {
             double firetime = 0;

             SetEventLoopTimerNextFireTime(ttimer, firetime);
             return;
         }
#endif
/*        if (((TickCount() - t) > 60) && (buffers_used < 2))
         {
             short itemHit;
             StandardAlert(kAlertStopAlert, "\pBuy a new Mac!",  
"\pYour Mac does not have enough spare CPU power for the selected  
game to play. Try quitting unnecessary processes.", NULL, &itemHit);
             m1sdr_PlayStop();
         }
*/
     }
}

static pascal void m1sdr_MacCallback(SndChannelPtr chan, SndCommand *)
{
     OSErr        err;

     if (buffers_used > 0)
     {
       //     	printf("play: B %d\n", buffers_read);
         memcpy(playBuffer, buffer[buffers_read], nDSoundSegLen * 2 * sizeof(UINT16));
         if (++buffers_read >= kMaxBuffers) buffers_read = 0;
         buffers_used--;
     }
     else
     {
         memset(header->samplePtr, 0, header->numFrames * 2 * sizeof 
(UINT16));
     }

     SndDoCommand(chan, &playCmd, true);
     SndDoCommand(chan, &callbackCmd, true);
}

INT16 m1sdr_Init(int sample_rate)
{
     header = (ExtSoundHeaderPtr)NewPtrClear(sizeof(ExtSoundHeader));
     header->loopStart = 0;
     header->loopEnd = 0;
     header->encode = extSH;
     header->baseFrequency = kMiddleC;
     header->sampleRate = sample_rate << 16;
     header->sampleSize = 16;
     header->numChannels = 2;

     callback = NewSndCallBackUPP(m1sdr_MacCallback);
     assert(callback);

     SndNewChannel(&channel, sampledSynth, initStereo, callback);
     assert(channel);

     playCmd.cmd = bufferCmd;
     playCmd.param1 = 0;
     playCmd.param2 = (long)header;
     callbackCmd.cmd = callBackCmd;
     callbackCmd.param1 = 0;
     callbackCmd.param2 = 0;

     return 1;
}

void m1sdr_Exit(void)
{
}

static void ClearBuffers(void)
{
     int i;

     for (i = 0; i < kMaxBuffers; i++)
     {
         memset(buffer[i], 0, nDSoundSegLen * 2 * sizeof(UINT16));
     }

     memset(playBuffer, 0, nDSoundSegLen * 2 * sizeof(UINT16));

     buffers_used = buffers_read = buffers_write = 0;
}

void mac_start_timer(void) 
{
#if 0
     EventLoopTimerUPP                gPlayTimerUPP;

     printf("Starting timer\n");

     // Playback timer
     gPlayTimerUPP = NewEventLoopTimerUPP(play_generation_callback);
     InstallEventLoopTimer(GetMainEventLoop(), 0, kEventDurationSecond / kMaxBuffers, gPlayTimerUPP, nil, &timer);
     RunApplicationEventLoop();
#endif
}

void m1sdr_PlayStart(void)
{
     SndCommand cmd;

     // Stop any existing sound
     m1sdr_PlayStop();

     // Clear buffers
     ClearBuffers();
     header->samplePtr = (char *)playBuffer;

     // Fire off a callback
     SndDoCommand(channel, &callbackCmd, true);

     playing = true;
}

void m1sdr_PlayStop(void)
{
     SndCommand cmd;

     // Stop playing
     playing = false;

     // Set up bits
     cmd.param1 = 0;
     cmd.param2 = 0;
     cmd.cmd = quietCmd;
     SndDoImmediate(channel, &cmd);
     cmd.cmd = flushCmd;
     SndDoImmediate(channel, &cmd);
}

void m1sdr_FlushAudio(void)
{
     m1sdr_PlayStart();
}

void m1sdr_SetSamplesPerTick(UINT32 spf)
{
     int i;

     nDSoundSegLen = spf;

     for (i = 0; i < kMaxBuffers; i++)
     {
         if (buffer[i])
         {
             free(buffer[i]);
             buffer[i] = nil;
         }

         buffer[i] = malloc(nDSoundSegLen * 2 * sizeof(UINT16));
         if (!buffer[i])
         {
             m1ui_message(NULL, M1_MSG_ERROR, "Unable to allocate memory for sound buffer.", 0);
             ExitToShell();
         }

         memset(buffer[i], 0, nDSoundSegLen * 2 * sizeof(UINT16));
     }

     playBuffer = malloc(nDSoundSegLen * 2 * sizeof(UINT16));
     if (!playBuffer)
     {
         m1ui_message(NULL, M1_MSG_ERROR, "Unable to allocate memory for playback buffer.", 0);
         ExitToShell();
     }

     memset(playBuffer, 0, nDSoundSegLen * 2 * sizeof(UINT16));

     header->samplePtr = (char *)playBuffer;
     header->numFrames = nDSoundSegLen;
}

void m1sdr_TimeCheck(void)
{
     while (playing && (buffers_used < (kMaxBuffers - 1)))
     {
       //     	printf("write: B %d\n", buffers_write);
         m1sdr_Callback(nDSoundSegLen, (short *)buffer[buffers_write]);
         waveLogFrame((unsigned char *)buffer[buffers_write], nDSoundSegLen << 2);

         if (++buffers_write >= kMaxBuffers) buffers_write = 0;
         buffers_used++;
     }
}

void m1sdr_SetCallback(void *fn)
{
     (void *)m1sdr_Callback = fn;
}


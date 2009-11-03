/****************************************************************************************
* timers.h - timer functions borrowed from mupen64 and mupen64plus, modified by sepp256
****************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
#ifndef __TIMERS_H__
#define __TIMERS_H__

typedef struct {
   float	fps;			//Current fps
   float	vis;			//Current VI/s
   char		frameDrawn;
   char		limitVIs;		//0: NO VI Limit, 1: Limit VI/s, 2: Wait only if frame drawn
   char		useFpsModifier;	//Modify FPS?
   int		fpsModifier;	//Framerate modifier in %
   unsigned long	lastFrameTime;
   unsigned long	lastViTime;
} timers;

//extern timers Timers;

void InitTimer();
void TimerUpdate(void);

#endif // __TIMERS_H__
#ifdef __cplusplus
}
#endif


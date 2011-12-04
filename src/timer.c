#include "timer.h"

struct timer
{
	struct timeval
		lastUpdate;
	float
		scale,
		elapsed,
		goal,			// like 2.00 for two seconds
		goalElapsed,	// the amount of time elapsed towards the goal
		lastTimestep;
	CLOCK
		* clock;
	enum
	{
		TIMER_PAUSED = true,
		TIMER_RUNNING = false
	} paused;
};

static void xtimer_createTimerRegistry ();
static void xtimer_registerTimer (TIMER t);
static void xtimer_unregisterTimer (TIMER t);

struct timeval timeval_subtract (const struct timeval * a, const struct timeval * b) {
  struct timeval diff;
  diff.tv_sec = a->tv_sec - b->tv_sec;
  diff.tv_usec = a->tv_usec - b->tv_usec;
  if (diff.tv_usec < 0) {
    diff.tv_usec += 1000000;
    diff.tv_sec -= 1;
  }
  return diff;
}

struct timeval timeval_add (const struct timeval * a, const struct timeval * b) {
  struct timeval diff;
  diff.tv_sec = a->tv_sec + b->tv_sec;
  diff.tv_usec = a->tv_usec + b->tv_usec;
  if (diff.tv_usec > 1000000) {
    diff.tv_usec -= 1000000;
    diff.tv_sec += 1;
  }
  return diff;
}

float timeval_cmp (const struct timeval * a, const struct timeval * b) {
  struct timeval diff = timeval_subtract (a, b);
  return timeval_asFloat (&diff);
}

float timeval_asFloat (const struct timeval * t) {
  return t->tv_sec + (t->tv_usec / 1000000.0);
}

static CLOCK
	* BaseClock = NULL;


CLOCK * clock_create () {
  CLOCK * c = xph_alloc (sizeof (CLOCK));
  memset (c, 0, sizeof (CLOCK));
	BaseClock = c;
  return c;
}

void clock_destroy (CLOCK * c) {
  xph_free (c);
}

void clock_update (CLOCK * c) {
  c->previous = c->now;
  gettimeofday (&c->now, NULL);
}


TIMER timerCreate ()
{
	TIMER t = xph_alloc (sizeof (struct timer));
	t->scale = 1;
	t->elapsed = 0.0;
	t->clock = BaseClock;
	t->paused = TIMER_RUNNING;
	t->lastTimestep = 0.0;
	memset (&t->lastUpdate, 0, sizeof (struct timeval));
	xtimer_registerTimer (t);
	return t;
}

void timerDestroy (TIMER t)
{
	xtimer_unregisterTimer (t);
	xph_free (t);
}

void timerSetClock (TIMER t, CLOCK * c)
{
	t->clock = c;
}

void timerSetScale (TIMER t, float scale)
{
	t->scale = scale;
}

void timerSetGoal (TIMER t, float goal)
{
	t->goal = goal;
	t->goalElapsed = 0.0;
}

void timerPause (TIMER t)
{
	t->paused = TIMER_PAUSED;
	t->elapsed = 0.0;
	t->lastUpdate.tv_sec = 0;
	t->lastUpdate.tv_usec = 0;
}

void timerStart (TIMER t)
{
	t->paused = TIMER_RUNNING;
}

bool outOfTime (TIMER t)
{
	if (t == NULL)
		return false;
	timerUpdate (t);
	return t->goal != 0.0 && t->goalElapsed >= t->goal;
}

void timerUpdate (TIMER t)
{
	float
		u;
	if (t->paused != TIMER_RUNNING || t->clock == NULL)
		return;
	clock_update (t->clock);
	if (t->lastUpdate.tv_sec == 0)
	{
		// If we're on the first update of the timer, skip calculating the difference, since it'll be huge and break everything.
		t->lastUpdate = t->clock->now;
		return;
	}
	u = timeval_cmp (&t->clock->now, &t->lastUpdate);
	t->elapsed += u * t->scale;
	if (t->goal)
		t->goalElapsed += u * t->scale;
	t->lastTimestep = u;
	t->lastUpdate = t->clock->now;
}

float lastTimestep (const TIMER t)
{
	return t->lastTimestep;
}

float timerGetTotalTimeElapsed (const TIMER t)
{
	return t->elapsed;
}

float timerGetTimeSinceLastUpdate (const TIMER t)
{
	if (t->lastUpdate.tv_sec == 0)
		return -1;
	return timeval_cmp (&t->clock->now, &t->lastUpdate) * t->scale;
}

float timerPercentageToGoal (const TIMER t)
{
	if (t->goal == 0.0)
		return 1.0;
	return t->goalElapsed / t->goal;
}

static Dynarr
	TimerRegistry = NULL;

void xtimerUpdateAll () {
  TIMER t = NULL;
  int i = 0;
  if (TimerRegistry == NULL)
    xtimer_createTimerRegistry ();
  while ((t = *(TIMER *)dynarr_at (TimerRegistry, i++)) != NULL) {
    timerUpdate (t);
  }
}

static void xtimer_registerTimer (TIMER t) {
  if (TimerRegistry == NULL)
    xtimer_createTimerRegistry ();
  dynarr_push (TimerRegistry, t);
}

static void xtimer_unregisterTimer (TIMER t) {
  if (TimerRegistry == NULL)
    xtimer_createTimerRegistry ();
  dynarr_remove_condense (TimerRegistry, t);
}

static void xtimer_createTimerRegistry () {
  if (NULL != TimerRegistry) {
    return;
  }
  TimerRegistry = dynarr_create (8, sizeof (TIMER));
}

void xtimerDestroyRegistry () {
  while (!dynarr_isEmpty (TimerRegistry)) {
    timerDestroy (*(TIMER *)dynarr_pop (TimerRegistry));
  }
  dynarr_destroy (TimerRegistry);
  TimerRegistry = NULL;
}

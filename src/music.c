/* 
 * Music add-on developed by:   Mark Parncutt
 *                              http://github.com/U-238
 * 
 * Based on example3.c from libsox
 * Copyright (c) 2007-9 robs@users.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef NDEBUG /* N.B. assert used with active statements so enable always. */
#undef NDEBUG /* Must undef above assert.h or other that might include it. */
#endif

#include <sox.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "musicdata.c"

#ifndef HAVE_FMEMOPEN
/* The music is temporarily written here if we can't use FMEMOPEN */
#define TMP_MUSIC_FILE "/tmp/nyancat.ogg"
#endif 

void *lsx_realloc(void *ptr, size_t newsize)
{
  if (ptr && newsize == 0) {
    free(ptr);
    return NULL;
  }

  if ((ptr = realloc(ptr, newsize)) == NULL) {
    lsx_fail("out of memory");
    exit(2);
  }

  return ptr;
}

#define lsx_malloc(size) lsx_realloc(NULL, (size))
#define lsx_calloc(n,s) (((n)*(s))? memset(lsx_malloc((n)*(s)),0,(n)*(s)) : NULL)

#ifdef min
#undef min
#endif
#define min(a, b) ((a) <= (b) ? (a) : (b))

typedef enum {RG_off, RG_track, RG_album, RG_default} rg_mode;

static void output_message(unsigned level, const char *filename, const char *fmt, va_list ap)
{
  char const * const str[] = {"FAIL", "WARN", "INFO", "DBUG"};
  if (sox_globals.verbosity >= level) {
    char base_name[128];
    sox_basename(base_name, sizeof(base_name), filename);
    fprintf(stderr, "%s %s: ", str[min(level - 1, 3)], base_name);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
  }
}

typedef struct {
  char * filename;

  char const * filetype;
  sox_signalinfo_t signal;
  sox_encodinginfo_t encoding;
  double volume;
  double replay_gain;
  sox_oob_t oob;
  sox_bool no_glob;

  sox_format_t * ft;  /* libSoX file descriptor */
  uint64_t volume_clips;
  rg_mode replay_gain_mode;
} file_t;

static char const * device_name(char const * const type)
{
  char * name = NULL, * from_env = getenv("AUDIODEV");

  if (!type)
    return NULL;

  if (0
      || !strcmp(type, "sunau")
      || !strcmp(type, "oss" )
      || !strcmp(type, "ossdsp")
      || !strcmp(type, "alsa")
      || !strcmp(type, "ao")
      || !strcmp(type, "sndio")
      || !strcmp(type, "coreaudio")
      || !strcmp(type, "pulseaudio")
      || !strcmp(type, "waveaudio")
      )
    name = "default";
  
  return name? from_env? from_env : name : NULL;
}

static char const * try_device(char const * name)
{
  sox_format_handler_t const * handler = sox_find_format(name, sox_false);
  if (handler) {
    sox_format_t format, * ft = &format;
    lsx_debug("Looking for a default device: trying format `%s'", name);
    memset(ft, 0, sizeof(*ft));
    ft->filename = (char *)device_name(name);
    ft->priv = lsx_calloc(1, handler->priv_size);
    if (handler->startwrite(ft) == SOX_SUCCESS) {
      handler->stopwrite(ft);
      free(ft->priv);
      return name;
    }
    free(ft->priv);
  }
  return NULL;
}

static char const * default_device()
{
  /* Default audio driver type in order of preference: */
  const char *name = NULL;
  if (!name) name = getenv("AUDIODRIVER");
  if (!name) name = try_device("coreaudio");
  if (!name) name = try_device("pulseaudio");
  if (!name) name = try_device("alsa");
  if (!name) name = try_device("waveaudio");
  if (!name) name = try_device("sndio");
  if (!name) name = try_device("oss");
  if (!name) name = try_device("sunau");
  if (!name) name = try_device("ao");

  if (!name) {
    fprintf(stderr, "Sorry, there is no default audio device configured");
    exit(1);
  }
  return name;
}

void *play_nyan_music(void *vargp)
{
  static sox_format_t * in, * out; /* input and output files */
  sox_effects_chain_t * chain;
  sox_effect_t * e;
  sox_signalinfo_t interm_signal;
  char * args[10];
  sigset_t sset;
  
  /* Block all signals that the main thread has a handler for,
     so that those signals go to the main thread */
  sigemptyset(&sset);
  sigaddset(&sset, SIGWINCH);
  sigaddset(&sset, SIGALRM);
  sigaddset(&sset, SIGINT);
  sigaddset(&sset, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &sset, NULL);

  sox_globals.output_message_handler = output_message;
  sox_globals.verbosity = 1;

  assert(sox_init() == SOX_SUCCESS);
#ifdef HAVE_FMEMOPEN
  assert(in = sox_open_mem_read(nyanmusic, nyanmusic_len, NULL, NULL, NULL));
#else
  FILE *fp = fopen(TMP_MUSIC_FILE, "wb" );
  fwrite(nyanmusic, 1, nyanmusic_len, fp);
  fclose(fp);
  assert(in = sox_open_read(TMP_MUSIC_FILE, NULL, NULL, NULL));
#endif
  assert(out= sox_open_write("default", &in->signal, NULL, default_device(), NULL, NULL));

  chain = sox_create_effects_chain(&in->encoding, &out->encoding);

  interm_signal = in->signal; /* NB: deep copy */

  e = sox_create_effect(sox_find_effect("input"));
  args[0] = (char *)in, assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
  assert(sox_add_effect(chain, e, &interm_signal, &in->signal) == SOX_SUCCESS);
  free(e);

  if (in->signal.rate != out->signal.rate) {
    e = sox_create_effect(sox_find_effect("rate"));
    assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
    assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
    free(e);
  }

  if (in->signal.channels != out->signal.channels) {
    e = sox_create_effect(sox_find_effect("channels"));
    assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
    assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
    free(e);
  }

  e = sox_create_effect(sox_find_effect("output"));
  args[0] = (char *)out, assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
  assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
  free(e);

  sox_flow_effects(chain, NULL, NULL);

  sox_delete_effects_chain(chain);
  sox_close(out);
  sox_close(in);
  sox_quit();

  return NULL;
}

void nyan_music_cleanup() {
#ifndef HAVE_FMEMOPEN
  unlink(TMP_MUSIC_FILE);
#endif
}

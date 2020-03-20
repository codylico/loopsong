/*
 * Copyright (C) 2019 by Cody Licorish
 * This source file is made available under the Unlicense.
 * See <http://unlicense.org> for more information about the Unlicense.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>

static int report_time_change(double real, double new_real);
static int enter_once_mode
  (ALLEGRO_AUDIO_STREAM* stream, double stream_length);
static double local_atof(char const* str);
static double local_atof_seconds(char const* str);
static char* local_strdup(char const* str);
static double parse_duration(char const* str);

/* for basic stop/computer sleep detection */
static int adjusted_al_enable = 0;
/* for basic stop/computer sleep detection */
static double adjusted_al_time = 0.0;
/* for basic stop/computer sleep detection */
static double adjusted_al_get_time(void);
/* for basic stop/computer sleep detection */
static void adjusted_al_rest(double seconds);

int report_time_change(double real, double new_real){
  if (real > new_real)
    return fprintf(stderr,
      "Removed %g unnecessary seconds of playback.\n",
      real - new_real);
  else
    return fprintf(stderr,
      "Added %g extra seconds of playback.\n",
      new_real - real);
}

int enter_once_mode(ALLEGRO_AUDIO_STREAM* stream, double stream_length){
  if (!al_set_audio_stream_loop_secs(stream, 0.0, stream_length)){
    fprintf(stderr, "\nfailed to set loop points\n");
    return 0;
  }
  if (!al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE)){
    fprintf(stderr, "\nfailed to set play mode\n");
    return 0;
  } else fprintf(stderr, "\nplay mode: ONCE\n");
  return 1;
}

double local_atof(char const* str){
  if (str == NULL) return -1;
  else return atof(str);
}

double local_atof_seconds(char const* str){
  if (str == NULL) return -1;
  else return parse_duration(str);
}

char* local_strdup(char const* str){
  if (str != NULL){
    size_t len = strlen(str);
    char* new_str = (char*)malloc(len+1);
    if (new_str != NULL){
      memcpy(new_str, str, len);
      new_str[len] = 0;
      return new_str;
    } else return NULL;
  } else return NULL;
}

double parse_duration(char const* str){
  char const *p, *q, *end;
  double out = 0.0;
  int sign = 0;
  if (*str == '-') {
    sign = -1;
    str += 1;
  } else if (*str == '+') {
    sign = +1;
    str += 1;
  }
  p = str;
  for (q = str; *q != 0; ++q){
    if (*q == ':'){
      /* pull and multiply by 60 */
      int fragment = atoi(p);
      out += fragment;
      out *= 60.;
      /* reset the numeric pointer */
      p = q+1;
    } else if (isdigit(*q)){
      continue;
    } else {
      break;
    }
  }
  if (p != q){
    double fragment = atof(p);
    out += fragment;
  }
  if (sign) out *= sign;
  return out;
}

double adjusted_al_get_time(void) {
  if (adjusted_al_enable)
    return al_get_time()-adjusted_al_time;
  else return al_get_time();
}
void adjusted_al_rest(double seconds) {
  if (adjusted_al_enable) {
    double diff_time;
    double const start_time = al_get_time();
    al_rest(seconds);
    diff_time = al_get_time()-start_time;
    if (diff_time > seconds*2.0) {
      adjusted_al_time += (diff_time - seconds*2.0);
    }
  } else al_rest(seconds);
  return;
}

int main(int argc, char**argv){
  int result = EXIT_SUCCESS;
  double duration = 5.0;
  double fade_duration = 5.0;
  char const* ini_file = NULL;
  double start = 0.0;
  double end = 5.0;
  double seek_point = 0.0;
  char const* song_file;
  char const* loop_def_name = NULL;

  /*
   * short-play switch:
   *   -1 short play possible; may finish early
   *    0 medium play; fades out at end
   *   +1 long play; takes extra time to finish properly
   */
  int shortplay_switch = 0;

  /* parse arguments */{
    int argi;
    int n = 0;
    int help_tf = 0;
    int opt_done = 0;
    for (argi = 1; argi < argc; ++argi){
      if ((!opt_done) && argv[argi][0] == '-'){
        if (strcmp(argv[argi],"--") == 0){
          opt_done = 1;
        } else if (strcmp(argv[argi],"-f") == 0){
          if (++argi < argc){
            fade_duration = parse_duration(argv[argi]);
          }
        } else if (strcmp(argv[argi],"-k") == 0){
          if (++argi < argc){
            seek_point = parse_duration(argv[argi]);
          }
        } else if (strcmp(argv[argi],"-s") == 0){
          shortplay_switch = -1;
        } else if (strcmp(argv[argi],"-a") == 0){
          adjusted_al_enable = 1;
        } else if (strcmp(argv[argi],"-m") == 0){
          shortplay_switch = 0;
        } else if (strcmp(argv[argi],"-l") == 0){
          shortplay_switch = +1;
        } else if (strcmp(argv[argi],"-d") == 0){
          if (++argi < argc){
            loop_def_name = argv[argi];
          }
        } else {
          fprintf(stderr, "unrecognized option %s\n", argv[argi]);
          help_tf = 1;
          break;
        }
      } else if (n == 0){
        ini_file = argv[argi];
        n += 1;
      } else if (n == 1){
        duration = parse_duration(argv[argi]);
        n += 1;
      }
    }
    if (help_tf){
      fputs("usage: loopsong [options] (file.ini) (seconds)\n"
        "  (file.ini)\n"
        "      song description file\n"
        "  (seconds)\n"
        "      minimum number of seconds to play file\n"
        "options:\n"
        "  -f (seconds)\n"
        "      duration of fade out in seconds\n"
        "  -k (seconds)\n"
        "      position from which to start playing\n"
        "  -s\n"
        "      finish early if needed\n"
        "  -m\n"
        "      fade out on time\n"
        "  -l\n"
        "      take extra time to finish out the song\n"
        "  -a\n"
        "      adjust for when the player sleeps too long\n"
        "  -d (name)\n"
        "      pull additional loop definition parameters\n"
        "      from section [name]\n",
        stderr);
      return EXIT_FAILURE;
    }
  }
  al_init();
  /* read the INI file */if (ini_file != NULL){
    ALLEGRO_CONFIG* cfg = al_load_config_file(ini_file);
    if (cfg != NULL){
      song_file = local_strdup(
          al_get_config_value(cfg, NULL, "song")
        );
      start = local_atof_seconds(
          al_get_config_value(cfg, NULL, "start")
        );
      end = local_atof_seconds(
          al_get_config_value(cfg, NULL, "end")
        );
      if (loop_def_name != NULL && loop_def_name[0] != '\0') {
        char const* const subsong_file =
           al_get_config_value(cfg, loop_def_name, "song");
        char const* const substart =
           al_get_config_value(cfg, loop_def_name, "start");
        char const* const subend =
           al_get_config_value(cfg, loop_def_name, "end");
        if (subsong_file != NULL) {
          free((char*)song_file);
          song_file = local_strdup(subsong_file);
        }
        if (substart != NULL) {
          start = local_atof_seconds(substart);
        }
        if (subend != NULL) {
          end = local_atof_seconds(subend);
        }
      }
      al_destroy_config(cfg);
    } else {
      result = EXIT_FAILURE;
    }
  }
  if (result != EXIT_FAILURE){
    if (song_file == NULL || (*song_file) == 0){
      fprintf(stderr, "song file name missing\n");
      result = EXIT_FAILURE;
    }
  }
  if (result != EXIT_FAILURE){
    fprintf(stderr, "song:  %s\n", song_file);
    fprintf(stderr, "start: %g\n", start);
    fprintf(stderr, "end:   %g\n", end);
    fprintf(stderr, "time:  %g\n", duration);
    fprintf(stderr, "fade:  %g\n", fade_duration);
    fprintf(stderr, "short: %i\n", shortplay_switch);
    fprintf(stderr, "seek:  %g\n", seek_point);
    /* initialize audio */{
      if (!al_install_audio()){
        fprintf(stderr, "failed to install audio\n");
        result = EXIT_FAILURE;
      } else if (!al_init_acodec_addon()){
        fprintf(stderr, "failed to install audio codecs\n");
        result = EXIT_FAILURE;
      }
    }
  }
  if (result != EXIT_FAILURE){
    int ok = 0;
    ALLEGRO_VOICE* voice = NULL;
    ALLEGRO_MIXER* mixer = NULL;
    ALLEGRO_AUDIO_STREAM* stream = NULL;
    double stream_length;
    /* connect and play audio */do {
      double real_start, real_stop;
      double loop_length;
      double loop_to_end;
      double right_now;
      int loop_tf = 0;
      voice =
        al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
          ALLEGRO_CHANNEL_CONF_2);
      if (voice == NULL){
        fprintf(stderr, "failed to create voice\n");
        break;
      }
      mixer =
        al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
          ALLEGRO_CHANNEL_CONF_2);
      if (mixer == NULL){
        fprintf(stderr, "failed to create voice\n");
        break;
      }
      stream = al_load_audio_stream(song_file, 4, 2048);
      if (stream == NULL){
        fprintf(stderr, "failed to load audio stream\n");
        break;
      }
      /* initialize key points */{
        stream_length = al_get_audio_stream_length_secs(stream);
        fprintf(stderr,"length: %g\n", stream_length);
        if (start < 0.) start = 0.;
        if (end < 0.) end = stream_length;
        if (seek_point <= 0.)
          seek_point = 0.;
        else if (seek_point > stream_length)
          seek_point = stream_length;
        loop_to_end = stream_length - start;
        loop_length = end-start;
      }
      if (end >= duration){
        /* shortcut */
        if (!al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE)){
          fprintf(stderr, "failed to set play mode\n");
        } else fprintf(stderr, "play mode: ONCE\n");
        loop_tf = 0;
      } else {
        if (!al_set_audio_stream_loop_secs(stream, start, end)){
          fprintf(stderr, "failed to set loop points\n");
          break;
        }
        if (!al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_LOOP)){
          fprintf(stderr, "failed to set play mode\n");
          break;
        } else fprintf(stderr, "play mode: LOOP\n");
        loop_tf = 1;
      }
      if (seek_point > 0.
      &&  seek_point < stream_length)
      {
        if (al_seek_audio_stream_secs(stream, seek_point)){
          fprintf(stderr, "sought to position %g\n",
            seek_point);
        } else fprintf(stderr, "failed to seek to position %g\n", seek_point);
      } else {
        fprintf(stderr, "ignoring seek positions\n");
      }
      al_attach_mixer_to_voice(mixer, voice);
      /* start the clock here */
      real_start = adjusted_al_get_time();
      al_attach_audio_stream_to_mixer(stream, mixer);
      real_stop = real_start+duration;
      right_now = adjusted_al_get_time();
      while (right_now < real_stop){
        double const time_left = real_stop - right_now;
        fprintf(stderr, "Time left: %8.2f \r", time_left);
        fflush(stderr);
        switch (shortplay_switch){
        case -1:
          /* play short but whole */if (loop_tf){
            if (time_left < loop_to_end){
              double const stream_point =
                al_get_audio_stream_position_secs(stream);
              double const new_real_stop
                  = (stream_length - stream_point) + right_now
                  + FLT_EPSILON;
              enter_once_mode(stream, stream_length);
              report_time_change(real_stop, new_real_stop);
              /* remove unnecessary time */
              real_stop = new_real_stop;
              loop_tf = 0;
            }
          }break;
        case 0:
          /* just let the song play out */
          break;
        case +1:
          /* play long and whole */if (loop_tf == 1){
            if (time_left < loop_to_end){
              double const stream_point =
                al_get_audio_stream_position_secs(stream);
              double const new_real_stop
                  = loop_to_end + right_now
                  + (end - stream_point) + FLT_EPSILON;
              report_time_change(real_stop, new_real_stop);
              /* add extra time to finish as needed */
              real_stop = new_real_stop;
              loop_tf = 2;
            }
          } else if (loop_tf == 2){
            if (time_left < loop_to_end){
              enter_once_mode(stream, stream_length);
              loop_tf = 0;
            }
          }break;
        default:
          if (loop_tf){
            if (time_left < loop_length){
              enter_once_mode(stream, stream_length);
              loop_tf = 0;
            }
          }break;
        }
        if (time_left > fade_duration){
          if (!al_get_audio_stream_playing(stream))
            break;
          else adjusted_al_rest(1.0);
        } else if (time_left > 0.1){
          al_set_mixer_gain(mixer, time_left/fade_duration);
          adjusted_al_rest(0.1);
        } else {
          adjusted_al_rest(time_left);
        }
        right_now = adjusted_al_get_time();
      }
      ok = 1;
    } while (0);
    fprintf(stderr,"\n");
    if (!ok) result = EXIT_FAILURE;
    if (stream != NULL){
      al_destroy_audio_stream(stream);
    }
    if (mixer != NULL){
      al_destroy_mixer(mixer);
    }
    if (voice != NULL){
      al_destroy_voice(voice);
    }
    al_uninstall_audio();
  }
  al_uninstall_system();
  return result;
}


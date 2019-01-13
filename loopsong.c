
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

double local_atof(char const* str){
  if (str == NULL) return -1;
  else return atof(str);
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
  return out;
}

int main(int argc, char**argv){
  int result = EXIT_SUCCESS;
  double duration = 5.0;
  double fade_duration = 5.0;
  char const* ini_file = NULL;
  double start = 0.0;
  double end = 5.0;
  char const* song_file;

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
        "      duration of fade out in seconds\n",
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
      start = local_atof(
          al_get_config_value(cfg, NULL, "start")
        );
      end = local_atof(
          al_get_config_value(cfg, NULL, "end")
        );
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
      double loop_length = end-start;
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
      stream_length = al_get_audio_stream_length_secs(stream);
      fprintf(stderr,"length: %g\n", stream_length);
      if (start < 0.) start = 0.;
      if (end < 0.) end = stream_length;
      if (end >= duration){
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
      al_attach_mixer_to_voice(mixer, voice);
      /* start the clock here */
      real_start = al_get_time();
      al_attach_audio_stream_to_mixer(stream, mixer);
      real_stop = real_start+duration;
      right_now = al_get_time();
      while (right_now < real_stop){
        double const time_left = real_stop - right_now;
        fprintf(stderr, "Time left: %.2f\r", time_left);
        fflush(stderr);
        if (loop_tf){
          if (time_left < loop_length){
            if (!al_set_audio_stream_loop_secs(stream, 0.0, stream_length)){
              fprintf(stderr, "\nfailed to set loop points\n");
              break;
            }
            if (!al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE)){
              fprintf(stderr, "\nfailed to set play mode\n");
              break;
            } else fprintf(stderr, "\nplay mode: ONCE\n");
            loop_tf = 0;
          }
        }
        if (time_left > fade_duration){
          al_rest(1.0);
        } else if (time_left > 0.1){
          al_set_mixer_gain(mixer, time_left/fade_duration);
          al_rest(0.1);
        } else {
          al_rest(time_left);
        }
        right_now = al_get_time();
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


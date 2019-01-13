
cmake_minimum_required(VERSION 3.0)

project(loopsong C)

find_library(allegro_LIBRARY
  NAMES allegro DOC "Path to Allegro 5 library")
find_library(allegro_audio_LIBRARY
  NAMES allegro_audio DOC "Path to Allegro 5 audio library")
find_library(allegro_acodec_LIBRARY
  NAMES allegro_acodec DOC "Path to Allegro 5 audio codec library")

find_path(allegro_INCLUDE NAMES "allegro5/allegro.h")

add_executable(loopsong "loopsong.c")

target_include_directories(loopsong
  PRIVATE "${allegro_INCLUDE}")
target_link_libraries(loopsong
  "${allegro_acodec_LIBRARY}" "${allegro_audio_LIBRARY}"
  "${allegro_LIBRARY}")

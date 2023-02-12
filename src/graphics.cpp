/*
 * The QED Programming Language
 * Copyright (C) 2022  Hocus Codus Software inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http: *www.gnu.org/licenses/>.
 */

#include "qni.hpp"
#include "vm.hpp"

extern ValueStack2 attStack;

VM *vm;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/val.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define SCREEN_SIZE_X 512
#define SCREEN_SIZE_Y 512

using emscripten::val;

// Use thread_local when you want to retrieve & cache a global JS variable once per thread.
thread_local const val document = val::global("document");

void initDisplay() {
//  const canvas = document.getElementById('canvas');
//  const ctx = canvas.getContext('2d');
  const auto canvas = document.call<emscripten::val, std::string>("querySelector", "canvas");
  auto ctx = canvas.call<emscripten::val, std::string>("getContext", "2d");
//  val canvas = document.call<val>("getElementById", "canvas");
//  val ctx = canvas.call<val>("getContext", "2d");

//  ctx.fillStyle = 'green';
//  ctx.fillRect(10, 10, 150, 100);
/*
  const auto width = canvas["width"].as<int>();
  const auto height = canvas["height"].as<int>();
  ctx.set("fillStyle", "green");
  ctx.call<void>("fillRect", 10, 10, 150, 100);
*/}

void uninitDisplay() {
}

void postMessage(void *param)
{
}

void suspend() {
  vm->repaint();
}

QNI_FN(rect) {
  long posP = AS_INT(args[0]);
  long sizeP = AS_INT(args[1]);
  Point pos = {(int) (posP >> 16), (int) (posP & 0xFFFF)};
  Point size = {(int) (sizeP >> 16), (int) (sizeP & 0xFFFF)};
  int color = AS_INT(attStack.get(ATTRIBUTE_COLOR));
  float opacity = AS_FLOAT(attStack.get(ATTRIBUTE_OPACITY));
  int opacityByte = (int) (opacity * 0xFF);
  const auto canvas = document.call<emscripten::val, std::string>("querySelector", "canvas");
  auto ctx = canvas.call<emscripten::val, std::string>("getContext", "2d");
  char colorBuffer[16];

  sprintf(colorBuffer, "#%02X%02X%02X", (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
  ctx.set("fillStyle", colorBuffer);
  ctx.set("globalAlpha", opacity);
  ctx.call<void>("fillRect", pos[0], pos[1], size[0], size[1]);
  ctx.set("globalAlpha", 1.0);
//  SDL_SetRenderDrawBlendMode(rend2, SDL_BLENDMODE_BLEND);
//	SDL_SetRenderDrawColor(rend2, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, opacityByte);
//  SDL_RenderFillRect(rend2, &rectangle);
  return VOID_VAL;
}

QNI_FN(oval) {
  long posP = AS_INT(args[0]);
  long sizeP = AS_INT(args[1]);
  Point pos = {(int) (posP >> 16), (int) (posP & 0xFFFF)};
  Point size = {(int) (sizeP >> 17), (int) ((sizeP & 0xFFFF) >> 1)};
  int rx = size[0] >> 1;
  int ry = size[1] >> 1;
  int color = AS_INT(attStack.get(ATTRIBUTE_COLOR));
  float opacity = AS_FLOAT(attStack.get(ATTRIBUTE_OPACITY));
  const auto canvas = document.call<emscripten::val, std::string>("querySelector", "canvas");
  auto ctx = canvas.call<emscripten::val, std::string>("getContext", "2d");
  char colorBuffer[16];

  sprintf(colorBuffer, "#%02X%02X%02X", (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
  ctx.set("fillStyle", colorBuffer);
  ctx.set("globalAlpha", opacity);
  ctx.call<void>("beginPath");
  ctx.call<void>("ellipse", pos[0] + size[0], pos[1] + size[1], size[0], size[1], 0, 0, 2*M_PI);
  ctx.call<void>("fill");
  ctx.set("globalAlpha", 1.0);

//  filledEllipseRGBA(rend2, pos[0] + rx, pos[1] + ry, rx, ry,
//                    (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (int) (opacity * 0xFF));
  return VOID_VAL;
}

QNI_FN(getTextSize) {
  int fontSize = AS_INT(attStack.get(ATTRIBUTE_FONTSIZE));
  const char *text = ((ObjString *) AS_OBJ(args[0]))->chars;
  const auto canvas = document.call<emscripten::val, std::string>("querySelector", "canvas");
  auto ctx = canvas.call<emscripten::val, std::string>("getContext", "2d");
  char fontBuffer[64];

  sprintf(fontBuffer, "%dpx Arial", fontSize);
  ctx.set("font", fontBuffer);
  auto textMetrics = ctx.call<emscripten::val, std::string>("measureText", text);
  float width = textMetrics["width"].as<float>();
  float height = textMetrics["fontBoundingBoxAscent"].as<float>() + textMetrics["fontBoundingBoxDescent"].as<float>();

  return INT_VAL((((long) width) << 16) | (long) height);
}

QNI_FN(displayText) {
  const char *text = ((ObjString *) AS_OBJ(args[0]))->chars;
  long posP = AS_INT(args[1]);
  long sizeP = AS_INT(args[2]);
  Point pos = {(int) (posP >> 16), (int) (posP & 0xFFFF)};
  Point size = {(int) (sizeP >> 16), (int) (sizeP & 0xFFFF)};
  int color = AS_INT(attStack.get(ATTRIBUTE_COLOR));
  float opacity = AS_FLOAT(attStack.get(ATTRIBUTE_OPACITY));
  int fontSize = AS_INT(attStack.get(ATTRIBUTE_FONTSIZE));
  const auto canvas = document.call<emscripten::val, std::string>("querySelector", "canvas");
  auto ctx = canvas.call<emscripten::val, std::string>("getContext", "2d");
  char colorBuffer[16];
  char fontBuffer[64];

  sprintf(fontBuffer, "%dpx Arial", fontSize);
  sprintf(colorBuffer, "#%02X%02X%02X", (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
  ctx.set("font", fontBuffer);
  ctx.set("fillStyle", colorBuffer);
  ctx.set("textBaseline", val("top"));
  ctx.set("globalAlpha", opacity);
  ctx.call<void>("fillText", std::string(text), pos[0], pos[1]);
  ctx.set("globalAlpha", 1.0);

  return VOID_VAL;
}
#else
// std
#include <assert.h>

// opengl
//#include <GL/glew.h>

// sdl
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL2_gfxPrimitives.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <SDL_timer.h>
#endif

#define SCREEN_SIZE_X 512
#define SCREEN_SIZE_Y 512

// creates a renderer to render our images
SDL_Renderer* rend2;
SDL_Surface *background2;

SDL_Window* win = NULL;
bool initFont = false;

static TTF_Font *getNewFont(int size) {
  if (!initFont) {
    TTF_Init();
    initFont = true;
  }

  return TTF_OpenFont("./res/font/arial.ttf", size);
}

static TTF_Font *getFont() {
  static TTF_Font *font = NULL;

  if (!font) {
    font = getNewFont(30);
  }
  return font;
}
/*
  if (SDL_MUSTLOCK(background)) SDL_LockSurface(background);

  Uint8 * pixels = (Uint8 *) background->pixels;

  for (int i=0; i < 1048576; i++) {
    char randomByte = rand() % 255;
    pixels[i] = randomByte;
  }

  if (SDL_MUSTLOCK(background)) SDL_UnlockSurface(background);

  SDL_Texture *screenTexture = SDL_CreateTextureFromSurface(rend2, background2);

  //    SDL_RenderClear(rend2);
  SDL_RenderCopy(rend2, screenTexture, NULL, NULL);

  // clears the screen
  SDL_RenderCopy(rend2, tex, NULL, &dest);

  // triggers the double buffers
  // for multiple rendering
  SDL_RenderPresent(rend2);

  SDL_DestroyTexture(screenTexture);
*/
void initDisplay() {
  if (!win) {
    // ----- Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
      fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
      assert(false);
    }
/*
    Uint32 ticks1 = SDL_GetTicks();
    SDL_Delay(5); // busy-wait
    Uint32 ticks2 = SDL_GetTicks();
    assert(ticks2 >= ticks1 + 5);
*/
      // ----- Create window
    SDL_GL_SetSwapInterval(1);

    win = SDL_CreateWindow("QED", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_SIZE_X, SCREEN_SIZE_Y, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!win) {
      fprintf(stderr, "Error creating window.\n");
      assert(false);
    }

    // triggers the program that controls
    // your graphics hardware and sets flags
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;

    // creates a renderer to render our images
    rend2 = SDL_CreateRenderer(win, -1, render_flags);
    background2 = SDL_CreateRGBSurface(0, SCREEN_SIZE_X, SCREEN_SIZE_Y, 32, 0, 0, 0, 0);

//    const SDL_Rect* dstrect;
//    SDL_Color color;
  }
/*
  #ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(mainLoop, 0, 1);
  #else
  while (quit == false) {
    mainLoop();
    SDL_Delay(1000 / 60);
  }
  #endif
*/
  SDL_SetRenderDrawColor(rend2, 0, 0, 0, 0);
  SDL_RenderClear(rend2);
}

void uninitDisplay() {
  if (win) {
//    TTF_CloseFont(getFont());
//    TTF_Quit();

    // destroy renderer
    SDL_DestroyRenderer(rend2);

    // destroy window
    SDL_DestroyWindow(win);

    // ----- Clean up
  //    SDL_GL_DeleteContext(glContext);
    win = NULL;
  }
}

QNI_FN(oval) {
  long posP = AS_INT(args[0]);
  long sizeP = AS_INT(args[1]);
  Point pos = {(int) (posP >> 16), (int) (posP & 0xFFFF)};
  Point size = {(int) (sizeP >> 16), (int) (sizeP & 0xFFFF)};
  int rx = size[0] >> 1;
  int ry = size[1] >> 1;
  int color = AS_INT(attStack.get(ATTRIBUTE_COLOR));
  float opacity = AS_FLOAT(attStack.get(ATTRIBUTE_OPACITY));

//  SDL_SetTextureBlendMode(rend2, SDL_BLENDMODE_ADD);
  filledEllipseRGBA(rend2, pos[0] + rx, pos[1] + ry, rx, ry,
                    (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (int) (opacity * 0xFF));
//  aaellipseRGBA(rend2, pos[0] + rx, pos[1] + ry, rx, ry,
//                    (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (int) (opacity * 0xFF));
  return VOID_VAL;
}

QNI_FN(rect) {
  long posP = AS_INT(args[0]);
  long sizeP = AS_INT(args[1]);
  Point pos = {(int) (posP >> 16), (int) (posP & 0xFFFF)};
  Point size = {(int) (sizeP >> 16), (int) (sizeP & 0xFFFF)};
  SDL_Rect rectangle;

  rectangle.x = pos[0];
  rectangle.y = pos[1];
  rectangle.w = size[0];
  rectangle.h = size[1];

  SDL_BlendMode blendMode;
  int color = AS_INT(attStack.get(ATTRIBUTE_COLOR));
  float opacity = AS_FLOAT(attStack.get(ATTRIBUTE_OPACITY));
  int opacityByte = (int) (opacity * 0xFF);

  SDL_SetRenderDrawBlendMode(rend2, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(rend2, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, opacityByte);
  SDL_RenderFillRect(rend2, &rectangle);
  return VOID_VAL;
}

QNI_FN(getTextSize) {
  int width;
  int height;
  int fontSize = AS_INT(attStack.get(ATTRIBUTE_FONTSIZE));
  const char *text = ((ObjString *) AS_OBJ(args[0]))->chars;
  TTF_Font *font = fontSize != -1 ? getNewFont(fontSize) : getFont();

  TTF_SizeUTF8(font, text, &width, &height);

  if (fontSize != -1)
    TTF_CloseFont(font);

  return INT_VAL((((long) width) << 16) | height);
}

QNI_FN(displayText) {
  SDL_Rect rectangle;
  const char *text = ((ObjString *) AS_OBJ(args[0]))->chars;
  long posP = AS_INT(args[1]);
  long sizeP = AS_INT(args[2]);
  Point pos = {(int) (posP >> 16), (int) (posP & 0xFFFF)};
  Point size = {(int) (sizeP >> 16), (int) (sizeP & 0xFFFF)};
  int color = AS_INT(attStack.get(ATTRIBUTE_COLOR));
  float opacity = AS_FLOAT(attStack.get(ATTRIBUTE_OPACITY));
  int fontSize = AS_INT(attStack.get(ATTRIBUTE_FONTSIZE));
  TTF_Font *font = fontSize != -1 ? getNewFont(fontSize) : getFont();
  SDL_Surface *textSurface = TTF_RenderText_Blended(font, text, {(Uint8) (color >> 16), (Uint8) (color >> 8), (Uint8) color, (Uint8) (Uint8) (opacity * 0xFF)});
  SDL_Texture *textTexture = SDL_CreateTextureFromSurface(rend2, textSurface);

  if (fontSize != -1)
    TTF_CloseFont(font);

  rectangle.x = pos[0];
  rectangle.y = pos[1];
  rectangle.w = size[0];
  rectangle.h = size[1];
  SDL_RenderCopy(rend2, textTexture, NULL, &rectangle);
  SDL_FreeSurface(textSurface);
  SDL_DestroyTexture(textTexture);

  return VOID_VAL;
}

void SDLCALL postMessage(void *param)
{
    SDL_Event event;
    SDL_UserEvent userevent;

    userevent.type = SDL_USEREVENT + 0; // should be a more official Timer type
    userevent.code = 0;
    userevent.data1 = param;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
}

void onEvent(Event event, Point pos) {
  if (vm->onEvent(event, pos)) {
    vm->repaint();
    SDL_RenderPresent(rend2);
  }
}

void suspend() {
  vm->repaint();

  SDL_RenderPresent(rend2);

  SDL_Event event;

  // Events management
  while (true) {
#ifdef __EMSCRIPTEN__
    emscripten_sleep(0);
#endif
    if (!SDL_WaitEvent(&event)) {
      printf("%s\n", SDL_GetError());
      exit(0);
    }

    switch (event.type) {
    case SDL_USEREVENT: {
        Value value = VOID_VAL;

        ((ObjInstance *) event.user.data1)->onReturn(value);
        vm->repaint();
        SDL_RenderPresent(rend2);
      }
      break;

    case SDL_MOUSEBUTTONDOWN:
      onEvent(EVENT_PRESS, {event.button.x, event.button.y});
      break;

    case SDL_MOUSEBUTTONUP:
      onEvent(EVENT_RELEASE, {event.button.x, event.button.y});
      break;

    case SDL_QUIT:
      exit(0);
      break;

    case SDL_KEYDOWN:
      // keyboard API for key pressed
      switch (event.key.keysym.scancode) {
      case SDL_SCANCODE_W:
      case SDL_SCANCODE_UP:
//        dest.y -= speed / 30;
        break;

      case SDL_SCANCODE_A:
      case SDL_SCANCODE_LEFT:
//        dest.x -= speed / 30;
        break;

      case SDL_SCANCODE_S:
      case SDL_SCANCODE_DOWN:
//        dest.y += speed / 30;
        break;

      case SDL_SCANCODE_D:
      case SDL_SCANCODE_RIGHT:
//        dest.x += speed / 30;
        break;

      default:
        break;
      }
  }
  }
}
#endif

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
#include <stdio.h>
#include <stdlib.h>
//#define TEST
#ifndef TEST
#include <string.h>
#include "compiler.hpp"
#include "vm.hpp"
#include "qni.hpp"

const char *qedLib =
"void println(String str);"
"//void print(String str)\n"
"int max(int a, int b);"
""
"var WIDTH = 1;"
"var HEIGHT = 2;"
"var OBLIQUE = 3;"
"int COLOR_RED = 0xFF0000;"
"int COLOR_GREEN = 0x00FF00;"
"int COLOR_BLUE = 0x0000FF;"
"float clock();"
"void oval(int pos, int size);"
"void rect(int pos, int size);"
"int getTextSize(String text);"
"int getInstanceSize(int instance);"
"void displayText(String text, int pos, int size);"
"void displayInstance(int instance, int pos, int size);"
"void onInstanceEvent(int instance, int pos);"
""
"void Timer(int timeoutMillis) {"
"  var _timerObj;"
""
"  void reset();"
"};"
""
"void CoList() {"
"  var _coListObj;"
""
"  void end();"
"  bool remove(int index);"
"  bool yield();"
"  bool process();"
"}\n"
;

static void repl() {
  char line[1024] = "";
  char buffer[32768] = "";
  int length = 0;

  strcpy(buffer, qedLib);
  Scanner scanner(buffer);
  Parser parser(scanner);
  Compiler compiler(parser, NULL);
  ObjInstance *instance = newInstance(NULL);
  VM vm(instance, false);
  ObjClosure *closure = instance->coThread->pushClosure(compiler.function);
  ObjFunction *function = compiler.compile();

//  strcpy(buffer, "");
//  scanner.reset(buffer);

  for (;;) {
    if (function != NULL && instance->coThread->call(closure, instance->coThread->stackTop - instance->coThread->fields - 1)) {
      if (vm.run() == INTERPRET_OK)
        length += strlen(line);
      else {
        buffer[length] = '\0';
        scanner.reset(&buffer[length]);
      }

      function->chunk.reset();
      instance->coThread->reset();
    }

    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    strcat(buffer, line);
    function = compiler.compile();
  }

  freeObjects();
}

static char *readFile(const char *path) {
  FILE *file = fopen(path, "rb");

  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);

  size_t fileSize = ftell(file);

  rewind(file);

  int qedLibLength = strlen(qedLib);
  char *buffer = (char *) malloc(qedLibLength + fileSize + 1);

  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }

  strcpy(buffer, qedLib);

  size_t bytesRead = fread(&buffer[qedLibLength], sizeof(char), fileSize, file);

  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }

  buffer[qedLibLength + bytesRead] = '\0';
  fclose(file);
  return buffer;
}

static void runFile(const char *path) {
  char *source = readFile(path);
  Scanner scanner(source);
  Parser parser(scanner);
  Compiler compiler(parser, NULL);
  ObjInstance *instance = newInstance(NULL);
  VM vm(instance, true);
  ObjClosure *closure = instance->coThread->pushClosure(compiler.function);
  ObjFunction *function = compiler.compile();

  if (function == NULL)
    exit(65);

  InterpretResult result = vm.interpret(closure);

  free(source);
//  freeObjects();

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char *argv[]) {
  if (argc == 1)
    repl();
  else if (argc == 2)
    runFile(argv[1]);
  else {
    fprintf(stderr, "Usage: qed [path]\n");
    exit(64);
  }

  return 0;
}
#else
// std
#include <assert.h>

// opengl
//#include <GL/glew.h>

// sdl
#include <SDL.h>
#include <SDL_image.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <SDL_timer.h>
#include <SDL_ttf.h>
#endif

#define SCREEN_SIZE_X 512
#define SCREEN_SIZE_Y 512

int fps = -1;
int fps2 = 0;

#ifdef __EMSCRIPTEN__
void my_callbackfunc(void *param)
#else
Uint32 SDLCALL my_callbackfunc(Uint32 interval, void *param)
#endif
{/*
    SDL_Event event;
    SDL_UserEvent userevent;

    userevent.type = SDL_USEREVENT;
    userevent.code = 0;
    userevent.data1 = NULL;
    userevent.data2 = param;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);*/
    fps = fps2;
    fps2 = 0;
    printf("FPS: %d\n", fps);
#ifdef __EMSCRIPTEN__
    emscripten_async_call(my_callbackfunc, NULL, 1000);
#else
    return interval;
#endif
}

// let us control our image position
// so that we can move it with our keyboard.
SDL_Rect dest;

// speed of box
int speed = 300;

// ----- Game loop
bool quit = false;

// creates a renderer to render our images
SDL_Renderer* rend;
SDL_Surface *background;
// loads image to our graphics hardware memory.
SDL_Texture* tex;

void mainLoop() {
  SDL_Event event;

  // Events management
  while (SDL_PollEvent(&event)) {
      switch (event.type) {

      case SDL_QUIT:
          // handling of close button
          quit = 1;
          break;

      case SDL_KEYDOWN:
          // keyboard API for key pressed
          switch (event.key.keysym.scancode) {
          case SDL_SCANCODE_W:
          case SDL_SCANCODE_UP:
              dest.y -= speed / 30;
              break;
          case SDL_SCANCODE_A:
          case SDL_SCANCODE_LEFT:
              dest.x -= speed / 30;
              break;
          case SDL_SCANCODE_S:
          case SDL_SCANCODE_DOWN:
              dest.y += speed / 30;
              break;
          case SDL_SCANCODE_D:
          case SDL_SCANCODE_RIGHT:
              dest.x += speed / 30;
              break;
          default:
              break;
          }
      }
  }

  // right boundary
  if (dest.x + dest.w > SCREEN_SIZE_X)
      dest.x = SCREEN_SIZE_X - dest.w;

  // left boundary
  if (dest.x < 0)
      dest.x = 0;

  // bottom boundary
  if (dest.y + dest.h > SCREEN_SIZE_Y)
      dest.y = SCREEN_SIZE_Y - dest.h;

  // upper boundary
  if (dest.y < 0)
      dest.y = 0;

  if (SDL_MUSTLOCK(background)) SDL_LockSurface(background);

  Uint8 * pixels = (Uint8 *) background->pixels;

  for (int i=0; i < 1048576; i++) {
    char randomByte = rand() % 255;
    pixels[i] = randomByte;
  }

  if (SDL_MUSTLOCK(background)) SDL_UnlockSurface(background);

  SDL_Texture *screenTexture = SDL_CreateTextureFromSurface(rend, background);

  //    SDL_RenderClear(rend);
  SDL_RenderCopy(rend, screenTexture, NULL, NULL);

  // clears the screen
  SDL_RenderCopy(rend, tex, NULL, &dest);

  // triggers the double buffers
  // for multiple rendering
  SDL_RenderPresent(rend);

  SDL_DestroyTexture(screenTexture);

  //        SDL_GL_SwapWindow(win);
  // calculates to 60 fps
  fps2++;
}

void get_text_and_rect(SDL_Renderer *renderer, int x, int y, char *text,
        TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect) {
    int text_width;
    int text_height;
    SDL_Surface *surface;
    SDL_Color textColor = {255, 255, 255, 0};

    surface = TTF_RenderText_Solid(font, text, textColor);
    *texture = SDL_CreateTextureFromSurface(renderer, surface);
    text_width = surface->w;
    text_height = surface->h;
    SDL_FreeSurface(surface);
    rect->x = x;
    rect->y = y;
    rect->w = text_width;
    rect->h = text_height;
}

int main (int argc, char* argv[])
{
  SDL_Event event;
  SDL_Rect rect1, rect2;
  SDL_Renderer *renderer;
  SDL_Texture *texture1, *texture2;
  SDL_Window *window;
  char *font_path = "./res/font/arial.ttf";
  int quit;

  // Init TTF.
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
  SDL_CreateWindowAndRenderer(512, 512, 0, &window, &renderer);
  TTF_Init();
  TTF_Font *font = TTF_OpenFont(font_path, 36);
  if (font == NULL) {
      fprintf(stderr, "error: font not found\n");
      exit(EXIT_FAILURE);
  }
  get_text_and_rect(renderer, 0, 0, "hello", font, &texture1, &rect1);
  get_text_and_rect(renderer, 0, rect1.y + rect1.h, "world", font, &texture2, &rect2);

  quit = 0;
  while (!quit) {
      while (SDL_PollEvent(&event) == 1) {
          if (event.type == SDL_QUIT) {
              quit = 1;
          }
      }
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
      SDL_RenderClear(renderer);

      // Use TTF textures.
      SDL_RenderCopy(renderer, texture1, NULL, &rect1);
      SDL_RenderCopy(renderer, texture2, NULL, &rect2);

      SDL_RenderPresent(renderer);
  }

  // Deinit TTF.
  SDL_DestroyTexture(texture1);
  SDL_DestroyTexture(texture2);
  TTF_Quit();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return EXIT_SUCCESS;

  // ----- Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
  {
      fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
      return 1;
  }

  Uint32 ticks1 = SDL_GetTicks();
  SDL_Delay(5); // busy-wait
  Uint32 ticks2 = SDL_GetTicks();
  assert(ticks2 >= ticks1 + 5);

    // ----- Create window
  SDL_GL_SetSwapInterval(1);

  SDL_Window* win = SDL_CreateWindow("My Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_SIZE_X, SCREEN_SIZE_Y, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!win)
    {
        fprintf(stderr, "Error creating window.\n");
        return 2;
    }
/*
    // ----- SDL OpenGL settings
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
*/
    // ----- SDL OpenGL context
//    SDL_GLContext glContext = SDL_GL_CreateContext(win);

    // ----- SDL v-sync
//    SDL_GL_SetSwapInterval(1);
/*
    // ----- GLEW
    glewInit();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
*/
    // triggers the program that controls
    // your graphics hardware and sets flags
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;
 
    // creates a renderer to render our images
    rend = SDL_CreateRenderer(win, -1, render_flags);

    background = SDL_CreateRGBSurface(0, 512, 512, 32, 0, 0, 0, 0);

    // creates a surface to load an image into the main memory
    SDL_Surface* surface;
 
    // please provide a path for your image
    surface = IMG_Load("qed-small6.png");

    // loads image to our graphics hardware memory.
    tex = SDL_CreateTextureFromSurface(rend, surface);
 
    // clears main-memory
    SDL_FreeSurface(surface);
 
    // connects our texture with dest to control position
    SDL_QueryTexture(tex, NULL, NULL, &dest.w, &dest.h);

     // adjust height and width of our image box.
//    dest.w /= 6;
//    dest.h /= 6;
 
    // sets initial x-position of object
    dest.x = (SCREEN_SIZE_X - dest.w) / 2;
 
    // sets initial y-position of object
    dest.y = (SCREEN_SIZE_Y - dest.h) / 2;

    #ifdef __EMSCRIPTEN__
    emscripten_async_call(my_callbackfunc, NULL, 1000);
    emscripten_set_main_loop(mainLoop, 0, 1);
    #else
    SDL_TimerID my_timer_id = SDL_AddTimer(1000, my_callbackfunc, NULL);
    while (quit == false) {
      mainLoop();
      SDL_Delay(1000 / 60);
    }
    #endif

    // destroy texture
    SDL_DestroyTexture(tex);

    // destroy renderer
    SDL_DestroyRenderer(rend);

    // destroy window
    SDL_DestroyWindow(win);

    // ----- Clean up
//    SDL_GL_DeleteContext(glContext);

    return 0;
}
#endif

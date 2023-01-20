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
#include <time.h>
#include "qni.hpp"

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
QNI_FN(println) {
  printf("%s\n", ((ObjString *) args[0].as.obj)->chars);
  return VOID_VAL;
}

QNI_FN(max) {
  long arg0 = args[0].as.integer;
  long arg1 = args[1].as.integer;

  return INT_VAL(arg0 > arg1 ? arg0 : arg1);
}

struct ValueStack2 {
	std::stack<Value> map[64];//ATTRIBUTE_END];

  ValueStack2() {
    push(4, INT_VAL(0xFFFFFF));
  }

	void push(int key, Value value) {
    map[key].push(value);
  }

	void pop(int key) {
    map[key].pop();
  }

	Value get(int key) {
    return map[key].top();
  }
};

ValueStack2 attStack;

QNI_FN(pushAttribute) {
  long uiIndex = args[0].as.integer;
  Value &value = args[1];

  attStack.push(uiIndex, value);
  return VOID_VAL;
}

QNI_FN(popAttribute) {
  long uiIndex = args[0].as.integer;

  attStack.pop(uiIndex);
  return VOID_VAL;
}

QNI_FN(rect) {
  long posP = args[0].as.integer;
  long sizeP = args[1].as.integer;
  Point pos = {posP >> 32, posP & 0xFFFFFFFF};
  Point size = {sizeP >> 32, sizeP & 0xFFFFFFFF};
  SDL_Rect rectangle;

  rectangle.x = pos[0];
  rectangle.y = pos[1];
  rectangle.w = size[0];
  rectangle.h = size[1];

  int color = attStack.get(4).as.integer;
	SDL_SetRenderDrawColor(rend2, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 0xFF);
  SDL_RenderFillRect(rend2, &rectangle);
  return VOID_VAL;
}

QNI_FN(oval) {
  long posP = args[0].as.integer;
  long sizeP = args[1].as.integer;
  Point pos = {posP >> 32, posP & 0xFFFFFFFF};
  Point size = {sizeP >> 32, sizeP & 0xFFFFFFFF};
  int rx = size[0] >> 1;
  int ry = size[1] >> 1;

  int color = attStack.get(4).as.integer;
  filledEllipseRGBA(rend2, pos[0] + rx, pos[1] + ry, rx, ry,
                    (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 0xFF);
  return VOID_VAL;
}

QNI_FN(clock) {
  return FLOAT_VAL(((double) clock()) / CLOCKS_PER_SEC);
}

SDL_Window* win = NULL;

static TTF_Font *getFont() {
  static TTF_Font *font = NULL;

  if (!font) {
    TTF_Init();
    font = TTF_OpenFont("./res/font/arial.ttf", 30);
  }
  return font;
}

void initDisplay() {
  if (!win) {
    // ----- Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
      fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
      assert(false);
    }

    Uint32 ticks1 = SDL_GetTicks();
    SDL_Delay(5); // busy-wait
    Uint32 ticks2 = SDL_GetTicks();
    assert(ticks2 >= ticks1 + 5);

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

    const SDL_Rect* dstrect;
    SDL_Color color;
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

  return;
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

  return;
}

QNI_FN(getTextSize) {
  int width;
  int height;
  const char *text = ((ObjString *) args[0].as.obj)->chars;

  TTF_SizeUTF8(getFont(), text, &width, &height);
  return INT_VAL((((long) width) << 32) | height);
}

QNI_FN(getInstanceSize) {
  ObjInstance *instance = (ObjInstance *) args[0].as.obj;
  Point size = instance->recalculateLayout();

  return INT_VAL((((long) size[0]) << 32) | size[1]);
}

QNI_FN(displayText) {
  const char *text = ((ObjString *) args[0].as.obj)->chars;
  long posP = args[1].as.integer;
  long sizeP = args[2].as.integer;
  Point pos = {posP >> 32, posP & 0xFFFFFFFF};
  Point size = {sizeP >> 32, sizeP & 0xFFFFFFFF};
  SDL_Surface *textSurface = TTF_RenderText_Blended(getFont(), text, {255, 255, 255, 0});
  SDL_Texture *textTexture = SDL_CreateTextureFromSurface(rend2, textSurface);
  SDL_Rect rectangle;

  rectangle.x = pos[0];
  rectangle.y = pos[1];
  rectangle.w = size[0];
  rectangle.h = size[1];
  SDL_RenderCopy(rend2, textTexture, NULL, &rectangle);
  SDL_FreeSurface(textSurface);
  SDL_DestroyTexture(textTexture);

  return VOID_VAL;
}

QNI_FN(displayInstance) {
  ObjInstance *instance = (ObjInstance *) args[0].as.obj;
  long posP = args[1].as.integer;
  long sizeP = args[2].as.integer;
  Point pos = {posP >> 32, posP & 0xFFFFFFFF};
  Point size = {sizeP >> 32, sizeP & 0xFFFFFFFF};

  instance->paint(pos, size);
  return VOID_VAL;
}

QNI_FN(onInstanceEvent) {
  ObjInstance *instance = (ObjInstance *) args[0].as.obj;
  long posP = args[1].as.integer;
  Point pos = {posP >> 32, posP & 0xFFFFFFFF};

  instance->onEvent(pos);
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

#ifdef __EMSCRIPTEN__
void timerCallback(void *param)
#else
Uint32 SDLCALL timerCallback(Uint32 interval, void *param)
#endif
{
    SDL_Event event;
    SDL_UserEvent userevent;

    userevent.type = SDL_USEREVENT + 0; // should be a more official Timer type
    userevent.code = 0;
    userevent.data1 = param;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
#ifndef __EMSCRIPTEN__
    return 0;
#endif
}

struct TimerInternal : Internal {
  SDL_TimerID timer;

  TimerInternal(int timeoutMillis, ObjInstance *instance) {
    timer = SDL_AddTimer(timeoutMillis, timerCallback, instance);
  }
};

QNI_CLASS(Timer) {
  ObjInstance *instance = vm.instance;
  ObjInternal *objInternal = (ObjInternal *) AS_OBJ(args[1]);

  objInternal->object = new TimerInternal(args[0].as.integer, instance);
  return {INTERPRET_HALT};
}

struct CoListThread : Internal {
  CoThread *coThread;
  CoListThread *previous;
  CoListThread *next;

  CoListThread(CoThread *coThread, CoListThread *previous, CoListThread *next);
};

CoListThread::CoListThread(CoThread *coThread, CoListThread *previous, CoListThread *next) {
  this->coThread = coThread;
  this->previous = previous;
  this->next = next;
}

QNI_CLASS(CoList) {
  ObjInstance *instance = vm.instance;
  ObjInternal *objInternal = (ObjInternal *) AS_OBJ(instance->coThread->fields[1]);
	CoListThread *main = new CoListThread(instance->coThread->caller, NULL, NULL);

  objInternal->object = main->next = main->previous = main;
  return {INTERPRET_HALT};
}

static InterpretValue qni__CoListEnd(VM &vm, int argCount, Value *args) {
  ObjNativeClass *objNativeClass = (ObjNativeClass *) vm.getFrame()->closure->function->native;
  ObjInternal *objInternal = (ObjInternal *) objNativeClass->arg;
  Value value = BOOL_VAL(true);
  CoListThread *main = (CoListThread *) objInternal->object;

  objInternal->object = main->next;
  main->next->coThread->onReturn(value);
  main->previous->next = main->next;
  main->next->previous = main->previous;
  main->coThread->caller = main->next->coThread;
  delete main;
  return {INTERPRET_HALT};
}

QNI_CLASS(CoList_yield) {
  ObjClosure *closure1 = vm.getFrame(1)->closure;
  ObjClosure *closure = vm.getFrame()->closure;
  ObjInstance *instance = NULL;//closure->instance;
  ObjInternal *objInternal = (ObjInternal *) AS_OBJ(instance->coThread->fields[1]);
  CoListThread *main = (CoListThread *) objInternal->object;

  if (vm.instance->coThread == main->coThread) {
		Value value = BOOL_VAL(true);

		objInternal->object = main->next;
		vm.instance->coThread = main->next->coThread;
		vm.instance->coThread->onReturn(value);
		return {INTERPRET_CONTINUE};
	} else {
    CoListThread *newUnit = new CoListThread(vm.instance->coThread, main->previous, main);

		main->previous->next = newUnit;
		main->previous = newUnit;
    ObjNativeClass *endObj = newNativeClass(qni__CoListEnd);
    closure1->function->native = &endObj->obj;
    endObj->arg = objInternal;
	  return {INTERPRET_HALT};
  }
}
/*
    		put("Timer", new Executer() {
    			public void run(VM &vm, Obj obj) {
    				obj.data[1] = process.setTimer((Integer) obj.data[0], process, obj);
    				haltOp.eval(process, obj);
    			}

    			public void cleanup(VM &vm, Obj obj) {
    				super.cleanup(process, obj);
    				process.resetTimer(0, obj.data[1]);
    			}
    		});

    		put("CoList", new Executer() {
    			public void run(VM &vm, Obj obj) {
    				ArrayList<Obj> list = new ArrayList<Obj>();

    				list.add(null);
    				obj.data[0] = new Object[] {list, new int[] {0}};
    				haltOp.eval(process, obj);
    			}
    		});

    		put("CoList.yield", new Executer() {
    			@SuppressWarnings("unchecked")
    			public void run(VM &vm, Obj obj) {
    				Object[] objs = (Object[]) obj.parent.data[0];
    				int[] index = (int[]) objs[1];
    				ArrayList<Obj> list = (ArrayList<Obj>) objs[0];

    				if (index[0] != 0) {
    					list.set(index[0], obj);

    					if (++index[0] >= list.size())
    						index[0] = 0;

    					process.removeCallSet(obj);
    					list.get(index[0]).onReturn(process, true);
    				}
    				else {
    					list.add(obj);
    					process.removeCallSet(obj);
    					Op.haltOp.eval(process, obj);
    				}
    			}
    		});

    		put("CoList.end", new Executer() {
    			@SuppressWarnings("unchecked")
    			public void run(VM &vm, Obj obj) {
    				Object[] objs = (Object[]) obj.parent.data[0];
    				int[] index = (int[]) objs[1];
    				ArrayList<Obj> list = (ArrayList<Obj>) objs[0];

    				if (index[0] != 0) {
    					boolean value = true;

    					list.remove(index[0]);

    					if (index[0] >= list.size()) {
    						index[0] = 0;
    						value = list.size() > 1;
    					}

    					process.removeCallSet(obj);
    					list.get(index[0]).onReturn(process, value);
    				}
    				else {
    					process.removeCallSet(obj);
    					Op.haltOp.eval(process, obj);
    				}
    			}
    		});

    		put("CoList.process", new Executer() {
    			@SuppressWarnings("unchecked")
    			public void run(VM &vm, Obj obj) {
    				Object[] objs = (Object[]) obj.parent.data[0];
    				int[] index = (int[]) objs[1];
    				ArrayList<Obj> list = (ArrayList<Obj>) objs[0];

    				if (index[0] == 0 && list.size() > 1) {
						list.set(0, obj);
						Op.haltOp.eval(process, obj);
				    	list.get(++index[0]).onReturn(process, true);
					}
					else
						obj.onReturn(process, false);
    			}
    		});
//
    		put("CoList.remove", new Executer() {
    			@SuppressWarnings("unchecked")
    			public void run(VM &vm, Obj obj) {
    				Object[] objs = (Object[]) obj.parent.data[0];
    				ArrayList<Obj> list = (ArrayList<Obj>) objs[0];
    				int index = (Integer) obj.data[0];

    				list.remove(index + 1);
    				obj.onReturn(process, true);
    			}
    		});
*/
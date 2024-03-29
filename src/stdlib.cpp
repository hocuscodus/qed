/*
 * The QED Programming Language
 * Copyright (C) 2022-2023  Hocus Codus Software inc.
 *
 * All rights reserved.
 */
#include <time.h>
#include "qni.hpp"

// std
#include <assert.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

ValueStack2 attStack;

QNI_FN(pushAttribute) {
  long uiIndex = AS_INT(args[0]);
  Value &value = args[1];

  attStack.push(uiIndex, value);
  return VOID_VAL;
}

QNI_FN(popAttribute) {
  long uiIndex = AS_INT(args[0]);

  attStack.pop(uiIndex);
  return VOID_VAL;
}

QNI_FN(getInstanceSize) {
  CoThread *coThread = (CoThread *) AS_OBJ(args[0]);
  Point size = coThread->recalculateLayout();

  return INT_VAL((((long) size[0]) << 16) | size[1]);
}

QNI_FN(displayInstance) {
  CoThread *coThread = (CoThread *) AS_OBJ(args[0]);
  long posP = AS_INT(args[1]);
  long sizeP = AS_INT(args[2]);
  Point pos = {(int) (posP >> 16), (int) (posP & 0xFFFF)};
  Point size = {(int) (sizeP >> 16), (int) (sizeP & 0xFFFF)};

  coThread->paint(pos, size);
  return VOID_VAL;
}

QNI_FN(onInstanceEvent) {
  CoThread *coThread = (CoThread *) AS_OBJ(args[0]);
  Event event = (Event) AS_INT(args[1]);
  long posP = AS_INT(args[2]);
  long sizeP = AS_INT(args[3]);
  Point pos = {(int) (posP >> 16), (int) (posP & 0xFFFF)};
  Point size = {(int) (sizeP >> 16), (int) (sizeP & 0xFFFF)};

  return BOOL_VAL(coThread->onEvent(event, pos, size));
}

QNI_FN(println) {
  printf("%s\n", ((ObjString *) AS_OBJ(args[0]))->chars);
  return VOID_VAL;
}

QNI_FN(max) {
  long arg0 = AS_INT(args[0]);
  long arg1 = AS_INT(args[1]);

  return INT_VAL(arg0 > arg1 ? arg0 : arg1);
}

QNI_FN(clock) {
  return FLOAT_VAL(((double) clock()) / CLOCKS_PER_SEC);
}
/*
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

  TimerInternal(int timeoutMillis, CoThread *coThread) {
//    timer = SDL_AddTimer(timeoutMillis, timerCallback, coThread);
  }
};
*/

QNI_CLASS(Timer) {
  ObjInternal *objInternal = (ObjInternal *) AS_OBJ(args[1]);

//  objInternal->object = new TimerInternal(AS_INT(args[0]), coThread);
  return {INTERPRET_OK};//HALT};
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

QNI_CLASS(CoList) {/*
  ObjInternal *objInternal = (ObjInternal *) AS_OBJ(current->fields[1]);
	CoListThread *main = new CoListThread(current->caller, NULL, NULL);

  objInternal->object = main->next = main->previous = main;*/
  return {INTERPRET_OK};//HALT};
}

static InterpretResult qni__CoListEnd(VM &vm, int argCount, Value *args) {
  ObjNativeClass *objNativeClass = NULL;//(ObjNativeClass *) vm.getFrame()->closure->function->native;
  ObjInternal *objInternal = (ObjInternal *) objNativeClass->arg;
  Value value = BOOL_VAL(true);
  CoListThread *main = (CoListThread *) objInternal->object;

  objInternal->object = main->next;
  main->next->coThread->onReturn(value);
  main->previous->next = main->next;
  main->next->previous = main->previous;
//  main->caller = main->next;
  delete main;
  return INTERPRET_OK;//HALT;
}

QNI_CLASS(CoList_yield) {
  ObjClosure *closure1 = NULL;//vm.getFrame(1)->closure;
  ObjClosure *closure = NULL;//vm.getFrame()->closure;
  CoThread *coThread = NULL;//closure->coThread;
  ObjInternal *objInternal = (ObjInternal *) AS_OBJ(coThread->fields[1]);
  CoListThread *main = (CoListThread *) objInternal->object;

  if (coThread == main->coThread) {
		Value value = BOOL_VAL(true);

		objInternal->object = main->next;
		coThread = main->next->coThread;
		coThread->onReturn(value);
		return {INTERPRET_OK};//INTERPRET_CONTINUE should switch thread here
	} else {
    CoListThread *newUnit = new CoListThread(coThread, main->previous, main);

		main->previous->next = newUnit;
		main->previous = newUnit;
    ObjNativeClass *endObj = newNativeClass(qni__CoListEnd);
    closure1->function->native = &endObj->obj;
    endObj->arg = objInternal;
	  return {INTERPRET_OK};//HALT};
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
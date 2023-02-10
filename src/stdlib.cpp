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
  ObjInstance *instance = (ObjInstance *) AS_OBJ(args[0]);
  Point size = instance->recalculateLayout();

  return INT_VAL((((long) size[0]) << 16) | size[1]);
}

QNI_FN(displayInstance) {
  ObjInstance *instance = (ObjInstance *) AS_OBJ(args[0]);
  long posP = AS_INT(args[1]);
  long sizeP = AS_INT(args[2]);
  Point pos = {(int) (posP >> 16), (int) (posP & 0xFFFF)};
  Point size = {(int) (sizeP >> 16), (int) (sizeP & 0xFFFF)};

  instance->paint(pos, size);
  return VOID_VAL;
}

QNI_FN(onInstanceEvent) {
  ObjInstance *instance = (ObjInstance *) AS_OBJ(args[0]);
  Event event = (Event) AS_INT(args[1]);
  long posP = AS_INT(args[2]);
  long sizeP = AS_INT(args[3]);
  Point pos = {(int) (posP >> 16), (int) (posP & 0xFFFF)};
  Point size = {(int) (sizeP >> 16), (int) (sizeP & 0xFFFF)};

  return BOOL_VAL(instance->onEvent(event, pos, size));
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

  TimerInternal(int timeoutMillis, ObjInstance *instance) {
//    timer = SDL_AddTimer(timeoutMillis, timerCallback, instance);
  }
};
*/
QNI_CLASS(Timer) {
  ObjInstance *instance = vm.instance;
  ObjInternal *objInternal = (ObjInternal *) AS_OBJ(args[1]);

//  objInternal->object = new TimerInternal(AS_INT(args[0]), instance);
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
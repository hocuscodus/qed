"use strict";
const canvas = document.getElementById("canvas");
let postHandler = null;
let attributeStacks = [];
const ctx = canvas.getContext("2d");
function toColor(color) {return "#" + color.toString(16).padStart(6, '0');}
let getAttribute = function(index) {
  return attributeStacks[index][attributeStacks[index].length - 1];
}
function voidHandler_() {
}
function anyHandler_(value) {
}
function intHandler_(value) {
}
function floatHandler_(value) {
}
function boolHandler_(value) {
}
function stringHandler_(value) {
}
let WIDTH = null;
let HEIGHT = null;
let OBLIQUE = null;
let COLOR_RED = null;
let COLOR_GREEN = null;
let COLOR_YELLOW = null;
let COLOR_BLUE = null;
let COLOR_BLACK = null;
function Yield(obj, _HandlerFn_) {
  this.obj = obj;
  obj.yieldHandler = (function() {
    _HandlerFn_(true);
  });
}
function process(obj) {
  if (obj instanceof Array) {
    let size = obj.size();

    for (let index = 0; index < size; index++)
      process(obj[index]);
  }
  else
    obj.yieldHandler();
}
function println(str) {
  console.log(str);
}
function post_(_HandlerFn_) {
  if (postHandler != null)
    console.log("postHandler not null");

  postHandler = _HandlerFn_;
}
function executeEvents_() {
  while (postHandler != null) {
    const fn = postHandler;
  
    postHandler = null;
    fn();
  }

  _refresh();
}
function max(a, b) {
  return a > b ? a : b;
}
function rand() {
  return Math.random();
}
function clock() {
}
function saveContext() {
  ctx.save();
}
function restoreContext() {
  ctx.restore();
}
function oval(pos, size) {
  ctx.fillStyle = toColor(getAttribute(10));
  ctx.globalAlpha = "" + getAttribute(11);
  ctx.beginPath();
  ctx.ellipse((pos >> 16) + (size >> 17), (pos & 65535) + ((size & 65535) >> 1), size >> 17, (size & 65535) >> 1, 0, 0, 2*Math.PI);
  ctx.fill();
//  ctx.clip();
}
function rect(pos, size) {
  ctx.fillStyle = toColor(getAttribute(10));
  ctx.globalAlpha = "" + getAttribute(11);
  ctx.beginPath();
  ctx.fillRect((pos >> 16), (pos & 65535), size >> 16, size & 65535);
}
function pushAttribute(index, value) {
  if (attributeStacks[index] == undefined)
    attributeStacks[index] = [];

  attributeStacks[index].push(value);
}
function pushAttribute$_(index, value) {
  pushAttribute(index, value);
}
function popAttribute(index) {
  attributeStacks[index].pop();
}
function getTextSize(text) {
  ctx.font = getAttribute(4) + "px Arial";

  const textSize = ctx.measureText(text);
  const height = textSize.fontBoundingBoxAscent + textSize.fontBoundingBoxDescent;
  return (textSize.width << 16) | height;
}
function displayText(text, pos, size) {
  let pos1 = [(pos >> 16), (pos & 0xFFFF)];
  let size1 = [(size >> 16), (size & 0xFFFF)];
  ctx.font = getAttribute(4) + "px Arial";
  ctx.fillStyle = toColor(getAttribute(10));
  ctx.globalAlpha = getAttribute(11);
  ctx.textBaseline = "top";
  ctx.fillText(text, pos1[0], pos1[1]);
}
function Timer(timeoutMillis, _HandlerFn_) {
  this.timeoutMillis = timeoutMillis;
  this.reset = function() {
  }
  setTimeout(function() {
    _HandlerFn_(true);
    _refresh();
  }, timeoutMillis);
  this.reset;
}
function Time(Func, _HandlerFn_) {
  this.Func = Func;
  console.time("Time");
  new Func(() => {
    console.timeEnd("Time");
    _HandlerFn_();
  });;
}
function time(func) {
    console.time("time");
  func();
  console.timeEnd("time");;
}
function Animation(_HandlerFn_) {
  requestAnimationFrame((millis) => {
  _HandlerFn_(true);
  _refresh();
});;
}
function QEDBaseArray_() {
  this.size = function() {
    return (0);
  }
  this.insert = function(pos, size$) {
  }
  this.Insert = function(pos, size$, _HandlerFn_) {
    this.pos = pos;
    this.size$ = size$;
  }
  this.Push = function(_HandlerFn_) {
  }
  this.pop = function() {
  }
  this.get = function(pos) {
  }
  this.set = function(pos, value) {
  }
  this.get$_ = function(index) {
  }
  this.set$_ = function(index, value) {
  }
  this.UI_ = function() {
    this.Layout_ = function() {
      this.size$ = null;
      this.paint = function(pos0, pos1, size0, size1) {
      }
      this.onEvent = function(event, pos0, pos1, size0, size1) {
      }
      this.size$ = 0;
      this.onEvent;
    }
    this.Layout_;
  }
  this.ui_ = null;
  this.setUI = function() {
    this.ui_ = new this.UI_();
  }
}
function InitFn(pos, _HandlerFn_) {
  this.pos = pos;
}
function SQEDArray(limits, Init, _HandlerFn_) {
  this.limits = limits;
  this.Init = Init;
  const SQEDArray$this = this;
  this.size = function() {
    let s = 1;
    {
      let index = this.dims.length - 1;
      while(index >= 0) {
        s *= this.dims[index];
        index--;
      }
    }
    return (s);
  }
  this.insert = function(pos, size$) {
  }
  this.Insert = function(pos, size$, _HandlerFn_$) {
    this.pos = pos;
    this.size$ = size$;
    const Insert$this = this;
    this.newSize = null;
    this.newSize = [...this.size$];
    {
      let index = SQEDArray$this.dims.length - 1;
      while(index >= 0) {
        this.newSize[index] += SQEDArray$this.dims[index];
        index--;
      }
    }
    new SQEDArray$this.InsertLevel(SQEDArray$this, SQEDArray$this.dims, this.pos, this.size$, this.newSize, new Array(this.size$.length).fill(0), 0, (function Lambda_() {
      SQEDArray$this.dims = Insert$this.newSize;
      post_(_HandlerFn_$);
      return;
    }));
  }
  this.InsertLevel = function(array, dims, pos, size$, newSize, pp, level, _HandlerFn_$) {
    this.array = array;
    this.dims = dims;
    this.pos = pos;
    this.size$ = size$;
    this.newSize = newSize;
    this.pp = pp;
    this.level = level;
    const InsertLevel$this = this;
    new (function W9$_(i1$_) {
      if (InsertLevel$this.level < SQEDArray$this.dims.length - 1) {
        {
          pp[level] = 0;
          (function while2$_() {
            if (pp[level] < pos[level]) {
              new SQEDArray$this.InsertLevel(array[pp[level]], InsertLevel$this.dims, InsertLevel$this.pos, InsertLevel$this.size$, InsertLevel$this.newSize, InsertLevel$this.pp, InsertLevel$this.level + 1, (function Lambda_() {
                pp[level]++;
                post_(while2$_);
              }));
            }
            else {
              if (InsertLevel$this.size$[InsertLevel$this.level] !== 0) {
                pp[level] = dims[level] - 1;
                while(pp[level] >= pos[level]) {
                  array[pp[level] + InsertLevel$this.size$[level]] = array[pp[level]];
                  pp[level]--;
                }
              }
              {
                pp[level] = pos[level];
                (function while4$_() {
                  if (pp[level] < pos[level] + InsertLevel$this.size$[level]) {
                    array[pp[level]] = [];
                    new SQEDArray$this.InsertLevel(array[pp[level]], new Array(InsertLevel$this.size$.length).fill(0), new Array(InsertLevel$this.size$.length).fill(0), InsertLevel$this.newSize, InsertLevel$this.newSize, InsertLevel$this.pp, InsertLevel$this.level + 1, (function Lambda_() {
                      pp[level]++;
                      post_(while4$_);
                    }));
                  }
                  else {
                    pp[level] = pos[level] + InsertLevel$this.size$[level];
                    (function while5$_() {
                      if (pp[level] < newSize[level]) {
                        new SQEDArray$this.InsertLevel(array[pp[level]], InsertLevel$this.dims, InsertLevel$this.pos, InsertLevel$this.size$, InsertLevel$this.newSize, InsertLevel$this.pp, InsertLevel$this.level + 1, (function Lambda_() {
                          pp[level]++;
                          post_(while5$_);
                        }));
                      }
                      else
                        i1$_();
                    })();
                  }
                })();
              }
            }
          })();
        }
      }
      else {
        if (InsertLevel$this.size$[InsertLevel$this.level] !== 0) {
          pp[level] = dims[level] - 1;
          while(pp[level] >= pos[level]) {
            array[pp[level] + InsertLevel$this.size$[level]] = array[pp[level]];
            pp[level]--;
          }
        }
        {
          pp[level] = pos[level];
          (function while7$_() {
            if (pp[level] < pos[level] + InsertLevel$this.size$[level]) {
              new SQEDArray$this.Init(InsertLevel$this.pp, (function Lambda_(_ret) {
                array[pp[level]] = _ret;
                pp[level]++;
                post_(while7$_);
              }));
            }
            else
              i1$_();
          })();
        }
      }
    })((function c8$_() {
      post_(_HandlerFn_$);
      return;
    }));
  }
  this.dims = new Array(this.limits.length).fill(0);
  new this.Insert(new Array(this.limits.length).fill(0), this.limits, (function Lambda_() {
    post_((function lambda_() {
      _HandlerFn_(SQEDArray$this);
    }));
    return;
    SQEDArray$this.size;
    SQEDArray$this.insert;
    SQEDArray$this.Insert;
    SQEDArray$this.InsertLevel;
  }));
}
function VInitFn(pos, _HandlerFn_) {
  this.pos = pos;
}
function VSQEDArray(limits, Init, _HandlerFn_) {
  this.limits = limits;
  this.Init = Init;
  const VSQEDArray$this = this;
  this.size = function() {
    let s = 1;
    {
      let index = this.dims.length - 1;
      while(index >= 0) {
        s *= this.dims[index];
        index--;
      }
    }
    return (s);
  }
  this.insert = function(pos, size$) {
  }
  this.Insert = function(pos, size$, _HandlerFn_$) {
    this.pos = pos;
    this.size$ = size$;
    const Insert$this = this;
    this.newSize = null;
    this.newSize = [...this.size$];
    {
      let index = VSQEDArray$this.dims.length - 1;
      while(index >= 0) {
        this.newSize[index] += VSQEDArray$this.dims[index];
        index--;
      }
    }
    new VSQEDArray$this.InsertLevel(VSQEDArray$this.dims, this.pos, this.size$, this.newSize, new Array(this.size$.length).fill(0), 0, (function Lambda_() {
      VSQEDArray$this.dims = Insert$this.newSize;
      post_(_HandlerFn_$);
      return;
    }));
  }
  this.InsertLevel = function(dims, pos, size$, newSize, pp, level, _HandlerFn_$) {
    this.dims = dims;
    this.pos = pos;
    this.size$ = size$;
    this.newSize = newSize;
    this.pp = pp;
    this.level = level;
    const InsertLevel$this = this;
    new (function W16$_(i10$_) {
      if (InsertLevel$this.level < VSQEDArray$this.dims.length - 1) {
        {
          pp[level] = 0;
          (function while11$_() {
            if (pp[level] < pos[level]) {
              new VSQEDArray$this.InsertLevel(InsertLevel$this.dims, InsertLevel$this.pos, InsertLevel$this.size$, InsertLevel$this.newSize, InsertLevel$this.pp, InsertLevel$this.level + 1, (function Lambda_() {
                pp[level]++;
                post_(while11$_);
              }));
            }
            else {
              pp[level] = pos[level];
              (function while12$_() {
                if (pp[level] < pos[level] + InsertLevel$this.size$[level]) {
                  array[pp[level]] = [];
                  new VSQEDArray$this.InsertLevel(new Array(InsertLevel$this.size$.length).fill(0), new Array(InsertLevel$this.size$.length).fill(0), InsertLevel$this.newSize, InsertLevel$this.newSize, InsertLevel$this.pp, InsertLevel$this.level + 1, (function Lambda_() {
                    pp[level]++;
                    post_(while12$_);
                  }));
                }
                else {
                  pp[level] = pos[level] + InsertLevel$this.size$[level];
                  (function while13$_() {
                    if (pp[level] < newSize[level]) {
                      new VSQEDArray$this.InsertLevel(InsertLevel$this.dims, InsertLevel$this.pos, InsertLevel$this.size$, InsertLevel$this.newSize, InsertLevel$this.pp, InsertLevel$this.level + 1, (function Lambda_() {
                        pp[level]++;
                        post_(while13$_);
                      }));
                    }
                    else
                      i10$_();
                  })();
                }
              })();
            }
          })();
        }
      }
      else {
        pp[level] = pos[level];
        (function while14$_() {
          if (pp[level] < pos[level] + InsertLevel$this.size$[level]) {
            new VSQEDArray$this.Init(InsertLevel$this.pp, (function Lambda_() {
              pp[level]++;
              post_(while14$_);
            }));
          }
          else
            i10$_();
        })();
      }
    })((function c15$_() {
      post_(_HandlerFn_$);
      return;
    }));
  }
  this.dims = new Array(this.limits.length).fill(0);
  new this.Insert(new Array(this.limits.length).fill(0), this.limits, (function Lambda_() {
    post_((function lambda_() {
      _HandlerFn_(VSQEDArray$this);
    }));
    return;
    VSQEDArray$this.size;
    VSQEDArray$this.insert;
    VSQEDArray$this.Insert;
    VSQEDArray$this.InsertLevel;
  }));
}
function sInitFn(pos) {
}
function Qui_(array, dims) {
  this.array = array;
}
function QEDArray(limits, init, Ui_, _HandlerFn_) {
  this.limits = limits;
  this.init = init;
  this.Ui_ = Ui_;
  this.size = function() {
    let s = 1;
    {
      let index = this.dims.length - 1;
      while(index >= 0) {
        s *= this.dims[index];
        index--;
      }
    }
    return (s);
  }
  this.insert = function(pos, size$) {
    let newSize = [...size$];
    {
      let index = QEDArray$this.dims.length - 1;
      while(index >= 0) {
        newSize[index] += QEDArray$this.dims[index];
        index--;
      }
    }
    QEDArray$this.insertLevel(QEDArray$this, QEDArray$this.dims, pos, size$, newSize, new Array(size$.length).fill(0), 0);
    QEDArray$this.dims = newSize;
    return;
  }
  this.insertLevel = function(array, dims, pos, size$, newSize, pp, level) {
    if (level < QEDArray$this.dims.length - 1) {
      {
        pp[level] = 0;
        while(pp[level] < pos[level]) {
          QEDArray$this.insertLevel(array[pp[level]], dims, pos, size$, newSize, pp, level + 1);
          pp[level]++;
        }
      }
      if (size$[level] !== 0) {
        pp[level] = dims[level] - 1;
        while(pp[level] >= pos[level]) {
          array[pp[level] + size$[level]] = array[pp[level]];
          pp[level]--;
        }
      }
      {
        pp[level] = pos[level];
        while(pp[level] < pos[level] + size$[level]) {
          array[pp[level]] = [];
          QEDArray$this.insertLevel(array[pp[level]], new Array(size$.length).fill(0), new Array(size$.length).fill(0), newSize, newSize, pp, level + 1);
          pp[level]++;
        }
      }
      {
        pp[level] = pos[level] + size$[level];
        while(pp[level] < newSize[level]) {
          QEDArray$this.insertLevel(array[pp[level]], dims, pos, size$, newSize, pp, level + 1);
          pp[level]++;
        }
      }
    }
    else {
      if (size$[level] !== 0) {
        pp[level] = dims[level] - 1;
        while(pp[level] >= pos[level]) {
          array[pp[level] + size$[level]] = array[pp[level]];
          pp[level]--;
        }
      }
      {
        pp[level] = pos[level];
        while(pp[level] < pos[level] + size$[level]) {
          array[pp[level]] = QEDArray$this.init(pp);
          pp[level]++;
        }
      }
    }
    return;
  }
  this.ui_ = null;
  this.setUI = function() {
    this.ui_ = new this.Ui_(QEDArray$this, QEDArray$this.dims);
  }
  this.dims = new Array(this.limits.length).fill(0);
  const QEDArray$this = this;
  this.insert(new Array(this.limits.length).fill(0), this.limits);
  this.size;
  this.insert;
  this.insertLevel;
}
function qedArray(limits, init, Ui_) {
  return (new QEDArray(limits, init, Ui_));
}
function vInitFn(pos) {
}
function VQEDArray(limits, init, _HandlerFn_) {
  this.limits = limits;
  this.init = init;
  this.size = function() {
    let s = 1;
    {
      let index = this.dims.length - 1;
      while(index >= 0) {
        s *= this.dims[index];
        index--;
      }
    }
    return (s);
  }
  this.insert = function(pos, size$) {
    let newSize = [...size$];
    {
      let index = VQEDArray$this.dims.length - 1;
      while(index >= 0) {
        newSize[index] += VQEDArray$this.dims[index];
        index--;
      }
    }
    VQEDArray$this.insertLevel(VQEDArray$this.dims, pos, size$, newSize, new Array(size$.length).fill(0), 0);
    VQEDArray$this.dims = newSize;
    return;
  }
  this.insertLevel = function(dims, pos, size$, newSize, pp, level) {
    if (level < VQEDArray$this.dims.length - 1) {
      {
        pp[level] = 0;
        while(pp[level] < pos[level]) {
          VQEDArray$this.insertLevel(dims, pos, size$, newSize, pp, level + 1);
          pp[level]++;
        }
      }
      {
        pp[level] = pos[level];
        while(pp[level] < pos[level] + size$[level]) {
          VQEDArray$this.insertLevel(new Array(size$.length).fill(0), new Array(size$.length).fill(0), newSize, newSize, pp, level + 1);
          pp[level]++;
        }
      }
      {
        pp[level] = pos[level] + size$[level];
        while(pp[level] < newSize[level]) {
          VQEDArray$this.insertLevel(dims, pos, size$, newSize, pp, level + 1);
          pp[level]++;
        }
      }
    }
    else {
      pp[level] = pos[level];
      while(pp[level] < pos[level] + size$[level]) {
        VQEDArray$this.init(pp);
        pp[level]++;
      }
    }
    return;
  }
  this.dims = new Array(this.limits.length).fill(0);
  this.insert(new Array(this.limits.length).fill(0), this.limits);
  this.size;
  this.insert;
  this.insertLevel;
}
function vqedArray(limits, init) {
  return (new VQEDArray(limits, init));
}
let ar = null;
function Ball(_HandlerFn_) {
  const Ball$this = this;
  this.Vector = function(pos, delta, _HandlerFn_$) {
    this.pos = pos;
    this.delta = delta;
    const Vector$this = this;
    this.move = function() {
      Vector$this.pos += Vector$this.delta;
      if (Vector$this.pos > 1 || Vector$this.pos < 0) {
        Vector$this.pos = Vector$this.delta > 0 ? 2 - Vector$this.pos : -Vector$this.pos;
        Vector$this.delta = -Vector$this.delta;
      }
    }
    this.move;
  }
  this.color = null;
  this.size = null;
  this.vectors = null;
  this.UI_ = function() {
    const UI_$this = this;
    this.v1 = null;
    this.v2 = null;
    this.v3 = null;
    this.v4 = null;
    this.Layout_ = function() {
      const Layout_$this = this;
      this.a1 = null;
      this.u2 = null;
      this.u3 = null;
      this.size$ = null;
      this.paint = function(pos0, pos1, size0, size1) {
        {
          pushAttribute(10, UI_$this.v2);
          let childSize0 = Layout_$this.u2;
          let posDiff0 = (size0 - childSize0) * UI_$this.v4[0];
          let size0$ = childSize0;
          let childSize1 = Layout_$this.u3;
          let posDiff1 = (size1 - childSize1) * UI_$this.v4[1];
          let size1$ = childSize1;
          let pos0$ = pos0 + posDiff0;
          let pos1$ = pos1 + posDiff1;
          oval(((pos0$ << 16) | pos1$), ((size0$ << 16) | size1$));
          popAttribute(10);
        }
      }
      this.onEvent = function(event, pos0, pos1, size0, size1) {
        let flag = false;
      }
      this.a1 = (UI_$this.v3 << 16) | UI_$this.v3;
      this.u2 = this.a1 >> 16;
      this.u3 = this.a1 & 65535;
      this.size$ = (this.u2 << 16) | this.u3;
      this.onEvent;
    }
    this.v1 = oval;
    this.v2 = Ball$this.color;
    this.v3 = Ball$this.size;
    this.v4 = [Ball$this.vectors[0].pos, Ball$this.vectors[1].pos];
    this.Layout_;
  }
  this.ui_ = null;
  this.setUI = function() {
    this.ui_ = new this.UI_();
  }
  this.color = Math.trunc(rand() * 16777215);
  this.size = rand() * 35 + 35;
  {
    this._d0 = 2;
    this.vectors = qedArray([this._d0], (function l(pos) {
      let _HandlerFn_$ = null;
      return (new Ball$this.Vector(rand(), ((((rand() * 1) / 100)) + ((0.025000) / 100)) * (rand() > 0.5 ? 1 : -1)));
    }), null);
    (function while17$_() {
      new Yield(Ball$this, (function Lambda_(_ret) {
        if (_ret) {
          Ball$this.vectors[0].move();
          Ball$this.vectors[1].move();
          while17$_();
        }
      }));
    })();
  }
}
let balls = null;
function UI_() {
  const UI_$this = this;
  this.v1 = null;
  this.v2 = null;
  this.v3 = null;
  this.v4 = null;
  this.v5 = null;
  this.v6 = null;
  this.v7 = null;
  this.v8 = null;
  this.v9 = null;
  this.v10 = null;
  this.v11 = null;
  this.v12 = null;
  this.Layout_ = function() {
    const Layout_$this = this;
    this.a1 = null;
    this.a2 = null;
    this.a3 = null;
    this.a4 = null;
    this.u5 = null;
    this.u6 = null;
    this.l6 = null;
    this.u7 = null;
    this.l7 = null;
    this.u8 = null;
    this.l8 = null;
    this.u9 = null;
    this.u10 = null;
    this.l10 = null;
    this.u11 = null;
    this.l11 = null;
    this.u12 = null;
    this.l12 = null;
    this.size = null;
    this.paint = function(pos0, pos1, size0, size1) {
      {
        {
          pushAttribute(10, UI_$this.v3);
          rect(((pos0 << 16) | pos1), ((size0 << 16) | size1));
          popAttribute(10);
        }
        {
          pushAttribute(10, UI_$this.v6);
          let childSize0 = Layout_$this.u6;
          let posDiff0 = (size0 - childSize0) * UI_$this.v7[0];
          let size0$ = childSize0;
          let childSize1 = Layout_$this.u10;
          let posDiff1 = (size1 - childSize1) * UI_$this.v7[1];
          let size1$ = childSize1;
          let pos0$ = pos0 + posDiff0;
          let pos1$ = pos1 + posDiff1;
          displayText(UI_$this.v4, ((pos0$ << 16) | pos1$), ((size0$ << 16) | size1$));
          popAttribute(10);
        }
        {
          pushAttribute(10, UI_$this.v10);
          let childSize0 = Layout_$this.u7;
          let posDiff0 = (size0 - childSize0) * UI_$this.v11[0];
          let size0$ = childSize0;
          let childSize1 = Layout_$this.u11;
          let posDiff1 = (size1 - childSize1) * UI_$this.v11[1];
          let size1$ = childSize1;
          let pos0$ = pos0 + posDiff0;
          let pos1$ = pos1 + posDiff1;
          displayText(UI_$this.v8, ((pos0$ << 16) | pos1$), ((size0$ << 16) | size1$));
          popAttribute(10);
        }
        {
          this.a4.paint(pos0, pos1, size0, size1);
        }
      }
    }
    this.onEvent = function(event, pos0, pos1, size0, size1) {
      let flag = false;
    }
    this.a1 = (UI_$this.v2 << 16) | UI_$this.v2;
    this.a2 = (UI_$this.v5 << 16) | UI_$this.v5;
    this.a3 = (UI_$this.v9 << 16) | UI_$this.v9;
    this.a4 = new UI_$this.v12.ui_.Layout_();
    this.u5 = this.a1 >> 16;
    this.u6 = this.a2 >> 16;
    this.l6 = max(this.u5, this.u6);
    this.u7 = this.a3 >> 16;
    this.l7 = max(this.l6, this.u7);
    this.u8 = this.a4.size >> 16;
    this.l8 = max(this.l7, this.u8);
    this.u9 = this.a1 & 65535;
    this.u10 = this.a2 & 65535;
    this.l10 = max(this.u9, this.u10);
    this.u11 = this.a3 & 65535;
    this.l11 = max(this.l10, this.u11);
    this.u12 = this.a4.size & 65535;
    this.l12 = max(this.l11, this.u12);
    this.size = (this.l8 << 16) | this.l12;
    this.onEvent;
  }
  this.v1 = rect;
  this.v2 = 800;
  this.v3 = 0;
  this.v4 = "Bouncing";
  this.v5 = 130;
  this.v6 = 1358954495;
  this.v7 = [((50) / 100), ((10) / 100)];
  this.v8 = "Balls!!";
  this.v9 = 160;
  this.v10 = 822083583;
  this.v11 = [((50) / 100), ((70) / 100)];
  this.v12 = balls;
  this.v12.setUI();
  this.Layout_;
  ui_ = this;
}
let ui_ = null;
function setUI() {
  ui_ = new UI_();
}
WIDTH = 1;
HEIGHT = 2;
OBLIQUE = 3;
COLOR_RED = 16711680;
COLOR_GREEN = 65280;
COLOR_YELLOW = 16776960;
COLOR_BLUE = 255;
COLOR_BLACK = 0;
Object.setPrototypeOf(QEDBaseArray_.prototype, Array.prototype);
Object.setPrototypeOf(SQEDArray.prototype, Array.prototype);
Object.setPrototypeOf(QEDArray.prototype, Array.prototype);
ar = ["Martin", "Savage"];
{
  let _d0 = 500;
  balls = qedArray([_d0], (function l(pos) {
    let _HandlerFn_ = null;
    return (new Ball());
  }), (function UI_(array, dims) {
    this.array = array;
    this.dims = dims;
    const UI_$this = this;
    this.Layout_ = function() {
      this.layouts = [];
      const Layout_$this = this;
      this.paint = function(pos0, pos1, size0, size1) {
        {
          let index = 0;
          while(index < UI_$this.dims[0]) {
            Layout_$this.layouts[index].paint(pos0, pos1, size0, size1);
            index++;
          }
        }
      }
      {
        let index = 0;
        while(index < UI_$this.dims[0]) {
          this.layouts[index] = new UI_$this.array[index].ui_.Layout_();
          index++;
        }
      }
    }
    {
      let index = 0;
      while(index < this.dims[0]) {
        this.array[index].ui_ = new this.array[index].UI_();
        index++;
      }
    }
    this.Layout_;
  }));
  (function while18$_() {
    new Animation((function Lambda_(_ret) {
      if (_ret) {
        process(balls);
        while18$_();
      }
    }));
  })();
}
pushAttribute(4, 20);
pushAttribute(10, 0);
pushAttribute(11, 1.0);
let layout_ = null;
function _refresh() {
//  if (ui_ != undefined && --postCount == 0) {
    setUI();
    layout_ = new ui_.Layout_();
    ctx.globalAlpha = 1.0;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    layout_.paint(0, 0, layout_.size >> 16, layout_.size & 65535);
//  }
}
executeEvents_();
canvas.addEventListener("mousedown", function(ev) {
  postCount++;
  var rect = canvas.getBoundingClientRect();
  layout_.onEvent(0, ev.clientX - rect.left, ev.clientY - rect.top, layout_.size >> 16, layout_.size & 65535);
  _refresh();
  });
canvas.addEventListener("mouseup", function(ev) {
  postCount++;
  var rect = canvas.getBoundingClientRect();
  layout_.onEvent(1, ev.clientX - rect.left, ev.clientY - rect.top, layout_.size >> 16, layout_.size & 65535);
  _refresh();
});
canvas.onselectstart = function () { return false; }

"use strict";
const canvas = null;//document.getElementById("canvas");
let postHandler = null;
let attributeStacks = [];
const ctx = null;//canvas.getContext("2d");
function toColor(color) {return "#" + color.toString(16).padStart(6, '0');}
let getAttribute = function(index) {
  return attributeStacks[index][attributeStacks[index].length - 1];
}
let ui_ = null;
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
let WIDTH = 1;
let HEIGHT = 2;
let OBLIQUE = 3;
let COLOR_RED = 16711680;
let COLOR_GREEN = 65280;
let COLOR_YELLOW = 16776960;
let COLOR_BLUE = 255;
let COLOR_BLACK = 0;
function println(str) {
  console.log(str);
}
function post_(handlerFn_) {
  if (postHandler != null)
    console.log("postHandler not null");

  postHandler = handlerFn_;
}
function executeEvents_() {
  while (postHandler != null) {
    const fn = postHandler;
  
    postHandler = null;
    fn();
  }

//  this._refresh();
}
function max(a, b) {
  return a > b ? a : b;
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
  ctx.clip();
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
function Timer(timeoutMillis, handlerFn_) {
  this.timeoutMillis = timeoutMillis;
  setTimeout(function() {
    handlerFn_(true);
    _refresh();
  }, timeoutMillis);
  this.reset = function() {
  }
}
function Time(Func, handlerFn_) {
  this.Func = Func;
  console.time("Time");
  new Func(() => {
    console.timeEnd("Time");
    handlerFn_();
  });;
}
function time(func) {
    console.time("time");
  func();
  console.timeEnd("time");;
}
function Animation(handlerFn_) {
  requestAnimationFrame((millis) => {
  handlerFn_(millis);
  _refresh();
});;
}
function InitFn(array, index, pos, handlerFn_) {
  this.array = array;
  this.index = index;
  this.pos = pos;
}
function QEDArray(Init, handlerFn_) {
  this.Init = Init;
  const QEDArray$this = this;
  this.size = function() {
    let s = 1;
    {
      let index = this.dims.size() - 1;
      while(index >= 0) {
        s *= this.dims[index];
        index--;
      }
    }
    return (s);
  }
  this.InitArray = function(size$, handlerFn_$) {
    this.size$ = size$;
    QEDArray$this.dims = new Array(this.size$.length).fill(0);
    new QEDArray$this.Insert(new Array(this.size$.length).fill(0), this.size$, (function Lambda_() {
      post_(handlerFn_$);
      return;
    }));
  }
  this.Insert = function(pos, size$, handlerFn_$) {
    this.pos = pos;
    this.size$ = size$;
    const Insert$this = this;
    this.newSize = [...this.size$];
    {
      this.index = QEDArray$this.dims.length - 1;
      while(this.index >= 0) {
        this.newSize[this.index] += QEDArray$this.dims[this.index];
        this.index--;
      }
    }
    new QEDArray$this.InsertLevel(QEDArray$this, QEDArray$this.dims, this.pos, this.size$, this.newSize, new Array(this.size$.length).fill(0), 0, (function Lambda_() {
      QEDArray$this.dims = Insert$this.newSize;
      post_(handlerFn_$);
      return;
    }));
  }
  this.InsertLevel = function(array, dims, pos, size$, newSize, pp, level, handlerFn_$) {
    this.array = array;
    this.dims = dims;
    this.pos = pos;
    this.size$ = size$;
    this.newSize = newSize;
    this.pp = pp;
    this.level = level;
    const InsertLevel$this = this;
    new (function W9$_(i1$_) {
      if (InsertLevel$this.level < QEDArray$this.dims.length - 1) {
        pp[level] = 0;
        (function while2$_() {
          if (pp[level] < pos[level])
            new QEDArray$this.InsertLevel(array[pp[level]], InsertLevel$this.dims, InsertLevel$this.pos, InsertLevel$this.size$, InsertLevel$this.newSize, InsertLevel$this.pp, InsertLevel$this.level + 1, (function Lambda_() {
              pp[level]++;
              post_(while2$_);
            }));
          else {
            if (InsertLevel$this.size$[InsertLevel$this.level] !== 0) {
              pp[level] = dims[level] - 1;
              while(pp[level] >= pos[level]) {
                array[pp[level] + InsertLevel$this.size$[level]] = array[pp[level]];
                pp[level]--;
              }
            }
            pp[level] = pos[level];
            (function while4$_() {
              if (pp[level] < pos[level] + InsertLevel$this.size$[level]) {
                array[pp[level]] = [];
                new QEDArray$this.InsertLevel(array[pp[level]], new Array(InsertLevel$this.size$.length).fill(0), new Array(InsertLevel$this.size$.length).fill(0), InsertLevel$this.newSize, InsertLevel$this.newSize, InsertLevel$this.pp, InsertLevel$this.level + 1, (function Lambda_() {
                  pp[level]++;
                  post_(while4$_);
                }));
              }
              else {
                pp[level] = pos[level] + InsertLevel$this.size$[level];
                (function while5$_() {
                  if (pp[level] < newSize[level])
                    new QEDArray$this.InsertLevel(array[pp[level]], InsertLevel$this.dims, InsertLevel$this.pos, InsertLevel$this.size$, InsertLevel$this.newSize, InsertLevel$this.pp, InsertLevel$this.level + 1, (function Lambda_() {
                      pp[level]++;
                      post_(while5$_);
                    }));
                  else
                    i1$_();
                })();
              }
            })();
          }
        })();
      }
      else {
        if (InsertLevel$this.size$[InsertLevel$this.level] !== 0) {
          pp[level] = dims[level] - 1;
          while(pp[level] >= pos[level]) {
            array[pp[level] + InsertLevel$this.size$[level]] = array[pp[level]];
            pp[level]--;
          }
        }
        pp[level] = pos[level];
        (function while7$_() {
          if (pp[level] < pos[level] + InsertLevel$this.size$[level])
            QEDArray$this.Init(InsertLevel$this.array, pp[level], InsertLevel$this.pp, (function Lambda_() {
              pp[level]++;
              post_(while7$_);
            }));
          else
            i1$_();
        })();
      }
    })((function c8$_() {
      post_(handlerFn_$);
      return;
    }));
  }
}
Object.setPrototypeOf(QEDArray.prototype, Array.prototype);
let ar = ["Martin", "Savage"];
let x = (function l() {
  let _d0 = 10;
  let _d1 = 11;
  let _d2 = 12;
  let _d3 = 13;
  {
    let _x0 = 0;
    while(_x0 < _d0) {
      let i = 0;
      while(i < _d1) {
        let _x2 = 0;
        while(_x2 < _d2) {
          let _x3 = 0;
          while(_x3 < _d3) {
            14;
            ++_x3;
          }
          ++_x2;
        }
        ++i;
      }
      ++_x0;
    }
  }
})();
println("" + ((20) / 100));
this.UI_ = function UI_(handlerFn_) {
  const UI_$this = this;
  this.v1 = "SS";
  function Layout_(handlerFn_) {
    this.a1 = getTextSize(UI_$this.v1);
    this.u2 = this.a1 >> 16;
    this.u3 = this.a1 & 65535;
    this.size = (this.u2 << 16) | this.u3;
    this.paint = function paint(pos0, pos1, size0, size1) {
      {
        {
          displayText(UI_$this.v1, ((pos0 << 16) | pos1), ((size0 << 16) | size1));
        }
      }
    }
    this.onEvent = function onEvent(event, pos0, pos1, size0, size1) {
      let flag = false;
    }
  }
}
pushAttribute(4, 20);
pushAttribute(10, 0);
pushAttribute(11, 1.0);
let layout_ = null;
function _refresh() {
//  if (ui_ != undefined && --postCount == 0) {
    ui_ = new UI_();
    layout_ = new ui_.Layout_();
    ctx.globalAlpha = 1.0;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    layout_.paint(0, 0, layout_.size >> 16, layout_.size & 65535);
//  }
}
/*
_refresh();
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
*/
let myFirstArray = new QEDArray(function l(array, x, pos, handlerFn_) {
  const i = (pos[0] + 1) * (pos[1] + 1);

  array[x] = i;
//  MainThis.Timer(200, function(ret) {
    post_(handlerFn_);
//  });
}, null);
new myFirstArray.InitArray([2, 3], function Lambda_() {
  new myFirstArray.Insert([1, 1], [3, 2], function Lambda_() {
    new myFirstArray.Insert([0, 0], [1, 1], function Lambda_() {
      console.log(myFirstArray);
            /*
      let i = 0;
      (function while10$_() {
        if (i < 3) {
          {
            new Timer(1000, (function Lambda_(_ret) {
              println("Count: " + i);
              i++;
              post_(while10$_);
            }));
          }
        }
      })();*/
    });
    });
});
executeEvents_();

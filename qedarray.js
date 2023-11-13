"use strict";
const MainThis = this;
const canvas = null;//document.getElementById("canvas");
let postCount = 0;
let postHandler = null;
let attributeStacks = [];
const ctx = null;//canvas.getContext("2d");
function toColor(color) {return "#" + color.toString(16).padStart(6, '0');}
let getAttribute = function(index) {
  return attributeStacks[index][attributeStacks[index].length - 1];
}
let ui_ = null;
this.voidHandler_ = function voidHandler_() {
}
this.anyHandler_ = function anyHandler_(value) {
}
this.intHandler_ = function intHandler_(value) {
}
this.floatHandler_ = function floatHandler_(value) {
}
this.boolHandler_ = function boolHandler_(value) {
}
this.stringHandler_ = function stringHandler_(value) {
}
let WIDTH = 1;
let HEIGHT = 2;
let OBLIQUE = 3;
let COLOR_RED = 16711680;
let COLOR_GREEN = 65280;
let COLOR_YELLOW = 16776960;
let COLOR_BLUE = 255;
let COLOR_BLACK = 0;
this.println = function println(str) {
  console.log(str);
}
this.post_ = function post_(handlerFn_) {
  if (postHandler != null)
    console.log("postHandler not null");

    postHandler = handlerFn_;
}
this.executeEvents_ = function executeEvents_() {
  while (postHandler != null) {
    const fn = postHandler;
  
    postHandler = null;
    fn();
  }

//  this._refresh();
}
this.max = function max(a, b) {
  return a > b ? a : b;
}
this.clock = function clock() {
}
this.saveContext = function saveContext() {
  ctx.save();
}
this.restoreContext = function restoreContext() {
  ctx.restore();
}
this.oval = function oval(pos, size) {
  ctx.fillStyle = toColor(getAttribute(10));
  ctx.globalAlpha = "" + getAttribute(11);
  ctx.beginPath();
  ctx.ellipse((pos >> 16) + (size >> 17), (pos & 65535) + ((size & 65535) >> 1), size >> 17, (size & 65535) >> 1, 0, 0, 2*Math.PI);
  ctx.fill();
  ctx.clip();
}
this.rect = function rect(pos, size) {
  ctx.fillStyle = toColor(getAttribute(10));
  ctx.globalAlpha = "" + getAttribute(11);
  ctx.beginPath();
  ctx.fillRect((pos >> 16), (pos & 65535), size >> 16, size & 65535);
}
this.pushAttribute = function pushAttribute(index, value) {
  if (attributeStacks[index] == undefined)
    attributeStacks[index] = [];

  attributeStacks[index].push(value);
}
this.pushAttribute$_ = function pushAttribute$_(index, value) {
  pushAttribute(index, value);
}
this.popAttribute = function popAttribute(index) {
  attributeStacks[index].pop();
}
this.getTextSize = function getTextSize(text) {
  ctx.font = getAttribute(4) + "px Arial";

  const textSize = ctx.measureText(text);
  const height = textSize.fontBoundingBoxAscent + textSize.fontBoundingBoxDescent;
  return (textSize.width << 16) | height;
}
this.displayText = function displayText(text, pos, size) {
  let pos1 = [(pos >> 16), (pos & 0xFFFF)];
  let size1 = [(size >> 16), (size & 0xFFFF)];
  ctx.font = getAttribute(4) + "px Arial";
  ctx.fillStyle = toColor(getAttribute(10));
  ctx.globalAlpha = getAttribute(11);
  ctx.textBaseline = "top";
  ctx.fillText(text, pos1[0], pos1[1]);
}
this.Timer = function Timer(timeoutMillis, handlerFn_) {
  setTimeout(function() {
    handlerFn_(true);
//    _refresh();
  }, timeoutMillis);
  function reset() {
  }
}
this.Time = function Time(Func, handlerFn_) {
    console.time("Time");
  new Func(() => {
    console.timeEnd("Time");
    handlerFn_();
  });;
}
this.time = function time(func) {
    console.time("time");
  func();
  console.timeEnd("time");;
}
this.Animation = function Animation(handlerFn_) {
  requestAnimationFrame((millis) => {
  handlerFn_(millis);
  _refresh();
});;
}
this.InitFn = function InitFn(array, index, pos, handlerFn_) {
}
this.QEDArray_ = function QEDArray_(Init, handlerFn_) {
  const QEDArray_$this = this;
  this.Init = Init;
  this.handlerFn_ = handlerFn_;
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
  this.InitArray = function(size, handlerFn_) {
    QEDArray_$this.dims = new Array(size.length).fill(0);
    new QEDArray_$this.Insert(new Array(size.length).fill(0), size, (function Lambda_() {
      MainThis.post_(handlerFn_);
      return;
    }));
  }
  this.Insert = function(pos, size, handlerFn_) {
    let newSize = [...size];
    {
      let index = QEDArray_$this.dims.length - 1;
      while(index >= 0) {
        newSize[index] += QEDArray_$this.dims[index];
        index--;
      }
    }
    new QEDArray_$this.InsertLevel(QEDArray_$this, QEDArray_$this.dims, pos, size, newSize, new Array(size.length).fill(0), 0, (function Lambda_() {
      QEDArray_$this.dims = newSize ;
      MainThis.post_(handlerFn_);
      return;
    }
));
  }
  this.InsertLevel = function(array, dims, pos, size, newSize, pp, level, handlerFn_) {
    new (function W9$_(i1$_) {
      if (level < dims.length - 1) {
        {
          {
            pp[level] = 0;
            (this.while2$_ = function while2$_() {
              if (pp[level] < pos[level]) {
                {
                  new QEDArray_$this.InsertLevel(array[pp[level]], dims, pos, size, newSize, pp, level + 1, (function Lambda_() {
                    pp[level]++;
                    MainThis.post_(while2$_);
                  }));
                }
              } else {
                if (size[level] !== 0) {
                  {
                    pp[level] = dims[level] - 1;
                    while(pp[level] >= pos[level]) {
                      array[pp[level] + size[level]] = array[pp[level]];
                      pp[level]--;
                    }
                  }
                }
                {
                  pp[level] = pos[level];
                  (function while4$_() {
                    if (pp[level] < pos[level] + size[level]) {
                      {
                        array[pp[level]] = [];
                        new QEDArray_$this.InsertLevel(array[pp[level]], new Array(size.length).fill(0), new Array(size.length).fill(0), newSize, newSize, pp, level + 1, (function Lambda_() {
                          pp[level]++;
                          MainThis.post_(while4$_);
                        }));
                      }
                    } else {
                      {
                        pp[level] = pos[level] + size[level];
                        (function while5$_() {
                          if (pp[level] < newSize[level]) {
                            {
                              new QEDArray_$this.InsertLevel(array[pp[level]], dims, pos, size, newSize, pp, level + 1, (function Lambda_() {
                                pp[level]++;
                                MainThis.post_(while5$_);
                              }));
                            }
                          }
                          else
                            i1$_();
                        })();
                      }
                    }
                  })();
                }
              }
            })();
          }
        }
      }
      else {
        {
          if (size[level] !== 0) {
            {
              pp[level] = dims[level] - 1;
              while(pp[level] >= pos[level]) {
                array[pp[level] + size[level]] = array[pp[level]];
                pp[level]--;
              }
            }
          }
          {
            pp[level] = pos[level];
            (function while7$_() {
              if (pp[level] < pos[level] + size[level]) {
                {
                  QEDArray_$this.Init(array, pp[level], pp, (function Lambda_() {
                    pp[level]++;
                    MainThis.post_(while7$_);
                  }));
                }
              }
              else
                i1$_();
            })();
          }
        }
      }
    }
)((this.c8$_ = function c8$_() {
      MainThis.post_(handlerFn_);
      return;
    }
));
  }
}
Object.setPrototypeOf(this.QEDArray_.prototype, Array.prototype);
let ar = ["Martin", "Savage"];
let x = (this.l = function l() {
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
}
)();
this.println("" + ((20) / 100));
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
this.pushAttribute(4, 20);
this.pushAttribute(10, 0);
this.pushAttribute(11, 1.0);
let layout_ = null;
function _refresh() {
//  if (ui_ != undefined && --postCount == 0) {
    this.ui_ = new this.UI_(null);
    this.layout_ = new this.ui_.Layout_();
    this.ctx.globalAlpha = 1.0;
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    this.layout_.paint(0, 0, this.layout_.size >> 16, this.layout_.size & 65535);
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
let myFirstArray = new this.QEDArray_(function l(array, x, pos, handlerFn_) {
  const i = (pos[0] + 1) * (pos[1] + 1);

  array[x] = i;
//  MainThis.Timer(200, function(ret) {
    MainThis.post_(handlerFn_);
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
            new MainThis.Timer(1000, (function Lambda_(_ret) {
              MainThis.println("Count: " + i);
              i++;
              MainThis.post_(while10$_);
            }));
          }
        }
      })();*/
    });
    });
});
this.executeEvents_();

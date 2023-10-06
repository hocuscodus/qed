const qedArrayPrototype = {
  initArrayIndex(index) {
    if (index < this.dims.length) {
      let array = []
  
      for (let ndx = 0; ndx < this.dims[index]; ndx++)
        array[ndx] = this.initArrayIndex(index + 1)
  
      return array
    }
    else
      return this.fn()
  }
};

function QEDArray(dims, fn) {
  this.dims = dims
  this.fn = fn
  this.array = this.initArrayIndex(0)
}

Object.assign(QEDArray.prototype, qedArrayPrototype);

let t = new QEDArray([3, 2], function L() {return "Martin"})

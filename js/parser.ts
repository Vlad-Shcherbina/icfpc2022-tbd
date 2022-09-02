'use strict'

import {tokToStr} from './lexer.js'

function stateToString(state:number) {
  return [ ".","%eof","Program","color","Block","Color","comment","cut","lb","lb","number","orientation","rb","rb","Block","Point","merge","Block","Block","swap","Block","Block","%eof","ProgramLine","newline","Program","comma","number","number","rb","comma","comma","comma","lb","number","number","number","number","rb","number","Move","lb","rb","BlockId","dot","BlockId" ][state]
}

function expectedSym(state:number) {
  return [ "Program","%eof","%eof","Block","Color","%eof/newline","%eof/newline","Block/Block","number","orientation/number","rb","rb","%eof/newline","lb","lb/Point","%eof/newline","Block","Block","%eof/newline","Block","Block","%eof/newline","%eof","%eof/newline","Program","%eof","number","rb","comma","%eof/newline","number","number","number","number","rb","comma","comma","comma","%eof/newline","rb/dot","%eof/newline","BlockId","%eof/lb/newline","rb","BlockId","rb" ][state]
}

const Action = [
  [46,46,6,47,47,47,47,47,47,19,16,7,3],
  [48,47,47,47,47,47,47,47,47,47,47,47,47],
  [1,47,47,47,47,47,47,47,47,47,47,47,47],
  [47,47,47,47,47,47,41,47,47,47,47,47,47],
  [47,47,47,47,47,47,33,47,47,47,47,47,47],
  [49,49,47,47,47,47,47,47,47,47,47,47,47],
  [50,50,47,47,47,47,47,47,47,47,47,47,47],
  [47,47,47,47,47,47,41,47,47,47,47,47,47],
  [47,47,47,47,47,47,47,47,10,47,47,47,47],
  [47,47,47,47,47,11,47,47,28,47,47,47,47],
  [47,47,47,47,47,47,47,12,47,47,47,47,47],
  [47,47,47,47,47,47,47,13,47,47,47,47,47],
  [51,51,47,47,47,47,47,47,47,47,47,47,47],
  [47,47,47,47,47,47,8,47,47,47,47,47,47],
  [47,47,47,47,47,47,9,47,47,47,47,47,47],
  [52,52,47,47,47,47,47,47,47,47,47,47,47],
  [47,47,47,47,47,47,41,47,47,47,47,47,47],
  [47,47,47,47,47,47,41,47,47,47,47,47,47],
  [53,53,47,47,47,47,47,47,47,47,47,47,47],
  [47,47,47,47,47,47,41,47,47,47,47,47,47],
  [47,47,47,47,47,47,41,47,47,47,47,47,47],
  [54,54,47,47,47,47,47,47,47,47,47,47,47],
  [55,47,47,47,47,47,47,47,47,47,47,47,47],
  [22,24,47,47,47,47,47,47,47,47,47,47,47],
  [46,46,6,47,47,47,47,47,47,19,16,7,3],
  [56,47,47,47,47,47,47,47,47,47,47,47,47],
  [47,47,47,47,47,47,47,47,27,47,47,47,47],
  [47,47,47,47,47,47,47,29,47,47,47,47,47],
  [47,47,47,26,47,47,47,47,47,47,47,47,47],
  [57,57,47,47,47,47,47,47,47,47,47,47,47],
  [47,47,47,47,47,47,47,47,34,47,47,47,47],
  [47,47,47,47,47,47,47,47,35,47,47,47,47],
  [47,47,47,47,47,47,47,47,36,47,47,47,47],
  [47,47,47,47,47,47,47,47,37,47,47,47,47],
  [47,47,47,47,47,47,47,38,47,47,47,47,47],
  [47,47,47,30,47,47,47,47,47,47,47,47,47],
  [47,47,47,31,47,47,47,47,47,47,47,47,47],
  [47,47,47,32,47,47,47,47,47,47,47,47,47],
  [58,58,47,47,47,47,47,47,47,47,47,47,47],
  [47,47,47,47,44,47,47,59,47,47,47,47,47],
  [60,60,47,47,47,47,47,47,47,47,47,47,47],
  [47,47,47,47,47,47,47,47,39,47,47,47,47],
  [61,61,47,47,47,47,61,47,47,47,47,47,47],
  [47,47,47,47,47,47,47,42,47,47,47,47,47],
  [47,47,47,47,47,47,47,47,39,47,47,47,47],
  [47,47,47,47,47,47,47,62,47,47,47,47,47]
  ]
const GOTO = [
  [40,2,23,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,4,0,0,0],
  [0,0,0,0,5,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,14,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,15,0],
  [0,0,0,0,0,0,0],
  [0,0,0,17,0,0,0],
  [0,0,0,18,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,20,0,0,0],
  [0,0,0,21,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [40,25,23,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,43],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0],
  [0,0,0,0,0,0,45],
  [0,0,0,0,0,0,0]
  ]

export class Parser {
  debug: boolean

  constructor(debug=false) {
    this.debug = debug
  }

  parse(tokens: Iterator<any>) {
    const stack: any[] = []
    function top() {
      if (stack.length > 0) return stack[stack.length-1][0]
      else return 0
    }
    let a = tokens.next().value
    while(true) {
      const action = Action[top()][a[0]]
      switch(action) {
      case 48: {
        stack.pop()
        return stack.pop()[1]
        break
      }
      case 61: {
        if (this.debug) console.log("Reduce using Block -> lb BlockId rb")
        const _3 = stack.pop()[1]
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][3] // Block
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(_2)])
        break
      }
      case 59: {
        if (this.debug) console.log("Reduce using BlockId -> number")
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][6] // BlockId
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(`${_1}`)])
        break
      }
      case 62: {
        if (this.debug) console.log("Reduce using BlockId -> number dot BlockId")
        const _3 = stack.pop()[1]
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][6] // BlockId
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(`${_1}.${_3}`)])
        break
      }
      case 58: {
        if (this.debug) console.log("Reduce using Color -> lb number comma number comma number comma number rb")
        const _9 = stack.pop()[1]
        const _8 = stack.pop()[1]
        const _7 = stack.pop()[1]
        const _6 = stack.pop()[1]
        const _5 = stack.pop()[1]
        const _4 = stack.pop()[1]
        const _3 = stack.pop()[1]
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][4] // Color
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,([_2,_4,_6,_8])])
        break
      }
      case 49: {
        if (this.debug) console.log("Reduce using Move -> color Block Color")
        const _3 = stack.pop()[1]
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][0] // Move
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(({type: "color", block: _2, color: _3}))])
        break
      }
      case 51: {
        if (this.debug) console.log("Reduce using Move -> cut Block lb orientation rb lb number rb")
        const _8 = stack.pop()[1]
        const _7 = stack.pop()[1]
        const _6 = stack.pop()[1]
        const _5 = stack.pop()[1]
        const _4 = stack.pop()[1]
        const _3 = stack.pop()[1]
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][0] // Move
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(({type: "cut-line", block: _2, dir: _4, num: _7}))])
        break
      }
      case 52: {
        if (this.debug) console.log("Reduce using Move -> cut Block Point")
        const _3 = stack.pop()[1]
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][0] // Move
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(({type: "cut-point", block: _2, point: _3}))])
        break
      }
      case 53: {
        if (this.debug) console.log("Reduce using Move -> merge Block Block")
        const _3 = stack.pop()[1]
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][0] // Move
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(({type: "merge", block1: _2, block2: _3}))])
        break
      }
      case 54: {
        if (this.debug) console.log("Reduce using Move -> swap Block Block")
        const _3 = stack.pop()[1]
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][0] // Move
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(({type: "swap", block1: _2, block2: _3}))])
        break
      }
      case 57: {
        if (this.debug) console.log("Reduce using Point -> lb number comma number rb")
        const _5 = stack.pop()[1]
        const _4 = stack.pop()[1]
        const _3 = stack.pop()[1]
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][5] // Point
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,([_2, _4])])
        break
      }
      case 55: {
        if (this.debug) console.log("Reduce using Program -> ProgramLine %eof")
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][1] // Program
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,([_1])])
        break
      }
      case 56: {
        if (this.debug) console.log("Reduce using Program -> ProgramLine newline Program")
        const _3 = stack.pop()[1]
        const _2 = stack.pop()[1]
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][1] // Program
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,([_1].concat(_3))])
        break
      }
      case 46: {
        if (this.debug) console.log("Reduce using ProgramLine -> ")

        const gt = GOTO[top()][2] // ProgramLine
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(null)])
        break
      }
      case 50: {
        if (this.debug) console.log("Reduce using ProgramLine -> comment")
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][2] // ProgramLine
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(({type: "comment", text: _1}))])
        break
      }
      case 60: {
        if (this.debug) console.log("Reduce using ProgramLine -> Move")
        const _1 = stack.pop()[1]
        const gt = GOTO[top()][2] // ProgramLine
        if (gt===0) throw new Error("No goto")
        if (this.debug) {
          console.log(`${top()} is now on top of the stack`)
          console.log(`${gt} will be placed on the stack`)
        }
        stack.push([gt,(_1)])
        break
      }
      case 47: {
        const lastSt = top()
        const parsed = [stateToString(lastSt)]
        while (stack.length > 0) {
          stack.pop()
          parsed.unshift(stateToString(top()))
        }
        throw new Error(
          `Rejection state reached after parsing "${parsed.join(' ')}", when encountered symbol "${tokToStr(a[0])}" in state ${lastSt}. Expected "${expectedSym(lastSt)}"`)
        break
      }
      default:
        if (this.debug) console.log(`Shift to ${action}`)
        stack.push([action, a[1]])
        a=tokens.next().value
      }
    }
  }
}

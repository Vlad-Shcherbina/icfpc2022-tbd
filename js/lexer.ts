'use strict'

export const TokenType = {
  eof: 0,
  Tok_newline: 1,
  Tok_comment: 2,
  Tok_comma: 3,
  Tok_dot: 4,
  Tok_orientation: 5,
  Tok_lb: 6,
  Tok_rb: 7,
  Tok_number: 8,
  Tok_swap: 9,
  Tok_merge: 10,
  Tok_cut: 11,
  Tok_color: 12
}

export function tokToStr(x: number) {
  switch(x) {
    case 0: return '%eof'
    case 1: return 'newline'
    case 2: return 'comment'
    case 3: return 'comma'
    case 4: return 'dot'
    case 5: return 'orientation'
    case 6: return 'lb'
    case 7: return 'rb'
    case 8: return 'number'
    case 9: return 'swap'
    case 10: return 'merge'
    case 11: return 'cut'
    case 12: return 'color'
  }
}

class Buf {
  current: Iterator<string, any, undefined> | null;
  stack: Array<Iterator<string, any, undefined> | null>;

  constructor(it: Iterable<string>) {
    this.current = it[Symbol.iterator]()
    this.stack = []
  }

  [Symbol.iterator]() {
    return this
  }

  next(): IteratorResult<string, any> | {done: true} {
    if (this.current === null) {
      return {done: true}
    }
    const res = this.current.next()
    if (res.done) {
      if (this.stack.length) {
        this.current = this.stack.pop()!
        return this.next()
      } else {
        this.current = null
      }
    }
    return res
  }

  empty() {
    return this.current === null
  }

  unshift(it: Iterable<string>) {
    this.stack.push(this.current)
    this.current = it[Symbol.iterator]()
  }
}

export function *lex(input:string, debug = false) {
  const inputBuf = new Buf(input)

  while(true) {
    let curCh = '\0'
    let accSt = -1
    let curSt = 0
    let buf = ""
    let tmp = ""
    while (curSt >= 0) {
      if ([1,2,3,4,5,6,7,8,12,15,19,22,25].includes(curSt)) {
        buf += tmp
        tmp = ""
        accSt = curSt
      }
      const t = inputBuf.next()
      if (t.done) break
      curCh = t.value
      tmp += curCh
      switch(curSt) {
      case 0:
        if (curCh === '\u{0000000a}') {
          curSt = 1
          continue
        } else if (curCh === ' ') {
          curSt = 2
          continue
        } else if (curCh === '#') {
          curSt = 3
          continue
        } else if (curCh === ',') {
          curSt = 4
          continue
        } else if (curCh === '.') {
          curSt = 5
          continue
        } else if (curCh === 'X' || curCh === 'Y' || curCh === 'x' || curCh === 'y') {
          curSt = 6
          continue
        } else if (curCh === '[') {
          curSt = 7
          continue
        } else if (curCh === ']') {
          curSt = 8
          continue
        } else if (curCh === 'c') {
          curSt = 9
          continue
        } else if (curCh === 'm') {
          curSt = 10
          continue
        } else if (curCh === 's') {
          curSt = 11
          continue
        } else if ((curCh >= '0' && curCh <= '9')) {
          curSt = 12
          continue
        }
        break
      case 2:
        if (curCh === ' ') {
          curSt = 2
          continue
        }
        break
      case 3:
        if (!(curCh === '\u{0000000a}')) {
          curSt = 3
          continue
        }
        break
      case 9:
        if (curCh === 'o') {
          curSt = 20
          continue
        } else if (curCh === 'u') {
          curSt = 21
          continue
        }
        break
      case 10:
        if (curCh === 'e') {
          curSt = 16
          continue
        }
        break
      case 11:
        if (curCh === 'w') {
          curSt = 13
          continue
        }
        break
      case 12:
        if ((curCh >= '0' && curCh <= '9')) {
          curSt = 12
          continue
        }
        break
      case 13:
        if (curCh === 'a') {
          curSt = 14
          continue
        }
        break
      case 14:
        if (curCh === 'p') {
          curSt = 15
          continue
        }
        break
      case 16:
        if (curCh === 'r') {
          curSt = 17
          continue
        }
        break
      case 17:
        if (curCh === 'g') {
          curSt = 18
          continue
        }
        break
      case 18:
        if (curCh === 'e') {
          curSt = 19
          continue
        }
        break
      case 20:
        if (curCh === 'l') {
          curSt = 23
          continue
        }
        break
      case 21:
        if (curCh === 't') {
          curSt = 22
          continue
        }
        break
      case 23:
        if (curCh === 'o') {
          curSt = 24
          continue
        }
        break
      case 24:
        if (curCh === 'r') {
          curSt = 25
          continue
        }
        break
      }
      break
    }

    if (tmp.length > 0) {
      inputBuf.unshift(tmp)
    }
    const text = buf
    switch(accSt) {
      case 1:
        if (debug) console.log("Lexed token newline: \"" + text + "\"")
        yield [TokenType.Tok_newline, null]
        continue
      case 2:
        if (debug) console.log("Skipping state 2: \"" + text + "\"")
        continue
      case 3:
        if (debug) console.log("Lexed token comment: \"" + text + "\"")
        yield [TokenType.Tok_comment,  text.slice(1) ]
        continue
      case 4:
        if (debug) console.log("Lexed token comma: \"" + text + "\"")
        yield [TokenType.Tok_comma, null]
        continue
      case 5:
        if (debug) console.log("Lexed token dot: \"" + text + "\"")
        yield [TokenType.Tok_dot, null]
        continue
      case 6:
        if (debug) console.log("Lexed token orientation: \"" + text + "\"")
        yield [TokenType.Tok_orientation,  text.toLowerCase() ]
        continue
      case 7:
        if (debug) console.log("Lexed token lb: \"" + text + "\"")
        yield [TokenType.Tok_lb, null]
        continue
      case 8:
        if (debug) console.log("Lexed token rb: \"" + text + "\"")
        yield [TokenType.Tok_rb, null]
        continue
      case 12:
        if (debug) console.log("Lexed token number: \"" + text + "\"")
        yield [TokenType.Tok_number,  parseInt(text, 10) ]
        continue
      case 15:
        if (debug) console.log("Lexed token swap: \"" + text + "\"")
        yield [TokenType.Tok_swap, null]
        continue
      case 19:
        if (debug) console.log("Lexed token merge: \"" + text + "\"")
        yield [TokenType.Tok_merge, null]
        continue
      case 22:
        if (debug) console.log("Lexed token cut: \"" + text + "\"")
        yield [TokenType.Tok_cut, null]
        continue
      case 25:
        if (debug) console.log("Lexed token color: \"" + text + "\"")
        yield [TokenType.Tok_color, null]
        continue
    }
    if (inputBuf.empty()) {
      if (debug) console.log(`Got EOF while lexing "${text}"`)
      yield [TokenType.eof, null]
      continue
    }
    throw new Error("Unexpected input: " + buf + tmp)
  }
}

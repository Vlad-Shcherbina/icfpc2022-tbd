import {lex} from './lexer.js'
import {Parser} from './parser.js'

type Command =
    | {type: "cut-point", block: BlockId, point: Point}
    | {type: "cut-line", block: BlockId, dir: Orientation, num: number}
    | {type: "color", block: BlockId, color: Color}
    | {type: "swap", block1: BlockId, block2: BlockId}
    | {type: "merge", block1: BlockId, block2: BlockId}
    | {type: "comment", text: string}

type BlockId = string
type Point = [number, number]
type Orientation = "x" | "y"
type Color = [number, number, number, number]
type Block = { x: number, y: number, c: HTMLCanvasElement }

const inputBox = document.getElementById("input") as HTMLTextAreaElement;
const run = document.getElementById("run")!;
const parser = new Parser();

function copyFrom(c: HTMLCanvasElement, sx: number, sy: number, sw: number, sh: number) {
    const newCanvas = document.createElement('canvas');
    newCanvas.width = sw;
    newCanvas.height = sh;
    const nctx = newCanvas.getContext('2d')!;
    const octx = c.getContext('2d')!;
    const data = octx.getImageData(sx, sy, sw, sh)
    nctx.putImageData(data, 0, 0);
    return newCanvas;
}

run.addEventListener('click', () => {
    const input = inputBox.value;
    const parsed = parser.parse(lex(input)).filter((x: any) => x !== null) as Command[];
    const blocks = new Map<string, Block>();
    let counter = 1;
    const canvas0 = document.createElement('canvas');
    canvas0.width=400
    canvas0.height=400
    const ctx0 = canvas0.getContext('2d')!;
    ctx0.fillStyle="rgba(0,0,0,0)"
    ctx0.fillRect(0, 0, 400, 400);
    blocks.set("0", {x: 0, y: 0, c: canvas0})

    function mergeBlocks(x:number, y: number, w: number, h: number, blk1: Block, blk2: Block) {
        const canvas1 = document.createElement('canvas');
        canvas1.width = w;
        canvas1.height = h;
        const newBlock: Block = {x, y, c: canvas1};
        const ctx = canvas1.getContext('2d')!;
        ctx.drawImage(blk1.c, blk1.x, blk1.y);
        ctx.drawImage(blk2.c, blk2.x, blk2.y);
        const blkName = `${counter}`;
        counter += 1;
        blocks.set(blkName, newBlock);
    }

    for (const cmd of parsed) {
        switch(cmd.type) {
            case "cut-point": {
                const blk = blocks.get(cmd.block);
                if (!blk) throw new Error(`No such block: ${cmd.block}`);
                if (cmd.point[0] == 0 || cmd.point[0] >= blk.c.width) throw new Error('Point is on block border in x dimension');
                if (cmd.point[1] == 0 || cmd.point[1] >= blk.c.height) throw new Error('Point is on block border in y dimension');
                blocks.delete(cmd.block);
                blocks.set(`${cmd.block}.0`, { x: blk.x, y: blk.y, c: copyFrom(blk.c, 0, 0, cmd.point[0], cmd.point[1]) });
                blocks.set(`${cmd.block}.1`, { x: blk.x + cmd.point[0], y: blk.y,
                    c: copyFrom(blk.c, cmd.point[0], 0, blk.c.width - cmd.point[0], cmd.point[1]) });
                blocks.set(`${cmd.block}.2`, { x: blk.x + cmd.point[0], y: blk.y + cmd.point[1],
                    c: copyFrom(blk.c, cmd.point[0], cmd.point[1], blk.c.width - cmd.point[0], blk.c.height - cmd.point[1]) });
                blocks.set(`${cmd.block}.3`, { x: blk.x, y: blk.y + cmd.point[1],
                    c: copyFrom(blk.c, 0, cmd.point[1], cmd.point[0], blk.c.height - cmd.point[1]) });
                break;
            }
            case "cut-line": {
                const blk = blocks.get(cmd.block);
                if (!blk) throw new Error(`No such block: ${cmd.block}`);
                const blksize = cmd.dir == "x" ? blk.c.width : blk.c.height;
                if (cmd.num == 0 || cmd.num >= blksize) throw new Error(`Point is on block border in ${cmd.dir} dimension`);
                blocks.delete(cmd.block);
                if (cmd.dir === "x") {
                    blocks.set(`${cmd.block}.0`, { x: blk.x, y: blk.y,
                        c: copyFrom(blk.c, 0, 0, cmd.num, blk.c.height) });
                    blocks.set(`${cmd.block}.1`, { x: blk.x + cmd.num, y: blk.y,
                        c: copyFrom(blk.c, cmd.num, 0, blk.c.width - cmd.num, blk.c.height) });
                } else if (cmd.dir === "y") {
                    blocks.set(`${cmd.block}.0`, { x: blk.x, y: blk.y,
                        c: copyFrom(blk.c, 0, 0, blk.c.width, cmd.num) });
                    blocks.set(`${cmd.block}.1`, { x: blk.x, y: blk.y + cmd.num,
                        c: copyFrom(blk.c, 0, cmd.num, blk.c.width, blk.c.height - cmd.num) });
                }
                break;
            }
            case "color": {
                const blk = blocks.get(cmd.block);
                if (!blk) throw new Error(`No such block: ${cmd.block}`);
                const ctx = blk.c.getContext('2d')!;
                // a hack to behave like the playground: pre-fill with white
                ctx.fillStyle='rgba(255,255,255,1)';
                ctx.fillRect(0, 0, blk.c.width, blk.c.height);
                // end hack
                ctx.fillStyle = `rgba(${cmd.color[0]}, ${cmd.color[1]}, ${cmd.color[2]}, ${cmd.color[3]/255.0})`;
                ctx.fillRect(0, 0, blk.c.width, blk.c.height);
                break;
            }
            case "swap": {
                const blk1 = blocks.get(cmd.block1);
                const blk2 = blocks.get(cmd.block2);
                if (!blk1) throw new Error(`No such block: ${cmd.block1}`);
                if (!blk2) throw new Error(`No such block: ${cmd.block2}`);
                if (blk1.c.width !== blk2.c.width || blk1.c.height !== blk2.c.height) {
                    throw new Error(`Block shapes are incompatible for ${cmd.block1} and ${cmd.block2}`);
                }
                const x1 = blk1.x;
                const y1 = blk1.y;
                blk1.x = blk2.x;
                blk1.y = blk2.y;
                blk2.x = x1;
                blk2.y = y1;
                break;
            }
            case "merge": {
                const blk1 = blocks.get(cmd.block1);
                const blk2 = blocks.get(cmd.block2);
                if (!blk1) throw new Error(`No such block: ${cmd.block1}`);
                if (!blk2) throw new Error(`No such block: ${cmd.block2}`);
                if (blk1.x === blk2.x) {
                    if (blk1.c.width !== blk2.c.width) throw new Error("Blocks are not compatible for merge: width differs");
                    const minY = Math.min(blk1.y, blk2.y);
                    const oldX = blk1.x;
                    blk1.y -= minY;
                    blk2.y -= minY;
                    blk1.x = blk2.x = 0;
                    mergeBlocks(oldX, minY, blk1.c.width, blk1.c.height + blk2.c.height, blk1, blk2);
                } else if (blk1.y === blk2.y) {
                    if (blk1.c.height !== blk2.c.height) throw new Error("Blocks are not compatible for merge: height differs");
                    const minX = Math.min(blk1.x, blk2.x);
                    const oldY = blk1.y;
                    blk1.x -= minX;
                    blk2.x -= minX;
                    blk1.y = blk2.y = 0;
                    mergeBlocks(minX, oldY, blk1.c.width + blk2.c.width, blk1.c.height, blk1, blk2);
                } else {
                    throw new Error("Blocks to be merged are not aligned");
                }
                blocks.delete(cmd.block1)
                blocks.delete(cmd.block2)
                break;
            }
            case "comment":
                break;
        }
    }
    const canvas = document.getElementById("canvas") as HTMLCanvasElement;
    const ctx = canvas.getContext("2d")!;
    for(const v of blocks.values()) {
        ctx.drawImage(v.c, v.x, v.y);
    }
});

import {lex} from './lexer.js'
import {Parser} from './parser.js'

type Command =
    | {type: "cut-point", block: BlockId, point: Point}
    | {type: "cut-line", block: BlockId, dir: Orientation, num: number}
    | {type: "color", block: BlockId, color: Color}
    | {type: "swap", block1: BlockId, block2: BlockId}
    | {type: "merge", block1: BlockId, block2: BlockId}
    | {type: "comment", text: string}

const base_costs: { [K in Command['type']]: number } = {
    "cut-line": 7,
    "cut-point": 10,
    "color": 5,
    "swap": 3,
    "merge": 1,
    "comment": 0,
}

type BlockId = string
type Point = [number, number]
type Orientation = "x" | "y"
type Color = [number, number, number, number]
type Block = { x: number, y: number, c: HTMLCanvasElement }

type Rect = {width:number, height:number}

const inputBox = document.getElementById("input") as HTMLTextAreaElement;
const run = document.getElementById("run")!;
const back = document.getElementById("back")!;
const forward = document.getElementById("forward")!;
const costSpan = document.getElementById("cost") as HTMLSpanElement;
const totalCostSpan = document.getElementById("total_cost") as HTMLSpanElement;
const diffSpan = document.getElementById("difference") as HTMLSpanElement;
const stepSpan = document.getElementById("step") as HTMLSpanElement;
const referenceFile = document.getElementById("reference") as HTMLInputElement;
const referenceSelector = document.getElementById("reference_select") as HTMLInputElement;
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

function size(rect: Rect) {
    return rect.width*rect.height;
}

function step_cost(type: keyof typeof base_costs, canvas: Rect, block: Rect) {
    return Math.round(base_costs[type] * size(canvas)/size(block));
}

let exec_steps: Array<{blocks: Map<string, Block>, cost: number, cmd: Command}> = []

function save_step(blocks: Map<string, Block>, this_step_cost: number, cmd: Command) {
    const step = { cost: this_step_cost, blocks: new Map<string, Block>(), cmd };
    for (const [k, v] of blocks.entries()) {
        const canvas = document.createElement('canvas');
        canvas.width = v.c.width;
        canvas.height = v.c.height;
        const ctx = canvas.getContext('2d')!;
        ctx.drawImage(v.c, 0, 0);
        step.blocks.set(k, {x: v.x, y: v.y, c: canvas});
    }
    exec_steps.push(step);
}

const mainCanvas = document.getElementById("canvas") as HTMLCanvasElement;
const mainCtx = mainCanvas.getContext("2d")!;
mainCtx.translate(0, mainCanvas.height);
mainCtx.scale(1, -1);

function render(blocks: Map<string, Block>) {
    for(const v of blocks.values()) {
        mainCtx.drawImage(v.c, v.x, v.y);
    }
}

let currentIndex = 0;

run.addEventListener('click', () => {
    const input = inputBox.value;
    const parsed = parser.parse(lex(input)).filter((x: any) => x !== null) as Command[];
    const blocks = new Map<string, Block>();
    exec_steps = [];
    currentIndex = 0;
    let counter = 1;
    const canvas0 = document.createElement('canvas');
    canvas0.width=400
    canvas0.height=400
    const ctx0 = canvas0.getContext('2d')!;
    ctx0.fillStyle="rgba(255,255,255,1)"
    ctx0.fillRect(0, 0, 400, 400);
    blocks.set("0", {x: 0, y: 0, c: canvas0})
    save_step(blocks, 0, {type: "comment", text: "Initial state"});

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
        return newBlock;
    }
    let cost = 0;

    for (const cmd of parsed) {
        let this_step_cost: number;
        const cost_est = step_cost.bind(this, cmd.type, canvas0);
        switch(cmd.type) {
            case "cut-point": {
                const blk = blocks.get(cmd.block);
                if (!blk) throw new Error(`No such block: ${cmd.block}`);
                this_step_cost = cost_est(blk.c);
                cmd.point[0] -= blk.x;
                cmd.point[1] -= blk.y;
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
                this_step_cost = cost_est(blk.c);
                const blksize = cmd.dir == "x" ? blk.c.width : blk.c.height;
                if (cmd.dir === "x") {
                    cmd.num -= blk.x;
                } else if (cmd.dir === "y") {
                    cmd.num -= blk.y;
                }
                if (cmd.num === 0 || cmd.num >= blksize) throw new Error(`Point is on block border in ${cmd.dir} dimension`);
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
                this_step_cost = cost_est(blk.c);
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
                this_step_cost = cost_est(blk1.c);
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
                this_step_cost = Math.max(cost_est(blk1.c), cost_est(blk2.c));
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
                this_step_cost = 0;
                break;
        }
        cost += this_step_cost;
        save_step(blocks, this_step_cost, cmd);
    }
    render(blocks);
    costSpan.innerText = `Cost: ${cost}`;
    const diff = computeDifference();
    diffSpan.innerText = `Difference: ${diff}`;
    totalCostSpan.innerText = `Total cost: ${cost + diff}`;
    updateHash();
});

function selectCurrentCommand() {
    const text = inputBox.value;
    const lines = text.split('\n');
    let posStart = 0;
    for(const line of lines.slice(0, currentIndex-1)) {
        posStart += line.length+1;
    }
    const lastLine = lines[currentIndex-1];
    if (lastLine === undefined) return;
    const posEnd = posStart + lastLine.length + 1;
    inputBox.focus();
    inputBox.setSelectionRange(posStart, posEnd);
}

back.addEventListener('click', () => {
    currentIndex -= 1;
    if (currentIndex < 0) currentIndex = exec_steps.length - 1;
    const step = exec_steps[currentIndex];
    if(!step) return;
    render(step.blocks);
    selectCurrentCommand();
    stepSpan.innerText = `Current step: ${JSON.stringify(step.cmd)}; Step cost: ${step.cost}`;
})

forward.addEventListener('click', () => {
    currentIndex += 1;
    if (currentIndex >= exec_steps.length) currentIndex = 0;
    const step = exec_steps[currentIndex];
    if(!step) return;
    render(step.blocks);
    selectCurrentCommand();
    stepSpan.innerText = `Current step: ${JSON.stringify(step.cmd)}; Step cost: ${step.cost}`;
})

referenceFile.addEventListener('input', async (evt: Event) => {
    const target = evt.target! as HTMLInputElement
    const file = target.files![0]
    console.log(file)
    const bitmap = await createImageBitmap(file);
    const canvas = document.getElementById("ref_canvas") as HTMLCanvasElement;
    canvas.width = bitmap.width;
    canvas.height = bitmap.height;
    const ctx = canvas.getContext("2d")!;
    ctx.drawImage(bitmap, 0, 0);
})

type RGBA = {r: number, g: number, b: number, a: number};

function sqr(x: number) {
    return x*x;
}

function pixelDiff(p1: RGBA, p2: RGBA) {
    const rDist = sqr(p1.r - p2.r);
    const gDist = sqr(p1.g - p2.g);
    const bDist = sqr(p1.b - p2.b);
    const aDist = sqr(p1.a - p2.a);
    return Math.sqrt(rDist + gDist + bDist + aDist);
}

function rgbaAt(i: number, img: ImageData): RGBA {
    return {r: img.data.at(i)!, g: img.data.at(i+1)!, b: img.data.at(i+2)!, a: img.data.at(i+3)!}
}

function computeDifference() {
    const ref_canvas = document.getElementById("ref_canvas") as HTMLCanvasElement;
    const canvas = document.getElementById("canvas") as HTMLCanvasElement;
    const diff_canvas = document.getElementById("diff_canvas") as HTMLCanvasElement;
    const ctx = diff_canvas.getContext('2d')!;
    // fill with red, show difference with alpha-channel
    ctx.fillStyle = 'red';
    ctx.fillRect(0,0,diff_canvas.width, diff_canvas.height);
    const ref_image = ref_canvas.getContext('2d')!.getImageData(0, 0, ref_canvas.width, ref_canvas.height);
    const image = canvas.getContext('2d')!.getImageData(0, 0, canvas.width, canvas.height);
    const diff_image = diff_canvas.getContext('2d')!.getImageData(0, 0, diff_canvas.width, diff_canvas.height);
    let diff = 0;
    let alpha = 0.005;
    for(let i = 0; i < image.data.length; i += 4) {
        const p1 = rgbaAt(i, ref_image);
        const p2 = rgbaAt(i, image);
        const pxdiff = pixelDiff(p1, p2);
        diff += pxdiff;
        // sets the alpha channel
        diff_image.data[i+3] = Math.round(pxdiff / 2);
    }
    ctx.putImageData(diff_image, 0, 0);
    return Math.round(diff*alpha);
}

function updateHash() {
    const state = {
        input: inputBox.value,
        reference: referenceSelector.value,
    }
    window.location.hash = btoa(JSON.stringify(state));
}

function loadReferenceImage() {
    const img = new Image();
    img.src = `${window.location.protocol}//${window.location.host}/data/problems/${referenceSelector.value}`;
    img.onload = () => {
        const canvas = document.getElementById("ref_canvas") as HTMLCanvasElement;
        canvas.width = img.width;
        canvas.height = img.height;
        const ctx = canvas.getContext("2d")!;
        ctx.drawImage(img, 0, 0);
        updateHash();
    }
}

document.addEventListener('DOMContentLoaded', () => {
    referenceSelector.addEventListener('change', () => {
        loadReferenceImage();
    })
    for(let i = 1; i<= 15; i++) {
        const option = document.createElement('option');
        option.value = `${i}.png`;
        option.innerText = option.value;
        referenceSelector.appendChild(option);
    }
    if(window.location.hash) {
        const state = JSON.parse(atob(window.location.hash.slice(1)));
        inputBox.value = state.input;
        referenceSelector.value = state.reference;
    }
    loadReferenceImage();
});

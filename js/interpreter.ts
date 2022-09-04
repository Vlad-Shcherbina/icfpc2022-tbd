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
const overlayToggle = document.getElementById("toggle_overlay") as HTMLInputElement;
const heatmapColor = document.getElementById("heatmap_color")! as HTMLInputElement;
const blocksToggle = document.getElementById("toggle_blocks_overlay") as HTMLInputElement;
const errorSpan = document.getElementById("error") as HTMLSpanElement;

const blocksCanvas = document.getElementById("blocks_canvas") as HTMLCanvasElement;
const blocksCtx = blocksCanvas.getContext('2d')!;
blocksCtx.translate(0, blocksCanvas.height);
blocksCtx.scale(1, -1);

const mainCanvas = document.getElementById("canvas") as HTMLCanvasElement;
const mainCtx = mainCanvas.getContext("2d")!;
mainCtx.translate(0, mainCanvas.height);
mainCtx.scale(1, -1);

const refCanvas = document.getElementById("ref_canvas") as HTMLCanvasElement;
const refCtx = refCanvas.getContext('2d')!;
const diffCanvas = document.getElementById("diff_canvas") as HTMLCanvasElement;
const diffCtx = diffCanvas.getContext('2d')!;

let currentBlocks = new Map<string,Block>();

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

type BlockSpec = {
    blockId: string,
    bottomLeft: Point,
    topRight: Point,
    color: Color,
}

type InitialCanvas = {
    width: number,
    height: number,
    blocks: BlockSpec[],
}

const defaultCanvas : InitialCanvas = {
    width: 400,
    height: 400,
    blocks: [{
        blockId: "0",
        bottomLeft: [0, 0],
        topRight: [400, 400],
        color: [255,255,255,255],
    }]
};

let initialCanvas : InitialCanvas = defaultCanvas;

function render(blocks: Map<string, Block>) {
    currentBlocks = blocks;
    blocksCtx.clearRect(0,0,blocksCanvas.width, blocksCanvas.height);
    if(blocksToggle.checked) {
        blocksCtx.strokeStyle = "white";
        blocksCtx.fillStyle = "white";
    } else {
        blocksCtx.strokeStyle = "black";
        blocksCtx.fillStyle = "black";
    }
    mainCtx.clearRect(0,0,mainCanvas.width, mainCanvas.height);
    for(const [k,v] of blocks.entries()) {
        mainCtx.drawImage(v.c, v.x, v.y);
        blocksCtx.strokeRect(v.x,v.y,v.c.width,v.c.height);
        blocksCtx.save();
        blocksCtx.scale(1, -1);
        blocksCtx.fillText(k, v.x + 2 , - v.y - 2, v.c.width);
        blocksCtx.restore();
    }
}

blocksCanvas.addEventListener('mousemove', (evt) => {
    const rect = blocksCanvas.getBoundingClientRect();
    const x = evt.clientX - rect.left;
    const y = rect.bottom - evt.clientY;
    for(const [k,v] of currentBlocks.entries()) {
        if (x >= v.x && x <= v.x + v.c.width && y >= v.y && y <= v.y + v.c.height) {
            blocksCanvas.title = `${k}\nBlock {x: ${v.x}, y: ${v.y}, width: ${v.c.width}, height: ${v.c.height}}`;
            blocksCanvas.title += `\nCursor pos: ${Math.round(x)}, ${Math.round(y)}`;
            break;
        }
    }
})

mainCanvas.addEventListener('mousemove', (evt) => {
    const rect = mainCanvas.getBoundingClientRect();
    const x = evt.clientX - rect.left;
    const y = rect.bottom - evt.clientY;
    const y_ = evt.clientY - rect.top;
    const pixel = mainCtx.getImageData(x, y_, 1, 1);
    const rgba = `[${pixel.data[0]}, ${pixel.data[1]}, ${pixel.data[2]}, ${pixel.data[3]}]`
    mainCanvas.title = `${Math.round(x)}, ${Math.round(y)}; ${rgba}`;
});

refCanvas.addEventListener('mousemove', (evt) => {
    const rect = refCanvas.getBoundingClientRect();
    const x = evt.clientX - rect.left;
    const y = rect.bottom - evt.clientY;
    const y_ = evt.clientY - rect.top;
    const pixel = refCtx.getImageData(x, y_, 1, 1);
    const rgba = `[${pixel.data[0]}, ${pixel.data[1]}, ${pixel.data[2]}, ${pixel.data[3]}]`
    refCanvas.title = `${Math.round(x)}, ${Math.round(y)}; ${rgba}`;
});

let currentIndex = 0;

run.addEventListener('click', async () => {
    const input = inputBox.value;
    const parsed = parser.parse(lex(input)).filter((x: any) => x !== null) as Command[];
    const blocks = new Map<string, Block>();
    errorSpan.innerText = "";
    exec_steps = [];
    currentIndex = 0;
    let counter = initialCanvas.blocks.length;
    for(const block of initialCanvas.blocks){
        const canvas = document.createElement('canvas');
        canvas.width = block.topRight[0] - block.bottomLeft[0]
        canvas.height = block.topRight[1] - block.bottomLeft[1]
        const ctx = canvas.getContext('2d')!;
        ctx.fillStyle = `rgba(${block.color[0]}, ${block.color[1]}, ${block.color[2]}, ${block.color[3]/255.0})`;
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        blocks.set(block.blockId, {x: block.bottomLeft[0], y: block.bottomLeft[1], c: canvas})
    }
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
    let lastCmd = null;
    let cmdCount = 0;

    try {
        for (const cmd of parsed) {
            lastCmd = cmd;
            cmdCount += 1;
            let this_step_cost: number;
            const cost_est = step_cost.bind(this, cmd.type, initialCanvas);
            switch(cmd.type) {
                case "cut-point": {
                    const blk = blocks.get(cmd.block);
                    if (!blk) throw new Error(`No such block: ${cmd.block}`);
                    this_step_cost = cost_est(blk.c);
                    let [x, y] = cmd.point
                    x -= blk.x;
                    y -= blk.y;
                    if (x <= 0 || x >= blk.c.width) {
                        throw new Error(`Point ${cmd.point} is on block border or outside of block ${cmd.block} in x dimension`);
                    }
                    if (y <= 0 || y >= blk.c.height) {
                        throw new Error(`Point ${cmd.point} is on block border or outside of block ${cmd.block} in y dimension`);
                    }
                    blocks.delete(cmd.block);
                    blocks.set(`${cmd.block}.0`, { x: blk.x, y: blk.y, c: copyFrom(blk.c, 0, 0, x, y) });
                    blocks.set(`${cmd.block}.1`, { x: blk.x + x, y: blk.y,
                        c: copyFrom(blk.c, x, 0, blk.c.width - x, y) });
                    blocks.set(`${cmd.block}.2`, { x: blk.x + x, y: blk.y + y,
                        c: copyFrom(blk.c, x, y, blk.c.width - x, blk.c.height - y) });
                    blocks.set(`${cmd.block}.3`, { x: blk.x, y: blk.y + y,
                        c: copyFrom(blk.c, 0, y, x, blk.c.height - y) });
                    break;
                }
                case "cut-line": {
                    const blk = blocks.get(cmd.block);
                    if (!blk) throw new Error(`No such block: ${cmd.block}`);
                    this_step_cost = cost_est(blk.c);
                    const blksize = cmd.dir == "x" ? blk.c.width : blk.c.height;
                    let num = cmd.num;
                    if (cmd.dir === "x") {
                        num -= blk.x;
                    } else if (cmd.dir === "y") {
                        num -= blk.y;
                    }
                    if (num <= 0 || num >= blksize) {
                        throw new Error(`Point ${cmd.num} is on block border or outside of block ${cmd.block} in ${cmd.dir} dimension`);
                    }
                    blocks.delete(cmd.block);
                    if (cmd.dir === "x") {
                        blocks.set(`${cmd.block}.0`, { x: blk.x, y: blk.y,
                            c: copyFrom(blk.c, 0, 0, num, blk.c.height) });
                        blocks.set(`${cmd.block}.1`, { x: blk.x + num, y: blk.y,
                            c: copyFrom(blk.c, num, 0, blk.c.width - num, blk.c.height) });
                    } else if (cmd.dir === "y") {
                        blocks.set(`${cmd.block}.0`, { x: blk.x, y: blk.y,
                            c: copyFrom(blk.c, 0, 0, blk.c.width, num) });
                        blocks.set(`${cmd.block}.1`, { x: blk.x, y: blk.y + num,
                            c: copyFrom(blk.c, 0, num, blk.c.width, blk.c.height - num) });
                    }
                    break;
                }
                case "color": {
                    const blk = blocks.get(cmd.block);
                    if (!blk) throw new Error(`No such block: ${cmd.block}`);
                    this_step_cost = cost_est(blk.c);
                    const ctx = blk.c.getContext('2d')!;
                    ctx.clearRect(0, 0, blk.c.width, blk.c.height);
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
                    // block_size = max(size(blk1), size(blk2)), according to organizers
                    // since cost is inversely proportional to block size, all else being equal,
                    // we use minimal cost of the two.
                    this_step_cost = Math.min(cost_est(blk1.c), cost_est(blk2.c));
                    if (blk1.x === blk2.x) {
                        if (blk1.c.width !== blk2.c.width) {
                            throw new Error(`Blocks ${cmd.block1} and ${cmd.block2} are not compatible for merge: width differs`);
                        }
                        if (blk2.y - blk1.y !== blk1.c.height && blk1.y-blk2.y !== blk2.c.height) {
                            throw new Error(`Blocks ${cmd.block1} and ${cmd.block2} are not adjacent`);
                        }
                        const minY = Math.min(blk1.y, blk2.y);
                        const oldX = blk1.x;
                        blk1.y -= minY;
                        blk2.y -= minY;
                        blk1.x = blk2.x = 0;
                        mergeBlocks(oldX, minY, blk1.c.width, blk1.c.height + blk2.c.height, blk1, blk2);
                    } else if (blk1.y === blk2.y) {
                        if (blk1.c.height !== blk2.c.height) {
                            throw new Error(`Blocks ${cmd.block1} and ${cmd.block2} are not compatible for merge: height differs`);
                        }
                        if (blk2.x - blk1.x !== blk1.c.width && blk1.x-blk2.x !== blk2.c.width) {
                            throw new Error(`Blocks ${cmd.block1} and ${cmd.block2} are not adjacent`);
                        }
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
                    cmdCount -= 1;
                    break;
            }
            cost += this_step_cost;
            save_step(blocks, this_step_cost, cmd);
            render(blocks);
            await new Promise((resolve) => setTimeout(resolve, 0));
        }
    } catch (e) {
        errorSpan.innerText = `${(e as Error).message} in command ${cmdCount}: ${JSON.stringify(lastCmd)}`;
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
    let curCmd = 0;
    let lastLine = lines[0];
    for(const line of lines) {
        lastLine = line;
        if (line) curCmd += 1;
        if (curCmd == currentIndex) break;
        posStart += line.length+1;
    }
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
    const bitmap = await createImageBitmap(file);
    refCanvas.width = bitmap.width;
    refCanvas.height = bitmap.height;
    refCtx.drawImage(bitmap, 0, 0);
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
    // fill with solid color, show difference with alpha-channel
    diffCtx.fillStyle = heatmapColor.value;
    diffCtx.fillRect(0,0,diffCanvas.width, diffCanvas.height);
    const ref_image = refCtx.getImageData(0, 0, refCanvas.width, refCanvas.height);
    const image = mainCtx.getImageData(0, 0, mainCanvas.width, mainCanvas.height);
    const diff_image = diffCtx.getImageData(0, 0, diffCanvas.width, diffCanvas.height);
    let diff = 0;
    let alpha = 0.005;
    for(let i = 0; i < image.data.length; i += 4) {
        const p1 = rgbaAt(i, ref_image);
        const p2 = rgbaAt(i, image);
        const pxdiff = pixelDiff(p1, p2);
        diff += pxdiff;
        // sets the alpha channel
        // max pxdiff is sqrt(4*255**2) = 2*255 => max alpha value is half max pxdiff.
        diff_image.data[i+3] = Math.round(pxdiff / 2);
    }
    diffCtx.putImageData(diff_image, 0, 0);
    return Math.round(diff*alpha);
}

function updateHash() {
    const state = {
        input: inputBox.value,
        reference: referenceSelector.value,
        overlay: overlayToggle.checked,
        heatmapColor: heatmapColor.value,
    }
    window.location.hash = btoa(JSON.stringify(state));
}

async function loadReferenceImage() {
    const img = new Image();
    img.src = `${window.location.protocol}//${window.location.host}/data/problems/${referenceSelector.value}`;
    await new Promise((resolve) => img.onload=resolve)
    refCanvas.width = img.width;
    refCanvas.height = img.height;
    refCtx.drawImage(img, 0, 0);
    try {
        initialCanvas = await (await fetch(`${window.location.protocol}//${window.location.host}/data/problems/${referenceSelector.value.replace("png","initial.json")}`)).json() as InitialCanvas;
    } catch (e) {
        console.error(e);
        initialCanvas = defaultCanvas;
    }
    updateHash();
}

function updateOverlayStyle() {
    if (overlayToggle.checked) {
        refCanvas.style.position = 'absolute';
        diffCanvas.style.position = 'absolute';
    } else {
        refCanvas.style.position = 'relative';
        diffCanvas.style.position = 'relative';
    }
    updateHash();
}

blocksToggle.addEventListener('change', () => {
    if (blocksToggle.checked) {
        mainCanvas.style.position = 'absolute';
        blocksCanvas.style.position = 'absolute';
    } else {
        mainCanvas.style.position = 'relative';
        blocksCanvas.style.position = 'relative';
    }
    render(currentBlocks);
})

document.addEventListener('DOMContentLoaded', async () => {
    referenceSelector.addEventListener('change', () => {
        loadReferenceImage();
    })
    for (let i = 1; i <= 35; i++) {
        const option = document.createElement('option');
        option.value = `${i}.png`;
        option.innerText = option.value;
        referenceSelector.appendChild(option);
    }
    overlayToggle.addEventListener('change', updateOverlayStyle)
    if(window.location.hash) {
        const state = JSON.parse(atob(window.location.hash.slice(1)));
        inputBox.value = state.input;
        referenceSelector.value = state.reference;
        overlayToggle.checked = state.overlay;
        heatmapColor.value = state.heatmapColor || "#ff0000";
        updateOverlayStyle();
    }
    await loadReferenceImage();
    if(window.location.hash) {
        run.click();
    }
});

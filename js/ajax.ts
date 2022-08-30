import assert from './assert.js';
import { FoobarRequest, FoobarResponse } from './types.js';

async function main() {
    let req: FoobarRequest = { x: 5 };
    let resp = await fetch('/example/foobar', {method: 'POST', body: JSON.stringify(req)});
    assert(resp.ok);
    let r = await resp.json() as FoobarResponse;

    console.log(r.s);
    console.log(r.y);
    assert(r.s === "*****");
}

main();

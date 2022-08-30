import assert from './assert.js';

async function main() {
    let response = await fetch('/example/hello/world');
    assert(response.ok);
    console.log(await response.text());
}

main();

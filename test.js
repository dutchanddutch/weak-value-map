'use strict';

var WeakValueMap = require('./');
var test = require('tape');

test('map', function(assert) {
	let map = new WeakValueMap();
	map.set(1, 'one')
	   .set(2, 2)
	   .set(3, true)
	   .set(4, false);
	let obj = new Date;
	assert.strictEqual(map.size, 4);
	assert.strictEqual(map.has(5), false);
	assert.strictEqual(map.has('5'), false);
	assert.strictEqual(map.get(5), undefined);
	map.set(5, {});
	assert.strictEqual(map.size, 5);
	assert.strictEqual(map.has(5), true);
	map.set(5, obj);
	assert.strictEqual(map.size, 5);
	assert.strictEqual(map.has(5), true);
	assert.strictEqual(map.get(1), 'one');
	assert.strictEqual(map.get(2), 2);
	assert.strictEqual(map.get(3), true);
	assert.strictEqual(map.get(4), false);
	assert.strictEqual(map.get(5), obj);
	assert.strictEqual(map.get('5'), obj);
	assert.strictEqual(map.get({ toString() { return 5; } }), obj);
	map.set(1, 'changed');
	assert.strictEqual(map.get(1), 'changed');
	assert.strictEqual(map.size, 5);
	map.delete(1);
	assert.strictEqual(map.has(1), false);
	assert.strictEqual(map.get(1), undefined);
	assert.strictEqual(map.size, 4);
	assert.end();
});

test('gc', function(assert) {
	let map = new WeakValueMap();
	let obj1 = {a: 1234, b: 'test', c: {d: 'testing'}};
	let obj2 = Object.assign({}, obj1);
	map.set(1, obj1);
	map.set(2, obj2);
	assert.strictEqual(map.get(1), obj1);
	assert.strictEqual(map.get(2), obj2);
	eval(""); gc();
	assert.strictEqual(map.get(1), obj1);
	assert.strictEqual(map.get(2), obj2);
	assert.strictEqual(map.size, 2);
	obj2 = null;
	eval(""); gc();
	assert.strictEqual(map.get(1), obj1);
	assert.strictEqual(map.get(2), undefined);
	assert.strictEqual(map.size, 1);
	obj1 = null;
	eval(""); gc();
	assert.strictEqual(map.get(1), undefined);
	assert.strictEqual(map.get(2), undefined);
	assert.strictEqual(map.size, 0);
	assert.end();
});

test('exception', function(assert) {
	let map = new WeakValueMap();
	let key = { toString() { throw 42 } };
	let obj = {};
	let err;
	try {
		map.set(key, obj);
	} catch( e ) {
		err = e;
	}
	assert.strictEqual(err, 42);
	assert.strictEqual(map.size, 0);
	assert.end();
});

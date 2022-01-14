function resolve(obj: any) {
    if (typeof obj === 'object') {
        if (Object.prototype.hasOwnProperty.call(obj, '$ref')) {
            return obj;
        }
        else {
            for (var k in obj) {
                if (Object.prototype.hasOwnProperty.call(obj, k)) {
                    console.log('1:'+k);
                    obj[k] = resolve(obj[k]);
                }
            }
            return obj;
        }    
    }
    else {
        for (var k in obj) {
            if (Object.prototype.hasOwnProperty.call(obj, k)) {
                console.log('2:'+k);
                obj[k] = resolve(obj[k]);
            }
        }
    }
}

let fn = Deno.args[0]
const decoder = new TextDecoder('utf-8')
const data = await Deno.readFile(fn)
console.log("processing "+fn)

var obj = resolve(JSON.parse(decoder.decode(data)))
console.log(obj)

function resolve(obj: any) {

    function inner_resolve(obj: any) {
        Object.keys(obj).some(function(k) {
            if (k === "$ref") {
                console.log("$ref found in "+obj+" : " + obj['$ref'])
                //value = obj[k];
            }
            if (obj[k] && typeof obj[k] === 'object') {
                inner_resolve(obj[k]);
            }
        });
        return null
    }

    inner_resolve(obj)
}

let fn = Deno.args[0]
const decoder = new TextDecoder('utf-8')
const data = await Deno.readFile(fn)
console.log("processing "+fn)

var obj = JSON.parse(decoder.decode(data))
resolve(obj)
//console.log(obj)

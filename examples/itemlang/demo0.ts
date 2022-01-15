
function assert(condition: any, msg?: string): asserts condition {
    if (!condition) {
        throw new Error(msg);
    }
}

function resolve_path(obj: object, path: string): object {
    let parts = path.split("/");
    assert(parts[0].length==0, "path must start with /");
    parts = parts.slice(1)
    if (parts[0].length==0) {
        return obj; // found path=='/'
    }
    parts.forEach( p => {
        const regexpArrayAccess = /(\w+)\[([0-9]+)\]/;
        const match = p.match(regexpArrayAccess);
        if (match) {
            p = match[1]
            let idx = Number(match[2])
            //console.log("resolve_path with idx: ..."+p+", "+idx);
            assert(p in obj, p+" not found, as part of "+path);
            obj = <object>(obj[p as keyof object][idx]);
        }
        else {
            //console.log("resolve_path: ..."+p);
            assert(p in obj, p+" not found, as part of "+path);
            obj = <object>(obj[p as keyof object]);
        }
    });
    return obj;
}

async function load(fn: string): Promise<object> {

    const decoder = new TextDecoder('utf-8')
    const data = await Deno.readFile(fn)
    //console.log("processing "+fn)
    var root_obj = JSON.parse(decoder.decode(data))

    function inner_resolve(obj: any) {
        Object.keys(obj).some(function(k) {
            if (k === "$ref") {
                //console.log("$ref found in "+obj+" : " + obj['$ref'])
                let [uri, ref] = obj['$ref'].split('#');
                if (uri.length==0) {
                    //console.log("inner --> "+ref)
                    return resolve_path(root_obj,ref);
                }
                else {
                    console.log(uri+","+ref)
                }
                //value = obj[k];
                return obj;
            }
            if (obj[k] && typeof obj[k] === 'object') {
                obj[k] == inner_resolve(obj[k]);
            }
        });
        return obj
    }

    root_obj = inner_resolve(root_obj)
    return root_obj
}

let fn = Deno.args[0]
let obj = await load(fn)
//console.log(obj)

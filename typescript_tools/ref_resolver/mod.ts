import { join, isAbsolute, normalize, dirname } from "https://deno.land/std@0.102.0/path/mod.ts";

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

export async function load(fn_main: string): Promise<object> {
    if (!isAbsolute(fn_main)) {
        fn_main = join(Deno.cwd(), fn_main);
    }
    fn_main = normalize(fn_main);
    const decoder = new TextDecoder('utf-8')
    const data = await Deno.readFile(fn_main)
    console.log("processing "+fn_main)
    let root_obj = JSON.parse(decoder.decode(data))
    let all_files = new Map<string, object>();

    async function inner_resolve(obj: any, fn_base: string): Promise<object> {
        Object.keys(obj).some(async function(k) {
            if (k === "$ref") {
                //console.log("$ref found in "+obj+" : " + obj['$ref'])
                let [uri, ref] = obj['$ref'].split('#');
                if (uri.length==0) {
                    //console.log("inner --> "+ref)
                    return resolve_path(root_obj,ref);
                }
                else {
                    let fn = uri;
                    if (!isAbsolute(fn)) {
                        fn = join(dirname(fn_base), fn);
                    }
                    fn = normalize(fn)
                    let other_obj: object = {};
                    if (!(fn in all_files)) {
                        const decoder = new TextDecoder('utf-8')
                        const data = await Deno.readFile(fn)
                        console.log("inner_processing "+fn)
                        other_obj = JSON.parse(decoder.decode(data))
                        all_files.set(fn, other_obj)
                        all_files.set(fn, inner_resolve(other_obj, fn));
                    }
                    else {
                        other_obj = all_files.get(fn) as object;
                    }
                    return resolve_path(other_obj,ref);
                }
                //value = obj[k];
                return obj;
            }
            if (obj[k] && typeof obj[k] === 'object') {
                obj[k] == inner_resolve(obj[k], fn_base);
            }
        });
        return obj
    }

    all_files.set(fn_main, root_obj);
    all_files.set(fn_main, inner_resolve(root_obj, fn_main));
    return root_obj
}

export function findObjByTypeype(obj: object, typename: string): object[] {
    let result: object[] = [];
    function find(obj: object, typename: string) {
        if ("$type" in obj) {
            if (obj["$type" as keyof object]==typename) {
                result.push(obj);
            }
        }
        Object.keys(obj).some(function(k) {
            if (obj[k as keyof object] && typeof obj[k as keyof object] === 'object') {
                find(obj[k as keyof object], typename);
            }
        });
    }
    find(obj, typename);
    return result;
}
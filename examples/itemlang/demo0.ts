import { load } from "../../typescript_tools/ref_resolver/mod.ts";

let obj = await load(Deno.args[0])
console.log(obj)

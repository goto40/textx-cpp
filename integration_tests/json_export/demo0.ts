import * as TextxObj from "../../typescript_tools/ref_resolver/mod.ts";
import * as Eta from "https://deno.land/x/eta@v1.12.3/mod.ts";
import { join } from "https://deno.land/std@0.102.0/path/mod.ts";


const __dirname = new URL('.', import.meta.url).pathname;
const simpleEta = join(__dirname, "simple.eta");
let obj = await TextxObj.load(Deno.args[0])
//console.log(obj)

let structs = TextxObj.findObjByTypeype(obj, "Struct") as any;
for(let i=0;i<structs.length;i++) {
    console.log( structs[i].name );
    let code = await Eta.renderFile(simpleEta, structs[i]) as string;
    await Deno.writeTextFile("src-gen/"+structs[i].name+".h", code);
}


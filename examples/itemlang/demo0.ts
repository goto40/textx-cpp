import * as TextxObj from "../../typescript_tools/ref_resolver/mod.ts";
import * as Eta from "https://deno.land/x/eta@v1.12.3/mod.ts";

let obj = await TextxObj.load(Deno.args[0])
//console.log(obj)

let structs = TextxObj.findObjByTypeype(obj, "Struct");
for(let i=0;i<structs.length;i++) {
    console.log( structs[i] );
}

//Eta.renderFile("simple.eta", obj);
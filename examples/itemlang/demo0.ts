let fn = Deno.args[0]
const decoder = new TextDecoder('utf-8')
const data = await Deno.readFile(fn)
console.log("processing "+fn)
console.log(JSON.parse(decoder.decode(data)))

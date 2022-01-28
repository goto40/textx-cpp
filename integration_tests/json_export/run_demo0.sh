# build the c++ project first 

../../build/itemlang ../../examples/itemlang/model/big_example.item
deno run --allow-write --allow-read demo0.ts src-gen/big_example.json 

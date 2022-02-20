# mgrep (model grep)
A grep-like command whoch uses a metamodel for each line to parse it...

Special functions: 
  * `filename(model)`: filename of processed line
  * `line(model)`: line number of processed line

## Example
```bash
./build/mgrep --transform "{% filename(model) %}:{% line(model) %} imports {% model.file %}" "Model: '#include' /[\"<]/ file=/[^\">]*/ /[\">]/;" src/textx/*.h
```
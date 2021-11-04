# mgrep (model grep)
A grep-like command which uses a meta model for each line to parse it...

Special functions: 
  * `filename(model)`: filename of processed line
  * `line(model)`: line number of processed line

## Example
You can provide a transform function:
```bash
./build/mgrep --transform "{% filename(model) %}:{% line(model) %} imports {% model.file %}" "Model: '#include' /[\"<]/ file=/[^\">]*/ /[\">]/;" src/textx/*.h
```

Without transform option you get the full model object:
```bash
./build/mgrep "Model: 'cpu' 'MHz' /[^\d]+/ mhz=NUMBER;" /proc/cpuinfo
```
```
Model{mhz=900.001}
Model{mhz=1600.64}
Model{mhz=2700}
Model{mhz=2700}
```

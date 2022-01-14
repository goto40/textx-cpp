from pprint import pprint
import jsonref

# An example json document
data = jsonref.load_uri("file://big_example.json")
pprint(data)  # Reference is not evaluated until here

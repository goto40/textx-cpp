import textx
import time

mm = textx.metamodel_from_str(r'''
        Model: classes+=Class;
        Class: "class" name=ID "{" attrs+=Attribute "{" refs+=Ref "}" "}";
        Attribute: name=ID ":" clazz=[Class];
        Ref: "ref" ref=[Attribute|FQN|+m:..attrs.(~clazz.attrs)*];
        Comment: /#.*?$/;
        FQN: ID('.'ID)*;
''')

def create_model(n):
        model_text=''
        for i in range(n):
                model_text += f"class A{i} {{\n"
                model_text += f"  a: A{i}\n"
                model_text += f"  b: A{((i+1)%n)}\n"
                model_text += f"  c: A{((i+n-1)%n)}\n"
                model_text += f"  d: A{((i+n-2)%n)}\n"
                model_text += f"  {{\n"
                model_text += f"  ref c\n"
                model_text += f"  ref a.a.a.a\n"
                model_text += f"  ref b.b.b\n"
                model_text += f"  ref c.c.c\n"
                model_text += f"  ref a.b.c.d\n"
                model_text += f"  ref b.b.b.b\n"
                model_text += f"  ref d.d.d.d\n"
                model_text += f"  }}\n"
                model_text += f"}}\n"
        return model_text

start = time.time()
m = mm.model_from_str(create_model(100));
end = time.time()
print(f"n=100: {end-start}")
assert m.classes[10].name=='A10'
assert m.classes[10].refs[0].ref.clazz.name=='A9'

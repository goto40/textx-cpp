#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"

TEST_CASE("parse_item_lang_grammar", "[textx/lang/item_lang_example]")
{
    {
        auto grammar1 = R"###(
            Model:
                imports*=Import
                (packages+=ExtPackage | package=Package);

            NestedPackage: '.' name=ID (package=NestedPackage|
                ('(' ('property_set' property_set=[PropertySet|FQN])?
                    ('.' 'description' '=' description=STRING)?
                ')')?
                (property_sets+=PropertySet|items+=Type|constants+=Constants)+);
            Package: 'package' name=ID (package=NestedPackage|
                ('(' ('property_set' property_set=[PropertySet|FQN])?
                    ('.' 'description' '=' description=STRING)?
                ')')?
                (property_sets+=PropertySet|items+=Type|constants+=Constants)+);
            ExtPackage: 'package' name=ID
                ('(' ('property_set' property_set=[PropertySet|FQN])?
                    ('.' 'description' '=' description=STRING)?
                ')')?
                '{'
                    (packages+=ExtPackage|propertydefinitions+=PropertyDefinition|items+=Type|constants+=Constants)+
                '}';

            AnyPackage: NestedPackage|Package|ExtPackage;

            Struct: 'struct' name=ID ( '(' properties*=Property[','] ')' )?
            '{'
                constant_entries*=Constant
                attributes*=Attribute
            '}';
            RawType: 'rawtype' name=ID internaltype=InternalType bits=INT;
            Enum: 'enum' name=ID ':' type=[RawType|FQN] ('(' '.' 'description' '=' description=STRING ')')?
            '{'
                enum_entries+=EnumEntry
            '}';
            Type: RawType|Struct|Enum;

            Attribute: ScalarAttribute|ArrayAttribute|VariantAttribute;
            VariantAttribute: (if_attr=IfAttribute)? 'variant' name=ID ':' variant_selector=AttrRef '->' '{' // TODO variant_selector --> only scalarattr allowed!
            mappings+=VariantMapping
            '}' ( '(' properties*=Property[','] ')' )?;
            ScalarAttribute: (if_attr=IfAttribute)? embedded?='embedded' 'scalar' name=ID ':' type=[Type|FQN]
                ( '(' properties*=Property[','] ')' )?;
            ArrayAttribute: (if_attr=IfAttribute)? embedded?='embedded' 'array' name=ID ':' type=[Type|FQN] dims+=Dim
                ( '(' properties*=Property[','] ')' )?;
            IfAttribute: 'if' predicate=Predicate;

            FormulaElement: ScalarAttribute|Constant|EnumEntry;
            EnumEntry: 'value' name=ID '=' value=Formula ( '(' properties*=Property[','] ')' )?;
            Dim: '[' dim=Formula ']';
            AttrRef: ref=
                [   FormulaElement|FQN|+mp:
                        parent(Attribute).(..).attributes.(~type.attributes)*,
                        parent(Attribute).~type.enum_entries,
                        parent(Constant).~type.enum_entries,
                        parent(Constants).constant_entries,
                        parent(Struct).constant_entries,
                        parent(Attribute).~variant_selector.~ref.~type.enum_entries,
                        ^(package,packages)*.constants.constant_entries,
                        ^(package,packages)*.items.constant_entries,
                        ^(package,packages)*.items.enum_entries
                ];
            VariantMapping: id=Formula ('=' the_name=ID)? ':' type=[Struct|FQN] ( '(' properties*=Property[','] ')' )?;

            Import: 'import' importURI=STRING;

            Predicate: '(' Predicate_Or ')';
            Predicate_Or: parts+=Predicate_And['or'];
            Predicate_And: parts+=Predicate_Cmp['and'];
            Predicate_Cmp: part0=Sum other_parts+=Predicate_CmpPart;
            Predicate_CmpPart: cmp_op=CmpOp part=Sum;
            CmpOp: '=='|'!='|'>'|'<'|'>='|'<=';

            Formula: Sum;
            Sum: parts+=Dif['+'];
            Dif: parts+=Mul['-'];
            Mul: parts+=Div['*'];
            Div: parts+=Val['/'];
            Val:
                (valueClassificator=ValueClassificator)?
                (
                    value=ExtNumber
                    | ref=AttrRef
                    | "(" sum=Sum ")"
                );

            ValueClassificator: 'CONST'|'ENUM';

            PropertySet: 'property_set' name=ID ('extends' extends=[PropertySet|FQN])?
            '{'
                property_definitions+=PropertyDefinition
            '}';

            // Note: times_per_message is only considered for scalar attributes
            PropertyDefinition: 'property'
                optional?='optional'
                (('applicable' 'for'|'applicable_for') applicable_for+=ApplicableFor[','])?
                name=ID ':'
                internaltype=InternalType
                ('(' '.' 'description' '=' description=STRING ')')?
                ('{' (numberOfPropRestriction=NumberPropertiesPerStructDefRestriction)? '}')?;

            NumberPropertiesPerStructDefRestriction: min=INT 'to' max=INT ('times_per_message'|'times' 'per' 'message');
            // Note: "array" is used to disallow "scalars"
            // Note: "scalar" is used to disallow "arrays"
            ApplicableFor: ApplicableForRawType|'array'|'scalar'|'variant'|'struct_definition'|'enum_value'|'enum'|'struct';
            ApplicableForRawType: 'rawtype' ('(' concrete_types+=[RawType|FQN][','] ')')?;
            Property: '.' definition=[
                PropertyDefinition|ID|+mp:
                    ^(~package,~packages)*.~property_set.~extends*.property_definitions,
                    'built_in'~package.'default_properties'~property_sets.property_definitions
                ] '=' (
                    textValue=TextValue |
                    numberValue=NumberValue
                );

            Constants: 'constants' name=ID ('(' '.' 'description' '=' description=STRING ')')?
            '{'
                constant_entries+=Constant
            '}';
            Constant: 'constant' name=ID ':' type=[RawType|FQN] '=' value=Formula ('(' '.' 'description' '=' description=STRING ')')?;

            TextValue: x=STRING;
            NumberValue: x=Formula;

            FQN: ID('.'ID)*;
            Comment: /\/\/.*?$/;
            InternalType: 'INT'|'UINT'|'FLOAT'|'STRING'|'BOOL'|'ATTRTYPE'|'ENUM';
            ExtNumber: BoolNumber|NUMBER|HexNumber;
            HexNumber: /0x[0-9a-fA-F]+/;
            BoolNumber: 'true'|'false';
        )###";

        auto mm = textx::metamodel_from_str(grammar1);
        
        // BENCHMARK("parse item grammar speed test") {
        //     return textx_grammar.parse_or_throw(grammar1); // The return is a handy way to avoid the compiler optimizing away the benchmark code. https://github.com/catchorg/Catch2/blob/devel/docs/benchmarks.md
        // };
    }
}

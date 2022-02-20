#include "catch.hpp"
#include "textx/istrings.h"

namespace {
    auto get_example_model() {
        auto mm = textx::metamodel_from_str(R"#(
            Model: 'shapes' info=STRING ':' shapes+=Shape[','];
            Shape: Point|ComplexShape;
            ComplexShape: Circle|Line;
            Point: type_name="Point" '(' x=NUMBER ',' y=NUMBER ')';
            Circle: type_name="Circle" '(' center=Point ',' r=NUMBER ')';
            Line: type_name='Line' '(' p1=Point ',' p2=Point ')';
        )#");
        auto m = mm->model_from_str(R"(
            shapes "My Shapes":
            Point(1,2),
            Circle(Point(333,4.5),9),
            Line(Point(0,0),Point(1,1))
        )");
        return m;
    }
}

TEST_CASE("istrings_metamodel0", "[textx/istrings]")
{
    auto model_text = R"(
        Hello World
        123
    )";
    auto m = textx::istrings::get_istrings_metamodel_workspace()->model_from_str(model_text);
    CHECK( (*m)["parts"].size()==0 );
    std::ostringstream s;
    for (auto &t: (*m)["text"]) {
        s << t["text"].str();
    }
    CHECK( s.str() == model_text );
    //std::cout << m->val() << "\n";
}

TEST_CASE("istrings_metamodel1", "[textx/istrings]")
{
    auto model_text = R"(
        Hello World {% model.x %}
        123
    )";
    auto mm = textx::istrings::get_istrings_metamodel_workspace();
    mm->get_metamodel_by_shortcut("ISTRINGS")->clear_builtin_models();
    mm->get_metamodel_by_shortcut("ISTRINGS")->add_builtin_model(
        mm->model_from_str("EXTERNAL_LINKAGE","object model")
    );
    auto m = mm->model_from_str(model_text);
    std::ostringstream s;
    for (auto &p: (*m)["parts"]) {
        for (auto &t: p["text"]) {
            s << t["text"].str();
        }
    }
    for (auto &t: (*m)["text"]) {
        s << t["text"].str();
    }
    CHECK( s.str().find("Hello World")>0 );
    CHECK( s.str().find("123")>0 );
    //std::cout << m->val() << "\n";
}

TEST_CASE("istrings_metamodel2_str", "[textx/istrings]")
{
    auto model = get_example_model();
    auto res = textx::istrings::i(
        R"(info="{% model.info %}")",
        { {"model", model->val().obj()} }
    );

    CHECK( res.size()>0 );
    CHECK( res == "info=\"My Shapes\"");

    // res = textx::istrings::i(
    //     R"(info='{% model.info %}\n123')",
    //     { {"model", model->val().obj()} }
    // );

    // CHECK( res.size()>0 );
    // CHECK( res == "info='My"Shapes"\n123");

}

TEST_CASE("istrings_metamodel3_forloop", "[textx/istrings]")
{
    auto model = get_example_model();
    auto res = textx::istrings::i(
        R"(
        info="{% model.info %}"
        -0------
        {% FOR o: model.shapes %}
        inner_type: {% o.type_name %}
        {% ENDFOR %}
        -------
        )",
        { {"model", model->val().obj()} }
    );

    CHECK( res.size()>0 );
    //std::cout << res << "\n";
    CHECK( res ==
R"(
info="My Shapes"
-0------
inner_type: Point
inner_type: Circle
inner_type: Line
-------
)");
}

TEST_CASE("istrings_metamodel3_forloop_indent_removed", "[textx/istrings]")
{
    auto model = get_example_model();
    auto res = textx::istrings::i(
        R"(
        info="{% model.info %}"
        -1------
        {% FOR o: model.shapes %}
            inner_type: {% o.type_name %}
        {% ENDFOR %}
        -------
        )",
        { {"model", model->val().obj()} }
    );

    CHECK( res.size()>0 );
    //std::cout << res << "\n";
    CHECK( res ==
R"(
info="My Shapes"
-1------
inner_type: Point
inner_type: Circle
inner_type: Line
-------
)");
}

TEST_CASE("istrings_metamodel3_forloop_indent_active", "[textx/istrings]")
{
    auto model = get_example_model();
    auto res = textx::istrings::i(
        R"(
        info="{% model.info %}"
        -2------
            {% FOR o: model.shapes %}
            inner_type: {% o.type_name %}
            {% ENDFOR %}
        -------
        )",
        { {"model", model->val().obj()} }
    );

    CHECK( res.size()>0 );
    //std::cout << res << "\n";
    CHECK( res ==
R"(
info="My Shapes"
-2------
    inner_type: Point
    inner_type: Circle
    inner_type: Line
-------
)");
}

TEST_CASE("istrings_metamodel3_forloop_inline", "[textx/istrings]")
{
    auto model = get_example_model();
    auto res = textx::istrings::i(
        R"(
        info="{% model.info %}"
        -------
            inline:{% FOR o: model.shapes %} {% o.type_name %}{% ENDFOR %}
        -------
        )",
        { {"model", model->val().obj()} }
    );

    CHECK( res.size()>0 );
    //std::cout << res << "\n";
    CHECK( res ==
R"(
info="My Shapes"
-------
    inline: Point Circle Line
-------
)");
}

TEST_CASE("istrings_metamodel4_functions", "[textx/istrings]")
{
    auto model = get_example_model();
    auto res = textx::istrings::i(
        R"(
        info="{% model.info %}"
        -------
            {% FOR o: model.shapes %}
            {% print(o) %}
            {% ENDFOR %}
        -------
        )",
        {
            {"model", model->val().obj()},
            {"print", [](std::shared_ptr<textx::object::Object> o) -> std::string {
                std::ostringstream s;
                o->print(s);
                return s.str();
            } }
        }
    );

    //std::cout << res << "\n";
    CHECK( res ==
R"(
info="My Shapes"
-------
    Point{
      type_name=
        "Point"
      x=
        1
      y=
        2
    }
    Circle{
      type_name=
        "Circle"
      center=
        Point{
          type_name=
            "Point"
          x=
            333
          y=
            4.5
        }
      r=
        9
    }
    Line{
      type_name=
        "Line"
      p1=
        Point{
          type_name=
            "Point"
          x=
            0
          y=
            0
        }
      p2=
        Point{
          type_name=
            "Point"
          x=
            1
          y=
            1
        }
    }
-------
)");
}

TEST_CASE("istrings_metamodel4_functions_plus_newline", "[textx/istrings]")
{
    auto model = get_example_model();
    auto res = textx::istrings::i(
        R"(
        info="{% model.info %}"
        -------
            {% FOR o: model.shapes %}
                {% print(o) %}

                  x

            {% ENDFOR %}
        -------
        )",
        {
            {"model", model->val().obj()},
            {"print", [](std::shared_ptr<textx::object::Object> o) -> std::string {
                std::ostringstream s;
                o->print(s);
                return s.str();
            } }
        }
    );

    //std::cout << res << "\n";
    CHECK( res ==
R"(
info="My Shapes"
-------
    Point{
      type_name=
        "Point"
      x=
        1
      y=
        2
    }

      x

    Circle{
      type_name=
        "Circle"
      center=
        Point{
          type_name=
            "Point"
          x=
            333
          y=
            4.5
        }
      r=
        9
    }

      x

    Line{
      type_name=
        "Line"
      p1=
        Point{
          type_name=
            "Point"
          x=
            0
          y=
            0
        }
      p2=
        Point{
          type_name=
            "Point"
          x=
            1
          y=
            1
        }
    }

      x

-------
)");
}

TEST_CASE("istrings_array_access", "[textx/istrings]")
{
    auto model = get_example_model();
    auto res = textx::istrings::i(
        R"(
        shape[1]="{% model.shapes[1].type_name %}"
        {% print(model.shapes[1]) %}
        )",
        {
            {"model", model->val().obj()},
            {"print", [](std::shared_ptr<textx::object::Object> o) -> std::string {
                std::ostringstream s;
                o->print(s);
                return s.str();
            } }
        }
    );
    //std::cout << res;
    CHECK( res == R"#(
shape[1]="Circle"
Circle{
  type_name=
    "Circle"
  center=
    Point{
      type_name=
        "Point"
      x=
        333
      y=
        4.5
    }
  r=
    9
}
)#");
}
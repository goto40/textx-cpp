#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/arpeggio.h"
#include "textx/grammar.h"

template<class F, class C>
auto test_parse(F f, const C& config, std::string_view s, textx::arpeggio::TextPosition pos={}) {
    textx::arpeggio::ParserState ps{s, "test-filename.txt"};
    return f(config, ps, pos);
}

TEST_CASE("str_match", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{.skip_text = skip_text_functions::nothing()};

    ParserState text = {"hello world", "test-filename.txt"};
    auto hello_pattern = str_match("hello");
    auto world_pattern = str_match("world");
    CHECK(hello_pattern(config, text, {}));
    CHECK(!world_pattern(config, text, {}));
    auto match = hello_pattern(config, text, {}).value();
    auto match2_opt = world_pattern(config, text, match.end().add(text,1));
    REQUIRE(match2_opt);

    CHECK(get_str(text, match) == "hello");
    CHECK(get_str(text, match2_opt.value()) == "world");
}

TEST_CASE("named", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{.skip_text = skip_text_functions::nothing()};

    ParserState text = {"hello world", "test-filename.txt"};
    auto hello_pattern = str_match("hello");
    auto named_hello_pattern = named("hello", str_match("hello"));
    auto match = hello_pattern(config, text, {}).value();
    auto named_match = named_hello_pattern(config, text, {}).value();
    {
        std::ostringstream o;
        o << match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match>"));
    }
    {
        std::ostringstream o;
        o << named_match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match:hello>"));
    }
}

TEST_CASE("captured", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{.skip_text = skip_text_functions::nothing()};

    ParserState text = {"hello world", "test-filename.txt"};
    auto hello_pattern = capture(str_match("hello"));
    auto named_hello_pattern = capture(named("hello", str_match("hello")));
    auto match = hello_pattern(config, text, {}).value();
    auto named_match = named_hello_pattern(config, text, {}).value();
    {
        std::ostringstream o;
        o << match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match captured=hello>"));
    }
    {
        std::ostringstream o;
        o << named_match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match:hello captured=hello>"));
    }
}

TEST_CASE("regex_match", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{.skip_text = skip_text_functions::nothing()};
    Config config_skipws{.skip_text = skip_text_functions::skipws()};

    ParserState text = {"hello123 world", "test-filename.txt"};
    auto word_pattern = capture(regex_match(R"(\w+)"));
    {
        CHECK(!test_parse(word_pattern, config, " space and a word", {}));
        CHECK(test_parse(word_pattern, config_skipws, " space and a word", {}));
        CHECK(test_parse(word_pattern, config, " space and a word", {1,1,1}));
        CHECK(test_parse(word_pattern, config, "\nspace and a word", {1,1,1}));

        auto match = word_pattern(config, text, {}).value();
        std::ostringstream o;
        o << match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<regex_match captured=hello123>"));
        CHECK(match.start().pos == 0);
        CHECK(match.end().pos == 0+8);
    }
}

TEST_CASE("regex_match2", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    auto words_pattern = capture(regex_match(R"(\w+\s+\w+)"));
    {
        CHECK(!test_parse(words_pattern, config, " a ", {}));
        CHECK(test_parse(words_pattern, config, " a b ", {}));
        CHECK(test_parse(words_pattern, config, " a\nb ", {}));
    }
}

TEST_CASE("sequence", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    ParserState text = {"hello123 world", "test-filename.txt"};
    auto two_word_pattern = sequence({capture(regex_match(R"(\w+)")),
                                      capture(regex_match(R"(\w+)"))});
    {
        auto match = two_word_pattern(config, text, {});
        CHECK(match);

        std::ostringstream o;
        o << match.value();
        // std::cout << o.str() << "\n";
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<regex_match captured=hello123>"));
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<regex_match captured=world>"));
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<sequence>"));

        CHECK(match.value().start().pos == 0);
        CHECK(match.value().start().col == 1);
        CHECK(match.value().start().line == 1);
        CHECK(match.value().end().pos > 0);
    }
}

TEST_CASE("ordered_choice", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    ParserState text = {"hello123 world", "test-filename.txt"};
    auto choice_pattern = ordered_choice({capture(str_match("hello")),
                                          capture(str_match("hello123")),
                                          capture(str_match("world"))});
    {
        auto match = choice_pattern(config, text, {});
        CHECK(match);

        std::ostringstream o;
        o << match.value();
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match captured=hello>"));
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<ordered_choice>"));
    }
    {
        auto match = choice_pattern(config, text, {9,1,1});
        CHECK(match);

        std::ostringstream o;
        o << match.value();
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match captured=world>"));
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<ordered_choice>"));

        CHECK(match.value().start().pos == 9);
        CHECK(match.value().end().pos > 9);
    }
}

TEST_CASE("one_or_more", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    ParserState text = {"a b c d e f g", "test-filename.txt"};
    auto words_pattern = one_or_more(regex_match(R"(\w+)"));
    {
        auto match = words_pattern(config, text, {});
        REQUIRE(match);
        CHECK(match.value().type() == MatchType::one_or_more);
        CHECK(match.value().children.size() == 7);

        CHECK(match.value().start().pos == 0);
        CHECK(match.value().start().col == 1);
        CHECK(match.value().start().line == 1);
        CHECK(match.value().end().pos > 0);
    }
}

TEST_CASE("zero_or_more", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    {
        ParserState text = {"a b c d e f g", "test-filename.txt"};
        auto words_pattern = zero_or_more(regex_match(R"(\w+)"));
        {
            auto match = words_pattern(config, text, {});
            REQUIRE(match);
            CHECK(match.value().type() == MatchType::zero_or_more);
            CHECK(match.value().children.size() == 7);
 
            CHECK(match.value().start().pos == 0);
            CHECK(match.value().start().col == 1);
            CHECK(match.value().start().line == 1);
            CHECK(match.value().end().pos > 0);
        }
    }
    {
        ParserState text = {"","test-filename.txt"};
        auto words_pattern = zero_or_more(regex_match(R"(\w+)"));
        {
            auto match = words_pattern(config, text, {});
            REQUIRE(match);
            CHECK(match.value().type() == MatchType::zero_or_more);
            CHECK(match.value().children.size() == 0);
        }
    }
}

TEST_CASE("optional", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    {
        ParserState text = {"hello", "test-filename.txt"};
        auto p1 = optional(str_match("hello"));
        auto p2 = optional(str_match("world"));
        {
            auto match = p1(config, text, {});
            CHECK(match);

            CHECK(match.value().start().pos == 0);
            CHECK(match.value().start().col == 1);
            CHECK(match.value().start().line == 1);
            CHECK(match.value().end().pos > 0);
            CHECK(match.value().children.size() == 1);
        }
        {
            auto match = p2(config, text, {});
            CHECK(match);

            CHECK(match.value().start().pos == 0);
            CHECK(match.value().start().col == 1);
            CHECK(match.value().start().line == 1);
            CHECK(match.value().end().pos == 0);
            CHECK(match.value().end().col == 1);
            CHECK(match.value().end().line == 1);
            CHECK(match.value().children.size() == 0);
        }
    }
}

TEST_CASE("positive_lookahead", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    {
        auto p = capture(sequence({str_match("A"), positive_lookahead(capture(str_match("B")))}));
        {
            auto match = test_parse(p, config, "AB", {});
            REQUIRE(match);
            CHECK(match.value().start().pos == 0);
            CHECK(match.value().end().pos == 1);

            std::ostringstream o;
            o << match.value();
            CHECK_THAT(o.str(), Catch::Matchers::Contains("<sequence captured=A>"));
            CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match captured=B>"));
        }
        {
            auto match = test_parse(p, config, "A", {});
            CHECK(!match);
        }
    }
}

TEST_CASE("negative_lookahead", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    {
        auto p = capture(sequence({negative_lookahead(str_match("keyword")),regex_match(R"(\w+)")}));
        {
            auto match = test_parse(p, config, "keyword", {});
            REQUIRE(!match);
        }
        {
            auto match = test_parse(p, config, "nokeyword", {});
            REQUIRE(match);

            std::ostringstream o;
            o << match.value();
            CHECK_THAT(o.str(), Catch::Matchers::Contains("<sequence captured=nokeyword>"));
        }
    }
}

TEST_CASE("end_of_file", "[arpeggio]")
{
    using namespace textx::arpeggio;
    auto grammar = textx::Grammar<textx::arpeggio::Pattern>{capture(sequence({
        zero_or_more(ordered_choice({
            str_match("A"),
            str_match("B"), 
        })),
        str_match("C"),
        end_of_file(),
    }))};

    CHECK(grammar.parse("ABBABC"));
    CHECK(grammar.parse("C"));
    CHECK(grammar.parse("C "));
    CHECK(grammar.parse("AB BAB C   "));
    CHECK(grammar.parse("AB \n BAB\nC   "));
    CHECK_THROWS(grammar.get_last_error_position()); // no error --> execption

    auto partly = grammar.parse("AB");
    CHECK(!partly);
    {
        auto err_text = grammar.get_last_error_string();
        CHECK_THAT(err_text, Catch::Matchers::Contains("expected"));
        CHECK_THAT(err_text, Catch::Matchers::Contains("str_match,A"));
        CHECK_THAT(err_text, Catch::Matchers::Contains("str_match,B"));
        CHECK_THAT(err_text, Catch::Matchers::Contains("str_match,C"));
    }
    CHECK(!partly.ok());
    // partly.err().match->print(std::cout);
    // for(auto e: partly.err().errors) {
    //     std::cout << e.error << "@" << e.pos.pos << ":\n";
    // }
    CHECK(partly->type()==MatchType::sequence);
    CHECK(partly->children.size()>0);
    CHECK(partly->children[0].type()==MatchType::zero_or_more);
    CHECK(partly->children[0].children.size()==2);
    
    TODO get state and check completion info

    CHECK(!grammar.parse("C C"));
    {
        auto err_text = grammar.get_last_error_string();
        CHECK_THAT(err_text, Catch::Matchers::Contains("end_of_file,"));
    }
}

TEST_CASE("grammar_no_main_rule", "[arpeggio]")
{
    using namespace textx;
    auto grammar = Grammar<textx::arpeggio::Pattern>{};
    CHECK_THROWS(grammar.parse(""));
}

TEST_CASE("comments", "[arpeggio]")
{
    using namespace textx::arpeggio;
    auto grammar = textx::Grammar<textx::arpeggio::Pattern>{capture(sequence({
        zero_or_more(ordered_choice({
            str_match("A"),
            str_match("B"), 
        })),
        str_match("C"),
        end_of_file(),
    }))};
    grammar.get_config().skip_text = textx::arpeggio::skip_text_functions::skip_cpp_style();

    CHECK(grammar.parse("A BB ABC "));
    CHECK(grammar.parse("A BB ABC // comment!"));
    CHECK(grammar.parse("A BB AB // comment!\n C"));
    CHECK(grammar.parse("A BB AB // comment!\n ABC"));
    CHECK(grammar.parse("A BB ABC /* comment! */"));
    CHECK(grammar.parse("A BB AB /* comment! */ C"));
    CHECK(grammar.parse("C"));
    CHECK(grammar.parse("C "));
    CHECK(grammar.parse("ABB\nAB// comment!\n C"));
    CHECK(grammar.parse("A\nBBAB\n // comment!\n C"));
    CHECK(grammar.parse("ABBAB \n// comment!\n C"));
    CHECK(grammar.parse("ABBAB\n// comment!\n C"));
}

TEST_CASE("unordered_group", "[arpeggio]")
{
    using namespace textx::arpeggio;
    auto grammar = textx::Grammar<textx::arpeggio::Pattern>{sequence({
        unordered_group({        
            capture(str_match("A")),
            capture(str_match("B")),
            capture(str_match("C")),
        }),
        end_of_file()
    })};
    grammar.get_config().skip_text = textx::arpeggio::skip_text_functions::skip_cpp_style();

    CHECK(grammar.parse_or_throw("ABC"));
    CHECK(grammar.parse_or_throw("CAB"));
    CHECK(grammar.parse_or_throw("BCA"));
    CHECK(grammar.parse_or_throw("BCA"));
    CHECK(grammar.parse_or_throw("BAC"));
    CHECK(!grammar.parse("BAA"));
    CHECK(!grammar.parse("BACA"));

    auto match = grammar.parse("CAB");
    REQUIRE(match);
    CHECK(match.value().children[0].children[0].captured.value() == "C");
    CHECK(match.value().children[0].children[1].captured.value() == "A");
    CHECK(match.value().children[0].children[2].captured.value() == "B");

}

TEST_CASE("unordered_group_optional1", "[arpeggio]")
{
    using namespace textx::arpeggio;
    auto grammar = textx::Grammar<textx::arpeggio::Pattern>{sequence({
        unordered_group({        
            optional(capture(str_match("A"))),
            optional(capture(str_match("B"))),
        }),
        end_of_file()
    })};
    grammar.get_config().skip_text = textx::arpeggio::skip_text_functions::skip_cpp_style();

    CHECK(grammar.parse_or_throw("AB"));
    CHECK(grammar.parse_or_throw("BA"));
    CHECK(grammar.parse_or_throw("A"));
    CHECK(grammar.parse_or_throw("B"));
    CHECK(!grammar.parse("BAA")); // more than the allowed elements --> error
    CHECK(grammar.parse("")); // all optionals unmatched --> ok

    auto match = grammar.parse_or_throw("B");
    CHECK(match.value().children[0].children.size()==1);
    CHECK(match.value().children[0].children[0].children[0].captured.value() == "B");
}

TEST_CASE("unordered_group_optional2", "[arpeggio]")
{
    using namespace textx::arpeggio;
    auto grammar = textx::Grammar<textx::arpeggio::Pattern>{sequence({
        unordered_group({        
            optional(capture(str_match("A"))),
            optional(capture(str_match("B"))),
            capture(str_match("C")),
        }),
        end_of_file()
    })};
    grammar.get_config().skip_text = textx::arpeggio::skip_text_functions::skip_cpp_style();

    CHECK(grammar.parse_or_throw("CAB"));
    CHECK(grammar.parse_or_throw("BCA"));
    CHECK(grammar.parse_or_throw("AC"));
    CHECK(grammar.parse_or_throw("CB"));
    CHECK(grammar.parse_or_throw("C"));
    CHECK(!grammar.parse("")); // all optionals unmatched --> error

    auto match = grammar.parse_or_throw("BC");
    CHECK(match.value().children[0].children.size()==2);
    CHECK(match.value().children[0].children[0].children[0].captured.value() == "B"); // optional B found
    CHECK(match.value().children[0].children[1].captured.value() == "C"); // req. C found
}

TEST_CASE("pattern_type", "[arpeggio]")
{
    using namespace textx::arpeggio;

    CHECK(regex_match(".*").type() == MatchType::regex_match);
    CHECK(one_or_more({str_match("x")}).type() == MatchType::one_or_more);
    CHECK(zero_or_more({str_match("x")}).type() == MatchType::zero_or_more);
    CHECK(unordered_group({str_match("x"),str_match("y")}).type() == MatchType::unordered_group);

    CHECK(rule(regex_match(".*")).type() == MatchType::regex_match);
    CHECK(named("x", one_or_more({str_match("x")})).type() == MatchType::one_or_more);
    CHECK(capture(named("x", one_or_more({str_match("x")}))).type() == MatchType::one_or_more);
}

TEST_CASE("details_get_is_optional", "[arpeggio]")
{
    using namespace textx::arpeggio;

    std::vector<Pattern> v={
        capture(optional(str_match("A"))),
        optional(capture(str_match("B"))),
        capture(str_match("C")),
    };

    CHECK(v[0].type() == MatchType::optional);
    CHECK(v[1].type() == MatchType::optional);
    CHECK(v[2].type() == MatchType::str_match);

    auto is_opt = textx::arpeggio::details::get_is_optional(v);
    CHECK(is_opt[0]);
    CHECK(is_opt[1]);
    CHECK(!is_opt[2]);
}

TEST_CASE("match_search", "[arpeggio]")
{
    using namespace textx::arpeggio;
    auto grammar = textx::Grammar<textx::arpeggio::Pattern>{sequence({
        unordered_group({        
            optional(capture(str_match("A"))),
            optional(capture(str_match("B"))),
            named("TEST123", capture(str_match("C"))),
        }),
        end_of_file()
    })};
    auto match = grammar.parse_or_throw("BC");
    CHECK(nullptr == match.value().search("UNKNOWN"));
    auto test123 = match.value().search("TEST123");
    CHECK(test123 != nullptr);
    CHECK(test123->captured.value() == "C");
}
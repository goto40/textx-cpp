#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/arpeggio.h"

TEST_CASE("str_match", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{.skip_text = skip_text_functions::nothing()};

    std::string text = "hello world";
    auto hello_pattern = str_match("hello");
    auto world_pattern = str_match("world");
    CHECK(hello_pattern(config, text, 0));
    CHECK(!world_pattern(config, text, 0));
    auto match = hello_pattern(config, text, 0).value();
    auto match2_opt = world_pattern(config, text, match.end + 1);
    REQUIRE(match2_opt);

    CHECK(get_str(text, match) == "hello");
    CHECK(get_str(text, match2_opt.value()) == "world");
}

TEST_CASE("named", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{.skip_text = skip_text_functions::nothing()};

    std::string text = "hello world";
    auto hello_pattern = str_match("hello");
    auto named_hello_pattern = named("hello", str_match("hello"));
    auto match = hello_pattern(config, text, 0).value();
    auto named_match = named_hello_pattern(config, text, 0).value();
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

    std::string text = "hello world";
    auto hello_pattern = capture(str_match("hello"));
    auto named_hello_pattern = capture(named("hello", str_match("hello")));
    auto match = hello_pattern(config, text, 0).value();
    auto named_match = named_hello_pattern(config, text, 0).value();
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

    std::string text = "hello123 world";
    auto word_pattern = capture(regex_match(R"(\w+)"));
    {
        CHECK(!word_pattern(config, " space and a word", 0));
        CHECK(word_pattern(config_skipws, " space and a word", 0));
        CHECK(word_pattern(config, " space and a word", 1));

        auto match = word_pattern(config, text, 0).value();
        std::ostringstream o;
        o << match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<regex_match captured=hello123>"));
    }
}

TEST_CASE("sequence", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    std::string text = "hello123 world";
    auto two_word_pattern = sequence({capture(regex_match(R"(\w+)")),
                                      capture(regex_match(R"(\w+)"))});
    {
        auto match = two_word_pattern(config, text, 0);
        CHECK(match);

        std::ostringstream o;
        o << match.value();
        // std::cout << o.str() << "\n";
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<regex_match captured=hello123>"));
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<regex_match captured=world>"));
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<sequence>"));
    }
}

TEST_CASE("ordered_choice", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    std::string text = "hello123 world";
    auto choice_pattern = ordered_choice({capture(str_match("hello")),
                                          capture(str_match("hello123")),
                                          capture(str_match("world"))});
    {
        auto match = choice_pattern(config, text, 0);
        CHECK(match);

        std::ostringstream o;
        o << match.value();
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match captured=hello>"));
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<ordered_choice>"));
    }
    {
        auto match = choice_pattern(config, text, 9);
        CHECK(match);

        std::ostringstream o;
        o << match.value();
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match captured=world>"));
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<ordered_choice>"));
    }
}

TEST_CASE("one_or_more", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    std::string text = "a b c d e f g";
    auto words_pattern = one_or_more(regex_match(R"(\w+)"));
    {
        auto match = words_pattern(config, text, 0);
        REQUIRE(match);
        CHECK(match.value().type == MatchType::one_or_more);
        CHECK(match.value().children.size() == 7);
    }
}

TEST_CASE("zero_or_more", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    {
        std::string text = "a b c d e f g";
        auto words_pattern = zero_or_more(regex_match(R"(\w+)"));
        {
            auto match = words_pattern(config, text, 0);
            REQUIRE(match);
            CHECK(match.value().type == MatchType::zero_or_more);
            CHECK(match.value().children.size() == 7);
        }
    }
    {
        std::string text = "";
        auto words_pattern = zero_or_more(regex_match(R"(\w+)"));
        {
            auto match = words_pattern(config, text, 0);
            REQUIRE(match);
            CHECK(match.value().type == MatchType::zero_or_more);
            CHECK(match.value().children.size() == 0);
        }
    }
}

TEST_CASE("optional", "[arpeggio]")
{
    using namespace textx::arpeggio;
    Config config{};

    {
        std::string text = "hello";
        auto p1 = optional(str_match("hello"));
        auto p2 = optional(str_match("world"));
        {
            auto match = p1(config, text, 0);
            CHECK(match);
        }
        {
            auto match = p2(config, text, 0);
            CHECK(match);
        }
    }
}

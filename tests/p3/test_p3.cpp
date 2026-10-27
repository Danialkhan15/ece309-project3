// tests/p3/test_p3.cpp — YOUR P3 TEST SUITE.
//
// The five tests below are worked examples. Keep them, and add your own
// until you have AT LEAST 15 (spec §6 lists what must be covered).
// Name every test <area>_<what>: reg_ for the registry, ws_ for the
// workspace, calc_ for the calculator.
//
// Build & run:   cmake --build build && ./build/test_p3
// One area:      ./build/test_p3 reg_        (names starting with "reg_")
// One test:      ./build/test_p3 =reg_add_and_find

#include "../mini_test.h"
#include "../test_helpers.h"     // TempDir, write_text, args1, ...

#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// A tool whose name you choose — use it to fill the registry in tests.
class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue   schema_;   // an empty JSON object: {}
};

// Tokenize with the provided tokenizer, then run YOUR calc_evaluate.
bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) return false;
    return calc_evaluate(tokens, value, error);
}

}  // namespace

// ---------------------------------------------------------------- examples

TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();                        // remember the address before moving
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);              // same object, not a copy
    CHECK(r.find("beta") == nullptr);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(ws_rejects_parent_escape) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));   // sanity: accepts *something*
    CHECK(out == ws.root() / "notes" / "todo.txt");
    CHECK(!ws.resolve("../etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

TEST(calc_precedence) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 + 3 * 4", &v, &err));
    CHECK(v == 14);
}

TEST(calc_division_by_zero) {
    {
        double v = 0;
        std::string err;
        CHECK(!eval("1 / 0", &v, &err));
        CHECK(err == "division by zero");
    }
    {
        double value = 123;
        std::string error;
        for (const auto& expr : {"1 / (2 - 2)", "7 / -0", "10 / (0 * 3)"}) {
            CHECK(!eval(expr, &value, &error));
            CHECK(error == "division by zero");
            CHECK(value == 123);
        }
    }
}

TEST(reg_empty_and_null_rejected) {
    {
        ToolRegistry r;
        CHECK(r.size() == 0);
        CHECK(r.bucket_count() >= 1);
        CHECK(r.bucket_count() <= 64);
        CHECK(r.load_factor() == 0.0);
        CHECK(r.max_probe_length() == 0);
        CHECK(r.find("absent") == nullptr);
        int visits = 0;
        r.for_each([&](const Tool&) { ++visits; });
        CHECK(visits == 0);
    }
    {
        ToolRegistry r;
        CHECK(!r.add(nullptr));
        CHECK(r.size() == 0);
        REQUIRE(r.add(std::make_unique<NamedTool>("alpha")));
        CHECK(r.size() == 1);
        CHECK(!r.add(nullptr));
        CHECK(r.size() == 1);
        CHECK(r.find("missing") == nullptr);
        CHECK(r.size() == 1);
    }
}

TEST(reg_owns_and_destroys_tools_once) {
    class CountedTool : public NamedTool {
    public:
        CountedTool(std::string name, int* destroyed)
            : NamedTool(std::move(name)), destroyed_(destroyed) {}
        ~CountedTool() override { ++*destroyed_; }
    private:
        int* destroyed_;
    };
    int destroyed = 0;
    {
        ToolRegistry r;
        auto first = std::make_unique<CountedTool>("alpha", &destroyed);
        Tool* original = first.get();
        REQUIRE(r.add(std::move(first)));
        CHECK(!r.add(std::make_unique<CountedTool>("alpha", &destroyed)));
        CHECK(destroyed == 1);
        CHECK(r.find("alpha") == original);
        CHECK(r.size() == 1);
        for (int i = 0; i < 200; ++i) {
            REQUIRE(r.add(std::make_unique<CountedTool>(std::to_string(i), &destroyed)));
        }
        CHECK(destroyed == 1);
        CHECK(r.find("alpha") == original);
        CHECK(r.size() == 201);
    }
    CHECK(destroyed == 202);
}

TEST(reg_for_each_visits_each_once) {
    ToolRegistry r;
    std::map<std::string, Tool*> expected;
    for (int i = 0; i < 100; ++i) {
        auto tool = std::make_unique<NamedTool>("visit_" + std::to_string(i));
        expected[tool->name()] = tool.get();
        REQUIRE(r.add(std::move(tool)));
    }
    std::map<std::string, int> visits;
    const ToolRegistry& view = r;
    view.for_each([&](const Tool& tool) {
        CHECK(expected.at(tool.name()) == &tool);
        ++visits[tool.name()];
    });
    CHECK(visits.size() == expected.size());
    for (const auto& entry : expected) CHECK(visits[entry.first] == 1);
}

TEST(reg_10000_tools_and_alternative_hash) {
    ToolRegistry r;
    std::vector<Tool*> addresses;
    for (int i = 0; i < 10000; ++i) {
        auto tool = std::make_unique<NamedTool>("tool_" + std::to_string(i));
        addresses.push_back(tool.get());
        REQUIRE(r.add(std::move(tool)));
        CHECK(r.size() == static_cast<std::size_t>(i + 1));
        REQUIRE(r.load_factor() <= 1.0);
    }
    CHECK(r.size() == 10000);
    for (int i = 0; i < 10000; ++i) {
        CHECK(r.find("tool_" + std::to_string(i)) == addresses[static_cast<std::size_t>(i)]);
    }
    CHECK(r.find("tool_0") == addresses.front());
    CHECK(r.find("tool_10000") == nullptr);
    CHECK(r.max_probe_length() <= 64);

    // Compute both bucket distributions independently of the table internals.
    std::vector<int> real(r.bucket_count(), 0);
    std::vector<int> weak(r.bucket_count(), 0);
    int longest_real = 0;
    int longest_weak = 0;
    for (int i = 0; i < 10000; ++i) {
        const std::string name = "tool_" + std::to_string(i);
        const auto real_index = ToolRegistry::hash(name) % r.bucket_count();
        const auto weak_index = (static_cast<unsigned char>(name[0]) + name.size()) % r.bucket_count();
        if (++real[real_index] > longest_real) longest_real = real[real_index];
        if (++weak[weak_index] > longest_weak) longest_weak = weak[weak_index];
    }
    CHECK(r.max_probe_length() == static_cast<std::size_t>(longest_real));
    CHECK(longest_weak == 9000);
    std::printf("    10000 tools: buckets=%zu load=%.8f max_chain=%zu; weak_hash_max_chain=%d\n",
                r.bucket_count(), r.load_factor(), r.max_probe_length(), longest_weak);
}

TEST(ws_accepts_and_normalizes_spec_paths) {
    TempDir d;
    Workspace ws(d.path());
    const std::vector<std::pair<std::string, fs::path>> cases = {
        {"notes/todo.txt", "notes/todo.txt"},
        {"./notes//todo.txt", "notes/todo.txt"},
        {".", ""},
        {"a/../b", "b"},
        {"a/..", ""},
        {"new/dir/file.txt", "new/dir/file.txt"},
        {"my..notes.txt", "my..notes.txt"},
    };
    for (const auto& entry : cases) {
        fs::path out;
        std::string reason;
        REQUIRE(ws.resolve(entry.first, &out, &reason));
        CHECK(out == (entry.second.empty() ? ws.root() : ws.root() / entry.second));
    }
}

TEST(ws_rejects_all_spec_escapes) {
    TempDir d;
    Workspace ws(d.path() / "ws");
    const std::vector<std::string> cases = {
        "", std::string("notes") + '\0' + "/x", "/etc/passwd", "..",
        "../etc/passwd", "a/../../b", "notes/../../ws/notes/todo.txt",
    };
    for (const auto& path : cases) {
        fs::path out = "unchanged";
        std::string reason;
        CHECK(!ws.resolve(path, &out, &reason));
        CHECK(!reason.empty());
        CHECK(out == "unchanged");
        if (path.empty()) CHECK(reason == "empty path");
    }
}

TEST(calc_left_to_right) {
    double value = 0;
    std::string error;
    REQUIRE(eval("10 - 4 - 3", &value, &error));
    CHECK(value == 3);
    REQUIRE(eval("100 / 10 / 5", &value, &error));
    CHECK(value == 2);
    REQUIRE(eval("12 / 3 * 2", &value, &error));
    CHECK(value == 8);
}

TEST(calc_parentheses_and_large_inputs) {
    {
        double value = 0;
        std::string error;
        REQUIRE(eval("(2 + 3) * 4", &value, &error));
        CHECK(value == 20);
        REQUIRE(eval("2 * (3 + 4) - 5", &value, &error));
        CHECK(value == 9);
        REQUIRE(eval("(20 - (3 + 2)) / (1 + 2)", &value, &error));
        CHECK(value == 5);
    }
    {
        std::string expression = "1";
        for (int i = 1; i < 20000; ++i) expression += "+1";
        double value = 0;
        std::string error;
        REQUIRE(eval(expression, &value, &error));
        CHECK(value == 20000);
    }
    {
        const std::string expression = std::string(5000, '(') + "2+3*4" + std::string(5000, ')');
        double value = 0;
        std::string error;
        REQUIRE(eval(expression, &value, &error));
        CHECK(value == 14);
    }
}

TEST(calc_negative_numbers) {
    double value = 0;
    std::string error;
    REQUIRE(eval("-3 * -3", &value, &error));
    CHECK(value == 9);
    REQUIRE(eval("2*-3 + (-1)", &value, &error));
    CHECK(value == -7);
    REQUIRE(eval("8--2", &value, &error));
    CHECK(value == 10);
}

TEST(calc_decimals) {
    double value = 0;
    std::string error;
    REQUIRE(eval("7 / 2", &value, &error));
    CHECK(value == 3.5);
    REQUIRE(eval("1.25 + 2.5 * 2 - 0.25", &value, &error));
    CHECK(value == 6);
    REQUIRE(eval("-0.5 * 8", &value, &error));
    CHECK(value == -4);
}

int main(int argc, char** argv) { return mt::run_all(argc, argv); }

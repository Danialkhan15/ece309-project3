# Project 3 Design Log

**P2 code used:** Project 2 implementation from
[the supplied repository](https://github.com/Danialkhan15/ece309-project2-starter),
commit `fc26d7ce3cd86d2c8450af449ec6cd94c90f48a3`, selected with
`USE_P2_REFERENCE=OFF`. The two Conversation and two SentinelScanner files were
copied into P3; P3's provided `message.h` was retained.
**Author:** Danial Khan
**Tools used:** OpenAI Codex implemented the three P3 components, expanded the
tests, carried over the P2 files, and drafted this log. CMake, Apple Clang, CTest, AddressSanitizer, and
UndefinedBehaviorSanitizer built and checked the project. GitHub CLI published
the public repository; GitHub Actions checked Linux with leak detection. Python
and Git packaged backups. This is an AI-assisted draft to review and personalize
before submission.

## 1. Hash table

The registry uses separate chaining: a vector contains buckets, and each bucket
is a linked list of entries. Each entry stores a name and a `unique_ptr<Tool>`.
Chaining keeps collision handling straightforward: lookup searches only the
selected list. Linear probing could improve locality, but requires searching
consecutive slots and keeping enough empty slots to avoid long searches.

The hash is 64-bit FNV-1a. Starting from its offset basis, it XORs each unsigned
byte into the accumulator and multiplies by the FNV prime. Unsigned overflow
provides the intended wrapping arithmetic. Every byte participates, and changing
byte order generally changes the result. The full hash is returned; taking its
remainder with the bucket count is a separate indexing step.

There are initially 16 buckets. The maximum load factor is 1.0, appropriate for
chaining because an occupied bucket can hold another entry. Before an insertion
would exceed that threshold, the table doubles. Rehashing recomputes every
entry's index using the new bucket count and splices list nodes into their new
buckets. Duplicate and null tools are rejected before growth.

The required experiment produced:

| Names | Buckets | Load factor | FNV-1a longest chain | Weak-hash longest chain |
|---|---:|---:|---:|---:|
| `tool_0` through `tool_9999` | 16,384 | 0.6103515625 | 6 | 9,000 |

The weak hash, `s[0] + s.size()`, groups all four-digit suffixes together. FNV-1a
uses those suffix bytes and spreads these names much more evenly. The test also
checks every lookup, every saved address, and the load factor after every add.

A resize costs O(n), but doubling leaves room for roughly n further insertions.
Across successive resizes, the total number of entries redistributed forms a
geometric series. Spreading that work over insertions gives amortized O(1) add,
assuming the expected short chains and bounded tool-name lengths.

## 2. Pointer stability

Each tool is separately allocated and has exactly one owning `unique_ptr`.
Rehashing splices existing list nodes; neither the tool nor its owner is copied.
Even moving a `unique_ptr` would preserve the allocated object's address. Thus
the pointer returned by `find` remains valid until registry destruction. A
rejected duplicate destroys only the incoming tool. Ownership tests count
destructions before and after multiple resizes to verify that behavior.

## 3. Calculator trace

The evaluator uses two vectors as stacks, with the back as the top. For
`2 * (3 + 4) - 5`, reading `2` puts `[2]` on the value stack. Reading `*` puts
`[*]` on the operator stack. The opening parenthesis gives `[*, (]`, preventing
the outer multiplication from consuming an inner operand.

Next, `3` makes the values `[2, 3]`; `+` makes the operators `[*, (, +]`.
Reading `4` gives `[2, 3, 4]`. At `)`, addition removes right operand 4 and left
operand 3, then pushes 7. Removing the opening parenthesis leaves values
`[2, 7]` and operators `[*]`.

When `-` arrives, the waiting multiplication has higher precedence, so it runs
first and leaves `[14]`. Subtraction is then pushed. Reading `5` leaves values
`[14, 5]`; the final reduction computes 14 minus 5, leaving the answer 9.
Reducing equal precedence too preserves left-to-right subtraction and division.
All reductions and parenthesis handling use loops, allowing deeply nested input
without recursive call-stack growth.

## 4. Errors as values

An unavailable file, forbidden path, or zero divisor can arise from ordinary
model input. Returning an error value lets the harness record that outcome and
give the model another chance. These failures should not unwind the conversation.
The resolver validates path text without querying the filesystem, and the
evaluator checks stack sizes before accessing operands. Programmer mistakes,
such as using an incompatible JSON accessor, warrant exceptions; the provided
dispatcher catches escaping tool exceptions as a final containment boundary.

## 5. What I would design differently

A future interface could give each tool one immutable registration name instead
of repeatedly returning names by value. The current entry caches its key, but
also duplicates the tool's name storage. An immutable name contract would make
that ownership and identity relationship clearer. The fixed public API is
preserved here. Sanitizer and golden-session checks passed on macOS and Linux;
the GitHub Actions run also passed with leak detection enabled.

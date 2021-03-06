// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/lib/line_input/line_input.h"

#include "gtest/gtest.h"

namespace line_input {

namespace {

// Some common terminal codes.
#define TERM_UP "\x1b[A"
#define TERM_DOWN "\x1b[B"
#define TERM_LEFT "\x1b[D"
#define TERM_RIGHT "\x1b[C"

// Dummy completion functions that return two completions.
std::vector<std::string> CompletionCallback(const std::string& line) {
  std::vector<std::string> result;
  result.push_back("one");
  result.push_back("two");
  return result;
}

}  // namespace

class TestLineInput : public LineInputBase {
 public:
  TestLineInput(const std::string& prompt) : LineInputBase(prompt) {}

  void ClearOutput() { output_.clear(); }
  std::string GetAndClearOutput() {
    std::string ret = output_;
    ClearOutput();
    return ret;
  }

  // This input takes a string instead of one character at a time, returning
  // the result of the last one.
  bool OnInputStr(const std::string& input) {
    bool result = false;
    for (char c : input)
      result = OnInput(c);
    return result;
  }

  void SetLine(const std::string& input) {
    cur_line() = input;
    set_pos(input.size());
  }

  void SetPos(size_t pos) { set_pos(pos); }

 protected:
  void Write(const std::string& data) { output_.append(data); }

 private:
  std::string output_;
};

TEST(LineInput, CursorCommands) {
  std::string prompt("Prompt ");
  TestLineInput input(prompt);

  // Basic prompt. "7C" at the end means cursor is @ 7th character.
  input.BeginReadLine();
  EXPECT_EQ("\rPrompt \x1b[0K\r\x1B[7C", input.GetAndClearOutput());

  // Basic input with enter.
  EXPECT_FALSE(input.OnInput('a'));
  EXPECT_FALSE(input.OnInput('b'));
  EXPECT_TRUE(input.OnInput('\r'));
  EXPECT_EQ("ab", input.line());

  input.BeginReadLine();
  EXPECT_FALSE(input.OnInputStr("abcd"));
  EXPECT_EQ(4u, input.pos());

  // Basic cursor movement.
  EXPECT_FALSE(input.OnInput(2));  // Control-B = left.
  EXPECT_EQ(3u, input.pos());
  EXPECT_FALSE(input.OnInput(6));  // Control-F = right.
  EXPECT_EQ(4u, input.pos());
  EXPECT_FALSE(input.OnInput(1));  // Control-A = home.
  EXPECT_EQ(0u, input.pos());
  EXPECT_FALSE(input.OnInput(5));  // Control-E = end.
  EXPECT_EQ(4u, input.pos());

  // Longer escaped sequences.
  EXPECT_FALSE(input.OnInputStr("\x1b[D"));  // Left.
  EXPECT_EQ(3u, input.pos());
  EXPECT_FALSE(input.OnInputStr("\x1b[C"));  // Right.
  EXPECT_EQ(4u, input.pos());
  EXPECT_FALSE(input.OnInputStr("\x1b[H"));  // Home.
  EXPECT_EQ(0u, input.pos());
  EXPECT_FALSE(input.OnInputStr("\x1b[F"));  // End.
  EXPECT_EQ(4u, input.pos());
  EXPECT_FALSE(input.OnInputStr("\x1b[1~"));  // Home. Alternate.
  EXPECT_EQ(0u, input.pos());
  EXPECT_FALSE(input.OnInputStr("\x1b[4~"));  // End. Alternate.
  EXPECT_EQ(4u, input.pos());

  // Backspace.
  EXPECT_FALSE(input.OnInput(127));  // Backspace.
  EXPECT_EQ(3u, input.pos());
  EXPECT_EQ("abc", input.line());

  // Delete. This one also tests the line refresh commands.
  EXPECT_FALSE(input.OnInput(1));  // Home.
  input.ClearOutput();
  EXPECT_FALSE(input.OnInputStr("\x1b[3~"));
  EXPECT_EQ("bc", input.line());
  // "7C" at the end means cursor is at the 7th character (the "b").
  EXPECT_EQ("\rPrompt bc\x1b[0K\r\x1B[7C", input.GetAndClearOutput());
  EXPECT_EQ(0u, input.pos());
}

TEST(LineInput, CtrlD) {
  std::string prompt("Prompt ");
  TestLineInput input(prompt);

  input.BeginReadLine();

  EXPECT_FALSE(input.OnInputStr("abcd"));
  // "abcd|"
  EXPECT_EQ(4u, input.pos());

  EXPECT_FALSE(input.OnInputStr("\x1b[D\x1b[D"));  // 2 x Left.
  // "ab|cd"
  EXPECT_EQ(2u, input.pos());

  EXPECT_FALSE(input.OnInput(4));  // Ctrl+D
  // "ab|d"
  EXPECT_EQ("abd", input.line());
  EXPECT_EQ(2u, input.pos());

  EXPECT_FALSE(input.OnInputStr("\x1b[C"));  // Right.
  // "abd|"
  EXPECT_EQ(3u, input.pos());
  EXPECT_EQ("abd", input.line());

  EXPECT_FALSE(input.OnInput(4));  // Ctrl+D
  // No change when hit Ctrl+D at the end of the line.
  EXPECT_EQ("abd", input.line());
  EXPECT_EQ(3u, input.pos());

  // Erase everything and then exit.

  EXPECT_FALSE(input.OnInputStr("\x1b[D\x1b[D\x1b[D"));  // 3 x Left.
  // "|abd"
  EXPECT_EQ(0u, input.pos());

  EXPECT_FALSE(input.OnInput(4));  // Ctrl+D
  // "|bd"
  EXPECT_EQ("bd", input.line());
  EXPECT_EQ(0u, input.pos());

  EXPECT_FALSE(input.OnInput(4));  // Ctrl+D
  // "|d"
  EXPECT_EQ("d", input.line());
  EXPECT_EQ(0u, input.pos());

  EXPECT_FALSE(input.OnInput(4));  // Ctrl+D
  // "|"
  EXPECT_EQ("", input.line());
  EXPECT_EQ(0u, input.pos());

  // Ctrl+D on an empty line is exit.

  EXPECT_TRUE(input.OnInput(4));  // Ctrl+D
  EXPECT_TRUE(input.eof());
}

TEST(LineInput, History) {
  TestLineInput input("");

  // Make some history.
  input.AddToHistory("one");
  input.AddToHistory("two");

  // Go up twice.
  input.BeginReadLine();
  EXPECT_FALSE(input.OnInputStr(TERM_UP TERM_UP));

  // Should have selected the first line and the cursor should be at the end.
  EXPECT_EQ("one", input.line());
  EXPECT_EQ(3u, input.pos());

  // Append a letter and accept it.
  input.OnInputStr("s\r");
  input.AddToHistory(input.line());

  // Start editing a new line with some input.
  input.BeginReadLine();
  input.OnInputStr("three");

  // Check history. Should be:
  //  ones
  //  two
  //  ones
  //  three
  EXPECT_EQ("three", input.line());
  EXPECT_FALSE(input.OnInputStr(TERM_UP));
  EXPECT_EQ("ones", input.line());
  EXPECT_FALSE(input.OnInputStr(TERM_UP));
  EXPECT_EQ("two", input.line());
  EXPECT_FALSE(input.OnInputStr(TERM_UP));
  EXPECT_FALSE(input.OnInputStr(TERM_UP));  // From here, these are extra to
  EXPECT_FALSE(input.OnInputStr(TERM_UP));  // test that going beyond the top
  EXPECT_FALSE(input.OnInputStr(TERM_UP));  // stays stopped.
  EXPECT_EQ("ones", input.line());

  // Going back to the bottom (also doing one extra one to test the boundary).
  EXPECT_FALSE(input.OnInputStr(TERM_DOWN TERM_DOWN TERM_DOWN TERM_DOWN));

  // Should have gotten the original non-accepted input back.
  EXPECT_EQ("three", input.line());
}

TEST(LineInput, HistoryEdgeCases) {
  TestLineInput input("");

  input.AddToHistory("one");
  ASSERT_EQ(input.history().size(), 2u);

  // If input is empty, it should not add to history.
  input.AddToHistory("");
  ASSERT_EQ(input.history().size(), 2u);

  // Same input should not work.
  input.AddToHistory("one");
  ASSERT_EQ(input.history().size(), 2u);

  // A past input should work.
  input.AddToHistory("two");
  ASSERT_EQ(input.history().size(), 3u);
  input.AddToHistory("one");
  ASSERT_EQ(input.history().size(), 4u);
}

TEST(LineInput, Completions) {
  TestLineInput input("");
  input.set_completion_callback(&CompletionCallback);

  input.BeginReadLine();
  input.OnInput('z');

  // Send one tab, should get the first suggestion.
  input.OnInput(9);
  EXPECT_EQ("one", input.line());
  EXPECT_EQ(3u, input.pos());

  // Second suggestion.
  input.OnInput(9);
  EXPECT_EQ("two", input.line());
  EXPECT_EQ(3u, input.pos());

  // Again should go back to original text.
  input.OnInput(9);
  EXPECT_EQ("z", input.line());
  EXPECT_EQ(1u, input.pos());

  // Should wrap around to the first suggestion.
  input.OnInput(9);
  EXPECT_EQ("one", input.line());
  EXPECT_EQ(3u, input.pos());

  // Typing should append.
  input.OnInput('s');
  EXPECT_EQ("ones", input.line());
  EXPECT_EQ(4u, input.pos());

  // Tab again should give the same suggestions.
  input.OnInput(9);
  EXPECT_EQ("one", input.line());
  EXPECT_EQ(3u, input.pos());

  // Send an escape sequence "left" which should accept the suggestion and
  // execute the sequence.
  input.OnInputStr("\x1b[D");
  EXPECT_EQ("one", input.line());
  EXPECT_EQ(2u, input.pos());
}

TEST(LineInput, Scroll) {
  TestLineInput input("ABCDE");
  input.set_max_cols(10);

  input.BeginReadLine();
  input.ClearOutput();

  // Write up to the 9th character, which should be the last character printed
  // until scrolling starts. It should have used the optimized "just write the
  // characters" code path for everything after the prompt.
  EXPECT_FALSE(input.OnInputStr("FGHI"));
  EXPECT_EQ("FGHI", input.GetAndClearOutput());

  // Add a 10th character. The whole line should scroll one to the left,
  // leaving the cursor at the last column (column offset 9 = "9C" at the end).
  EXPECT_FALSE(input.OnInput('J'));
  EXPECT_EQ("\rBCDEFGHIJ\x1b[0K\r\x1B[9C", input.GetAndClearOutput());

  // Move left, the line should scroll back.
  EXPECT_FALSE(input.OnInput(2));  // 2 = Control-B.
  EXPECT_EQ("\rABCDEFGHIJ\x1b[0K\r\x1B[9C", input.GetAndClearOutput());
}

TEST(LineInput, NegAck) {
  TestLineInput input("ABCDE");
  input.BeginReadLine();

  // Empty should remain with them prompt.
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlU));
  EXPECT_EQ(input.line(), "");

  // Adding characters and then Control-U should clear.
  input.OnInputStr("12345");
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlU));
  EXPECT_EQ(input.line(), "");

  // In the middle of the line should clear until the cursor.
  input.OnInputStr("0123456789");
  EXPECT_FALSE(input.OnInputStr(TERM_LEFT));
  EXPECT_FALSE(input.OnInputStr(TERM_LEFT));
  EXPECT_FALSE(input.OnInputStr(TERM_LEFT));
  EXPECT_FALSE(input.OnInputStr(TERM_LEFT));
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlU));
  EXPECT_EQ(input.line(), "6789");
  EXPECT_EQ(input.pos(), 0u);
}

TEST(LineInput, EndOfTransimission) {
  TestLineInput input("[prompt] ");
  input.BeginReadLine();

  //             v
  input.SetLine("First Second Third");
  input.SetPos(0);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlW));
  EXPECT_EQ(input.line(), "First Second Third");

  //               v
  input.SetLine("First Second Third");
  input.SetPos(2);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlW));
  EXPECT_EQ(input.line(), "rst Second Third");

  //                  v
  input.SetLine("First Second Third");
  input.SetPos(5);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlW));
  EXPECT_EQ(input.line(), " Second Third");

  //                     v
  input.SetLine("First Second Third");
  input.SetPos(8);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlW));
  EXPECT_EQ(input.line(), "First cond Third");

  //                         v
  input.SetLine("First Second Third");
  input.SetPos(12);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlW));
  EXPECT_EQ(input.line(), "First  Third");

  //                            v
  input.SetLine("First Second Third");
  input.SetPos(15);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlW));
  EXPECT_EQ(input.line(), "First Second ird");

  //                               v
  input.SetLine("First Second Third");
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlW));
  EXPECT_EQ(input.line(), "First Second ");
}

TEST(LineInput, Transpose) {
  TestLineInput input("[prompt] ");
  input.BeginReadLine();

  //             v
  input.SetLine("First Second Third");
  input.SetPos(0);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlT));
  EXPECT_EQ(input.line(), "First Second Third");

  //              v
  input.SetLine("First Second Third");
  input.SetPos(1);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlT));
  EXPECT_EQ(input.line(), "First Second Third");

  //               v
  input.SetLine("First Second Third");
  input.SetPos(2);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlT));
  EXPECT_EQ(input.line(), "iFrst Second Third");

  //                               v
  input.SetLine("First Second Third");
  input.SetPos(18);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlT));
  EXPECT_EQ(input.line(), "First Second Thidr");
}

TEST(LineInput, DeleteEnd) {
  TestLineInput input("[prompt] ");
  input.BeginReadLine();

  //             v
  input.SetLine("First Second Third");
  input.SetPos(0);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlK));
  EXPECT_EQ(input.line(), "");

  //               v
  input.SetLine("First Second Third");
  input.SetPos(2);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlK));
  EXPECT_EQ(input.line(), "Fi");

  //                  v
  input.SetLine("First Second Third");
  input.SetPos(5);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlK));
  EXPECT_EQ(input.line(), "First");

  //                     v
  input.SetLine("First Second Third");
  input.SetPos(8);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlK));
  EXPECT_EQ(input.line(), "First Se");

  //                         v
  input.SetLine("First Second Third");
  input.SetPos(12);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlK));
  EXPECT_EQ(input.line(), "First Second");

  //                               v
  input.SetLine("First Second Third");
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlK));
  EXPECT_EQ(input.line(), "First Second Third");
}

TEST(LineInput, CancelCommand) {
  TestLineInput input("[prompt] ");
  input.BeginReadLine();

  //             v
  input.SetLine("First Second Third");
  input.SetPos(0);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlC));
  EXPECT_EQ(input.line(), "");

  //               v
  input.SetLine("First Second Third");
  input.SetPos(2);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlC));
  EXPECT_EQ(input.line(), "");

  //                               v
  input.SetLine("First Second Third");
  input.SetPos(18);
  EXPECT_FALSE(input.OnInput(SpecialCharacters::kKeyControlC));
  EXPECT_EQ(input.line(), "");
}

TEST(LineInput, ReverseHistory_Select) {
  TestLineInput input("> ");

  // Add some history.
  input.AddToHistory("prefix postfix1");  // Index 5.
  input.AddToHistory("prefix postfix2");  // Index 4.
  input.AddToHistory("prefix postfix3");  // Index 3.
  input.AddToHistory("other prefix");     // Index 2.
  input.AddToHistory("different");        // Index 1.

  input.BeginReadLine();
  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyControlR));
  ASSERT_TRUE(input.in_reverse_history_mode());

  EXPECT_FALSE(input.OnInputStr("post"));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`post': ");
  EXPECT_EQ(input.reverse_history_index(), 3u);
  // Pos:                                        |       v
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "prefix postfix3");
  EXPECT_EQ(input.pos(), 7u);

  // Selecting should get this suggestion out.
  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyEnter));
  ASSERT_FALSE(input.in_reverse_history_mode());
  // Pos:                 |               v
  EXPECT_EQ(input.line(), "prefix postfix3");
  EXPECT_EQ(input.pos(), 15u);
}

TEST(LineInput, ReverseHistory_SpecificSearch) {
  TestLineInput input("> ");

  // Add some history.
  input.AddToHistory("prefix postfix1");  // Index 5.
  input.AddToHistory("prefix postfix2");  // Index 4.
  input.AddToHistory("prefix postfix3");  // Index 3.
  input.AddToHistory("other prefix");     // Index 2.
  input.AddToHistory("different");        // Index 1.

  input.BeginReadLine();

  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyControlR));
  ASSERT_TRUE(input.in_reverse_history_mode());

  ASSERT_FALSE(input.OnInput('f'));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`f': ");
  EXPECT_EQ(input.reverse_history_index(), 1u);
  // Pos:                                        |  v
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "different");
  EXPECT_EQ(input.pos(), 2u);

  ASSERT_FALSE(input.OnInput('i'));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`fi': ");
  EXPECT_EQ(input.reverse_history_index(), 2u);
  // Pos:                                        |         v
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "other prefix");
  EXPECT_EQ(input.pos(), 9u);

  ASSERT_FALSE(input.OnInput('x'));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`fix': ");
  EXPECT_EQ(input.reverse_history_index(), 2u);
  // Pos:                                        |         v
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "other prefix");
  EXPECT_EQ(input.pos(), 9u);

  ASSERT_FALSE(input.OnInput('3'));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`fix3': ");
  EXPECT_EQ(input.reverse_history_index(), 3u);
  // Pos:                                        |           v
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "prefix postfix3");
  EXPECT_EQ(input.pos(), 11u);

  ASSERT_FALSE(input.OnInput('3'));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`fix33': ");
  EXPECT_EQ(input.reverse_history_index(), 0u);
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "");
  EXPECT_EQ(input.pos(), 0u);

  // Deleting should return to the suggestion.
  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyBackspace));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`fix3': ");
  EXPECT_EQ(input.reverse_history_index(), 3u);
  // Pos:                                        |           v
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "prefix postfix3");
  EXPECT_EQ(input.pos(), 11u);
}

TEST(LineInput, ReverseHistory_RepeatedSearch) {
  TestLineInput input("> ");

  // Add some history.
  input.AddToHistory("prefix postfix1");  // Index 5.
  input.AddToHistory("prefix postfix2");  // Index 4.
  input.AddToHistory("prefix postfix3");  // Index 3.
  input.AddToHistory("other prefix");     // Index 2.
  input.AddToHistory("different");        // Index 1.

  input.BeginReadLine();
  ASSERT_FALSE(input.in_reverse_history_mode());

  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyControlR));

  // We should be in reverse history mode, but no suggestion should be made.
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`': ");
  EXPECT_EQ(input.reverse_history_index(), 0u);
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "");
  EXPECT_EQ(input.pos(), 0u);

  // Start writing should match.
  ASSERT_FALSE(input.OnInput('f'));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`f': ");
  EXPECT_EQ(input.reverse_history_index(), 1u);
  // Pos:                                        |  v
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "different");
  EXPECT_EQ(input.pos(), 2u);

  // Ctrl-R should move to the next suggestion.
  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyControlR));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`f': ");
  EXPECT_EQ(input.reverse_history_index(), 2u);
  // Pos:                                        |         v
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "other prefix");
  EXPECT_EQ(input.pos(), 9u);

  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyControlR));
  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyControlR));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`f': ");
  EXPECT_EQ(input.reverse_history_index(), 4u);
  // Pos:                                        |   v
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "prefix postfix2");
  EXPECT_EQ(input.pos(), 3u);

  // More Ctrl-R should roll-over.
  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyControlR));
  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyControlR));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`f': ");
  EXPECT_EQ(input.reverse_history_index(), 0u);
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "");
  EXPECT_EQ(input.pos(), 0u);

  // One more should start again.
  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyControlR));
  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyControlR));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`f': ");
  EXPECT_EQ(input.reverse_history_index(), 2u);
  // Pos:                                        |         v
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "other prefix");
  EXPECT_EQ(input.pos(), 9u);

  // Deleting should show no suggestion.
  ASSERT_FALSE(input.OnInput(SpecialCharacters::kKeyBackspace));
  ASSERT_TRUE(input.in_reverse_history_mode());
  EXPECT_EQ(input.GetReverseHistoryPrompt(), "(reverse-i-search)`': ");
  EXPECT_EQ(input.reverse_history_index(), 0u);
  EXPECT_EQ(input.GetReverseHistorySuggestion(), "");
  EXPECT_EQ(input.pos(), 0u);
}

}  // namespace line_input
